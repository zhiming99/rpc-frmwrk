/*
 * =====================================================================================
 *
 *       Filename:  buffer.cpp
 *
 *    Description:  implementation of CBuffer
 *
 *        Version:  1.0
 *        Created:  07/09/2016 10:19:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */


#include <string>
#include "defines.h"
#include "autoptr.h"
#include "buffer.h"

// CBuffer implementation
CBuffer::operator ObjPtr&() const
{
    if( empty() || DataTypeObjPtr != GetDataType() )
    {
        throw std::invalid_argument(
            "the buffer has bad data type" );
    }
    return *reinterpret_cast< ObjPtr* >( ptr() );
}

CBuffer::operator DMsgPtr&() const
{
    if( empty() || DataTypeMsgPtr != GetDataType() )
    {
        throw std::invalid_argument(
            "the buffer has bad data type" );
    }
    return *reinterpret_cast< DMsgPtr* >( ptr() );
}

CBuffer::operator DMsgPtr*() const
{
    if( empty() || DataTypeMsgPtr != GetDataType() )
    {
        throw std::invalid_argument(
            "the buffer has bad data type" );
    }
    return reinterpret_cast< DMsgPtr* >( ptr() );
}

// specialized type casting
CBuffer::operator stdstr() const
{
    if( empty() ||
        DataTypeMem != GetDataType() ||
        typeString != GetExDataType() )
    {
        throw std::invalid_argument(
            "The buffer is empty" );

    }
    return stdstr( ptr() );
}

CBuffer::operator ObjPtr*() const
{
    if( empty() || DataTypeObjPtr != GetDataType() )
    {
        throw std::invalid_argument(
            "The buffer has a bad data type" );
    }
    return reinterpret_cast< ObjPtr* >( ptr() );
}

CBuffer::operator char*() const
{
    return ptr();
}

CBuffer::operator const char*() const
{
    return ( const char* )ptr();
}

template<> CBuffer&
CBuffer::operator=<stdstr>( const stdstr& rhs )
{

    // with one terminal character
    guint32 len =
        rhs.size() + sizeof( std::string::value_type );

    Resize( len );

    if( ptr() )
    {
        memcpy( ptr(), rhs.c_str(), len );
        *( ptr() + len - 1 ) = 0;
        SetDataType( DataTypeMem );
        SetExDataType( typeString );
    }
    return *this;
}

template<> CBuffer&
CBuffer::operator=<CObjBase>( const CObjBase& rhs )
{
    // specilization for CObjBase
    // we will serialize the CObjBase and put the
    // data-block in the buffer
    gint32 ret = rhs.Serialize( *this );
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Serialization of the buffer failed" );
        throw std::runtime_error( strMsg );
    }

    return *this;
}


template<> CBuffer&
CBuffer::operator=<ObjPtr>( const ObjPtr& rhs )
{
    // for smart pointer
    Resize( sizeof( ObjPtr ) );
    if( ptr() != nullptr )
    {
        ObjPtr* pObj = reinterpret_cast< ObjPtr* >( ptr() );
        new( pObj ) ObjPtr( rhs );

        SetDataType( DataTypeObjPtr );
        SetExDataType( typeObj );
        // return *pObj;
        return *this;
    }
    else
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "Bad assignment of objptr" );
        throw std::runtime_error( strMsg );
    }
    // return *( ObjPtr* )nullptr;
    return *this;
}

template<> CBuffer&
CBuffer::operator=<DMsgPtr>( const DMsgPtr& rhs )
{
    // for smart pointer
    Resize( sizeof( DMsgPtr ) );
    if( ptr() != nullptr )
    {
        DMsgPtr* pMsg = ( DMsgPtr* )ptr();
        new( pMsg ) DMsgPtr( rhs );

        SetDataType( DataTypeMsgPtr );
        SetExDataType( typeDMsg );
        // return *pMsg;
        return *this;
    }
    else
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "Bad assignment of DMsgPtr" );
        throw std::runtime_error( strMsg );
    }
    // return *( DMsgPtr* )nullptr;
    return *this;
}

template<> cchar*
CBuffer::operator=<char>( cchar* rhs )
{

    // specilization of T*
    gint32 len = strlen( rhs ) + sizeof( cchar );
    Resize( len );
    if( ptr() )
    {
        memcpy( ptr(), rhs, len );
        SetExDataType( typeString );
    }
    return ptr();
    // return *this;
}

