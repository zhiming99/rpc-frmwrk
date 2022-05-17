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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include <dbus/dbus.h>
#include "defines.h"
#include "autoptr.h"
#include "dbuserr.h"

#define DMSG_MAX_SIZE       16384
#define DMSG_FIX_TYPE_SIZE  8
#define DMSG_MAX_ARGS       16

#define VALID_INITIAL_NAME_CHAR(c)         \
  ( ((c) >= 'A' && (c) <= 'Z') ||               \
    ((c) >= 'a' && (c) <= 'z') ||               \
    ((c) == '_') || ((c) == '-'))

#define VALID_NAME_CHAR(c)                 \
  ( ((c) >= '0' && (c) <= '9') ||               \
    ((c) >= 'A' && (c) <= 'Z') ||               \
    ((c) >= 'a' && (c) <= 'z') ||               \
    ((c) == '_') || ((c) == '-'))

#define VALID_PATH_CHAR(c)                 \
  ( VALID_NAME_CHAR(c) || (c) == '/' )

#define VALID_DBUS_NAME_CHAR(c)                 \
  ( VALID_NAME_CHAR(c) || (c) == '.' )

namespace rpcf
{

bool IsValidName(
    const std::string& strName );

bool IsValidDBusPath(
    const std::string& strName ); 

bool IsValidDBusName(
    const std::string& strName ); 

class DMsgPtr : public IAutoPtr
{
    private:

    DBusMessage* m_pObj;

    public:

    // NOTE: cannot convert between different object
    // classes
    DMsgPtr( DBusMessage* pObj = nullptr,
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

    DMsgPtr( const DMsgPtr& oInitPtr )
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

    ~DMsgPtr()
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

    DMsgPtr& operator=( const DMsgPtr& rhs ) noexcept
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

    bool operator==( const DMsgPtr& rhs ) noexcept
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

    DBusMessage* ptr()
    { return m_pObj; }
    const DBusMessage* ptr() const
    { return m_pObj; }

    gint32 Serialize( CBuffer* pBuf );
    gint32 Deserialize( CBuffer* pBuf );
    gint32 Deserialize(
        const char* pBuf, guint32 dwSize );

    gint32 Clone( DBusMessage* pSrcMsg );

    gint32 CopyHeader( DBusMessage* pMsg )
    {
        gint32 ret = 0;
        do{
            DMsgPtr pSrcMsg( pMsg );
            gint32 iType =
                dbus_message_get_type( pMsg );

            ret = NewObj( ( EnumClsid ) iType );
            if( ERROR( ret ) )
                break;

            SetSerial(
                dbus_message_get_serial( pMsg ) );

            ret = SetDestination(
                pSrcMsg.GetDestination() );
            if( ERROR( ret ) )
                break;

            SetInterface(
                pSrcMsg.GetInterface() );

            ret = SetPath( pSrcMsg.GetPath() );
            if( ERROR( ret ) )
                break;

            SetMember( pSrcMsg.GetMember() );
            SetSender( pSrcMsg.GetSender() );

            if( iType ==
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                guint32 dwSerial =
                    dbus_message_get_reply_serial( pMsg );
                SetReplySerial( dwSerial );
            }

        }while( 0 );

        return ret;
    }

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
        const std::string& strMember );

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

        if( !IsValidDBusName( strInterface ) )
            return -EINVAL;

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

        if( strPath.empty() )
            return -EINVAL;

        if( !IsValidDBusPath( strPath ) )
            return -EINVAL;

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

        if( !IsValidDBusName( strSender ) )
            return -EINVAL;

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

        if( strDestination.empty() )
            return -EINVAL;

        if( !IsValidDBusName( strDestination ) )
            return -EINVAL;

        if( !dbus_message_set_destination(
            m_pObj, strDestination.c_str() ) )
            return -ENOMEM;
           
        return 0; 
    }

    std::string GetSignature() const
    {
        if( IsEmpty() )
            return std::string( "" ); 

        const char* pszSignature =
            dbus_message_get_signature( m_pObj );
        if( pszSignature == nullptr )
            pszSignature = "";
           
        return std::string( pszSignature ); 
    }

    gint32 GetErrno() const
    {
        if( IsEmpty() )
            return -EFAULT;

        if( GetType() != DBUS_MESSAGE_TYPE_ERROR )
            return -EINVAL;

        const char* pszError =
            dbus_message_get_error_name( m_pObj );

        return ErrnoFromDbusErr( pszError );
    }

    gint32 GetSerial( guint32& dwSerial ) const
    {
        if( IsEmpty() )
            return -EFAULT; 

        dwSerial = dbus_message_get_serial( m_pObj );
        return 0;
    }

    gint32 GetReplySerial( guint32& dwSerial ) const
    {
        if( IsEmpty() )
            return -EFAULT; 

        dwSerial = dbus_message_get_reply_serial( m_pObj );
        return 0;
    }

    gint32 SetSerial( guint32 dwSerial )
    {
        if( IsEmpty() )
            return -EFAULT; 

        dbus_message_set_serial( m_pObj, dwSerial );
        return 0;
    }

    gint32 SetReplySerial( guint32 dwSerial )
    {
        if( dwSerial == 0 )
            return -EINVAL;

        if( IsEmpty() )
            return -EFAULT; 

        if( !dbus_message_set_reply_serial( m_pObj, dwSerial ) )
            return -ENOMEM;

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
        std::vector< ARG_ENTRY >& vecArgs ) const;

    gint32 GetIntArgAt( gint32 iIndex, guint32& val );
    gint32 GetFdArgAt( gint32 iIndex, gint32& val );
    gint32 GetByteArgAt( gint32 iIndex, guint8& val );
    gint32 GetBoolArgAt( gint32 iIndex, bool& val );
    gint32 GetDoubleArgAt( gint32 iIndex, double& val );
    gint32 GetInt64ArgAt( gint32 iIndex, guint64& val );
    gint32 GetStrArgAt( gint32 iIndex, std::string& val );
    gint32 GetMsgArgAt( gint32 iIndex, DMsgPtr& val );
    gint32 GetObjArgAt( gint32 iIndex, ObjPtr& val );

    gint32 GetArgAt( gint32 iIndex,
        BufPtr& pArg, gint32& iType );

    std::string DumpMsg() const;

    protected:

    gint32 GetValue( DBusMessageIter& itr,
        BufPtr& pBuf, gint32& iType ) const;

    /**
     * Gets the alignment requirement for the
     * given type; will be 1, 4, or 8.
     *
     * @param typecode the type
     * @returns alignment of 1, 4, or 8
     */
    int
    GetTypeBytes( int typecode ) const;
};

}
