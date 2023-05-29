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
#include "counters.h"

#define IFBASE2( _bProxy ) std::conditional< \
    _bProxy, CStreamProxyWrapper, CStreamServerWrapper>::type

#define IFBASE1( _bProxy ) std::conditional< \
    _bProxy, CStreamProxyAsync, CStreamServerAsync>::type

#define IFBASE3( _bProxy ) std::conditional< \
    _bProxy, CAggInterfaceProxy, CAggInterfaceServer>::type

#define MAX_REQCHAN_PER_SESS 200
#define DBUS_STREAM_BUS_DRIVER  "DBusStreamBusDrv"
#define CTRLCODE_SKEL_READY             0x81

namespace rpcf
{

struct FASTRPC_MSG
{
    guint32 m_dwSize = 0;
    guint8 m_bType = typeObj;
    guint8 m_szSig[ 3 ] = { 'F', 'R', 'M' };
    CfgPtr m_pCfg;

    gint32 Serialize( CBuffer& oBuf );
    gint32 Deserialize( CBuffer& oBuf );
};

struct SESS_INFO
{
    stdstr m_strSessHash;
    stdstr m_strRouterPath;
};

template< bool bProxy >
class CRpcStmChanBase :
    public IFBASE1( bProxy ) 
{
    protected:
    timespec m_tsStartTime;

    CSharedLock m_oSharedLock;

    std::hashmap< HANDLE, SESS_INFO > m_mapStm2Sess;
    std::map< stdstr, guint32 > m_mapSessRefs;

    using MSGQUE=
        std::pair< guint32, std::deque< CfgPtr > >;
    std::hashmap< HANDLE, MSGQUE  > m_mapMsgRead;
    using BUFQUE= std::deque< BufPtr >;
    std::hashmap< HANDLE, BUFQUE > m_mapBufReading;
    std::set< HANDLE > m_setStreams;

    IPort* m_pPort = nullptr;

    public:
    typedef typename IFBASE1( bProxy ) super;

    CSharedLock& GetSharedLock()
    { return m_oSharedLock; }

    CRpcStmChanBase( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {
        clock_gettime(
            CLOCK_REALTIME, &m_tsStartTime );
    }

    inline void SetPort( IPort* pPort )
    { m_pPort = pPort; }

    gint32 OnClose( HANDLE hstm,
        IEventSink* pCallback = nullptr ) override
    {
        gint32 ret = 0;
        do{
            InterfPtr pUxIf;
            ret = this->GetUxStream( hstm, pUxIf );
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pUxIf );

            oIfCfg.SetBoolProp(
                propOnline, false );

            PortPtr pPdoPort;
            auto pBus = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 

            ret = pBus->GetStreamPort(
                hstm, pPdoPort );

            if( ERROR( ret ) )
                break;

            pPdoPort->OnEvent(
                eventDisconn, hstm, 0, nullptr );

        }while( 0 );

        return ret;
    }

    gint32 ActiveClose( HANDLE hstm,
        IEventSink* pCallback = nullptr )
    {
        if( hstm == INVALID_HANDLE )
            return -EINVAL;

        InterfPtr pUxIf;
        gint32 ret = this->GetUxStream( hstm, pUxIf );
        if( ERROR( ret ) )
            return ret;
        OnStmClosing( hstm );
        return IStream::OnClose( hstm, pCallback );
    }

    gint32 GetSessHash(
        HANDLE hStream, stdstr& strSess ) const
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
            strSess = itr->second.m_strSessHash;

        }while( ret );
        return ret;
    }

    gint32 GetSessInfo( HANDLE hStream,
        SESS_INFO& stmInfo )
    {
        gint32 ret = 0;
        do{
            auto itr =
                m_mapStm2Sess.find( hStream );
            if( itr == m_mapStm2Sess.end() )
            {
                ret = -ENOENT;
                break;
            }
            stmInfo = itr->second;

        }while( ret );
        return ret;
    }

    gint32 OnStreamReadyProxy( HANDLE hstm )
    {
        gint32 ret = 0;
        do{
            CWriteLock oLock( this->GetSharedLock() );

            m_setStreams.insert( hstm );

            auto& msgElem =
                    m_mapMsgRead[ hstm ];
            UNREFERENCED( msgElem );
            auto& bufElem =
                    m_mapBufReading[ hstm ];
            UNREFERENCED( bufElem );

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

            if( !this->IsServer() )
            {
                ret = OnStreamReadyProxy( hstm );
                break;
            }

            CfgPtr pDesc;
            ret = this->GetDataDesc( hstm, pDesc );
            if( ERROR( ret ) )
                break;

            IConfigDb* ptctx = nullptr;

            CCfgOpener oDesc(
                    ( IConfigDb* )pDesc );

            ret = oDesc.GetPointer(
                    propTransCtx, ptctx );
            if( ERROR( ret ) )
                break;

            stdstr strSess, strPath;
            CCfgOpener oCtx( ptctx );
            ret = oCtx.GetStrProp(
                propSessHash, strSess );
            if( ERROR( ret ) )
                break;
            ret = oCtx.GetStrProp(
                propRouterPath, strPath );
            if( ERROR( ret ) )
                break;

            CWriteLock oLock( this->GetSharedLock() );

            m_setStreams.insert( hstm );

            auto& msgElem =
                    m_mapMsgRead[ hstm ];
            UNREFERENCED( msgElem );
            auto& bufElem =
                    m_mapBufReading[ hstm ];
            UNREFERENCED( bufElem );

            m_mapStm2Sess[ hstm ] =
                    { strSess, strPath };

            oLock.Unlock();

            ret = m_pPort->OnEvent(
                eventNewConn, hstm, 0, nullptr );

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

            auto& msgElem = itr->second;
            auto& dwReqCount = msgElem.first;
            dwReqCount++;

            oPortLock.Unlock();
            oLock.Unlock();

            ret = FillReadBuf( hstm );
            if( SUCCEEDED( ret ) )
            {
                TaskletPtr pTask;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    -1, pTask, this,
                    &CRpcStmChanBase::DispatchData, 
                    ( guint64 )hstm );
                
                if( ERROR( ret ) )
                    break;

                auto pMgr = this->GetIoMgr();
                ret = pMgr->RescheduleTask( pTask );
            }

        }while( 0 );

        return ret;
    }

    gint32 OnStmClosingProxy( HANDLE hstm )
    {
        gint32 ret = 0;
        do{
            ret = super::OnStmClosing( hstm );

            CWriteLock oLock( this->GetSharedLock() );
            m_mapMsgRead.erase( hstm );
            m_mapBufReading.erase( hstm );
            m_setStreams.erase( hstm );

        }while( 0 );

        return ret;
    }

    gint32 OnStmClosing( HANDLE hstm ) override
    {
        gint32 ret = 0;
        do{
            if( !this->IsServer() )
            {
                ret = OnStmClosingProxy( hstm );
                break;
            }

            ret = super::OnStmClosing( hstm );

            CWriteLock oLock( this->GetSharedLock() );

            m_mapMsgRead.erase( hstm );
            m_mapBufReading.erase( hstm );
            m_setStreams.erase( hstm );

            auto itr =
                m_mapStm2Sess.find( hstm );
            if( itr == m_mapStm2Sess.end() )
                break;

            stdstr strSess =
                itr->second.m_strSessHash;
            m_mapStm2Sess.erase( itr );

            auto itr2 = m_mapSessRefs.find( strSess );
            if( itr2 == m_mapSessRefs.end() )
                break;

            if( itr2->second <= 1 )
            {
                m_mapSessRefs.erase( itr2 );
                break;
            }
            --itr2->second;

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
            IrpCtxPtr& pCtx = pIrp->GetTopStack();

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
        IConfigDb* pFwrdMsg,
        CfgPtr& pNewMsg )
    {
        // for compatible format with the message from
        // the bridge
        gint32 ret = 0;
        do{
            if( !this->IsServer() )
            {
                pNewMsg = pFwrdMsg;
                break;
            }
            // append a session hash for access
            // control
            CCfgOpener oReqCtx;
            SESS_INFO si;
            ret = GetSessInfo( hstm, si );
            if( ERROR( ret ) )
                break;

            oReqCtx.SetStrProp( propRouterPath,
                si.m_strRouterPath );

            oReqCtx.SetStrProp( propSessHash,
                si.m_strSessHash );

            oReqCtx.SetIntPtr( propStmHandle,
                ( guint32* )hstm );

            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            oReqCtx.SetQwordProp(
                propTimestamp, ts.tv_sec );

            CCfgOpener oReq( pFwrdMsg );
            oReq.SetPointer( propContext,
                ( IConfigDb* )oReqCtx.GetCfg() );

            pNewMsg = pFwrdMsg;

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

            auto& msgElem = itr->second;
            auto& msgQue = msgElem.second;

            BufPtr pBufReady;
            auto itr1 =
                m_mapBufReading.find( hstm );
            if( itr1 == m_mapBufReading.end() )
            {
                ret = -ENOENT;
                break;
            }
            
            BUFQUE& queBuf = itr1->second;
            if( queBuf.empty() )
            {
                guint32 dwSize = 0;
                memcpy( &dwSize, pBuf->ptr(),
                    sizeof( dwSize ) );
                dwSize = ntohl( dwSize );
                if( dwSize > MAX_BYTES_PER_BUFFER )
                {
                    ret = -EBADMSG;
                    break;
                }
                guint32 dwPayload =
                    pBuf->size() - sizeof( dwSize );
                if( dwPayload == dwSize )
                {
                    CfgPtr pMsg;
                    FASTRPC_MSG oMsg;
                    ret = oMsg.Deserialize( *pBuf );
                    if( ERROR( ret ) )
                        break;
                    if( oMsg.m_bType != typeObj )
                    {
                        ret = -EBADMSG;
                        break;
                    }
                    pMsg = oMsg.m_pCfg;
                    stdstr strSender = ":";
                    strSender +=
                        std::to_string( hstm );
                    CCfgOpener oPayload(
                        ( IConfigDb* )pMsg );
                    oPayload[ propSrcDBusName ] =
                        strSender;

                    CfgPtr pNewMsg;
                    ret = BuildNewMsgToSvr(
                        hstm, pMsg, pNewMsg );
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
                    pHeadBuf->Append(
                        pBuf->ptr(), pBuf->size() );

                    CfgPtr pMsg;
                    FASTRPC_MSG oMsg;
                    ret = oMsg.Deserialize( *pBuf );
                    if( ERROR( ret ) )
                        break;
                    if( oMsg.m_bType != typeObj )
                    {
                        ret = -EBADMSG;
                        break;
                    }
                    pMsg = oMsg.m_pCfg;
                    stdstr strSender = ":";
                    strSender +=
                        std::to_string( hstm );

                    CCfgOpener oPayload(
                        ( IConfigDb* )pMsg );

                    oPayload[ propSrcDBusName ] =
                        strSender;

                    CfgPtr pNewMsg;
                    ret = BuildNewMsgToSvr(
                        hstm, pMsg, pNewMsg );
                    if( ERROR( ret ) )
                        break;
                    msgQue.push_back( pNewMsg );
                    queBuf.pop_front();
                }
                else if( dwPayload < dwSize )
                {
                    ret = pHeadBuf->Append(
                        pBuf->ptr(), pBuf->size() );
                    if( ERROR( ret ) )
                        break;
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
            bool bDispatch = false;
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
            {
                ret = -ENOENT;
                break;
            }

            CPort* pPdo = pPdoPort;
            CStdRMutex oPortLock( pPdo->GetLock() );

            auto& msgElem = itr->second;
            auto& msgQue = msgElem.second;
            auto& dwReqCount = msgElem.first;

            if( dwReqCount == 0 )
            {
                ret = -EAGAIN;
                break;
            }

            gint32 iToRead =
                dwReqCount - msgQue.size();
            if( iToRead <= 0 )
            {
                // ready to dispatch
                break;
            }

            if( msgQue.size() )
                bDispatch = true;

            oPortLock.Unlock();
            oLock.Unlock();

            for( int i = 0; i < iToRead; ++i )
            {
                BufPtr pBuf;
                ret = this->ReadStreamNoWait(
                    hstm, pBuf );
                if( ERROR( ret ) )
                    break;
                ret = FillReadBuf( hstm, pBuf );
                if( ERROR( ret ) )
                    break;
                if( unlikely( ret ==
                    STATUS_MORE_PROCESS_NEEDED ) )
                {
                    i--;
                    continue;
                }
                bDispatch = true;
            }

            if( bDispatch )
                ret = 0;

        }while( 0 );

        return ret;
    }

    gint32 DispatchData( guint64 data )
    {
        gint32 ret = 0;    
        do{
            HANDLE hstm = ( HANDLE )data;
            auto pPort = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 
            PortPtr pPdoPort;
            ret = pPort->GetStreamPort(
                hstm, pPdoPort );
            if( ERROR( ret ) )
                break;

            std::vector< CfgPtr > vecMsgs;
            CReadLock oLock( this->GetSharedLock() );
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

            auto& msgElem = itr->second;
            auto& msgQue = msgElem.second;
            if( msgQue.empty() )
                break;

            guint32 dwCount = ( gint32 )std::min(
                ( guint32 )msgElem.first,
                ( guint32 )msgQue.size() );

            gint32 iCount = ( gint32 )dwCount;
            while( iCount > 0 )
            {
                --iCount;
                vecMsgs.push_back( msgQue.front() );
                msgQue.pop_front();
            }

            msgElem.first -= dwCount;
            oPortLock.Unlock();
            oLock.Unlock();

            BufPtr pBuf( true );
            dwCount = 0;
            for( auto& elem : vecMsgs )
            {
                *pBuf = elem;
                auto ptr = ( CBuffer* )pBuf;
                gint32 iRet = pPdoPort->OnEvent(
                    cmdDispatchData, 0, 0,
                    ( LONGWORD* )ptr );
                if( ERROR( iRet ) )
                    ++dwCount;
            }
            if( dwCount > 0 )
            {
                oLock.Lock();
                oPortLock.Lock();
                // restore the count for failed dispatch
                auto itr = m_mapMsgRead.find( hstm );
                if( itr == m_mapMsgRead.end() )
                    break;
                auto& msgElem = itr->second;
                msgElem.first += dwCount;
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

        ret = FillReadBuf( hstm );
        if( SUCCEEDED( ret ) )
            DispatchData( ( guint64 )hstm );
        else if( ret == -EAGAIN )
            ret = 0;
        return ret;
    }

    gint32 OnStartStopComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx )
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
            
            bool bDone = false;
            auto pMgr = this->GetIoMgr();
            do{
                CStdRMutex oIrpLock( pIrp->GetLock() );
                ret = pIrp->CanContinue(
                    IRP_STATE_READY );
                if( ERROR( ret ) )
                { 
                    pIrp = nullptr;
                    break;
                }

                if( pIrp->GetStackSize() == 0 )
                {
                    pIrp = nullptr;
                    ret = -EINVAL;
                    break;
                }

                IrpCtxPtr& pCtx =
                        pIrp->GetTopStack();
                if( ERROR( iRet ) )
                {
                    pCtx->SetStatus( iRet );
                }
                else
                {
                    gint32* pRet = &iRet;
                    CCfgOpener oResp( pResp );
                    ret = oResp.GetIntProp(
                        propReturnValue,
                        *( guint32* )pRet );

                    if( ERROR( ret ) )
                        pCtx->SetStatus( ret );
                    else
                        pCtx->SetStatus( iRet );
                }

                bool bStart = false;
                guint32 dwMinCmd =
                    pCtx->GetMinorCmd();

                ret = pCtx->GetStatus();
                if( dwMinCmd == IRP_MN_PNP_START )
                    bStart = true;
                if( bStart && SUCCEEDED( ret ) )
                {
                    ObjPtr pIf = this;
                    // binding the two
                    pPort->SetStreamIf( pIf );
                    SetPort( pPort );
                }

                oIrpLock.Unlock();

                if( !bStart || SUCCEEDED( ret ) )
                {
                    pMgr->CompleteIrp( pIrp );
                    bDone = true;
                    break;
                }

            }while( 0 );

            if( bDone )
                break;

            // failed to start, stop the interface
            TaskGrpPtr pTaskGrp;
            do{
                CParamList oParams;
                oParams[ propIfPtr ] = ObjPtr( this );

                ret = pTaskGrp.NewObj(
                    clsid( CIfTaskGroup ),
                    oParams.GetCfg() );
                if( ERROR( ret ) )
                    break;
                pTaskGrp->SetRelation( logicNONE );

                TaskletPtr pStopTask;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pStopTask, ObjPtr( this ),
                    &CRpcServices::StopEx, nullptr );
                if( ERROR( ret ) )
                    break;
                pTaskGrp->AppendTask( pStopTask );

                if( pIrp != nullptr )
                {
                    TaskletPtr pCompTask;
                    ret = DEFER_OBJCALL_NOSCHED(
                        pCompTask, pMgr,
                        &CIoManager::CompleteIrp, pIrp );
                    if( ERROR( ret ) )
                        break;
                    pTaskGrp->AppendTask( pCompTask );
                }
                TaskletPtr pTask = pTaskGrp;
                ret = pMgr->RescheduleTask( pTask );   

            }while( 0 );

            if( ERROR( ret ) && !pTaskGrp.IsEmpty() )
            {
                ( *pTaskGrp )( eventCancelTask );
                if( pIrp != nullptr )
                    pMgr->CompleteIrp( pIrp );
            }

        }while( 0 );

        return ret;
    }

#define OnStopStmComplete OnStartStmComplete

    gint32 OnStartStmComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx )
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
            CDBusStreamPdo* pPort = nullptr;
            ret = oReqCtx.GetPointer(
                    propPortPtr, pPort );
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

            IrpCtxPtr& pCtx =
                    pIrp->GetTopStack();
            IConfigDb* pResp = nullptr;
            gint32 iRet = oReqCfg.GetPointer(
                propRespPtr, pResp );
            
            CCfgOpener oResp( pResp );
            if( ERROR( iRet ) )
            {
                pCtx->SetStatus( iRet );
                ret = iRet;
            }
            else
            {
                gint32* pRet = &iRet;
                ret = oResp.GetIntProp(
                    propReturnValue,
                    *( guint32* )pRet );
                if( ERROR( ret ) )
                    pCtx->SetStatus( ret );
                else
                {
                    pCtx->SetStatus( iRet );
                    ret = iRet;
                }
            }

            guint32 dwCmd = pCtx->GetMinorCmd();
            if( dwCmd == IRP_MN_PNP_START &&
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

            auto pMgr = this->GetIoMgr();
            pMgr->CompleteIrp( pIrp );

        }while( 0 );

        return ret;
    }
    gint32 OnPostStop(
        IEventSink* pCallback )
    {
        return 0;
    }
};