CBuffer& CBuffer::operator=( const CBuffer& rhs )
{
    Resize( rhs.size() );
    if( size() == 0 )
        return *this;

    if( ptr() == nullptr )
    {
        throw std::runtime_error(
            "no memory" );
    }
    switch( rhs.GetDataType() )
    {
    case DataTypeMem:
        {
            SetDataType( DataTypeMem );
            memcpy( ptr(), rhs.ptr(), size() );   
            SetExDataType( rhs.GetExDataType() );
            break;
        }
    case DataTypeMsgPtr:
        {
            DMsgPtr pMsg;
            pMsg = ( DMsgPtr& )rhs;
            *this = pMsg;
            break;
        }
    case DataTypeObjPtr:
        {
            ObjPtr pObj;
            pObj = ( ObjPtr& )rhs;
            *this = pObj;
            break;
        }
    default:
        {
            throw std::invalid_argument(
                "the source type is not supported" );
        }
    }
    return *this;
}

CBuffer::CBuffer( guint32 dwSize )
    : m_dwType( 0 )
{
    SetClassId( clsid( CBuffer ) );

    SetDataType( DataTypeMem );
    SetExDataType( typeNone );

    m_pData = nullptr;
    m_dwSize = 0;

    if( dwSize > BUF_MAX_SIZE )
        return;

    Resize( dwSize );
}

CBuffer::CBuffer( const char* pData, guint32 dwSize )
    : CBuffer( dwSize )
{
    if( pData == nullptr )
        return;

    if( ptr() )
        memcpy( ptr(), pData, size() );

    return;
}

CBuffer::~CBuffer()
{
    Resize( 0 );
}

CBuffer::CBuffer( const ObjPtr& rhs )
    : CObjBase(),
     m_dwType( 0 )
{
    Resize( sizeof( ObjPtr ) );
    SetDataType( DataTypeObjPtr );
    SetExDataType( typeObj );

    new ( ptr() ) ObjPtr( rhs );
}

CBuffer::CBuffer( const DMsgPtr& rhs )
    : CObjBase(),
     m_dwType( 0 )
{
    Resize( sizeof( DMsgPtr ) );
    SetDataType( DataTypeMsgPtr );
    SetExDataType( typeDMsg );

    new ( ptr() ) DMsgPtr( rhs );
}

CBuffer::CBuffer( const CBuffer& rhs )
    : CObjBase()
{
    *this = rhs;
}

char* CBuffer::ptr() const
{
    return m_pData + offset();
}

guint32 CBuffer::size() const
{
    return m_dwSize - offset();
}

guint32 CBuffer::type() const
{
    return ( m_dwType & DATATYPE_MASK ) ;
}

gint32 CBuffer::ResizeWithOffset( guint32 dwSize )
{
    switch( GetDataType() )
    {
    case DataTypeMem:
        {
            break;
        }
    default:
        {
            return -ENOTSUP;
        }
    }

    gint32 ret = 0;
    do{
        char* pBase = ptr() - offset();
        guint32 dwActSize = size() + offset();

        bool bArrBuf = false;

        if( pBase == m_arrBuf )
            bArrBuf = true;

        if( dwSize == 0 )
        {
            if( pBase != nullptr && !bArrBuf )
                free( pBase );

            SetBuffer( nullptr, 0 );
            break;
        }

        if( dwSize == size() )
            break;

        if( sizeof( m_arrBuf ) >= dwSize &&
            sizeof( m_arrBuf ) >= size() )
        {
            if( size() > 0 )
            {
                memmove( m_arrBuf, ptr(),
                    std::min( size(), dwSize ) );
            }
            SetBuffer( m_arrBuf, dwSize );
            if( !bArrBuf )
                free( pBase );
        }
        else if( dwSize > sizeof( m_arrBuf ) &&
            sizeof( m_arrBuf ) >= size() )
        {
            char* pData = nullptr;
            if( bArrBuf )
            {
                pData = ( char* )calloc( 1, dwSize );
                if( size() > 0 )
                    memcpy( pData, ptr(), size() );
            }
            else
            {
                if( dwSize <= dwActSize )
                {
                    pData = pBase;
                    memmove( pData, ptr(), size() );
                }
                else
                {
                    pData = ( char* )realloc( pBase, dwSize );
                    if( pData == nullptr )
                        break;

                    memmove( pData,
                        pData + offset(), size() );
                }
                
            }

            if( pData == nullptr )
            {
                ret = -ENOMEM;
                break;
            }
            SetBuffer( pData, dwSize );
        }
        else if( size() > sizeof( m_arrBuf ) &&
            sizeof( m_arrBuf ) >= dwSize )
        {
            if( bArrBuf )
            {
                ret = -EFAULT;
                break;
            }
            memcpy( m_arrBuf, ptr(), dwSize );
            free( pBase );
            SetBuffer( m_arrBuf, dwSize );
        }
        else if( std::min( dwSize, size() ) > sizeof( m_arrBuf ) )
        {
            if( bArrBuf )
            {
                ret = -EFAULT;
                break;
            }
            char* pData = nullptr;
            if( dwSize <= dwActSize )
            {
                pData = pBase;
                memmove( pData, ptr(),
                    std::min( dwSize, size() ) );
            }
            else
            {
                memmove( pBase, ptr(), size() );
                pData = ( char* )realloc( pBase, dwSize );
                if( pData == nullptr )
                    break;
                
            }
            SetBuffer( pData, dwSize );
        }

        SetExDataType( typeByteArr );

    }while( 0 );

    return ret;
}

