/*
 * =====================================================================================
 *
 *       Filename:  wsfido.h
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
#include "frmwrk.h"
#include "tcpport.h"
#include "wsproto/WebSocket.h"

namespace rpcf
{
#define PORT_CLASS_WEBSOCK_FIDO "RpcWebSockFido"

typedef enum 
{
    DECL_CLSID( CRpcWebSockFido ) = clsid( ClassFactoryStart ) + 11 ,
    DECL_CLSID( CRpcWebSockFidoDrv ),
    DECL_CLSID( CWsHandshakeTask ),
    DECL_CLSID( CWsPingPongTask ),
    DECL_CLSID( CWsCloseTask ),

} EnumMyClsid;

typedef enum 
{
    // a string to the URL
    propSecKey = propReservedEnd + 105,
    propHsSent,
    propHsReceived,
    propIrpPtr1,

} EnumWsPropId;

class CWsTaskBase :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    CWsTaskBase( const IConfigDb* pCfg ) :
        super( pCfg )
    {}

    gint32 OnCancel( guint32 dwContext );
    gint32 OnTaskComplete( gint32 iRet );
};

class CWsHandshakeTask :
    public CWsTaskBase
{
    public:
    typedef CWsTaskBase super;

    CWsHandshakeTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CWsHandshakeTask ) ); }

    gint32 RunTask();
    gint32 OnIrpComplete( IRP* pIrp );
};

class CWsPingPongTask :
    public CWsTaskBase
{
    public:
    typedef CWsTaskBase super;

    CWsPingPongTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CWsPingPongTask ) ); }

    gint32 OnIrpComplete( IRP* pIrp );
    gint32 RunTask();
};

class CWsCloseTask :
    public CWsTaskBase
{
    public:
    typedef CWsTaskBase super;

    CWsCloseTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CWsCloseTask ) ); }

    gint32 OnIrpComplete( IRP* pIrp );
    gint32 RunTask();
};

class CRpcWebSockFido : public CPort
{
    std::atomic< bool > m_bCloseSent;
    bool        m_bClient = false;

    BufPtr      m_pCurFrame;
    WebSocket   m_oWebSock;

    std::vector< TaskletPtr > m_vecTasks;

    inline guint64 GetSecKey() const
    { return GetObjId(); }

    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );

    gint32 CompleteFuncIrp( IRP* pIrp );
    gint32 CompleteWriteIrp( IRP* pIrp );

    gint32 CompleteIoctlIrp( IRP* pIrp );
    gint32 CompleteListeningIrp( IRP* pIrp );

    gint32 StartWsHandshake( PIRP pIrp );
    gint32 StartPingPong( PIRP pIrp );

    gint32 AdvanceHandshake(
         IEventSink* pCallback );

    gint32 AdvanceHandshakeServer(
         IEventSink* pCallback );

    gint32 AdvanceHandshakeClient(
         IEventSink* pCallback );

    public:

    typedef enum{
        enumHsTask,
        enumCloseTask,
        enumPingTask,

    } EnumTasks;

    typedef CPort super;

    CRpcWebSockFido( const IConfigDb* pCfg );
    ~CRpcWebSockFido();

    virtual gint32 OnSubmitIrp( IRP* pIrp );
    virtual gint32 PostStart( IRP* pIrp );
    virtual gint32 PreStop( IRP* pIrp );
    virtual gint32 Stop( IRP* pIrp );
    gint32 SchedulePongTask( BufPtr& pPong );
    gint32 ScheduleCloseTask( PIRP pIrp,
        gint32 dwReason, bool bStart );

    inline BufPtr& GetCurFrame() const
    { return const_cast< BufPtr& >( m_pCurFrame ); }

    inline bool IsClient() const
    { return m_bClient; }

    gint32 DoHandshake(
        IEventSink* pCallback, bool bFirst );
    gint32 ReceiveData( const BufPtr& pBuf );


    gint32 EncodeAndSend( PIRP pIrp,
        CfgPtr pCfg );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext ) const;

    inline gint32 MakeFrame(
        WebSocketFrameType frame_type,
        unsigned char* msg,
        guint32 msg_length,
        BufPtr& dest_buf )
    {
        return m_oWebSock.makeFrame( frame_type,
            msg, msg_length, dest_buf );
    }

    gint32 ClearTask( gint32 iIdx );

    gint32 SendWriteReq(
        IEventSink* pCallback,
        BufPtr& pBuf );

    gint32 SendListenReq(
        IEventSink* pCallback,
        BufPtr& pBuf );

};

class CRpcWebSockFidoDrv :
    public CRpcTcpFidoDrv
{
    public:
    typedef CRpcTcpFidoDrv super;

    CRpcWebSockFidoDrv(
        const IConfigDb* pCfg = nullptr );

    ~CRpcWebSockFidoDrv();
	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

}