class CRpcStmChanCli :
    public CRpcStmChanBase< true >
{
    public:
    typedef CRpcStmChanBase< true > super;
    CRpcStmChanCli( const IConfigDb* pCfg )
        : _MyVirtBase( pCfg ), super( pCfg )
    {}
};

class CRpcStmChanSvr :
    public CRpcStmChanBase< false >
{
    public:
    typedef CRpcStmChanBase< false > super;
    CRpcStmChanSvr( const IConfigDb* pCfg )
        : _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 AcceptNewStream(
        IEventSink* pCallback,
        IConfigDb* pDataDesc ) override;
};

class CFastRpcServerState :
    public CIfServerState
{
    public:
    typedef CIfServerState super;
    CFastRpcServerState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CFastRpcServerState ) ); }

    gint32 SubscribeEvents()
    {
        std::vector< EnumPropId > vecEvtToSubscribe =
            { propRmtSvrEvent };
        return SubscribeEventsInternal(
            vecEvtToSubscribe );
    }
};

class CFastRpcProxyState :
    public CRemoteProxyState
{
    public:
    typedef CRemoteProxyState super;
    CFastRpcProxyState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CFastRpcProxyState ) ); }

    gint32 SubscribeEvents()
    {
        std::vector< EnumPropId > vecEvtToSubscribe =
            { propRmtSvrEvent };
        return SubscribeEventsInternal(
            vecEvtToSubscribe );
    }
};

