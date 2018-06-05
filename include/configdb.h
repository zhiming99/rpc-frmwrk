/*
 * =====================================================================================
 *
 *       Filename:  configdb.h
 *
 *    Description:  declaration of the IConfigDb and CConfigDb
 *
 *        Version:  1.0
 *        Created:  07/21/2016 09:46:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */
#pragma once
#include <arpa/inet.h>

#include "autoptr.h"
#include "buffer.h"
#include <stdexcept>

class CIoManager;
extern const BufPtr g_pNullBuf;

class IConfigDb : public CObjBase
{

    public:
    typedef CObjBase    super;
    virtual void BuildConfig( const std::string& strJson ) = 0;
    virtual void BuildConfig(
        const std::map<gint32, std::string>& strPropValues ) = 0;

    virtual gint32 RemoveProperty( gint32 iProp ) = 0;
    virtual void RemoveAll() = 0;

    virtual const CBuffer& operator[]( gint32 iProp ) const = 0;
    virtual CBuffer& operator[]( gint32 iProp ) = 0;

    virtual gint32 size() const = 0;
    virtual bool exist( gint32 iProp ) const = 0;
    virtual const IConfigDb& operator=( const IConfigDb& oCfg ) = 0;

    virtual gint32 SetProperty( gint32 iProp, const BufPtr& pBuf ) = 0;
    virtual gint32 GetProperty( gint32 iProp, BufPtr& pBuf ) const = 0;

    // defined in CObjBase
    virtual gint32 GetProperty( gint32 iProp, CBuffer& oBuf ) const = 0;
    virtual gint32 SetProperty( gint32 iProp, const CBuffer& oProp ) = 0;
    virtual gint32 GetPropertyType( gint32 iProp, gint32& iType ) const = 0;

    virtual gint32 EnumProperties( std::vector< gint32 >& vecProps ) const = 0;

    // more helpers
    virtual gint32 Clone( const IConfigDb& oCfg ) = 0;
};

template< class T1, typename T=
    typename std::enable_if< std::is_base_of< CObjBase, T1 >::value, T1 >::type >
class CCfgDbOpener
{
    protected:

    T*          m_pCfg;
    const T*    m_pConstCfg;

    public:
    CCfgDbOpener( T* pObj )
    {
        m_pCfg = nullptr;
        m_pConstCfg = nullptr;

        if( pObj == nullptr )
        {
            return;
        }

        m_pConstCfg = pObj;
        m_pCfg = pObj;
    }

    CCfgDbOpener( const T* pObj )
    {
        m_pCfg = nullptr;
        m_pConstCfg = nullptr;

        if( pObj == nullptr )
        {
            return;
        }

        m_pConstCfg = pObj;
        m_pCfg = nullptr;
    }

    gint32 GetProperty(
        gint32 iProp, CBuffer& oBuf ) const
    {
        if( m_pConstCfg == nullptr )
            return -EFAULT;
        return m_pConstCfg->GetProperty( iProp, oBuf );
    }