void CBuffer::Resize( guint32 dwSize )
{
    switch( GetDataType() )
    {
    case DataTypeMsgPtr:
        {
            // release the content
            if( ptr() != nullptr )
            {
                DMsgPtr* pMsg = ( DMsgPtr* )ptr();
                pMsg->Clear();
            }
            SetDataType( DataTypeMem );
            SetExDataType( typeNone );
            Resize( 0 );
            if( dwSize > 0 )
                Resize( dwSize );
            break;
        }
    case DataTypeObjPtr:
        {
            // release the content
            if( ptr() != nullptr )
            {
                ObjPtr* pObj = ( ObjPtr* )ptr();
                pObj->Clear();
            }
            SetDataType( DataTypeMem );
            SetExDataType( typeNone );
            Resize( 0 );
            if( dwSize > 0 )
                Resize( dwSize );
            break;
        }
    case DataTypeMem:
        {
            if( offset() > 0 )
            {
                ResizeWithOffset( dwSize );
                break;
            }

            if( dwSize == 0 )
            {
                if( ptr() != nullptr && ptr() != m_arrBuf )
                    free( ptr() );

                SetBuffer( nullptr, 0 );
                break;
            }

            if( dwSize == size() )
                break;

            if( sizeof( m_arrBuf ) >= dwSize &&
                sizeof( m_arrBuf ) >= size() )
            {
                SetBuffer( m_arrBuf, dwSize );
            }
            else if( dwSize > sizeof( m_arrBuf ) &&
                sizeof( m_arrBuf ) >= size() )
            {
                char* pData = ( char* )calloc( 1, dwSize );
                if( pData == nullptr )
                    break;

                if( size() > 0 )
                    memcpy( pData, m_arrBuf, size() );

                SetBuffer( pData, dwSize );
            }
            else if( size() > sizeof( m_arrBuf ) &&
                sizeof( m_arrBuf ) >= dwSize )
            {
                memcpy( m_arrBuf, ptr(), dwSize );
                free( ptr() );
                SetBuffer( m_arrBuf, dwSize );
            }
            else if( std::min( dwSize, size() ) > sizeof( m_arrBuf ) )
            {
                char* pNewBuf = ( char* )realloc( ptr(), dwSize );
                if( pNewBuf != nullptr )
                {
                    SetBuffer( pNewBuf, dwSize );
                }
                else
                {
                    // keep the old pointer
                    break;
                }
            }

            SetExDataType( typeByteArr );

            break;
        }
    default:
        break;
    }

    return;
}