class CFastRpcServerBase :
    public virtual IFBASE3( false )
{
    protected:
    std::hashmap< HANDLE, InterfPtr > m_mapSkelObjs;
    std::atomic< guint32 > m_dwSeqNo;
    guint32 m_dwBusId = 0xFFFFFFFF;

    gint32 OnPreStartComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    public:
    typedef IFBASE3( false ) super;
    CFastRpcServerBase( const IConfigDb* pCfg ) :
        super( pCfg ), m_dwSeqNo( 0 ) 
    {}

    inline gint32 NewSeqNo()
    { return m_dwSeqNo++; }

    gint32 GetStmSkel(
        HANDLE hstm, InterfPtr& pIf );

    gint32 AddStmSkel(
        HANDLE hstm, InterfPtr& pIf );

    gint32 RemoveStmSkel(
        HANDLE hstm );

    gint32 EnumStmSkels(
        std::vector< InterfPtr >& vecIfs );

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort ) override;

    gint32 OnPreStop(
        IEventSink* pCallback ) override;

    virtual gint32 CreateStmSkel(
        HANDLE hStream,
        guint32 dwPortId,
        InterfPtr& pIf ) = 0;

    gint32 GetStream(
        IEventSink* pCallback,
        HANDLE& hStream );

    gint32 OnStartSkelComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    gint32 BroadcastEvent(
        IConfigDb* pReqCall,
        IEventSink* pCallback ) override;

    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 OnPostStop(
        IEventSink* pCallback ) override;

    inline guint32 GetBusId() const
    { return m_dwBusId; }

    inline void SetBusId( guint32 dwBusId )
    { m_dwBusId = dwBusId; }
};