    gint32 SetProperty(
        gint32 iProp, const CBuffer& oBuf )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->SetProperty( iProp, oBuf );
    }

    
    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 GetPointer( gint32 iProp, U*& pObj ) const
    {
        gint32 ret = 0;

        do{
            ObjPtr objPtr;

            ret = GetObjPtr( propIoMgr, objPtr );

            if( ERROR( ret ) )
                break;

            pObj = objPtr;

            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }

        }while( 0 );

        return ret;
    }

    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 SetPointer( gint32 iProp, U* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }

            ObjPtr objPtr( pObj );

            ret = SetObjPtr( iProp, objPtr );

        }while( 0 );

        return ret;
    }

    gint32 GetObjPtr( gint32 iProp, ObjPtr& pObj ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr( true ); 

            ret = m_pConstCfg->GetProperty(
                iProp, *bufPtr );

            if( ERROR( ret ) )
                break;

            try{
                pObj = ( ObjPtr& )*bufPtr;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetObjPtr( gint32 iProp, const ObjPtr& pObj )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr bufPtr( true ); 
            if( !pObj.IsEmpty() )
                *bufPtr = pObj;

            ret = m_pCfg->SetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;
        
        }while( 0 );

        return ret;
    }

    gint32 GetStrProp( gint32 iProp, std::string& strVal ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr( true );
            ret = m_pConstCfg->GetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;

            //FIXME: CBuffer does not have
            //a proper type cast to convert this, so
            //we use the rogue approach for temporary
            //workaround
            strVal = static_cast< stdstr >( *bufPtr );

        }while( 0 );

        return ret;
    }

    gint32 SetStrProp( gint32 iProp, const std::string& strVal )
    {

        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            BufPtr bufPtr( true );
            *bufPtr = strVal;
            ret = m_pCfg->SetProperty( iProp, *bufPtr );

        }while( 0 );

        return ret;
    }

    gint32 GetStringProp( gint32 iProp, const char*& pStr ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr( true );
            ret = m_pConstCfg->GetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;
            pStr = bufPtr->ptr();
        }while( 0 );

        return ret;
    }

    gint32 GetMsgPtr( gint32 iProp, DMsgPtr& pMsg ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr( true ); 

            ret = m_pConstCfg->GetProperty(
                iProp, *bufPtr );

            if( ERROR( ret ) )
                break;

            try{
                pMsg = ( DMsgPtr& )*bufPtr;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetMsgPtr( gint32 iProp, const DMsgPtr& pMsg )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr bufPtr( true ); 
            *bufPtr = pMsg;

            ret = m_pCfg->SetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;
        
        }while( 0 );

        return ret;
    }

    gint32 CopyProp( gint32 iProp, const CObjBase* pSrcCfg )
    {
        CCfgDbOpener< CObjBase > a( pSrcCfg );

        BufPtr pBuf( true );
        gint32 ret = a.GetProperty( iProp, *pBuf );

        if( ERROR( ret ) )
            return ret;

        return SetProperty( iProp, *pBuf );
    }

    gint32 MoveProp( gint32 iProp, CObjBase* pSrcCfg )
    {
        gint32 ret = CopyProp( iProp, pSrcCfg );
        if( ERROR( ret ) )
            return ret;

        ret = pSrcCfg->RemoveProperty( iProp );
        return ret;
    }

    gint32 SwapProp( gint32 iProp1, gint32 iProp2 )
    {
        BufPtr pBuf1( true );
        gint32 ret = GetProperty( iProp1, *pBuf1 );
        if( ERROR( ret ) )
            return ret;

        BufPtr pBuf2( true );
        ret = GetProperty( iProp2, *pBuf2 );
        if( ERROR( ret ) )
            return ret;

        SetProperty( iProp1, *pBuf2 );
        SetProperty( iProp2, *pBuf1 );
        return ret;
    }

    template< typename PrimType >
    gint32 GetPrimProp( gint32 iProp, PrimType& val ) const 
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr( true );

            ret = m_pConstCfg->GetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;

            val = ( PrimType& )( *bufPtr );

        }while( 0 );

        return ret;
    }

    template< typename PrimType >
    gint32 SetPrimProp( gint32 iProp, PrimType val )
    {
        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr bufPtr( true );
            *bufPtr = val;

            ret = m_pCfg->SetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }
    gint32 GetIntPtr( gint32 iProp, guint32*& val ) const
    {
        guint32 val1;
        gint32 ret = GetIntProp( iProp, val1 );
        if( ERROR( ret ) )
            return ret;
        val = reinterpret_cast< guint32* >( val1 );
        return 0;
    }
    gint32 SetIntPtr( gint32 iProp, guint32* val )
    {
        return SetIntProp( iProp, ( guint32 )val );
    }

    gint32 GetShortProp( gint32 iProp, guint16& val ) const
    {
        guint16 sval = 0;
        gint32 ret = GetPrimProp( iProp, sval );
        if( ERROR( ret ) )
            return ret;
        val = ntohs( sval );
        return 0;
    }

    gint32 SetShortProp( gint32 iProp, guint16 val )
    {
        return SetPrimProp( iProp, htons( val ) );
    }

    gint32 GetIntProp( gint32 iProp, guint32& val ) const
    {
        gint32 iVal = 0;
        gint32 ret = GetPrimProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;
        val = ntohl( iVal );
        return 0;
    }

    gint32 SetIntProp( gint32 iProp, guint32 val )
    {
        return SetPrimProp( iProp, htonl( val ) );
    }
    gint32 GetBoolProp( gint32 iProp, bool& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetBoolProp( gint32 iProp, bool val )
    {
        return SetPrimProp( iProp, val );
    }

    gint32 GetByteProp( gint32 iProp, guint8& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetByteProp( gint32 iProp, guint8 val )
    {
        return SetPrimProp( iProp, val );
    }
    gint32 GetFloatProp( gint32 iProp, float& val ) const
    {
        guint32 iVal = 0;
        gint32 ret = GetIntProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;

        float* pfVal = ( float* )&iVal;
        val = *pfVal;
        return 0;
    }
    gint32 SetFloatProp( gint32 iProp, float val )
    {
        guint32 iVal = *( gint32* )&val;
        return SetIntProp( iProp, iVal );
    }

    gint32 GetQwordProp( gint32 iProp, guint64& val ) const
    {
        guint64 iVal = 0;
        gint32 ret = GetPrimProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;
        val = ntohll( iVal );
        return 0;
    }

    gint32 SetQwordProp( gint32 iProp, guint64 val )
    {
        return SetPrimProp( iProp, htonll( val ) );
    }

    gint32 GetDoubleProp( gint32 iProp, double& val ) const
    {
        guint64 iVal = 0;
        gint32 ret = GetQwordProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;

        double* pdbVal = ( double* )&iVal;
        val = *( double* )pdbVal;
        return 0;
    }
    gint32 SetDoubleProp( gint32 iProp, double val )
    {
        guint64 iVal = *( gint64* )&val;
        return SetQwordProp( iProp, val );
    }

    template< typename PrimType >
    gint32 IsEqual( gint32 iProp, PrimType val )
    {
        // c++11 required
        // static_assert( !std::is_base_of< CObjBase, PrimType >::value );
        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = m_pCfg->GetProperty( iProp, *pBuf );
            if( ERROR( ret ) )
                break;

            BufPtr pBuf2( true );
            *pBuf2 = val;

            if( *pBuf == *pBuf2 )
                break;

            ret = ERROR_FALSE;
        }while( 0 );

        return ret;
    }

    gint32 IsEqualProp( gint32 iProp, CObjBase* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = pObj->GetProperty( iProp, *pBuf );
            if( ERROR( ret ) )
                break;

            BufPtr pBuf2( true );
            ret = this->GetProperty( iProp, *pBuf2 );
            if( ERROR( ret ) )
                break;

            if( *pBuf == *pBuf2 )
                break;

            ret = ERROR_FALSE;

        }while( 0 );
        return ret;
    }
};

