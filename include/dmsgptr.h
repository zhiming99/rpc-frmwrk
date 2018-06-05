/*
 * =====================================================================================
 *
 *       Filename:  dmsgptr.h
 *
 *    Description:  definition/implementation of the DMsgPtr
 *
 *        Version:  1.0
 *        Created:  01/08/2017 12:19:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */
#pragma once
#include <dbus/dbus.h>
#include "defines.h"
#include "buffer.h"
#include "autoptr.h"
#include "dbuserr.h"

#define DMSG_MAX_SIZE       16384
#define DMSG_FIX_TYPE_SIZE  8
#define DMSG_MAX_ARGS       16

template<>
class CAutoPtr< Clsid_Invalid, DBusMessage >
{
    private:

    DBusMessage* m_pObj;

    public:

    // NOTE: cannot convert between different object
    // classes
    CAutoPtr( DBusMessage* pObj = nullptr,
        bool bAddRef = true )
    {
        m_pObj = nullptr;
        if( pObj != nullptr )
        {
            m_pObj = pObj;
            if( m_pObj && bAddRef )
            {
                dbus_message_ref( m_pObj );
            }
            else
            {
                printf( "%s(%d): bad cast happens\n",
                    __func__, __LINE__ );
            }
        }
    }

    CAutoPtr( const CAutoPtr& oInitPtr )
    {
        m_pObj = oInitPtr.m_pObj;
        if( m_pObj )
        {
            dbus_message_ref( m_pObj );
        }
        else
        {
            printf( "%s(%d): bad cast happens\n", __func__, __LINE__ );
        }
    }

    ~CAutoPtr()
    {
        Clear();
    }

    DBusMessage& operator*() const noexcept
    {
        return *m_pObj;
    }

    DBusMessage* operator->() const noexcept
    {
        return m_pObj;
    }

    operator DBusMessage*() const
    {
        return m_pObj;
    }

    DBusMessage* operator=( DBusMessage* rhs ) 
    {
        Clear();
        if( rhs != nullptr )
        {
            m_pObj = rhs;
            dbus_message_ref( m_pObj );
        }
        return m_pObj;
    }

    CAutoPtr& operator=( const CAutoPtr& rhs ) noexcept
    {
        Clear();

        DBusMessage* pObj = rhs.m_pObj;
        if( pObj )
        {
            m_pObj = pObj;
            dbus_message_ref( m_pObj );
        }

        return *this;
    }

    bool operator==( const CAutoPtr& rhs ) noexcept
    {
        DBusMessage* pObj = rhs.m_pObj;
        return ( m_pObj == pObj );
    }

    bool operator==( const DBusMessage* rhs ) noexcept
    {
        return m_pObj == rhs;
    }

    bool IsEmpty() const
    {
        return ( m_pObj == nullptr );
    }

    void Clear()
    {
        if( m_pObj )
        {
            dbus_message_unref( m_pObj );
        }
        m_pObj = nullptr;
    }

    gint32 NewObj( EnumClsid iNewClsid =
        ( EnumClsid ) DBUS_MESSAGE_TYPE_METHOD_CALL,
        const IConfigDb* pCfg = nullptr )
    {
        gint32 ret = 0;

        Clear();

        // with clsid invalid, we don't 
        // support object creation dynamically.

        DBusMessage* pObj = nullptr;

        pObj = dbus_message_new( ( guint32 )iNewClsid );

        if( pObj != nullptr )
        {
            m_pObj = pObj;
            if( m_pObj == nullptr )
            {
                dbus_message_unref( pObj );
                printf( "%s(%d): bad cast happens\n", __func__, __LINE__ );
                ret = -ENOTSUP;
            }
        }
        return ret;
    }

    gint32 NewResp( DBusMessage* pReqMsg )
    {
        gint32 ret = 0;

        if( pReqMsg == nullptr )
            return -EINVAL;

        Clear();

        DBusMessage* pObj =
            dbus_message_new_method_return( pReqMsg );

        if( pObj != nullptr )
        {
            m_pObj = pObj;
        }
        else
        {
            ret = -EFAULT;
        }

        std::string strVal;
        DMsgPtr pReq( pReqMsg );

        strVal = pReq.GetMember();
        if( !strVal.empty() )
            SetMember( strVal );

        strVal = pReq.GetSender();
        if( !strVal.empty() )
            SetDestination( strVal );

        strVal = pReq.GetDestination();
        if( !strVal.empty() )
            SetSender( strVal );

        strVal = pReq.GetPath();
        if( !strVal.empty() )
            SetPath( strVal );

        strVal = pReq.GetInterface();
        if( !strVal.empty() )
            SetInterface( strVal );

        return ret;
    }


    gint32 Serialize( CBuffer* pBuf );
    gint32 Deserialize( CBuffer* pBuf );
    gint32 Clone( DBusMessage* pSrcMsg );

    std::string GetMember() const
    {
        if( IsEmpty() )
            return std::string( "" );

        const char* pszMember =
            dbus_message_get_member( m_pObj );

        if( pszMember == nullptr )
            pszMember = "";

        return std::string( pszMember );
    }

    gint32 SetMember(
        const std::string& strMember )
    {
        if( IsEmpty() )
            return -EFAULT; 

        if( !dbus_message_set_member(
            m_pObj, strMember.c_str() ) )
            return -ENOMEM;
           
        return 0; 
    }