class CFastRpcProxyBase :
    public virtual IFBASE3( true )
{
    InterfPtr m_pSkelObj;
    std::atomic< guint32 > m_dwSeqNo;
    guint32 m_dwBusId = 0xFFFFFFFF;

    gint32 OnPreStartComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    public:
    typedef IFBASE3( true ) super;

    CFastRpcProxyBase( const IConfigDb* pCfg ):
        super( pCfg ) 
    {}

    inline gint32 NewSeqNo()
    { return m_dwSeqNo++; }

    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 OnPostStart(
        IEventSink* pCallback ) override;

    gint32 OnPreStop(
        IEventSink* pCallback ) override;

    gint32 OnPostStop(
        IEventSink* pCallback ) override;

    virtual gint32 CreateStmSkel(
        InterfPtr& pIf ) = 0;

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort ) override;

    InterfPtr GetStmSkel() const
    { return m_pSkelObj; }

    guint32 GetBusId() const
    { return m_dwBusId; }

    void SetBusId( guint32 dwBusId )
    { m_dwBusId = dwBusId; }
};

template< bool bProxy >
class CFastRpcSkelBase :
    public IFBASE3( bProxy )
{
    InterfPtr m_pParent;

    public:

    typedef typename IFBASE3( bProxy ) super;

    CFastRpcSkelBase(
        const IConfigDb* pCfg ) :
        super( pCfg )
    {
        gint32 ret = 0;
        do{
            CCfgOpener oCfg( pCfg );
            ObjPtr pObj;
            ret = oCfg.GetObjPtr(
                propParentPtr, pObj );
            if( ERROR( ret ) )
                break;
            m_pParent = pObj;

        }while( 0 );

        return;
    }

    // this is for serialization
    CRpcServices* GetStreamIf()
    { return m_pParent; }

    InterfPtr& GetParentIf()
    { return m_pParent; };

    void SetParentIf( InterfPtr& pIf )
    { return m_pParent = pIf; };

    gint32 BuildBufForIrpInternal( BufPtr& pBuf,
        IConfigDb* pReqCall )
    {
        if( pReqCall == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            if( pBuf.IsEmpty() )
            {
                ret = pBuf.NewObj();
                if( ERROR( ret ) )
                    break;
            }

            CReqOpener oReq( pReqCall );
            gint32 iReqType;
            ret = oReq.GetReqType(
                ( guint32& )iReqType );

            if( ERROR( ret ) )
                break;

            if( iReqType == 
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                ret = -EINVAL;
                break;
            }

            stdstr strMethod;
            ret = oReq.GetMethodName( strMethod );
            if( ERROR( ret ) )
                break;

            if( strMethod == IF_METHOD_ENABLEEVT ||
                strMethod == IF_METHOD_DISABLEEVT ||
                strMethod == IF_METHOD_LISTENING )
            {
                // the m_reqData is an MatchPtr
                ret = this->BuildBufForIrpIfMethod(
                    pBuf, pReqCall );
                break;
            }

            guint32 dwSeqNo = 0;
            if( this->IsServer() )
            {
                CFastRpcServerBase* pSvr = GetParentIf();
                dwSeqNo = pSvr->NewSeqNo();
            }
            else
            {
                CFastRpcProxyBase* pProxy = GetParentIf();
                dwSeqNo = pProxy->NewSeqNo();
            }

            // remove redudant information from the
            // request to send
            oReq.RemoveProperty( propDestDBusName );
            oReq.RemoveProperty( propSrcDBusName );
            oReq.SetIntProp( propSeqNo, dwSeqNo );

            *pBuf = ObjPtr( pReqCall );

        }while( 0 );

        return ret;
    }

    gint32 CustomizeRequest(
        IConfigDb* pReqCfg,
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            CReqOpener oReqr( pReqCfg );
            CReqBuilder oReqw( pReqCfg );
            guint32 dwFlags = 0;
            ret = oReqr.GetCallFlags( dwFlags );
            if( ERROR( ret ) )
                break;
            dwFlags |= CF_NON_DBUS;
            oReqw.SetCallFlags( dwFlags );

        }while( 0 );

        return ret;
    }
    gint32 IncCounter( EnumPropId iProp,
        guint32 dwVal = 1 ) override
    {
        gint32 ret = 0;
        do{
            if( this->IsServer() )
            {
                CStatCountersServer* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->IncCounter( iProp, dwVal );
            }
            else
            {
                CStatCountersProxy* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->IncCounter( iProp, dwVal );
            }
        }while( 0 );
        return ret;
    }

    gint32 DecCounter( EnumPropId iProp,
        guint32 dwVal = 1 ) override
    {
        gint32 ret = 0;
        do{
            if( this->IsServer() )
            {
                CStatCountersServer* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->DecCounter( iProp, dwVal );
            }
            else
            {
                CStatCountersProxy* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->DecCounter( iProp, dwVal );
            }
        }while( 0 );
        return ret;
    }

    gint32 SetCounter( EnumPropId iProp,
        guint32 dwVal ) override
    {
        gint32 ret = 0;
        do{
            if( this->IsServer() )
            {
                CStatCountersServer* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->SetCounter( iProp, dwVal );
            }
            else
            {
                CStatCountersProxy* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->SetCounter( iProp, dwVal );
            }
        }while( 0 );
        return ret;
    }

    gint32 SetCounter( EnumPropId iProp,
        guint64 qwVal ) override
    {
        gint32 ret = 0;
        do{
            if( this->IsServer() )
            {
                CStatCountersServer* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->SetCounter( iProp, qwVal );
            }
            else
            {
                CStatCountersProxy* pIf =
                    GetParentIf();

                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pIf->SetCounter( iProp, qwVal );
            }
        }while( 0 );
        return ret;
    }
};