// template<> class CCfgDbOpener< CObjBase >;

template<> 
class CCfgDbOpener< IConfigDb >
{
    protected:

    IConfigDb*          m_pCfg;
    const IConfigDb*    m_pConstCfg;

    public:
    CCfgDbOpener( IConfigDb* pObj )
        : m_pCfg( nullptr ),
        m_pConstCfg( nullptr )
    {
        if( pObj == nullptr )
        {
            return;
        }

        m_pConstCfg = pObj;
        m_pCfg = pObj;
    }

    CCfgDbOpener( const IConfigDb* pObj )
        : m_pCfg( nullptr ),
        m_pConstCfg( nullptr )
    {
        if( pObj == nullptr )
            return;

        m_pConstCfg = pObj;
        m_pCfg = nullptr;
    }

    gint32 GetProperty(
        gint32 iProp, BufPtr& pBuf ) const
    {
        if( m_pConstCfg == nullptr )
            return -EFAULT;
        return m_pConstCfg->GetProperty( iProp, pBuf );
    }

    gint32 SetProperty(
        gint32 iProp, const BufPtr& pBuf )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->SetProperty( iProp, pBuf );
    }

    gint32 GetProperty(
        gint32 iProp, CBuffer& oBuf ) const
    {
        if( m_pConstCfg == nullptr )
            return -EFAULT;
        return m_pConstCfg->GetProperty( iProp, oBuf );
    }

    gint32 SetProperty(
        gint32 iProp, const CBuffer& oBuf )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->SetProperty( iProp, oBuf );
    }

    const CBuffer& operator[]( gint32 iProp ) const
    {
        BufPtr pBuf;
        gint32 ret = m_pCfg->GetProperty(
            iProp, pBuf );

        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                DebugMsg( ret, "Property not found" ) );
        }
        return *pBuf;
    }

    CBuffer& operator[]( EnumPropId iProp )
    {
        BufPtr pBuf;
        gint32 ret = m_pCfg->GetProperty(
            iProp, pBuf );

        if( ERROR( ret ) )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
            {
                throw std::runtime_error(
                    DebugMsg( ret, "No Memory" ) );
            }
            m_pCfg->SetProperty( iProp, pBuf );
        }
        return *pBuf;
    }

    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 GetPointer( gint32 iProp, U*& pObj ) const
    {
        gint32 ret = 0;

        do{
            ObjPtr objPtr;

            ret = GetObjPtr( propIoMgr, objPtr );

            if( ERROR( ret ) )
                break;

            pObj = objPtr;

            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }

        }while( 0 );

        return ret;
    }

    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 SetPointer( gint32 iProp, U* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }

            ObjPtr objPtr( pObj );

            ret = SetObjPtr( iProp, objPtr );

        }while( 0 );

        return ret;
    }

    gint32 GetObjPtr( gint32 iProp, ObjPtr& pObj ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr; 

            ret = m_pConstCfg->GetProperty(
                iProp, bufPtr );

            if( ERROR( ret ) )
                break;

            try{
                pObj = ( ObjPtr& )*bufPtr;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetObjPtr( gint32 iProp, const ObjPtr& pObj )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr bufPtr( true ); 
            if( !pObj.IsEmpty() )
                *bufPtr = pObj;

            ret = m_pCfg->SetProperty( iProp, bufPtr );
            if( ERROR( ret ) )
                break;
        
        }while( 0 );

        return ret;
    }

    gint32 GetStrProp( gint32 iProp, std::string& strVal ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr;
            ret = m_pConstCfg->GetProperty( iProp, bufPtr );
            if( ERROR( ret ) )
                break;

            strVal = ( stdstr )( *bufPtr );

        }while( 0 );

        return ret;
    }

    gint32 SetStrProp( gint32 iProp, const std::string& strVal )
    {

        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            BufPtr bufPtr( true );
            *bufPtr = strVal;
            ret = m_pCfg->SetProperty( iProp, bufPtr );

        }while( 0 );

        return ret;
    }

    gint32 GetStringProp( gint32 iProp, const char*& pStr ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            const BufPtr bufPtr;
            ret = m_pConstCfg->GetProperty( iProp, *bufPtr );
            if( ERROR( ret ) )
                break;
            pStr = bufPtr->ptr();
        }while( 0 );

        return ret;
    }

    gint32 GetMsgPtr( gint32 iProp, DMsgPtr& pMsg ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr( true ); 

            ret = m_pConstCfg->GetProperty(
                iProp, *bufPtr );

            if( ERROR( ret ) )
                break;

            try{
                pMsg = ( DMsgPtr& )*bufPtr;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetMsgPtr( gint32 iProp, const DMsgPtr& pMsg )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr bufPtr( true ); 
            *bufPtr = pMsg;

            ret = m_pCfg->SetProperty( iProp, bufPtr );
            if( ERROR( ret ) )
                break;
        
        }while( 0 );

        return ret;
    }

    gint32 CopyProp( gint32 iProp, const CObjBase* pSrcCfg )
    {
        CCfgDbOpener< CObjBase > a( pSrcCfg );

        BufPtr pBuf( true );
        gint32 ret = a.GetProperty( iProp, *pBuf );

        if( ERROR( ret ) )
            return ret;

        return SetProperty( iProp, pBuf );
    }

    gint32 MoveProp( gint32 iProp, CObjBase* pSrcCfg )
    {
        gint32 ret = CopyProp( iProp, pSrcCfg );
        if( ERROR( ret ) )
            return ret;

        ret = pSrcCfg->RemoveProperty( iProp );
        return ret;
    }

    gint32 SwapProp( gint32 iProp1, gint32 iProp2 )
    {
        BufPtr pBuf1;
        gint32 ret = GetProperty( iProp1, pBuf1 );
        if( ERROR( ret ) )
            return ret;

        BufPtr pBuf2;
        ret = GetProperty( iProp2, pBuf2 );
        if( ERROR( ret ) )
            return ret;

        SetProperty( iProp1, pBuf2 );
        SetProperty( iProp2, pBuf1 );
        return ret;
    }

    template< typename PrimType >
    gint32 GetPrimProp( gint32 iProp, PrimType& val ) const 
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            BufPtr bufPtr;

            ret = m_pConstCfg->GetProperty( iProp, bufPtr );
            if( ERROR( ret ) )
                break;

            val = ( PrimType& )( *bufPtr );

        }while( 0 );

        return ret;
    }

    template< typename PrimType >
    gint32 SetPrimProp( gint32 iProp, PrimType val )
    {
        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr bufPtr( true );
            *bufPtr = val;

            ret = m_pCfg->SetProperty( iProp, bufPtr );
            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }
    gint32 GetIntPtr( gint32 iProp, guint32*& val ) const
    {
        guint32 val1;
        gint32 ret = GetIntProp( iProp, val1 );
        if( ERROR( ret ) )
            return ret;
        val = reinterpret_cast< guint32* >( val1 );
        return 0;
    }
    gint32 SetIntPtr( gint32 iProp, guint32* val )
    {
        return SetIntProp( iProp, ( guint32 )val );
    }

    gint32 GetShortProp( gint32 iProp, guint16& val ) const
    {
        guint16 sval = 0;
        gint32 ret = GetPrimProp( iProp, sval );
        if( ERROR( ret ) )
            return ret;
        val = ntohs( sval );
        return 0;
    }

    gint32 SetShortProp( gint32 iProp, guint16 val )
    {
        return SetPrimProp( iProp, htons( val ) );
    }

    gint32 GetIntProp( gint32 iProp, guint32& val ) const
    {
        gint32 iVal = 0;
        gint32 ret = GetPrimProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;
        val = ntohl( iVal );
        return 0;
    }

    gint32 SetIntProp( gint32 iProp, guint32 val )
    {
        return SetPrimProp( iProp, htonl( val ) );
    }
    gint32 GetBoolProp( gint32 iProp, bool& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetBoolProp( gint32 iProp, bool val )
    {
        return SetPrimProp( iProp, val );
    }

    gint32 GetByteProp( gint32 iProp, guint8& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetByteProp( gint32 iProp, guint8 val )
    {
        return SetPrimProp( iProp, val );
    }
    gint32 GetFloatProp( gint32 iProp, float& val ) const
    {
        guint32 iVal = 0;
        gint32 ret = GetIntProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;

        float* pfVal = ( float* )&iVal;
        val = *pfVal;
        return 0;
    }
    gint32 SetFloatProp( gint32 iProp, float val )
    {
        guint32* pVal = ( guint32* )&val;
        guint32 iVal = *pVal;
        return SetIntProp( iProp, iVal );
    }

    gint32 GetQwordProp( gint32 iProp, guint64& val ) const
    {
        guint64 iVal = 0;
        gint32 ret = GetPrimProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;
        val = ntohll( iVal );
        return 0;
    }

    gint32 SetQwordProp( gint32 iProp, guint64 val )
    {
        return SetPrimProp( iProp, htonll( val ) );
    }

    gint32 GetDoubleProp( gint32 iProp, double& val ) const
    {
        guint64 iVal = 0;
        gint32 ret = GetQwordProp( iProp, iVal );
        if( ERROR( ret ) )
            return ret;

        double* pdbVal = ( double* )&iVal;
        val = *( double* )pdbVal;
        return 0;
    }
    gint32 SetDoubleProp( gint32 iProp, double val )
    {
        guint64* pVal = ( guint64* )&val;
        guint64 iVal = *pVal;
        return SetQwordProp( iProp, iVal );
    }

    template< typename PrimType >
    gint32 IsEqual( gint32 iProp, PrimType val )
    {
        // c++11 required
        // static_assert( !std::is_base_of< CObjBase, PrimType >::value );
        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf;
            ret = m_pCfg->GetProperty( iProp, pBuf );
            if( ERROR( ret ) )
                break;

            BufPtr pBuf2;
            *pBuf2 = val;

            if( *pBuf == *pBuf2 )
                break;

            ret = ERROR_FALSE;
        }while( 0 );

        return ret;
    }

    gint32 IsEqualProp( gint32 iProp, CObjBase* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = pObj->GetProperty( iProp, *pBuf );
            if( ERROR( ret ) )
                break;

            BufPtr pBuf2;
            ret = this->GetProperty( iProp, pBuf2 );
            if( ERROR( ret ) )
                break;

            if( *pBuf == *pBuf2 )
                break;

            ret = ERROR_FALSE;

        }while( 0 );
        return ret;
    }
};

