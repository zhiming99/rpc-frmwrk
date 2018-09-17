/*
 * ============================================================================
 *
 *       Filename:  namespc.h
 *
 *    Description:  name spaces for interface, object, module and registries
 *
 *        Version:  1.0
 *        Created:  11/20/2016 09:05:17 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * ============================================================================
 */

#pragma once

#include <dbus/dbus.h>
#include <objfctry.h>


#define REG_DRV_ROOT                     "/drivers"
#define REG_PRELOADABLE_ROOT             "/preloadable"

#define DBUS_SYS_INTERFACE              DBUS_INTERFACE_DBUS
#define DBUS_SYS_OBJPATH                DBUS_PATH_DBUS

#define PORT_CLASS_DBUS_PROXY_PDO       "DBusProxyPdo"
#define PORT_CLASS_DBUS_PROXY_FDO       "DBusProxyFdo"
#define PORT_CLASS_RPC_TCP_FIDO         "RpcTcpFido"
#define PORT_CLASS_TCP_STREAM_PDO       "TcpStreamPdo"
#define PORT_CLASS_LOCALDBUS_PDO        "DBusLocalPdo"
#define PORT_CLASS_LOCALDBUS            "DBusBusPort"
#define PORT_CLASS_RPC_TCPBUS           "RpcTcpBus"
#define PORT_CLASS_LOOPBACK_PDO         "DBusLoopbackPdo"

// registry path for event subscription from the
// IoManager
#define REG_IO_EVENTMAP_DIR_FMT \
     "/bulletin_board/%s/connpoint_container"

#define BUILD_STRING2( fmt, strVal, strVal2, strRet ) \
do{ \
    char szBuf[ REG_MAX_NAME << 1 ]; \
    szBuf[ sizeof( szBuf ) - 1 ] = 0; \
    snprintf( szBuf, sizeof( szBuf ) - 1,\
        fmt, strVal.c_str(), strVal2.c_str() ); \
    strRet = szBuf; \
}while( 0 )

#define BUILD_STRING1( fmt, strVal, strRet ) \
do{ \
    char szBuf[ REG_MAX_NAME << 1 ]; \
    szBuf[ sizeof( szBuf ) - 1 ] = 0; \
    snprintf( szBuf, sizeof( szBuf ) - 1,\
        fmt, strVal.c_str() ); \
    strRet = szBuf; \
}while( 0 )

inline std::string REG_IO_EVENTMAP_DIR(
    const std::string& strModName )
{
    std::string strRet;

    BUILD_STRING1( REG_IO_EVENTMAP_DIR_FMT,
        strModName, strRet );

    return strRet;
}

#define DBUS_NAME_PREFIX                "org.rpcfrmwrk."
#define DBUS_IF_NAME_FMT                DBUS_NAME_PREFIX"%s.%s"
#define DBUS_DEST_PREFIX_FMT            DBUS_NAME_PREFIX"%s"

inline std::string DBUS_IF_NAME(
    const std::string& strIfName )
{
    std::string strRet;
    std::string strIfSpc = std::string( "Interf" );

    BUILD_STRING2( DBUS_IF_NAME_FMT,
        strIfSpc , strIfName, strRet );

    return strRet;
}

inline std::string IF_NAME_FROM_DBUS(
    const std::string& strIfName )
{
    // reverse of DBUS_IF_NAME
    std::string strRet;
    std::string strIfSpc = DBUS_NAME_PREFIX;
    strIfSpc += std::string( "Interf." );

    size_t pos = strIfName.find( strIfSpc, 0 );
    if( pos == std::string::npos )
        return strIfName;

    if( pos + strIfSpc.size() >= strIfName.size() )
        return strIfName;

    return strIfName.substr( pos + strIfSpc.size(),
        255 - strIfSpc.size() );
}

inline std::string DBUS_DESTINATION(
    const std::string& strModName )
{
    std::string strRet;
    BUILD_STRING1( DBUS_DEST_PREFIX_FMT,
        strModName, strRet );

    return strRet;
}

#define LOOPBACK_DESTINATION   "org.rpcfrmwrk.loopback"

/**
* @name Modue name module name is used to register
* on the dbus as a well-known address, to receive
* messages from other modules.
*
* @{ It usually consists 3 part, one is the prefix
* `Mod_', the second is the base name, which is
* better derived from the class name with capatal
* 'C' and can be found by CoGetClassId, and the
* third is the instance number, combined with
* underline.
*
* The module name usually comes from the config
* file, except some system modules*/
/**  @} */

