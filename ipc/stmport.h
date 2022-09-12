/*
 * =====================================================================================
 *
 *       Filename:  stmpdo.h
 *
 *    Description:  declaration of stream pdo class 
 *
 *        Version:  1.0
 *        Created:  08/14/2022 09:31:38 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#include <dbus/dbus.h>
#include "defines.h"
#include "port.h"
#include "autoptr.h"
#include "msgmatch.h"
#include "dbusport.h"

#define SESS_HASH_SIZE 48

namespace rpcf
{

struct IRPCTX_EXTDSP
{
    bool m_bWaitWrite = false;
};

gint32 GetPreStopStep(
    PIRP pIrp, guint32& dwStepNo );

gint32 SetPreStopStep(
    PIRP pIrp, guint32 dwStepNo );

class CDBusStreamPdo :
    public CRpcPdoPort
{
    bool m_bServer = false;
    HANDLE m_hStream = INVALID_HANDLE;
    std::atomic< guint32 > m_dwDisconned;

    stdstr m_strHash;
    stdstr m_strPath;

    CTimestampSvr m_oTs;
    // sem_t   m_semFireSync;

    std::hashmap< guint32, DMsgPtr > m_mapRespMsgs;

    protected:

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch ) override
    { return 0; }

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch ) override
    { return 0; }

    gint32 OnRespMsgNoIrp(
        DBusMessage* pDBusMsg );

    gint32 CompleteIoctlIrp(
            IRP* pIrp ) override;

    gint32 CompleteSendReq( IRP* pIrp );
    gint32 CompleteListening( IRP* pIrp );

    public:

    typedef CRpcPdoPort super;

    CDBusStreamPdo(
        const IConfigDb* pCfg );

    inline HANDLE GetStream() const
    { return m_hStream; }

    inline void SetStream( HANDLE hStream )
    { m_hStream = hStream; }

    ObjPtr& GetStreamIf();

    bool IsServer() const
    { return m_bServer; }

    gint32 CheckExistance( ObjPtr& pIf );
    gint32 SubmitIoctlCmd(
        IRP* pIrp ) override;

    gint32 HandleSendReq(
        IRP* pIrp ) override;

    gint32 HandleSendEvent(
        IRP* pIrp ) override;

    gint32 HandleListening(
        IRP* pIrp );

    bool Unloadable() override
    { return true; }

    gint32 SendDBusMsg(
        PIRP pIrp, DMsgPtr& pMsg );

    gint32 PreStop( IRP* pIrp ) override;
    gint32 PostStart( IRP* pIrp ) override;
    gint32 OnPortReady( IRP* pIrp ) override;

    gint32 IsIfSvrOnline(
        const DMsgPtr& pMsg ) override;

    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* data ) override;

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext ) const;

};

class CDBusStreamBusPort : 
    public CGenericBusPortEx
{
    gint32 CreateDBusStreamPdo(
        IConfigDb* pCfg,
        PortPtr& pNewPort );

    gint32 StopStreamChan( IRP* pIrp );
    ObjPtr m_pIf;

    std::hashmap< HANDLE, PortPtr > m_mapStm2Port;
    bool m_bServer = false;

    public:
    typedef CGenericBusPortEx super;

    inline ObjPtr& GetStreamIf()
    { return m_pIf; }

    inline void SetStreamIf( ObjPtr pIf )
    { m_pIf = pIf; }

    bool IsServer() const
    { return m_bServer; }

    CDBusStreamBusPort ( const IConfigDb* pCfg );

    gint32 BroadcastDBusMsg(
        PIRP pIrp, DMsgPtr& pMsg );

    gint32 BuildPdoPortName(
        IConfigDb* pCfg,
        std::string& strPortName ) override;

    gint32 CreatePdoPort(
        IConfigDb* pConfig,
        PortPtr& pNewPort ) override;

    gint32 Stop( IRP* pIrp ) override;
    gint32 PostStart( IRP* pIrp ) override;

    void BindStreamPort(
        HANDLE hStream, PortPtr pPort );

    void RemoveBinding( HANDLE hStream );

    gint32 GetStreamPort(
        HANDLE hStream, PortPtr& pPort );

    gint32 OnNewConnection(
        HANDLE hStream, PortPtr& pPort );

    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* data ) override;
};

class CDBusStreamBusDrv :
    public CGenBusDriverEx
{
    public:
    typedef CGenBusDriverEx super;
    CDBusStreamBusDrv( const IConfigDb* pCfg );
	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

}