template< typename T >
class CCfgOpenerT : public CCfgDbOpener< T >
{
    protected:
    CfgPtr m_pNewCfg;

    public:

    typedef CCfgDbOpener< T > super;
    using super::m_pCfg;
    using super:: m_pConstCfg;

    CCfgOpenerT( T* pCfg )
        :CCfgDbOpener< T >( pCfg )
    {

    }

    CCfgOpenerT( const T* pCfg )
        :CCfgDbOpener< T >( pCfg )
    {

    }

    CCfgOpenerT() :
        CCfgDbOpener< T >( ( T* )nullptr ),
        m_pNewCfg( true )
    {
        m_pCfg = m_pNewCfg;
        m_pConstCfg = m_pNewCfg;
    }

    CfgPtr& GetCfg() const
    {
        return const_cast< CfgPtr& >( m_pNewCfg );
    }

    void Clear()
    {
        if( !m_pNewCfg.IsEmpty() )
        {
            m_pNewCfg->RemoveAll();
        }
    }

    gint32 RemoveProperty( gint32 iPropId )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->RemoveProperty( iPropId );
    }

    bool exist( gint32 iPropId ) const
    {
        if( m_pCfg != nullptr
            && m_pCfg->GetClsid() == clsid( CConfigDb ) )
        {
            return static_cast< IConfigDb* >
                ( m_pCfg )->exist( iPropId );
        }
        return false;
    }
};

