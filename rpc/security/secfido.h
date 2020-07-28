/*
 * =====================================================================================
 *
 *       Filename:  secfido.h
 *
 *    Description:  Declaration of security filter port and the driver
 *
 *        Version:  1.0
 *        Created:  07/14/2020 07:01:33 PM
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

#include "defines.h"
#include "port.h"
#include "frmwrk.h"
#include "tcportex.h"
#include "secclsid.h"

class CRpcSecFidoDrv : public CRpcTcpFidoDrv
{
    public:
    typedef CRpcTcpFidoDrv super;

    CRpcSecFidoDrv(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

struct SEC_PACKET_HEADER
{
    guint32 m_dwMagic = ENC_PACKET_MAGIC;
    guint32 m_dwSize = 0;;
};

class CRpcSecFido : public CPort
{
    BufPtr      m_pInBuf;

    gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 CompleteWriteIrp( IRP* pIrp );
    gint32 CompleteIoctlIrp( IRP* pIrp );

    virtual gint32 SubmitWriteIrp( IRP* pIrp );
    virtual gint32 CompleteListeningIrp(
        IRP* pIrp );

    gint32 EncEnabled();

    public:

    typedef CPort super;

    CRpcSecFido( const IConfigDb* pCfg );

    gint32 CompleteFuncIrp( IRP* pIrp );
    gint32 OnSubmitIrp( IRP* pIrp );

    gint32 IsClient() const;
    gint32 DoEncrypt( PIRP pIrp,
        BufPtr& pOutBuf,
        bool bEncrypt );

    bool HasSecCtx() const;
    gint32 GetSecCtx( ObjPtr& pObj ) const;
};
