/*
 * =====================================================================================
 *
 *       Filename:  sslfido.h
 *
 *    Description:  declaration of OpenSSL filter port and related classes 
 *
 *        Version:  1.0
 *        Created:  11/26/2019 08:29:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

#include "defines.h"
#include "port.h"
#include "mainloop.h"
#include "msgmatch.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "tcpport.h"
#include "frmwrk.h"
#include "tcportex.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

namespace rpcf
{

#define PORT_CLASS_OPENSSL_FIDO "RpcOpenSSLFido"
#define JSON_ATTR_CERTFILE      "CertFile"
#define JSON_ATTR_KEYFILE       "KeyFile"
#define JSON_ATTR_CACERT        "CACertFile"

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( ClassFactoryStart ) + 1,
    DECL_CLSID( CRpcOpenSSLFido ),
    DECL_CLSID( CRpcOpenSSLFidoDrv ),
    DECL_CLSID( COpenSSLHandshakeTask ),
    DECL_CLSID( COpenSSLShutdownTask ),
    DECL_CLSID( COpenSSLResumeWriteTask ),
};

typedef enum{

    propCertPath = propReservedEnd + 100,
    propKeyPath,
    propSSLCtx,

}EnumSSLPropId;

#define _P( propid ) ( static_cast< EnumPropId >( propid ) )

class COpenSSLHandshakeTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    COpenSSLHandshakeTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( COpenSSLHandshakeTask ) ); }

    gint32 RunTask();
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnTaskComplete( gint32 iRet );
};

class COpenSSLShutdownTask :
    public COpenSSLHandshakeTask
{
    public:
    typedef COpenSSLHandshakeTask super;

    COpenSSLShutdownTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( COpenSSLShutdownTask ) ); }

    gint32 RunTask();
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnTaskComplete( gint32 iRet );
};

class COpenSSLResumeWriteTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    COpenSSLResumeWriteTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( COpenSSLResumeWriteTask ) ); }

    gint32 GetIrp( IrpPtr& pIrp );
    gint32 RemoveIrp();
    gint32 RunTask();
};

class CRpcOpenSSLFido : public CPort
{
    enum EnumRenegStat
    {
        rngstatNormal,
        rngstatWantRead,
        rngstatResumeWrite,
    };

    bool        m_bStopReady = false;
    bool        m_bSvrOnline = false;
    bool        m_bClient = false;
    bool        m_bFirstRead = true;

    SSL_CTX     *m_pSSLCtx = nullptr;
    SSL         *m_pSSL = nullptr;
    BIO         *m_prbio = nullptr;
    BIO         *m_pwbio = nullptr;
    std::deque< TaskletPtr > m_queWriteTasks;

    EnumRenegStat m_dwRenegStat = rngstatNormal;
    sem_t       m_semWriteSync;
    sem_t       m_semReadSync;
    BufPtr      m_pOutBuf;
    guint32     m_dwNumSent = 0;

    gint32 ResumeWriteTasks();
    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );

    gint32 CompleteFuncIrp( IRP* pIrp );
    gint32 CompleteWriteIrp( IRP* pIrp );

    gint32 CompleteIoctlIrp( IRP* pIrp );
    gint32 CompleteListeningIrp( IRP* pIrp );

    gint32 StartSSLHandshake( PIRP pIrp );

    gint32 SendWriteReq( IEventSink* pCallback,
        BufPtr& pBuf );

    gint32 SendListenReq( IEventSink* pCallback,
        BufPtr& pBuf );

    gint32 StartSSLShutdown( PIRP pIrp );

    gint32 RemoveIrpFromMap( IRP* pIrp );
    gint32 InitSSL();

    gint32 BuildResumeTask(
        TaskletPtr& pResumeTask, PIRP pIrp,
        gint32 iIdx, guint32 dwOffset,
        guint32 dwTotal );

    gint32 AdvanceHandshake(
         IEventSink* pCallback,
         BufPtr& pHandshake );

    gint32 AdvanceShutdown(
         IEventSink* pCallback,
         BufPtr& pHandshake );

    inline char* GetOutBuf()
    {
        char* pBuf;
        if( unlikely( m_pOutBuf.IsEmpty() ) )
            return nullptr;
        pBuf = ( char* )m_pOutBuf->ptr();
        m_dwNumSent++;
        return pBuf;
    }

    inline guint32 GetOutSize() 
    { return m_pOutBuf->size(); }

    public:

    typedef CPort super;

    CRpcOpenSSLFido( const IConfigDb* pCfg );
    ~CRpcOpenSSLFido();

    virtual gint32 OnSubmitIrp(
        IRP* pIrp );

    // init the ssl context and start the
    // handshake task
    virtual gint32 PostStart( IRP* pIrp );
    virtual gint32 PreStop( IRP* pIrp );
    virtual gint32 Stop( IRP* pIrp );

    inline bool IsClient() const
    { return m_bClient; }

    gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );

    gint32 DoHandshake( IEventSink* pCallback,
        BufPtr& pHandshake );

    gint32 DoShutdown( IEventSink* pCallback,
        BufPtr& pHandshake );

    gint32 EncryptAndSend( PIRP pIrp,
        CfgPtr pCfg, gint32 iIdx,
        guint32 dwTotal, guint32 dwOffset,
        bool bResume );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext ) const;
};

class CRpcOpenSSLFidoDrv : public CRpcTcpFidoDrv
{
    std::string m_strCertPath;
    std::string m_strKeyPath;
    SSL_CTX     *m_pSSLCtx = nullptr;

    public:
    typedef CRpcTcpFidoDrv super;

    CRpcOpenSSLFidoDrv(
        const IConfigDb* pCfg = nullptr );

    ~CRpcOpenSSLFidoDrv();

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );

    gint32 LoadSSLSettings();
    gint32 InitSSLContext( bool bServer );
};

}