// module name for rpc router which will serve as
// the dbus destination where the requst to rpc
// router will send
#define MODNAME_RPCROUTER               "Mod_RpcRouter_0"
#define MODNAME_INVALID                 "Mod_Invalid"

// the interface that provide req forwarder
// service
#define IFNAME_REQFORWARDER \
    CoGetClassName( clsid( CRpcReqForwarder ) )

#define IFNAME_TCP_BRIDGE \
    CoGetClassName( clsid( CRpcTcpBridge ) )

#define IFNAME_INVALID                  "If_Invalid" 
// the obj name which will provide two interfaces
// CRpcReqForwarder and CRpcReqForwarderProxy 

#define OBJNAME_RPCROUTER               "Obj_RpcRouter"
#define OBJNAME_IOMANAGER               "Obj_IoManager"
#define OBJNAME_INVALID                 "Obj_Invalid"

#define DBUS_DEF_OBJ                    OBJNAME_IOMANAGER

#define DBUS_DEFAULT_OBJ_PATH_FMT       "/org/rpcfrmwrk/%s/objs/%s"

#define DBUS_RESP_KEEP_ALIVE            "KeepAlive"

#define DBUS_RMT_CANCEL_REQ             "NotifyCancelReq"

// obj path consists of three part, one is the
// fixed part, and the modname, and then the
// objname. The prefix must be consistant with
// the DBUS_DESTINATION, except that all '.' are
// replaced with '/' 
inline std::string DBUS_OBJ_PATH(
    const std::string& strModName,
    std::string strObjName = "" )
{
    std::string strRet;

    if( strObjName.empty() )
        strObjName = DBUS_DEF_OBJ;

    BUILD_STRING2( DBUS_DEFAULT_OBJ_PATH_FMT,
        strModName, strObjName, strRet );

    return strRet;
}

#define SYS_METHOD_SENDDATA             "RpcCall_SendData"
#define SYS_METHOD_FETCHDATA            "RpcCall_FetchData"
#define SYS_METHOD_FORWARDREQ           "RpcCall_ForwardRequest"
#define SYS_METHOD_OPENRMTPORT          "RpcCall_OpenRemotePort"
#define SYS_METHOD_CLOSERMTPORT         "RpcCall_CloseRemotePort"
#define SYS_METHOD_ENABLERMTEVT         "RpcCall_EnableRemoteEvent"
#define SYS_METHOD_DISABLERMTEVT        "RpcCall_DisableRemoteEvent"


#define SYS_METHOD_USERCANCELREQ        "RpcCall_UserCancelRequest"

#define SYS_EVENT_KEEPALIVE             "RpcEvt_KeepAlive"
#define SYS_EVENT_FORWARDEVT            "RpcEvt_ForwardEvent"

#define IF_METHOD_ENABLEEVT             "IfReq_EnableEvt"
#define IF_METHOD_DISABLEEVT            "IfReq_DisableEvt"
#define IF_METHOD_LISTENING             "IfReq_Listening"
#define IF_EVENT_PROGRESS               "IfEvt_Progress"

// Bridge-specific command between proxy and
// server
#define BRIDGE_METHOD_OPENSTM           "Brdg_Req_OpenStream"
#define BRIDGE_METHOD_CLOSESTM          "Brdg_Req_CloseStream"
#define BRIDGE_EVENT_INVALIDSTM         "Brdg_Evt_InvalidStream"
#define BRIDGE_EVENT_SVROFF             "Brdg_Evt_ServerOffline"
#define BRIDGE_EVENT_MODOFF             "Brdg_Evt_ModOffline"

#define USER_METHOD_PREFIX              "UserMethod_"
#define SYS_METHOD_PREFIX              	"RpcCall_"
#define USER_EVENT_PREFIX               "UserEvent_"
#define SYS_EVENT_PREFIX                "RpcEvt_"

inline std::string SYS_EVENT(
    const std::string& strMethod )
{
    std::string strName = SYS_EVENT_PREFIX;
    strName += strMethod; 
    return strName;
}

inline std::string USER_EVENT(
    const std::string& strMethod )
{
    std::string strName = USER_EVENT_PREFIX;
    strName += strMethod; 
    return strName;
}

inline std::string USER_METHOD(
    const std::string& strMethod )
{
    std::string strName = USER_METHOD_PREFIX;
    strName += strMethod; 
    return strName;
}

inline std::string SYS_METHOD(
    const std::string& strMethod )
{
    std::string strName = SYS_METHOD_PREFIX;
    strName += strMethod; 
    return strName;
}
