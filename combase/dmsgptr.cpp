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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetArgs( std::vector< ARG_ENTRY >& vecArgs ) const
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetIntArgAt( gint32 iIndex, guint32& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetFdArgAt( gint32 iIndex, gint32& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetByteArgAt( gint32 iIndex, guint8& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetBoolArgAt( gint32 iIndex, bool& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetDoubleArgAt( gint32 iIndex, double& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetInt64ArgAt( gint32 iIndex, guint64& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetStrArgAt( gint32 iIndex, std::string& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetMsgArgAt( gint32 iIndex, DMsgPtr& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetObjArgAt( gint32 iIndex, ObjPtr& val )
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetArgAt( gint32 iIndex,
    BufPtr& pArg, gint32& iType )
{
    // get one single argument
    gint32 ret = 0;
    CDBusError oError;

    if( IsEmpty() || pArg.IsEmpty() )
        return -EINVAL;

    if( iIndex < 0 || iIndex > DMSG_MAX_ARGS )
        return -EINVAL;

    DBusMessageIter itr;
    if( !dbus_message_iter_init( m_pObj, &itr ) )
        return -ENOENT;

    gint32 i = 0;

    // let's get the first argument, which should
    do{
        BufPtr pBuf( true );

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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetValue( DBusMessageIter& itr,
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
int CAutoPtr< Clsid_Invalid, DBusMessage >
    ::GetTypeBytes( int typecode ) const
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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::Serialize( CBuffer* pBuf )
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

        ret = pBuf->Append(
            ( guint8* )&dwSize, sizeof( dwSize ) );

        if( ERROR( ret ) )
            break;

        ret = pBuf->Append( pData, dwSize );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( pData != nullptr )
    {
        dbus_free( pData );
    }

    return ret;
}

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::Deserialize( CBuffer* pBuf )
{
    gint32 ret = 0;

    if( pBuf == nullptr
        || pBuf->size() == 0 )
        return -EINVAL;

    do{
        Clear();

        guint32 dwSize =
            *( guint32* )pBuf->ptr();

        guint8* pMsg =
            ( guint8* )pBuf->ptr() + sizeof( guint32 );

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

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::Clone( DBusMessage* pSrcMsg )
{
    if( pSrcMsg == nullptr )
        return -EINVAL;

    m_pObj = dbus_message_copy( pSrcMsg );
    if( m_pObj == nullptr )
        return -EFAULT;

    return 0;
}

std::string CAutoPtr< Clsid_Invalid, DBusMessage >::
    DumpMsg() const
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
