/*
 * =====================================================================================
 *
 *       Filename:  reqopen.h
 *
 *    Description:  declaration of CReqOpener and CReqBuilder
 *
 *        Version:  1.0
 *        Created:  08/25/2018 06:47:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once
#include <configdb.h>

// CallFlags for SubmitIoRequest & InvokeMethod
#define CF_MESSAGE_TYPE_MASK            0x07
#define CF_ASYNC_CALL                   0x08 
#define CF_WITH_REPLY                   0x10

// the server will send keep-alive heartbeat to
// the client at the specified interval specified
// by propIntervalSec
#define CF_KEEP_ALIVE                   0x20

// the request to send is not a dbus message,
// instead a ConfigDb
#define CF_NON_DBUS                     0x40

#define MAX_BYTES_PER_TRANSFER ( 1024 * 1024 )
#define MAX_BYTES_PER_FILE     ( 512 * 1024 * 1024 )

class CReqOpener : public CParamList
{
    gint32 GetCallOptions( CfgPtr& pCfg ) const;
    public:
    typedef CParamList super;
    CReqOpener( IConfigDb* pCfg ) : super( pCfg )
    {;}
    gint32 GetIfName( std::string& strIfName ) const;
    gint32 GetObjPath( std::string& strObjPath ) const; 
    gint32 GetDestination( std::string& strDestination ) const; 
    gint32 GetMethodName( std::string& strMethodName ) const; 
    gint32 GetSender( std::string& strSender ) const;
    gint32 GetKeepAliveSec( guint32& dwTimeoutSec ) const; 
    gint32 GetTimeoutSec( guint32& dwTimeoutSec) const; 
    gint32 GetCallFlags( guint32& dwFlags ) const; 
    gint32 GetReturnValue( gint32& iRet ) const;
    gint32 GetIpAddr( std::string& strIpAddr,
        bool bSrc ) const;
    gint32 GetTaskId( guint64& dwTaskid ) const;

    bool HasReply() const;
    bool IsKeepAlive() const;
    gint32 GetReqType( guint32& dwType ) const;
};

class CRpcBaseOperations;
class CReqBuilder : public CParamList
{
    CRpcBaseOperations* m_pIf;
    gint32 GetCallOptions( CfgPtr& pCfg );

    public:
    typedef CParamList super;
    CReqBuilder( CRpcBaseOperations* pIf );
    CReqBuilder( const IConfigDb* pReq = nullptr ); // copy constructor
    CReqBuilder( IConfigDb* pReq );
    gint32 SetIfName( const std::string& strIfName );
    gint32 SetObjPath( const std::string& strObjPath ); 
    gint32 SetDestination( const std::string& strDestination ); 
    gint32 SetMethodName( const std::string& strMethodName ); 
    gint32 SetSender( const std::string& strSender );
    gint32 SetKeepAliveSec( guint32 dwTimeoutSec ); 
    gint32 SetTimeoutSec( guint32 dwTimeoutSec); 
    gint32 SetCallFlags( guint32 dwFlags );
    gint32 SetReturnValue( gint32 iRet );
    gint32 SetIpAddr(
        const std::string& strIpAddr, bool bSrc );
    gint32 SetTaskId( guint64 dwTaskid );
};

