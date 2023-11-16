/*
 * =====================================================================================
 *
 *       Filename:  dmsgptr.cpp
 *
 *    Description:  implementation of DMsgPtr class
 *
 *        Version:  1.0
 *        Created:  06/16/2018 04:13:12 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
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

#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <map>
#include "defines.h"
#include "dmsgptr.h"
#include "buffer.h"
#include "configdb.h"
#include "registry.h"

namespace rpcf
{

std::string DMsgPtr::GetInterface() const
{
    if( IsEmpty() )
        return std::string( "" );

    const char* pszInterface =
        dbus_message_get_interface( m_pObj );

    if( pszInterface == nullptr )
    {
        sched_yield();
        pszInterface =
            dbus_message_get_interface( m_pObj );
        if( pszInterface == nullptr )
            pszInterface = "";
    }

    return std::string( pszInterface );
}

gint32 DMsgPtr::SetInterface(
    const std::string& strInterface )
{
    if( IsEmpty() )
        return -EFAULT; 

    if( !IsValidDBusName( strInterface ) )
        return -EINVAL;

    if( !dbus_message_set_interface(
        m_pObj, strInterface.c_str() ) )
        return -ENOMEM;
       
    return 0; 
}

std::string DMsgPtr::GetPath() const
{
    if( IsEmpty() )
        return std::string( "" );

    const char* pszPath =
        dbus_message_get_path( m_pObj );

    if( pszPath == nullptr )
    {
        sched_yield();
        pszPath =
            dbus_message_get_interface( m_pObj );
        if( pszPath == nullptr )
            pszPath = "";
    }
    return std::string( pszPath );
}

gint32 DMsgPtr::SetPath(
    const std::string& strPath )
{
    if( IsEmpty() )
        return -EFAULT; 

    if( strPath.empty() )
        return -EINVAL;

    if( !IsValidDBusPath( strPath ) )
        return -EINVAL;

    if( !dbus_message_set_path(
        m_pObj, strPath.c_str() ) )
        return -ENOMEM;
       
    return 0; 
}

gint32 DMsgPtr::GetArgs(
    std::vector< ARG_ENTRY >& vecArgs ) const
{
    gint32 ret = 0;
    CDBusError oError;

    DBusMessageIter itr;
    if( !dbus_message_iter_init( m_pObj, &itr ) )
        return 0;

    // let's get the first argument, which should
    do{

        BufPtr pBuf( true );
        if( pBuf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        gint32 iType = DBUS_TYPE_INVALID;

        ret = GetValue( itr, pBuf, iType );

        if( ERROR( ret ) )
            break;

        ARG_ENTRY ae;
        ae.first = iType;
        ae.second = pBuf;
        vecArgs.push_back( ae );

        if( dbus_message_iter_next( &itr ) )
            continue;

        break;

    }while( 1 );

    if( ERROR( ret ) )
        vecArgs.clear();

    return ret;
}

gint32 DMsgPtr::GetIntArgAt( gint32 iIndex, guint32& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;

    gint32 ret = GetArgAt( iIndex,
        pBuf, iType );

    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_UINT32
        && iType != DBUS_TYPE_INT32 )
    {
        ret = -EINVAL;
        return ret;
    }
    val = ( guint32& )*pBuf;
    return 0;
}

gint32 DMsgPtr::GetFdArgAt( gint32 iIndex, gint32& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;

    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_UNIX_FD )
    {
        ret = -EINVAL;
        return ret;
    }
    val = ( gint32& )*pBuf;
    return 0;
}

gint32 DMsgPtr::GetByteArgAt( gint32 iIndex, guint8& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_BYTE )
    {
        ret = -EINVAL;
        return ret;
    }
    val = ( gint8& )*pBuf;
    return 0;
}

gint32 DMsgPtr::GetBoolArgAt( gint32 iIndex, bool& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_BOOLEAN )
    {
        ret = -EINVAL;
        return ret;
    }
    guint32 dwVal = *pBuf;

    if( dwVal == 1 )
        val = true;
    else if( dwVal == 0 )
        val = false;
    else
        ret = -ERANGE;

    return ret;
}

gint32 DMsgPtr::GetDoubleArgAt(
    gint32 iIndex, double& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_DOUBLE )
    {
        ret = -EINVAL;
        return ret;
    }
    val = ( double& )*pBuf;
    return 0;
}

gint32 DMsgPtr::GetInt64ArgAt(
    gint32 iIndex, guint64& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_INT64
        && iType != DBUS_TYPE_UINT64 )
    {
        ret = -EINVAL;
        return ret;
    }
    val = ( guint64& )*pBuf;
    return 0;
}

gint32 DMsgPtr::GetStrArgAt(
    gint32 iIndex, std::string& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_STRING )
    {
        ret = -EINVAL;
        return ret;
    }
    val = *pBuf;
    return 0;
}

gint32 DMsgPtr::GetMsgArgAt(
    gint32 iIndex, DMsgPtr& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_ARRAY )
    {
        ret = -EINVAL;
        return ret;
    }

    DMsgPtr pMsg;
    ret = pMsg.Deserialize( *pBuf );
    if( ERROR( ret ) )
        return ret;

    val = pMsg;

    if( val.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 DMsgPtr::GetObjArgAt(
    gint32 iIndex, ObjPtr& val )
{
    BufPtr pBuf( true );
    gint32 iType = 0;
    gint32 ret = GetArgAt( iIndex, pBuf, iType );
    if( ERROR( ret ) )
        return ret;

    if( iType != DBUS_TYPE_ARRAY )
    {
        ret = -EINVAL;
        return ret;
    }
  
    return DeserializeObj( *pBuf, val );
}

gint32 DMsgPtr::GetArgAt( gint32 iIndex,
    BufPtr& pArg, gint32& iType )
{
    // get one single argument
    gint32 ret = 0;
    CDBusError oError;

    if( IsEmpty() )
        return -EINVAL;

    if( iIndex < 0 || iIndex > DMSG_MAX_ARGS )
        return -EINVAL;

    DBusMessageIter itr;
    if( !dbus_message_iter_init( m_pObj, &itr ) )
        return -ENOENT;

    gint32 i = 0;

    // let's get the first argument, which should
    do{
        if( i < iIndex )
        {
            if( dbus_message_iter_next( &itr ) )
            {
                ++i;
                continue;
            }

            // index exceeds the max args in
            // this message
            ret = -ERANGE;
            break;
        }

        if( i == iIndex )
        {
            BufPtr pBuf( true );
            ret = GetValue( itr, pBuf, iType );
            if( ERROR( ret ) )
            {
                pBuf->Resize( 0 );
                break;
            }

            pArg = pBuf;
        }

        break;
    }while( 1 );

    return ret;
}
/**
* @name GetValue: retrieve the argument at itr from the
* dbus message to the `pBuf', and the data type is
* returned in the `iType'.
* @{ */
/**  @} */