typedef CCfgOpenerT< IConfigDb > CCfgOpener;
typedef CCfgOpenerT< CObjBase > CCfgOpenerObj;

class CParamList : public CCfgOpener
{
    void InitParamList()
    {
        if( !GetCfg()->exist( propParamCount ) )
            SetIntProp( propParamCount, 0 );
    }

    public:
    typedef CCfgOpener  super;

    CParamList()
        :CCfgOpener()
    {
        // this is for push/pop to a new cfg
        gint32 ret = GetCfg().IsEmpty();

        if( ERROR( ret ) )
            return;

        InitParamList();
    }

    ~CParamList()
    {
        // we cannot remove the propParamCount
        // because the m_pCfg will be passed on to
        // other users
    }

    CParamList( IConfigDb* pCfg )
    {
        // this is for pop an existing cfg you
        // should make sure the properties are
        // strictly numbered from 0~(size-1) or an
        // earlier cfg generated from this class
        m_pNewCfg = pCfg;
        m_pCfg = m_pNewCfg;
        m_pConstCfg = m_pNewCfg;

        if( !GetCfg().IsEmpty() )
        {
            InitParamList();
        }
    }

    gint32 CopyParams(
        const IConfigDb* pCfg )
    {
        CCfgOpener oCfg( pCfg );
        gint32 ret = 0;

        do{
            guint32 dwSize = 0;
            ret =  GetIntProp( propParamCount, dwSize );

            if( ERROR( ret ) )
                break;

            for( guint32 i = 0; i < dwSize; ++i )
            {
                ret = CopyProp( i, pCfg );
                if( ERROR( ret ) )
                    break;
            }
            SetIntProp( propParamCount, dwSize );

        }while ( 0 );
        
        return ret;
    }

