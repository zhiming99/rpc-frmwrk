/*
 * =====================================================================================
 *
 *       Filename:  fastrpc.h
 *
 *    Description:  declarations of support classes for rpc-over-stream 
 *
 *        Version:  1.0
 *        Created:  08/14/2022 04:04:42 PM
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
#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include "streamex.h"
#include "stmport.h"

#define IFBASE2( _bProxy ) std::conditional< \
    _bProxy, CStreamProxyWrapper, CStreamServerWrapper>::type

#define IFBASE( _bProxy ) std::conditional< \
    _bProxy, CStreamProxyAsync, CStreamServerAsync>::type

#define IFBASE3( _bProxy ) std::conditional< \
    _bProxy, CAggInterfaceProxy, CAggInterfaceServer>::type

#define MAX_REQCHAN_PER_SESS 200

namespace rpc{

using SESS_INFO=std::pair< stdstr, stdstr >;

template< bool bProxy >
class CRpcStmChanBase :
    public virtual IFBASE( bProxy ) 
{
    timespec m_tsStartTime;

    CSharedLock m_oSharedLock;

    std::hashmap< HANDLE, SESS_INFO > m_mapStm2Sess;
    std::map< stdstr, guint32 > m_mapSessRefs;

    using MSGQUE=
        std::pair< guint32, std::deque< DMsgPtr > >;
    std::hashmap< HANDLE, MSGQUE  > m_mapMsgRead;
    using BUFQUE= std::deque< BufPtr >;
    std::hashmap< HANDLE, BUFQUE > m_mapBufReading;
    std::set< HANDLE > m_setStreams;

    IPort* m_pPort = nullptr;

    public:
    typedef typename IFBASE( bProxy ) super;

    CSharedLock& GetSharedLock()
    { return m_oSharedLock; }

    CRpcReqStreamBase( const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
    {
        clock_gettime(
            CLOCK_REALTIME, &m_tsStartTime );
    }

    inline void SetPort( IPort* pPort )
    { m_pPort = pPort; }

    gint32 GetSessHash(
        HANDLE hStream, stdstr& strSess ) const
    {
        gint32 ret = 0;
        do{
            CReadLock oLock( this->GetLock() );
            auto itr =
                m_mapStm2Sess.find( hStream );
            if( itr == m_mapStm2Sess.end() )
            {
                ret = -ENOENT;
                break;
            }
            strSess = itr.first;

        }while( ret );
        return ret;
    }

    gint32 GetSessInfo( HANDLE hStream,
        SESS_INFO& stmInfo ) const
    {
        gint32 ret = 0;
        do{
            CReadLock oLock( this->GetSharedLock() );
            auto itr =
                m_mapStm2Sess.find( hStream );
            if( itr == m_mapStm2Sess.end() )
            {
                ret = -ENOENT;
                break;
            }
            stmInfo = *itr;

        }while( ret );
        return ret;
    }

    gint32 AcceptNewStream(
        IEventSink* pCallback,
        IConfigDb* pDataDesc ) override
    {
        gint32 ret = 0;
        do{
            if( pDataDesc == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = super::AcceptNewStream(
                pCallback, pDataDesc );
            if( ERROR( ret ) )
                break;

            CCfgOpener oDesc( pDataDesc );
            ret = oCfg.GetPointer(
                propTransCtx, ptctx );
            if( ERROR( ret ) )
                break;

            stdstr strSess;
            CCfgOpener oCtx( ptctx ); 
            ret = oCtx.GetStrProp(
                propSessHash, strSess );
            if( ERROR( ret ) )
                break;

            stdstr strPath;
            ret = oCtx.GetStrProp(
                propRouterPath, strPath );
            if( ERROR( ret ) )
                break;
                
            CWriteLock oLock( this->GetSharedLock() );

            auto itr = m_mapSessRefs.find(
                strSess );

            if( itr == m_mapSessRefs.end() )
                m_mapSessRefs[ strSess ] = 1;
            else
            {
                if( itr->second >=
                    MAX_REQCHAN_PER_SESS )
                {
                    ret = -ERANGE;
                    break;
                }
                itr->second++;
            }

            m_mapStm2Sess[ hStream ] =
                    { strSess, strPath };

        }while( 0 );

        return ret;
    }

    gint32 OnStreamReady( HANDLE hstm ) override
    {
        gint32 ret = 0;
        do{
            ret = super::OnStreamReady( hstm );
            if( ERROR( ret ) )
                break;

            CWriteLock oLock( this->GetSharedLock() );

            m_setStreams.insert( hstm );

            auto& msgElem =
                    _mapMsgRead[ hstm ];
            auto& bufElem =
                    m_mapBufReading[ hstm ];

            oLock.Unlock();

            ret = m_pPort->OnEvent( eventNewConn,
                hstm, 0, nullptr );

        }while( 0 );

        return ret;
    }

    gint32 AddRecvReq( HANDLE hstm )
    {
        gint32 ret = 0;
        do{
            auto pPort = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 
            PortPtr pPdoPort;
            ret = pPort->GetStreamPort(
                hstm, pPdoPort );
            if( ERROR( ret ) )
                break;

            CReadLock oLock( this->GetSharedLock() );
            auto itr = m_mapMsgRead.find( hstm );
            if( itr == m_mapMsgRead.end() )
                break;

            CPort* pPdo = pPdoPort;
            CStdRMutex oPortLock( pPdo->GetLock() );

            auto& msgElem = itr.second;
            auto& dwReqCount = msgElem.first;
            dwReqCount++;

            oPortLock.Unlock();
            oLock.Unlock();

            FillReadBuf( hstm );

            TaskletPtr pTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, this,
                &CRpcReqStreamBase::DispatchData, 
                hstm );
            
            if( ERROR( ret ) )
                break;

            auto pMgr = this->GetIoMgr();
            ret = pMgr->RescheduleTask( pTask );

        }while( 0 );

        return ret;
    }

    gint32 OnStmClosing( HANDLE hstm ) override
    {
        gint32 ret = 0;
        do{
            auto pPort = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 
            PortPtr pPdoPort;
            pPort->GetStreamPort( hstm, pPdoPort );

            if( !pPdoPort.IsEmpty() )
            {
                pPdoPort->OnEvent(
                    eventDisconn, 0, 0, nullptr );
            }

            ret = super::OnStmClosing( hstm );

            CWriteLock oLock( this->GetSharedLock() );
            auto itr =
                m_mapStm2Sess.find( hStream );
            if( itr == m_mapStm2Sess.end() )
                break;

            auto itr2 =
                m_mapSessRefs.find( itr->second );

            if( itr2 == m_mapSessRefs.end() )
            {
                m_mapStm2Sess.erase( itr );
                break;
            }
            if( itr2->second <= 1 )
            {
                m_mapStm2Sess.erase( itr );
                m_mapSessRefs.erase( itr2 );
                break;
            }
            --itr2->second;

            m_mapMsgRead.erase( hstm );
            m_mapBufReading.erase( hstm );
            m_setStreams.erase( hstm );

        }while( 0 );

        return ret;
    }

    bool HasOutQueLimit() const override
    { return false; }

    inline gint32 CompleteWriteIrp(
        HANDLE hStream,
        gint32 iRet, BufPtr& pBuf,
        IConfigDb* pCtx )
    {
        gint32 ret = 0;
        do{
            PIRP pIrp;
            CCfgOpener oCtx( pCtx );
            ret = oCtx.GetPointer(
                propIrpPtr, pIrp );
            if( ERROR( ret ) )
                break;

            CStdRMutex oIrpLock(
                pIrp->GetLock() );

            ret = pIrp->CanContinue(
                IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            if( pIrp->GetStackSize() == 0 )
            {
                ret = -EINVAL;
                break;
            }
            pIrpCtxPtr& pCtx = Irp->GetTopStack();

            pCtx->SetStatus( iRet );
            oIrpLock.Unlock();

            CIoManager* pMgr = this->GetIoMgr();
            pMgr->CompleteIrp( pIrp );

        }while( 0 );

        return ret;
    }

    gint32 OnWriteStreamComplete(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx ) override
    {
        if( pCtx == nullptr )
            return 0;

        return CompleteWriteIrp(
            hStream, iRet, pBuf, pCtx );
    }

    gint32 BuildNewMsgToSvr(
        HANDLE hstm,
        DMsgPtr& pFwrdMsg,
        DMsgPtr& pNewMsg )
    {
        // for compatible format with the message from
        // the bridge
        gint32 ret = 0;
        do{
            // append a session hash for access
            // control
            CCfgOpener oReqCtx;
            SESS_INFO si;
            ret = GetSessInfo( hstm, si );
            if( ERROR( ret ) )
                break;

            oReqCtx.SetStrProp(
                propRouterPath, si.second );

            oReqCtx.SetStrProp(
                propSessHash, si.first );

            oReqCtx.SetIntPtr( propStmHandle,
                ( guint32* )hstm );

            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            oReqCtx.SetQwordProp(
                propTimestamp, ts.tv_sec );

            pNewMsg.CopyHeader( pFwrdMsg );

            BufPtr pArg( true );
            gint32 iType = 0;
            ret = pFwrdMsg.GetArgAt(
                0, pArg, iType );

            BufPtr pCtxBuf( true );
            ret = oReqCtx.Serialize( pCtxBuf );
            if( ERROR( ret ) )
                break;

            const char* pData = pArg->ptr();
            const char* pCtx = pCtxBuf->ptr();
            if( !dbus_message_append_args(
                pNewMsg, DBUS_TYPE_ARRAY,
                DBUS_TYPE_BYTE, &pData,
                pArg->size(), DBUS_TYPE_ARRAY,
                DBUS_TYPE_BYTE, &pCtx,
                pCtxBuf->size(),
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }

        }while( 0 );

        return ret;
    }

    gint32 FillReadBuf(
        HANDLE hstm, BufPtr& pBuf ) 
    {
        gint32 ret = 0;
        auto pPort = static_cast
            < CDBusStreamBusPort* >( m_pPort ); 
        PortPtr pPdoPort;
        ret = pPort->GetStreamPort(
            hstm, pPdoPort );
        if( ERROR( ret ) )
            return ret;

        CReadLock oLock( this->GetSharedLock() );
        do{
            auto itr = m_mapMsgRead.find( hstm );
            if( itr == m_mapMsgRead.end() )
            {
                ret = -ENOENT;
                break;
            }

            CPort* pPdo = pPdoPort;
            CStdRMutex oPortLock( pPdo->GetLock() );

            auto& msgElem = itr.second;
            auto& msgQue = msgElem.second;

            BufPtr pBufReady;
            auto itr =
                m_mapBufReading.find( hstm );
            if( itr == m_mapBufReading.end() )
            {
                ret = -ENOENT;
                break;
            }
            
            BUFQUE queBuf = itr->second;
            if( queBuf.empty() )
            {
                guint32 dwSize = 0;
                memcpy( &dwSize, pBuf->ptr(),
                    sizeof( dwSize ) );
                dwSize = ntohl( dwSize );
                if( dwSize > MAX_BYTES_PER_TRANSFER )
                {
                    ret = -EBADMSG;
                    break;
                }
                guint32 dwPayload =
                    pBuf->size() - sizeof( dwSize );
                if( dwPayload == dwSize )
                {
                    DMsgPtr pMsg;
                    ret = pMsg.Deserialize( *pBuf );
                    if( ERROR( ret ) )
                        break;
                    stdstr strSender = ":";
                    strSender +=
                        std::to_string( hstm )
                    pMsg.SetSender( strSender );
                    DMsgPtr pNewMsg;

                    ret = BuildNewMsgToSvr(
                        pMsg, pNewMsg );
                    if( ERROR( ret ) )
                        break;
                    msgQue.push_back( pNewMsg );
                }
                else if( dwPayload < dwSize )
                {
                    queBuf.push_back( pBuf );
                    ret = STATUS_MORE_PROCESS_NEEDED;
                }
                else 
                {
                    ret = -EBADMSG;
                    break;
                }
            }
            else
            {
                guint32 dwSize = 0;
                BufPtr& pHeadBuf = queBuf.front();
                memcpy( &dwSize, pHeadBuf->ptr(),
                    sizeof( dwSize ) );
                dwSize = ntohl( dwSize );
                guint32 dwPayload = pHeadBuf->size() -
                    sizeof( dwSize ) + pBuf->size();
                if( dwPayload == dwSize )
                {
                    pHeadBuf->append(
                        pBuf->ptr(), pBuf->size() );

                    DMsgPtr pMsg;
                    ret = pMsg.Deserialize( *pHeadBuf );
                    if( ERROR( ret ) )
                        break;
                    stdstr strSender = ":";
                    strSender +=
                        std::to_string( hstm )
                    pMsg.SetSender( strSender );
                    DMsgPtr pNewMsg;
                    ret = BuildNewMsgToSvr(
                        pMsg, pNewMsg );
                    if( ERROR( ret ) )
                        break;
                    msgQue.push_back( pNewMsg );
                    queBuf.pop_front( hstm );
                }
                else if( dwPayload < dwSize )
                {
                    pHeadBuf->append(
                        pBuf->ptr(), pBuf->size() );
                    ret = STATUS_MORE_PROCESS_NEEDED;
                }
                else
                {
                    ret = -EBADMSG;
                }
            }

        }while( 0 );

        return ret;
    }

    gint32 FillReadBuf( HANDLE hstm )
    {
        if( hstm == INVALID_HANDLE )
            return -EINVAL;

        gint32 ret = 0;
        do{
            auto pPort = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 
            PortPtr pPdoPort;
            ret = pPort->GetStreamPort(
                hstm, pPdoPort );
            if( ERROR( ret ) )
                break;

            CReadLock oLock( this->GetSharedLock() );
            auto itr = m_mapMsgRead.find( hstm );
            if( itr == m_mapMsgRead.end() )
                break;

            CPort* pPdo = pPdoPort;
            CStdRMutex oPortLock( pPdo->GetLock() );

            auto& msgElem = itr.second;
            auto& msgQue = msgElem.second;
            auto& dwReqCount = msgElem.first;

            if( dwReqCount <= msgQue.size() )
                break;

            oPortLock.Unlock();
            oLock.Unlock();

            BufPtr pBuf;
            ret = this->ReadStreamNoWait(
                hstm, pBuf );

            if( ERROR( ret ) )
                break;

            ret = FillReadBuf( hstm, pBuf );

        }while( 0 );

        return ret;
    }

    gint32 DispatchData( HANDLE hstm )
    {
        gint32 ret = 0;    
        do{
            auto pPort = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 
            PortPtr pPdoPort;
            ret = pPort->GetStreamPort(
                hstm, pPdoPort );
            if( ERROR( ret ) )
                break;

            std::vector< DMsgPtr > vecMsgs;
            CReadLock oLock( this->GetLock() );
            if( this->GetState() != stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }

            auto itr = m_mapMsgRead.find( hstm );
            if( itr == m_mapMsgRead.end() )
                break;

            CPort* pPdo = pPdoPort;
            CStdRMutex oPortLock( pPdo->GetLock() );

            auto& msgElem = itr.second;
            auto& msgQue = msgElem.second;
            if( msgQue.empty() )
                break;

            guint32 dwCount = ( gint32 )std::min(
                ( guint32 )msgElem.first,
                msgQue.size() );

            gint32 iCount = ( gint32 )dwCount;
            while( iCount > 0 )
            {
                --iCount;
                vecMsgs.push_back( msgQue.front() )
                msgQue.pop_front();
            }

            msgElem.first -= dwCount;
            oPortLock.Unlock();
            oLock.Unlock();

            BufPtr pBuf( true );
            for( auto& elem : vecMsgs )
            {
                *pBuf = elem;
                auto ptr = ( CBuffer* )pBuf;
                pPdoPort->OnEvent(
                    cmdDispatchData, 0, 0,
                    ( LONGWORD* )ptr );
            }

        }while( 0 );

        return ret;
    }

    gint32 OnStmRecv( HANDLE hstm,
        BufPtr& pBuf ) override
    {
        gint32 ret = 0;
        ret = super::OnStmRecv( hstm, pBuf );
        if( ERROR( ret ) )
            return ret;

        FillReadBuf( hstm );
        DispatchData( hstm );
        return ret;
    }

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort ) override
    {
        gint32 ret = super::OnRmtSvrEvent(
             iEvent, pEvtCtx, hPort );
        if( ERROR( ret ) )
            return ret;

        if( IsServer() )
            return ret;

        if( iEvent == eventRmtSvrOnline )
        {
            return ret;
        }

        return ret;
    }

    gint32 OnStartStopComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );
    {
        if( pIoReq == nullptr ||
            pReqCtx == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CCfgOpener oReqCtx( pReqCtx );
            PIRP pIrp = nullptr;
            ret = oReqCtx.GetPointer(
                    propIrpPtr, pIrp );
            if( ERROR( ret ) )
                break;

            CDBusStreamBusPort* pPort = nullptr;
            ret = oReqCtx.GetPointer(
                    propPortPtr, pPort );
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oReqCfg( pIoReq );
            IConfigDb* pResp;
            gint32 iRet = oReqCfg.GetPointer(
                propRespPtr, pResp );
            
            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue(
                IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            if( pIrp->GetStackSize() == 0 )
            {
                ret = -EINVAL;
                break;
            }

            pIrpCtxPtr& pCtx =
                    pIrp->GetTopStack();
            if( ERROR( iRet ) )
            {
                pCtx->SetStatus( iRet );
            }
            else
            {
                CCfgOpener oResp( pResp );
                ret = oResp.GetIntProp(
                    propReturnValue, iRet );
                if( ERROR( ret ) )
                    pCtx->SetStatus( ret );
                else
                    pCtx->SetStatus( iRet );
            }

            guint32 dwMinCmd =
                    pCtx->GetMinorCmd();
            if( dwMinCmd == IRP_MN_PNP_START &&
                SUCCEEDED( pCtx->GetStatus() ) )
            {
                ObjPtr pIf = this;
                // binding the two
                pPort->SetStreamIf( pIf );
                SetPort( pPort );
            }

            oIrpLock.Unlock();

            auto pMgr = GetIoMgr();
            pMgr->CompleteIrp( pIrp );

        }while( 0 );

        return ret;
    }

#define OnStopStmComplete OnStartStmComplete

    gint32 OnStartStmComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );
    {
        if( pIoReq == nullptr ||
            pReqCtx == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CCfgOpener oReqCtx( pReqCtx );
            PIRP pIrp = nullptr;
            ret = oReqCtx.GetPointer(
                    propIrpPtr, pIrp );
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oReqCfg( pIoReq );
            IConfigDb* pResp;
            gint32 iRet = oReqCfg.GetPointer(
                propRespPtr, pResp );
            
            CDBusStreamPdo* pPort = nullptr;
            ret = oReqCtx.GetPointer(
                    propPortPtr, pPort );
            if( ERROR( ret ) )
                break;

            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue(
                IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            if( pIrp->GetStackSize() == 0 )
            {
                ret = -EINVAL;
                break;
            }

            CCfgOpener oResp( pResp );
            pIrpCtxPtr& pCtx =
                    pIrp->GetTopStack();
            if( ERROR( iRet ) )
            {
                pCtx->SetStatus( iRet );
            }
            else
            {
                ret = oResp.GetIntProp(
                    propReturnValue, iRet );
                if( ERROR( ret ) )
                    pCtx->SetStatus( ret );
                else
                    pCtx->SetStatus( iRet );
            }

            guint32 dwMinCmd =
                    pCtx->GetMinorCmd();
            if( dwMinCmd == IRP_MN_PNP_START &&
                SUCCEEDED( pCtx->GetStatus() ) )
            {
                HANDLE hstm = INVALID_HANDLE;
                ret = oResp.GetIntPtr(
                    1, ( guint32*& )hstm );
                if( ERROR( ret ) )
                    pCtx->SetStatus( ret );
                else
                    pPort->SetStream( hstm );
            }
            oIrpLock.Unlock();

            auto pMgr = GetIoMgr();
            pMgr->CompleteIrp( pIrp );

        }while( 0 );

        return ret;
    }
    
};

class CRpcStmChanCli :
    public CRpcReqStreamBase< true >
{
    public:
    typedef CRpcStmChanBase< true > super;
    CRpcStmChanCli( const IConfigDb* pCfg )
        : super( pCfg )
    {}

};

class CRpcStmChanSvr :
    public CRpcStmChanBase< false >
{
    public:
    typedef CRpcStmChanBase< false > super;
    CRpcStmChanSvr( const IConfigDb* pCfg )
        : super( pCfg )
    {}
};

template< bool bProxy >
class CFastRpcSkelBase
    public IFBASE2< bProxy >
{
    InterfPtr m_pStreamIf;

    public:

    typedef typename IFBASE2( bProxy ) super;
    typedef super::super _MyVirtBase;

    CFastRpcSkeltonBase(
        const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
    {}

    InterfPtr GetStreamIf()
    { return m_pStreamIf; }

    void SetStreamIf( InterfPtr pIf )
    { m_pStreamIf = pIf; }
};

class CRpcSkelProxyState :
    public CInterfaceState
{
    public:
    typedef CInterfaceState super;
    CRpcSkelProxyState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRpcSkelProxyState ) ); }
    gint32 SubscribeEvents();
};

class CRpcSkelServerState :
    public CIfServerState
{
    public:
    typedef CIfServerState super;
    CRpcSkelProxyState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRpcSkelProxyState ) ); }
    gint32 SubscribeEvents();
};

class CFastRpcServerState :
    public CIfServerState
{
    public:
    typedef CIfServerState super;
    CFastRpcServerState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CFastRpcServerState ) ); }
    gint32 SubscribeEvents();
};

class CFastRpcProxyState :
    public CRemoteProxyState
{
    public:
    typedef CRemoteProxyState super;
    CFastRpcProxyState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CFastRpcProxyState ) ); }
    gint32 SubscribeEvents();
};

class CFastRpcSkelProxyBase :
    public CFastRpcSkelBase< true >
{
    public:
    typedef CFastRpcSkelBase< true > super;
    CFastRpcSkelProxyBase( const IConfigDb* pCfg )
        : _MyVirtBase( pCfg ), super( pCfg )
    {}
};

class CFastRpcSkelSvrBase :
    public CFastRpcSkelBase< false >
{
    public:
    typedef CFastRpcSkelBase< false > super;
    CFastRpcSkelServerBase( const IConfigDb* pCfg )
        : _MyVirtBase( pCfg ), super( pCfg )
    {}
};

class CFastRpcServerBase :
    public IFBASE3< false >
{
    public:
    typedef IFBASE3< false > super;

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort ) override;

    gint32 OnPostStart(
        IEventSink* pCallback ) override;

    gint32 OnPreStop(
        IEventSink* pCallback ) override;
};

class CFastRpcProxyBase :
    public IFBASE3< true >
{
    InterfPtr m_pSkelImp;
    public:
    typedef IFBASE3< true > super;

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort ) override;

    gint32 OnPostStart(
        IEventSink* pCallback ) override;

    gint32 OnPreStop(
        IEventSink* pCallback ) override;
};

DECLARE_AGGREGATED_SERVER(
     CRpcStreamChannelSvr,
     CRpcStmChanSvr ); 

DECLARE_AGGREGATED_PROXY(
    CRpcStreamChannelCli,
    CRpcStmChanCli );

}