gint32 CBuffer::Serialize( CBuffer& toBuf ) const
{
    gint32 ret = 0;
    // the serialization is simple to add
    // prepend a SERI_HEADER to the *ptr()
    SERI_HEADER* pheader = nullptr;
    SERI_HEADER oHeader;

    toBuf.Resize( sizeof( SERI_HEADER ) );

    if( ptr() == nullptr || size() == 0 )
    {
        // empty buffer
        pheader = reinterpret_cast< SERI_HEADER* >( toBuf.ptr() );

        oHeader.dwClsid = htonl( clsid( CBuffer ) );
        oHeader.dwSize = 0;
        oHeader.dwType = DataTypeMem;
        oHeader.hton();
        *pheader = oHeader;

        return 0;
    }

    switch( GetDataType() )
    {
    case DataTypeMsgPtr:
        {
            BufPtr pNewBuf( true );
            if( ERROR( ret ) )
                break;

            DMsgPtr& pMsg = *( DMsgPtr* )ptr();
            if( pMsg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = pMsg.Serialize( *pNewBuf );
            if( ERROR( ret ) )
                break;

            toBuf.Resize( sizeof( SERI_HEADER ) + pNewBuf->size() );
            pheader = reinterpret_cast< SERI_HEADER* >( toBuf.ptr() );
            oHeader.dwClsid = clsid( CBuffer );
            oHeader.dwSize = pNewBuf->size();
            oHeader.dwType = m_dwType;
            oHeader.dwObjClsid = pMsg.GetType();

            oHeader.hton();
            *pheader = oHeader;

            if( pNewBuf->size() > 0 )
            {
                memcpy( ( void* )&pheader[ 1 ],
                    pNewBuf->ptr(), pNewBuf->size() );
            }
            break;
        }
    case DataTypeObjPtr:
        {
            BufPtr pNewBuf( true );
            if( ERROR( ret ) )
                break;

            ObjPtr& pObj = *( ObjPtr* )ptr();
            if( pObj.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ret = pObj->Serialize( *pNewBuf );
            if( ERROR( ret ) )
                break;

            toBuf.Resize( sizeof( SERI_HEADER ) + pNewBuf->size() );
            pheader = reinterpret_cast< SERI_HEADER* >( toBuf.ptr() );
            oHeader.dwClsid = clsid( CBuffer );
            oHeader.dwSize = pNewBuf->size();
            oHeader.dwType = m_dwType;
            oHeader.dwObjClsid = ( guint32 )pObj->GetClsid();

            oHeader.hton();
            *pheader = oHeader;

            if( pNewBuf->size() > 0 )
            {
                memcpy( ( void* )&pheader[ 1 ],
                    pNewBuf->ptr(), pNewBuf->size() );
            }

            break;
        }
    case DataTypeMem:
        {
            toBuf.Resize( sizeof( SERI_HEADER ) + size() );
            pheader = reinterpret_cast< SERI_HEADER* >( toBuf.ptr() );
            oHeader.dwClsid = clsid( CBuffer );
            oHeader.dwSize = size();
            oHeader.dwType = m_dwType;

            oHeader.hton();
            *pheader = oHeader;

            ret = SerializePrimeType( ( guint8* )&pheader[ 1 ] );

            break;
        }

    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

gint32 CBuffer::SerializePrimeType( guint8* pDest ) const
{
    gint32 ret = 0;
    if( pDest == nullptr )
        return -EINVAL;

    switch( GetExDataType() )
    {
    case typeNone:
    case typeByteArr:
        {
            memcpy( pDest, ptr(), size() );
            break;
        }
    case typeByte: 
        {
            *pDest = *ptr();
            break;
        }
    case typeUInt16:
        {
            *( guint16* )pDest =
                htons( *( guint16* )ptr() );
            break;
        }
    case typeUInt32:
    case typeFloat:
        {
            *( guint32* )pDest =
                htonl( *( guint32* )ptr() );
            break;
        }
    case typeUInt64:
    case typeDouble:
        {
            *( guint64* )pDest =
                htonll( *( guint64* )ptr() );
            break;
        }
    case typeString:
        {
            strncpy( ( char* )pDest, ptr(), size() );
            pDest[ size() - 1 ] = 0;
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }
    return ret;
}

gint32 CBuffer::DeserializePrimeType( guint8* pSrc )
{
    if( pSrc == nullptr  )
        return -EINVAL;

    gint32 ret = 0;
    switch( GetExDataType() )
    {
    case typeNone:
    case typeByteArr:
        {
            memcpy( ptr(), pSrc, size() );
            break;
        }
    case typeByte: 
        {
            *ptr() = *pSrc;
            break;
        }
    case typeUInt16:
        {
            *( guint16* )ptr() =
                ntohs( *( guint16* )pSrc );
            break;
        }
    case typeUInt32:
    case typeFloat:
        {
            *( guint32* )ptr() =
                ntohl( *( guint32* )pSrc );
            break;
        }
    case typeUInt64:
    case typeDouble:
        {
            *( guint64* )ptr() =
                htonll( *( guint64* )pSrc );
            break;
        }
    case typeString:
        {
            strncpy( ptr(), ( char* )pSrc, size() );
            ptr()[ size() - 1 ] = 0;
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }
    return ret;
}

gint32 CBuffer::Deserialize( const CBuffer& oBuf )
{
    // if( oBuf.GetDataType() != DataTypeMem )
    //     return -EINVAL;

    return Deserialize( oBuf.ptr() );
}

gint32 CBuffer::Deserialize( const char* pBuf )
{
    const SERI_HEADER* pheader =
        ( const SERI_HEADER* )pBuf;

    gint32 ret = 0;
    SERI_HEADER oHeader( *pheader );
    // memcpy( &oHeader, pheader, sizeof( oHeader ) );
    oHeader.ntoh();

    if( oHeader.dwClsid != clsid( CBuffer ) )
    {
        return -EINVAL;
    }

    if( oHeader.dwSize > BUF_MAX_SIZE )
    {
        return -EINVAL;
    }

    switch( oHeader.dwType & DATATYPE_MASK )
    {
    case DataTypeMem:
        {
            // this is an empty buffer, we are OK
            // with it
            if( oHeader.dwSize == 0 )
            {
                Resize( 0 );
                break;
            }

            Resize( oHeader.dwSize );
            if( ptr() == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            m_dwType = oHeader.dwType;
            ret = DeserializePrimeType(
                ( guint8* )( pheader + 1 ) );

            break;
        }
    case DataTypeObjPtr:
        {
            Resize( sizeof( ObjPtr ) );
            if( ptr() == nullptr )
            {
                // invalid buffer
                ret = -ENOMEM;
                break;
            }

            new ( ptr() )ObjPtr();
            SetDataType( DataTypeObjPtr );
            SetExDataType( typeObj );

            ObjPtr& ptrObj = *reinterpret_cast< ObjPtr* >( ptr() );


            if( oHeader.dwSize > 0
                && oHeader.dwSize <= BUF_MAX_SIZE )
            {
                // create the object pointed to by the ptr object
                ret = ptrObj.NewObj( ( EnumClsid )oHeader.dwObjClsid );
                if( ret )
                    break;

                CBuffer a( (char*) &pheader[ 1 ], oHeader.dwSize );
                ret = ptrObj->Deserialize( a );
            }
            else
            {
                Resize( 0 );
                ret = -ERANGE;
            }

            break;
        }
    case DataTypeMsgPtr:
        {
            Resize( sizeof( DMsgPtr ) );
            if( ptr() == nullptr )
            {
                // invalid buffer
                ret = -ENOMEM;
                break;
            }

            new ( ptr() )DMsgPtr();

            SetDataType( DataTypeMsgPtr );
            SetExDataType( typeDMsg );

            DMsgPtr& pMsg = *reinterpret_cast< DMsgPtr* >( ptr() );

            if( oHeader.dwSize > 0 &&
                oHeader.dwSize <= BUF_MAX_SIZE )
            {
                // create the object pointed to by the ptr object
                CBuffer a( (char*) &pheader[ 1 ], oHeader.dwSize );
                ret = pMsg.Deserialize( a );
            }
            else
            {
                Resize( 0 );
                ret = -ERANGE;
            }

            break;
        }
    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

bool CBuffer::operator==( const CBuffer& rhs ) const
{
    if( size() != rhs.size()  
        || GetDataType() != rhs.GetDataType() )
        return false;

    bool ret = false;
    switch( GetDataType() )
    {
    case DataTypeMem:
        {
            if( bcmp( ptr(), rhs.ptr(), size() ) == 0 )
                ret = true;
            break;
        }
    case DataTypeObjPtr:
        {
            ObjPtr& pObj1 = *reinterpret_cast< ObjPtr* >( ptr() );
            ObjPtr& pObj2 = *reinterpret_cast< ObjPtr* >( rhs.ptr() );
            ret = ( pObj1 == pObj2 );
            break;
        }
    case DataTypeMsgPtr:
        {
            DMsgPtr& pDMsg1 = *reinterpret_cast< DMsgPtr* >( ptr() );
            DMsgPtr& pDMsg2 = *reinterpret_cast< DMsgPtr* >( rhs.ptr() );
            ret = ( pDMsg1 == pDMsg2 );
            if( ptr() == rhs.ptr() )
                ret = true;
            break;
        }
    default:
        {
            break;
        }

    }
    return ret;
}

void CBuffer::SetDataType( EnumDataType dt )
{
    m_dwType = ( m_dwType & ~DATATYPE_MASK )|
        ( dt & DATATYPE_MASK );
}

EnumDataType CBuffer::GetDataType() const
{
    return ( EnumDataType )type();
}

void CBuffer::SetExDataType( EnumTypeId dt )
{
    m_dwType = ( m_dwType & ~DATATYPE_MASK_EX ) |
        ( ( dt << 4 ) & DATATYPE_MASK_EX );
}

EnumTypeId CBuffer::GetExDataType() const
{
    return ( EnumTypeId )
        ( ( m_dwType & DATATYPE_MASK_EX ) >> 4 );
}

gint32 CBuffer::Append(
    const guint8* pBlock, guint32 dwSize )
{
    if( GetDataType() != DataTypeMem )
        return -EINVAL;

    if( pBlock == nullptr ||
        dwSize > BUF_MAX_SIZE ||
        dwSize == 0 )
        return -EINVAL;

    guint32 dwOldSize = size();
    Resize( size() + dwSize );
    memcpy( ptr() + dwOldSize, pBlock, dwSize );
    SetExDataType( typeByteArr );
    return 0;
}