    gint32 Append( IConfigDb* pCfg )
    {
        if( pCfg == nullptr )
            return -EINVAL;

        std::vector< gint32 > vecProps;
        gint32 ret = pCfg->EnumProperties( vecProps );
        if( ERROR( ret ) )
            return ret;

        for( auto iProp : vecProps )
        {
            ret = CopyProp( iProp, pCfg );
            if( ERROR( ret ) )
                break;
        }
        return ret;
    }

    gint32 ClearParams()
    {
        guint32 dwSize = 0;

        gint32 ret = GetSize( dwSize );
        if( ERROR( ret ) )
            return ret;

        for( guint32 i = 0; i < dwSize; ++i )
        {
            RemoveProperty( i );
        }
        SetIntProp( propParamCount, 0 );

        return 0;
    }

    inline gint32 GetSize( guint32 &dwSize )
    {
        return GetIntProp(
            propParamCount, dwSize );
    }

    gint32 GetPos()
    {
        gint32 ret = 0;
        gint32 iPos = 0;

        if( GetCfg().IsEmpty() )
            return -EFAULT;

        ret = GetIntProp(
            propParamCount, ( guint32& )iPos );

        if( ERROR( ret ) )
        {
            return ret;
        }
        return iPos - 1;
    }

    gint32 SetPos( gint32 iPos )
    {
        if( iPos < -1 )
            return -EINVAL;

        if( GetCfg().IsEmpty() )
            return -EFAULT;

        gint32 ret = SetIntProp( propParamCount,
            ( guint32& )iPos + 1 );

        if( ERROR( ret ) )
        {
            return ret;
        }
        return 0;
    }