    std::string GetInterface() const
    {
        if( IsEmpty() )
            return std::string( "" );

        const char* pszInterface =
            dbus_message_get_interface( m_pObj );

        if( pszInterface == nullptr )
            pszInterface = "";

        return std::string( pszInterface );
    }

    gint32 SetInterface(
        const std::string& strInterface )
    {
        if( IsEmpty() )
            return -EFAULT; 

        if( !dbus_message_set_interface(
            m_pObj, strInterface.c_str() ) )
            return -ENOMEM;
           
        return 0; 
    }

    std::string GetPath() const
    {
        if( IsEmpty() )
            return std::string( "" );

        const char* pszPath =
            dbus_message_get_path( m_pObj );

        if( pszPath == nullptr )
            pszPath = "";

        return std::string( pszPath );
    }

    gint32 SetPath(
        const std::string& strPath )
    {
        if( IsEmpty() )
            return -EFAULT; 

        if( !dbus_message_set_path(
            m_pObj, strPath.c_str() ) )
            return -ENOMEM;
           
        return 0; 
    }

    std::string GetSender() const
    {
        if( IsEmpty() )
            return std::string( "" );

        const char* pszSender =
            dbus_message_get_sender( m_pObj );

        if( pszSender == nullptr )
            pszSender = "";

        return std::string( pszSender );
    }

    gint32 SetSender(
        const std::string& strSender )
    {
        if( IsEmpty() )
            return -EFAULT; 

        if( !dbus_message_set_sender(
            m_pObj, strSender.c_str() ) )
            return -ENOMEM;
           
        return 0; 
    }

    std::string GetDestination() const
    {
        if( IsEmpty() )
            return std::string( "" );

        const char* pszDestination =
            dbus_message_get_destination( m_pObj );

        if( pszDestination == nullptr )
            pszDestination = "";

        return std::string( pszDestination );
    }

    gint32 SetDestination(
        const std::string& strDestination )
    {
        if( IsEmpty() )
            return -EFAULT; 

        if( !dbus_message_set_destination(
            m_pObj, strDestination.c_str() ) )
            return -ENOMEM;
           
        return 0; 
    }

    std::string GetSignature()
    {
        if( IsEmpty() )
            return std::string( "" ); 

        const char* pszSignature =
            dbus_message_get_signature( m_pObj );
        if( pszSignature == nullptr )
            pszSignature = "";
           
        return std::string( pszSignature ); 
    }

    gint32 GetErrno()
    {
        if( IsEmpty() )
            return -EFAULT;

        if( GetType() != DBUS_MESSAGE_TYPE_ERROR )
            return -EINVAL;

        const char* pszError =
            dbus_message_get_error_name( m_pObj );

        return ErrnoFromDbusErr( pszError );
    }

    gint32 GetSerial( guint32& dwSerial )
    {
        if( IsEmpty() )
            return -EFAULT; 

        dwSerial = dbus_message_get_serial( m_pObj );
        return 0;
    }

    gint32 SetSerial( guint32 dwSerial )
    {
        if( IsEmpty() )
            return -EFAULT; 

        dbus_message_set_serial( m_pObj, dwSerial );
        return 0;
    }

    gint32 GetType() const
    {
        if( IsEmpty() )
            return -EFAULT; 

        return dbus_message_get_type( m_pObj );
    }

    inline void SetNoReply( bool bNoReply )
    {
        dbus_message_set_no_reply(
            m_pObj, bNoReply );
    }

    inline bool NoReply() const
    {
        return dbus_message_get_no_reply( m_pObj );
    }

    inline bool IsBasicType( gint32 iType ) const
    {
        return dbus_type_is_basic( iType );
    }

    inline bool IsFixedType( gint32 iType ) const
    {
        return dbus_type_is_fixed( iType );
    }

    typedef std::pair< gint32, BufPtr > ARG_ENTRY;

    gint32 GetArgs(
        std::vector< ARG_ENTRY >& vecArgs )
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

    gint32 GetIntArgAt( gint32 iIndex, guint32& val )
    {
        BufPtr pBuf( true );
        gint32 iType = 0;
        gint32 ret = GetArgAt( iIndex, pBuf, iType );
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

    gint32 GetFdArgAt( gint32 iIndex, gint32& val )
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

    gint32 GetByteArgAt( gint32 iIndex, guint8& val )
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

    gint32 GetBoolArgAt( gint32 iIndex, bool& val )
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
        val = ( bool& )*pBuf;
        return 0;
    }

    gint32 GetDoubleArgAt( gint32 iIndex, double& val )
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

    gint32 GetInt64ArgAt( gint32 iIndex, guint64& val )
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

    gint32 GetStrArgAt( gint32 iIndex, std::string& val )
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

    gint32 GetMsgArgAt( gint32 iIndex, DMsgPtr& val )
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

    gint32 GetObjArgAt( gint32 iIndex, ObjPtr& val )
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

        BufPtr pObjBuf( true );
        ret = pObjBuf->Deserialize( *pBuf );
        if( ERROR( ret ) )
            return ret;

        if( pObjBuf->type() != DataTypeObjPtr )
            return -EFAULT;

        val = ( ObjPtr& )*pObjBuf;

        if( val.IsEmpty() )
            return -EFAULT;

        return 0;
    }

    gint32 GetArgAt( gint32 iIndex,
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

    protected:

    gint32 GetValue( DBusMessageIter& itr,
        BufPtr& pBuf, gint32& iType )
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
    int
    GetTypeBytes( int typecode )
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
};