class CFastRpcSkelProxyState :
    public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;
    CFastRpcSkelProxyState( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CFastRpcSkelProxyState ) ); }

    gint32 SubscribeEvents()
    { return 0; }

    bool IsMyDest(
        const stdstr& strModName ) override
    { return false; }

    gint32 SetupOpenPortParams(
        IConfigDb* pCfg ) override;
};

class CFastRpcSkelServerState :
    public CIfServerState
{
    public:
    typedef CIfServerState super;
    CFastRpcSkelServerState( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CFastRpcSkelServerState ) ); }

    gint32 SubscribeEvents()
    { return 0; }

    bool IsMyDest(
        const stdstr& strModName ) override
    { return false; }

    gint32 SetupOpenPortParams(
        IConfigDb* pCfg ) override;
};

class CFastRpcSkelProxyBase :
    public CFastRpcSkelBase< true >
{
    public:
    typedef CFastRpcSkelBase< true > super;
    CFastRpcSkelProxyBase( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    gint32 BuildBufForIrp( BufPtr& pBuf,
        IConfigDb* pReqCall ) override;

    gint32 OnKeepAliveTerm(
        IEventSink* pTask ) override;
};

class CIfStartRecvMsgTask2 :
    public CIfStartRecvMsgTask
{
    public:
    typedef CIfStartRecvMsgTask super;
    CIfStartRecvMsgTask2( const IConfigDb* pCfg )
        : super( pCfg )
    {}
    gint32 StartNewRecv( ObjPtr& pCfg ) override;
    gint32 OnIrpComplete( PIRP pIrp ) override;
    template< typename T1, 
        typename T=typename std::enable_if<
            std::is_same<T1, CfgPtr>::value ||
            std::is_same<T1, DMsgPtr>::value, T1 >::type >
    gint32 HandleIncomingMsg2( ObjPtr& ifPtr,  T1& pMsg );
};

class CIfInvokeMethodTask2 :
    public CIfInvokeMethodTask
{
    public:
    typedef CIfInvokeMethodTask super;
    CIfInvokeMethodTask2( const IConfigDb* pCfg )
        : super( pCfg )
    {}
    gint32 OnComplete( gint32 iRet ) override;
};

class CFastRpcSkelSvrBase :
    public CFastRpcSkelBase< false >
{
    guint32 m_dwPendingInv = 0;
    std::deque< TaskletPtr > m_queStartTasks;

    public:
    typedef CFastRpcSkelBase< false > super;
    CFastRpcSkelSvrBase( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    gint32 NotifySkelReady( PortPtr& pPort );
    inline guint32 GetPendingInvCount() const
    { return m_dwPendingInv; }

    inline void QueueStartTask( TaskletPtr& pTask )
    { m_queStartTasks.push_back( pTask ); }

    gint32 AddAndRunInvTask(
        TaskletPtr& pTask,
        bool bImmediate = false );
    gint32 NotifyInvTaskComplete();

    gint32 StartRecvTasks(
        std::vector< MatchPtr >& vecMatches ) override;

    gint32 SendResponse(
        IEventSink* pInvTask,
        IConfigDb* pReq,
        CfgPtr& pRespData ) override;

    gint32 BuildBufForIrp( BufPtr& pBuf,
        IConfigDb* pReqCall ) override;

    gint32 OnKeepAliveOrig(
        IEventSink* pTask ) override;

    gint32 OnPreStop(
        IEventSink* pCallback ) override;
};

DECLARE_AGGREGATED_SERVER(
     CRpcStreamChannelSvr,
     CRpcStmChanSvr ); 

DECLARE_AGGREGATED_PROXY(
    CRpcStreamChannelCli,
    CRpcStmChanCli );

#define DECLARE_AGGREGATED_SKEL_PROXY( ClassName, ...) \
    DECLARE_AGGREGATED_PROXY_INTERNAL( CFastRpcSkelProxyBase, ClassName, ##__VA_ARGS__ )

#define DECLARE_AGGREGATED_SKEL_SERVER( ClassName, ...) \
    DECLARE_AGGREGATED_SERVER_INTERNAL( CFastRpcSkelSvrBase, ClassName, ##__VA_ARGS__ )
}