    gint32 GetType( gint32 iPos, gint32& iType )
    {
        BufPtr pBuf( true );
        gint32 ret = GetCfg()->
            GetPropertyType( iPos, iType );

        return ret;
    }

    gint32 IncDecPos( bool bInc )
    {
        gint32 ret = 0;
        gint32 iPos = 0;

        if( GetCfg().IsEmpty() )
            return -EFAULT;

        if( !GetCfg()->exist( propParamCount ) )
        {
            ret = SetIntProp( propParamCount, 0 );
            if( ERROR( ret ) )
                return ret;
        }

        ret = GetIntProp(
            propParamCount, ( guint32& )iPos );

        if( ERROR( ret ) )
        {
            return ret;
        }
        if( bInc )
        {
            ++iPos;
        }
        else 
        {
            if( iPos > 0 )
                --iPos;
        }
        SetIntProp( propParamCount, iPos );
        return iPos;
    }

    template< typename T >
    gint32 Push( T&& val  )
    {
        gint32 ret = 0;
        do{
            gint32 iPos = GetPos();
            if( iPos != -1 && ERROR( iPos ) )
            {
                ret = iPos;
                break;
            }

            ++iPos;
            BufPtr pBuf( true );

            try{
                *pBuf = val;

                ret = GetCfg()->SetProperty(
                    iPos, *pBuf );

                if( SUCCEEDED( ret ) )
                    SetPos( iPos );
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }

        }while( 0 );

        return ret;
    }


