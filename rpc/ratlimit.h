/*
 * =====================================================================================
 *
 *       Filename:  ratlimit.h
 *
 *    Description:  Declarations of rate limiter related classes 
 *
 *        Version:  1.0
 *        Created:  11/01/2023 03:27:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

#include "defines.h"
#include "port.h"
#include "frmwrk.h"
#include "tcportex.h"
#include "iftasks.h"
namespace rpcf
{

typedef enum 
{
    DECL_CLSID( CRateLimiterDrv ) =
        clsid( ClassFactoryStart ) + 100 ,
    DECL_CLSID( CRateLimiterFido ),

} EnumRatLimitClsid;

class CRateLimiterFido;
class CRateLimiterDrv : public CRpcTcpFidoDrv
{
    public:
    typedef CRpcTcpFidoDrv super;

    CRateLimiterDrv(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

typedef std::pair< IrpPtr, guint32 > IRPQUE_ELEM;
class CRateLimiterFido : public CPort
{
    public:

    class CBytesWriter
    {
        IrpPtr m_pIrp;
        gint32 m_iBufIdx = -1;
        guint32 m_dwOffset = 0;
        CRateLimiterFido* m_pPort;
        guint32 m_dwBytesToSend = 0;
        public:

        CBytesWriter( CRateLimiterFido* pPdo );
        gint32 SetIrpToSend(
            IRP* pIrp, guint32 dwSize );
        gint32 SendImmediate( PIRP pIrp );
        gint32 SetSendDone();
        bool IsSendDone() const;
        IrpPtr GetIrp() const;
        gint32 OnSendReady();
        gint32 CancelSend(
            const bool& bCancelIrp );
        gint32 Clear();
    };

    class CBytesReader
    {
        IrpPtr m_pIrp;
        gint32 m_iBufIdx = -1;
        guint32 m_dwOffset = 0;
        CRateLimiterFido* m_pPort;
        public:

        CBytesReader( CRateLimiterFido* pPdo );
        gint32 StartRead();
        gint32 ReadImmediate( PIRP pIrp );
        gint32 SetReadDone();
        bool IsReadDone() const;
        IrpPtr GetIrp() const;
        gint32 OnReadReady();
        gint32 CancelRead(
            const bool& bCancelIrp );
        gint32 Clear();
    };

    protected:
    std::deque< IRPQUE_ELEM >  m_queWriteIrps;
    std::deque< IRPQUE_ELEM >  m_queReadIrps;

    TaskletPtr  m_pReadTb;
    TaskletPtr  m_pWriteTb;
    MloopPtr    m_pLoop;
    CBytesWriter m_oWriter;
    CBytesReader m_oReader;

    public:
    typedef CPort super;
    CRateLimiterFido( const IConfigDb* pCfg ) :
        super( pCfg ),
        m_oWriter( this ),
        m_oReader( this )
    {
        SetClassId( clsid( CRateLimiterFido ) );
        m_dwFlags &= ~PORTFLG_TYPE_MASK;
        m_dwFlags |= PORTFLG_TYPE_FIDO;
    }

    TaskletPtr GetTokenTask( bool bWrite )
    {
        if( bWrite )
            return m_pWriteTb;
        else
            return m_pReadTb;
    }

    gint32 PostStart( PIRP pIrp ) override;
    gint32 OnPortReady( PIRP pIrp ) override;
    gint32 PreStop( PIRP pIrp ) override
    gint32 ResumeWrite();
    gint32 ResumeRead();
    gint32 SubmitWriteIrp( pIrp );
    gint32 OnSubmitIrp( PIRP pIrp ) override;
    gint32 CompleteListeningIrp( PIRP pIrp );
    gint32 CompleteWriteIrp( PIRP pIrp );
    gint32 CompleteIoctlIrp( PIRP pIrp ) override;
    gint32 CompleteFuncIrp( PIRP pIrp ) override;
    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1 = 0,
            LONGWORD dwParam2 = 0,
            LONGWORD* pData ) override;
};

}
