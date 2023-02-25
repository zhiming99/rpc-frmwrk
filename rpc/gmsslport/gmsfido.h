/*
 * =====================================================================================
 *
 *       Filename:  gmsfido.h
 *
 *    Description:  class declarations for GmSSL's fido for rpc-frmwrk 
 *
 *        Version:  1.0
 *        Created:  01/21/2023 11:41:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "defines.h"
#include "port.h"
#include "mainloop.h"
#include "msgmatch.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "frmwrk.h"
#include "dbusport.h"
#include "tcportex.h"
#include "agmsapi.h"
#pragma once

#define PORT_CLASS_GMSSL_FIDO "RpcGmSSLFido"

using namespace gmssl;

namespace rpcf
{

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( ClassFactoryStart ) + 80,
    DECL_CLSID( CRpcGmSSLFido ),
    DECL_CLSID( CRpcGmSSLFidoDrv ),
    DECL_CLSID( CGmSSLHandshakeTask ),
    DECL_CLSID( CGmSSLShutdownTask ),
};

gint32 BufToIove( BufPtr& pSrc,
    PIOVE& pDest, bool bCopy );

gint32 IoveToBuf( PIOVE& pSrc,
    BufPtr& pDest, bool bCopy );

class CGmSSLHandshakeTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    CGmSSLHandshakeTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CGmSSLHandshakeTask ) ); }

    gint32 RunTask();
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnTaskComplete( gint32 iRet );
};

class CGmSSLShutdownTask :
    public CGmSSLHandshakeTask
{
    public:
    typedef CGmSSLHandshakeTask super;

    CGmSSLShutdownTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CGmSSLShutdownTask ) ); }

    gint32 RunTask();
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnTaskComplete( gint32 iRet );
};

class CRpcGmSSLFido : public CPort
{
    std::unique_ptr< AGMS >       m_pSSL;

    bool m_bClient = true;

    gint32 SubmitWriteIrpGroup(
        PIRP pIrp, ObjVecPtr& pvecBufs );

    gint32 SubmitWriteIrpSingle(
        PIRP pIrp, CfgPtr& pBufs );

    gint32 SubmitWriteIrpIn( IRP* pIrp );
    gint32 SubmitWriteIrpOut( IRP* pIrp );

    gint32 SubmitIoctlCmd( IRP* pIrp );

    gint32 CompleteFuncIrp( IRP* pIrp );
    gint32 CompleteWriteIrp( IRP* pIrp );

    gint32 CompleteIoctlIrp( IRP* pIrp );
    gint32 CompleteListeningIrp( IRP* pIrp );

    gint32 StartSSLHandshake( PIRP pIrp );

    inline gint32 SendWriteReq(
        IEventSink* pCallback,
        BufPtr& pBuf )
    {
        return rpcf::SendWriteReq(
            this, pCallback, pBuf );
    }

    inline gint32 SendListenReq(
        IEventSink* pCallback,
        BufPtr& pBuf )
    {
        return rpcf::SendListenReq(
            this, pCallback, pBuf );
    }

    gint32 StartSSLShutdown( PIRP pIrp );

    gint32 RemoveIrpFromMap( IRP* pIrp );
    gint32 InitSSL();

    gint32 BuildResumeTask(
        TaskletPtr& pResumeTask, PIRP pIrp,
        gint32 iIdx, guint32 dwOffset,
        guint32 dwTotal );

    gint32 AdvanceHandshakeInternal(
         IEventSink* pCallback,
         BufPtr& pHandshake,
         bool bShutdown );

    gint32 AdvanceHandshake(
         IEventSink* pCallback,
         BufPtr& pHandshake );

    gint32 AdvanceShutdown(
         IEventSink* pCallback,
         BufPtr& pHandshake );

    public:

    typedef CPort super;

    CRpcGmSSLFido( const IConfigDb* pCfg );
    ~CRpcGmSSLFido()
    {}

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

    gint32 SendImmediateResp();
};

class CRpcGmSSLFidoDrv : public CRpcTcpFidoDrv
{
    std::string m_strCertPath;
    std::string m_strKeyPath;
    std::string m_strCAPath;
    std::string m_strSecret;
    bool m_bVerifyPeer = false;
    bool m_bPassword = false;

    gint32 LoadSSLSettings();
    public:
    typedef CRpcTcpFidoDrv super;

    CRpcGmSSLFidoDrv(
        const IConfigDb* pCfg = nullptr );

    ~CRpcGmSSLFidoDrv();

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL ) override;

    gint32 Start() override;
};

}