gint32 DMsgPtr::GetValue( DBusMessageIter& itr,
        BufPtr& pBuf, gint32& iType ) const
{
    gint32 ret = 0;

    do{
        iType =
            dbus_message_iter_get_arg_type( &itr );
        
        if( iType == DBUS_TYPE_INVALID )
        {
            ret = -ENOENT;
            break;
        }

        if( iType == DBUS_TYPE_STRING )
        {
            char* pszVal = nullptr;
            dbus_message_iter_get_basic( &itr, &pszVal );
            if( pszVal == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            *pBuf = std::string( pszVal );
        }
        else if( IsBasicType( iType ) )
        {
            pBuf->Resize( DMSG_FIX_TYPE_SIZE );

            if( pBuf->size() != DMSG_FIX_TYPE_SIZE )
            {
                ret = -ENOMEM;
                break;
            }

            dbus_message_iter_get_basic( &itr, pBuf->ptr() );
        }
        else if( iType == DBUS_TYPE_ARRAY )
        {
            gint32 iElemType =
                dbus_message_iter_get_element_type( &itr );

            if( !IsFixedType( iElemType ) )
            {
                ret = -ENOTSUP;
                break;
            }

            guint32 dwElemSize = GetTypeBytes( iElemType );

            DBusMessageIter oArrItr;
            const char* pData = nullptr;
            guint32 dwCount = 0;

            dbus_message_iter_recurse(
                &itr, &oArrItr );

            dbus_message_iter_get_fixed_array(
                &oArrItr, &pData, ( int* )&dwCount);

            if( pData == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            guint32 dwSize = dwCount * dwElemSize;
            pBuf->Resize( dwSize );
            memcpy( pBuf->ptr(), pData, dwSize );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

/**
 * Gets the alignment requirement for the
 * given type; will be 1, 4, or 8.
 *
 * @param typecode the type
 * @returns alignment of 1, 4, or 8
 */
int DMsgPtr::GetTypeBytes( int typecode ) const
{
  switch (typecode)
    {
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_VARIANT:
    case DBUS_TYPE_SIGNATURE:
      return 1;
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
      return 2;
    case DBUS_TYPE_BOOLEAN:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_UNIX_FD:
      /* this stuff is 4 since it starts with
       * a length */
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_ARRAY:
      return 4;
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_DOUBLE:
      /* struct is 8 since it could contain an
       * 8-aligned item and it's simpler to
       * just always align structs to 8; we
       * want the amount of padding in a
       * struct of a given type to be
       * predictable, not location-dependent.
       * DICT_ENTRY is always the same as
       * struct.
       */
    case DBUS_TYPE_STRUCT:
    case DBUS_TYPE_DICT_ENTRY:
      return 8;

    default:
        {
            std::string strMsg =
                DebugMsg( -EINVAL, "Unknown dataType" );
            throw std::invalid_argument( strMsg );
        }
      return 0;
    }
}

gint32 DMsgPtr::Serialize( CBuffer* pBuf )
{
    if( IsEmpty() || pBuf == nullptr )
        return -EINVAL;

    guint32 dwSize = 0;
    guint8 *pData = nullptr;
    gint32 ret = 0;
            
    do{

        if( !dbus_message_marshal(
            m_pObj, ( char** )&pData, ( int* )&dwSize ) )
        {
            ret = -ENOMEM;
            break;
        }

        if( dwSize > DMSG_MAX_SIZE )
        {
            ret = -ENOMEM;
            break;
        }

        pBuf->Resize( 0 );

        guint32 dwNetSize = htonl( dwSize );
        ret = pBuf->Append(
            ( guint8* )&dwNetSize, sizeof( dwNetSize ) );

        if( ERROR( ret ) )
            break;

        ret = pBuf->Append( pData, dwSize );
        if( ERROR( ret ) )
            break;
#ifdef DUMP_DMSG
        do{
            static bool bWritten = false;
            if( !bWritten )
            {
                FILE* fp = fopen( "dmsgdmp", "a+" );
                if( fp != nullptr )
                {
                    fwrite( pData, 1, dwSize, fp );
                    fclose( fp );
                    bWritten = true;
                }
            }
        }while( 0 );
#endif

    }while( 0 );

    if( pData != nullptr )
    {
        dbus_free( pData );
    }

    return ret;
}

gint32 DMsgPtr::Deserialize( CBuffer* pBuf )
{
    gint32 ret = 0;

    if( pBuf == nullptr || pBuf->size() == 0 )
        return -EINVAL;

    return Deserialize(
        pBuf->ptr(), pBuf->size() );
}

gint32 DMsgPtr::Deserialize(
    const char* pBuf, guint32 dwSizeMax )
{
    gint32 ret = 0;
    if( pBuf == nullptr )
        return -EINVAL;

    do{
        Clear();

        guint32 dwSize;
        memcpy( &dwSize, pBuf, sizeof( dwSize ) );
        dwSize = ntohl( dwSize );
        if( dwSize > dwSizeMax )
        {
            ret = -EINVAL;
            break;
        }

        guint8* pMsg =
            ( guint8* )pBuf + sizeof( guint32 );

        CDBusError dbusError;
        m_pObj = dbus_message_demarshal(
            ( char* )pMsg, dwSize, dbusError );

        if( m_pObj == nullptr )
        {
            ret = dbusError.Errno();
            break;
        }

    }while( 0 );

    return ret;
}

gint32 DMsgPtr::SetMember(
    const std::string& strMember )
{
    if( IsEmpty() )
        return -EFAULT; 

    if( !IsValidName( strMember ) )
        return -EINVAL;

    if( !dbus_message_set_member(
        m_pObj, strMember.c_str() ) )
        return -ENOMEM;
       
    return 0; 
}

gint32 DMsgPtr::Clone( DBusMessage* pSrcMsg )
{
    if( pSrcMsg == nullptr )
        return -EINVAL;

    m_pObj = dbus_message_copy( pSrcMsg );
    if( m_pObj == nullptr )
        return -EFAULT;

    return 0;
}

std::string DMsgPtr::DumpMsg() const
{
    std::string strRet = "\n\t";
    std::string val = GetMember();
    strRet += std::string( "Member: " ) + val + "\n\t";

    val = GetInterface();
    strRet += std::string( "Interface: " ) + val + "\n\t";

    val = GetPath();
    strRet += std::string( "Path: " ) + val + "\n\t";

    val = GetSender();
    strRet += std::string( "Sender: " ) + val + "\n\t";

    val = GetDestination();
    strRet += std::string( "Destination: " ) + val + "\n\t";

    val = GetSignature();
    strRet += std::string( "Signature: " ) + val + "\n\t";

    guint32 dwSerial = 0;
    gint32 ret = GetSerial( dwSerial );

    if( SUCCEEDED( ret ) )
    {
        strRet += std::string( "Serial: " ) +
            std::to_string( dwSerial ) + "\n\t";
    }
    ret = GetReplySerial( dwSerial );
    if( SUCCEEDED( ret ) )
    {
        strRet += std::string( "Reply Serial: " ) +
            std::to_string( dwSerial ) + "\n\t";
    }
    
    switch( GetType() )
    {

    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        {
            val = "MethodCall";
            break;
        }
    case DBUS_MESSAGE_TYPE_SIGNAL:
        {
            val = "Signal";
            break;
        }
    case DBUS_MESSAGE_TYPE_ERROR:
        {
            val = "Error";
            break;
        }
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        {
            val = "Return";
            break;
        }
    default:
        {
            val = "Unknown Type";
            break;
        }
    }
    strRet += std::string( "Type: " ) + val + "\n\t";

    std::vector< ARG_ENTRY > vecArgs;
    ret = GetArgs( vecArgs );
    if( SUCCEEDED( ret ) )
    {
        gint32 idx = 0;
        for( auto oPair : vecArgs )
        {
            switch( oPair.first )
            {
            case DBUS_TYPE_STRING:
                val = *oPair.second;
                break;
            case DBUS_TYPE_INT32:
            case DBUS_TYPE_UINT32:
            case DBUS_TYPE_BOOLEAN:
                {
                    guint32 i = *oPair.second;
                    val = std::to_string( i );
                    break;
                }
            case DBUS_TYPE_BYTE:
                {
                    guint8 i = *oPair.second;
                    val = std::to_string( i );
                    break;
                }
            case DBUS_TYPE_INT16:
            case DBUS_TYPE_UINT16:
                {
                    guint16 i = *oPair.second;
                    val = std::to_string( i );
                    break;
                }
            case DBUS_TYPE_INT64:
            case DBUS_TYPE_UINT64:
                {
                    guint64 i = *oPair.second;
                    val = std::to_string( i );
                    break;
                }
            default:
                val = "N/A";
            }

            strRet += std::string( "Arg " ) +
                std::to_string( idx ) +
                std::string ( ": " ) +
                val + "\n\t";

            ++idx;    
        }
    }
    return strRet;
}

bool IsValidName( const stdstr& strName )
{
    bool ret = false;
    do{
        if( strName.empty() ||
            strName.size() > REG_MAX_NAME )
            break;

        const char* ptr = strName.c_str();
        const char* pend = ptr + strName.size();
        if( !VALID_INITIAL_NAME_CHAR( *ptr ) )
            break;
        ++ptr;
        while( ptr != pend )
        {
            if( !VALID_NAME_CHAR( *ptr ) )
                break;
            ++ptr;
        }
        if( ptr == pend )
            ret = true;

    }while( 0 );

    return ret;
}

bool IsValidDBusPath( const stdstr& strName )
{
    bool ret = false;
    do{
        if( strName.empty() ||
            strName.size() > REG_MAX_PATH )
            break;

        const char* ptr = strName.c_str();
        const char* pend = ptr + strName.size();
        if( *ptr != '/' )
            break;
        while( ptr != pend )
        {
            if( *ptr == '/' )
            {
                const char* pnext = ptr + 1;
                if( pnext == pend )
                    break;
                if( unlikely( !VALID_INITIAL_NAME_CHAR( *pnext ) ) )
                    break;
                ptr += 2;
                continue;
            }
            if( unlikely( !VALID_NAME_CHAR( *ptr ) ) )
                break;
            ++ptr;
        }
        if( ptr == pend )
            ret = true;

    }while( 0 );

    return ret;
}

bool IsValidDBusName( const stdstr& strName )
{
    bool ret = false;
    do{
        if( strName.empty() ||
            strName.size() > REG_MAX_PATH )
            break;

        const char* ptr = strName.c_str();
        const char* pend = ptr + strName.size();
        if( *ptr == ':' )
        {   ++ptr;
            while( ptr != pend )
            {
                if( *ptr == '.' )
                {
                    const char* pnext = ptr + 1;
                    if( pnext == pend )
                    {
                        // '.' not allowed as the last
                        // one
                        break;
                    }
                    if( unlikely( !VALID_NAME_CHAR( *pnext ) ) )
                    {
                        break;
                    }
                    ptr += 2;
                    continue;
                }
                if( unlikely( !VALID_NAME_CHAR( *ptr ) ) )
                    break;
                ++ptr;
            }
            if( ptr == pend )
                ret = true;
        }
        else if( *ptr == '.' )
        {
            // cannot begin with '.'
            break;
        }

        ++ptr;
        while( ptr != pend )
        {
            if( *ptr == '.' )
            {
                const char* pnext = ptr + 1;
                if( pnext == pend )
                    break;
                if( unlikely( !VALID_INITIAL_NAME_CHAR( *pnext ) ) )
                    break;
                ptr += 2;
                continue;
            }
            if( unlikely( !VALID_NAME_CHAR( *ptr ) ) )
                break;
            ++ptr;
        }
        if( ptr == pend )
            ret = true;

    }while( 0 );

    return ret;
}

}