    template< typename T >
    gint32 Pop( T& val )
    {
        gint32 ret = 0;
        do{
            if( IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }

            gint32 iPos = ( gint32 )GetPos();
            if( iPos < 0 )
            {
                ret = -ENOENT;
                break;
            }

            BufPtr pBuf( true );
            try{
                ret = GetCfg()->GetProperty(
                    iPos, *pBuf );

                if( ERROR( ret ) )
                    break;

                val = ( T& )( *pBuf );
                ret = GetCfg()->RemoveProperty( iPos );
                --iPos;

                if( SUCCEEDED( ret ) )
                {
                    SetPos( iPos );
                }
                else
                {
                    break;
                }
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }

        }while( 0 );

        return ret;
    }

    bool IsEmpty()
    {
        guint32 dwSize = 0;
        gint32 ret = GetSize( dwSize );

        if( ERROR( ret ) )
            return true;

        return ( dwSize == 0 );
    }
};

#define CFGDB_MAX_SIZE  ( 16 * 1024 * 1024 )
#define CFGDB_MAX_ITEM  1024

class CConfigDb : public IConfigDb
{
    protected:
    std::map<gint32, BufPtr> m_mapProps;

    public:
    typedef IConfigDb   super;

    struct SERI_HEADER : public SERI_HEADER_BASE
    {
        typedef SERI_HEADER_BASE super;
        guint32 dwCount;

        SERI_HEADER() : SERI_HEADER_BASE()
        {
            dwClsid = htonl( clsid( CConfigDb ) );
            dwCount = 0;
        }

        SERI_HEADER( const SERI_HEADER& rhs )
            :SERI_HEADER_BASE( rhs )
        {
            dwCount = rhs.dwCount;
        }

        SERI_HEADER& operator=( const SERI_HEADER& rhs )
        {
            super::operator=( rhs );
            dwCount = rhs.dwCount;
            return *this;
        }

        void ntoh()
        {
            super::ntoh();
            dwCount = ntohl( dwCount );
        }

        void hton()
        {
            super::hton();
            dwCount = htonl( dwCount );
        }
    };

    CConfigDb( const IConfigDb* pCfg = nullptr );
    ~CConfigDb();

    void BuildConfig( const std::string& strJson );
    void BuildConfig( const std::map<gint32, std::string>& strPropValues );


    gint32 SetProperty( gint32 iProp, const BufPtr& pBuf );
    gint32 GetProperty( gint32 iProp, BufPtr& pBuf ) const;

    gint32 GetProperty( gint32, CBuffer& oBuf ) const;
    gint32 SetProperty( gint32, const CBuffer& oProp );
    gint32 GetPropertyType( gint32 iProp, gint32& iType ) const;
    gint32 RemoveProperty( gint32 iProp );
    void RemoveAll();

    gint32 GetPropIds( std::vector<gint32>& vecIds ) const;

    const CBuffer& operator[]( gint32 iProp ) const;
    CBuffer& operator[]( gint32 iProp );

    virtual gint32 Serialize( CBuffer& oBuf ) const;
    virtual gint32 Deserialize( const CBuffer& oBuf );
    virtual gint32 Deserialize( const char* oBuf, guint32 dwSize );

    gint32 size() const
    { return m_mapProps.size(); }

    bool exist( gint32 iProp ) const
    { 
        return m_mapProps.cend() != m_mapProps.find( iProp ) ;
    }

    const IConfigDb& operator=( const IConfigDb& oCfg );
    gint32 Clone( const IConfigDb& oCfg );

    virtual gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const; 
};
