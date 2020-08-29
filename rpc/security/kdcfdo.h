/*
 * =====================================================================================
 *
 *       Filename:  kdcfdo.h
 *
 *    Description:  declaration of the port dedicated for KDC traffic as KDC's client.
 *
 *        Version:  1.0
 *        Created:  07/15/2020 10:33:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2020 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#include "tcportex.h"
#include "secclsid.h"

class CKdcRelayPdo :
    public CTcpStreamPdo2
{
    public:
    typedef CTcpStreamPdo2 super;
    CKdcRelayPdo( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CKdcRelayPdo ) );
    }

    gint32 OnStmSockEvent(
        STREAM_SOCK_EVENT& sse );
};

class CKdcRelayFdoDrv : public CRpcTcpFidoDrv
{
    public:
    typedef CRpcTcpFidoDrv super;

    CKdcRelayFdoDrv(
        const IConfigDb* pCfg = nullptr )
        : super( pCfg )
    {
        SetClassId( clsid( CKdcRelayFdoDrv ) );
    }

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

class CKdcRelayFdo : public CPort
{
    BufPtr      m_pInBuf;
    std::deque< IrpPtr > m_queWaitingIrps;
    bool        m_bResubmit = false;

    protected:

    gint32 CompleteWriteIrp( IRP* pIrp );
    gint32 RemoveIrpFromMap( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 CompleteListeningIrp( IRP* pIrp );
    gint32 ResubmitIrp();
    gint32 HandleNextIrp( IRP* pIrp );
    gint32 CompleteIoctlIrp( IRP* pIrp );

    gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );

    public:

    typedef CPort super;

    CKdcRelayFdo( const IConfigDb* pCfg );
    ~CKdcRelayFdo()
    {}

    gint32 OnSubmitIrp( IRP* pIrp );
};
