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

#define IFBASE1( _bProxy ) std::conditional< \
    _bProxy, CStreamProxyAsync, CStreamServerAsync>::type

#define STMBASE( _bProxy ) std::conditional< \
    _bProxy, CStreamProxy, CStreamServer>::type

#define IFBASE3( _bProxy ) std::conditional< \
    _bProxy, CAggInterfaceProxy, CAggInterfaceServer>::type

#define MAX_REQCHAN_PER_SESS 200

namespace rpcf
{

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
        std::pair< guint32, std::deque< DMsgPtr > >;
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

            PortPtr pPdoPort;
            auto pBus = static_cast
                < CDBusStreamBusPort* >( m_pPort ); 
            CStdRMutex oBusLock( pBus->GetLock() );
            pBus->GetStreamPort( hstm, pPdoPort );
            if( pPdoPort.IsEmpty() )
            {
                CCfgOpenerObj oIfCfg(
                    ( CObjBase* )pUxIf );
                // notify the stream pdo, a close
                // needed, in case a 'tokError' or
                // 'tokClose' arrives too early
                oIfCfg.SetBoolProp( propOnline, false );
                break;
            }
            oBusLock.Unlock();

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
            return 0;
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

            FillReadBuf( hstm );

            TaskletPtr pTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                -1, pTask, this,
                &CRpcStmChanBase::DispatchData, 
                ( guint64 )hstm );
            
            if( ERROR( ret ) )
                break;

            auto pMgr = this->GetIoMgr();
            ret = pMgr->RescheduleTask( pTask );

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
        DMsgPtr& pFwrdMsg,
        DMsgPtr& pNewMsg )
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
                        std::to_string( hstm );
                    pMsg.SetSender( strSender );
                    DMsgPtr pNewMsg;

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

                    DMsgPtr pMsg;
                    ret = pMsg.Deserialize( *pHeadBuf );
                    if( ERROR( ret ) )
                        break;
                    stdstr strSender = ":";
                    strSender +=
                        std::to_string( hstm );
                    pMsg.SetSender( strSender );
                    DMsgPtr pNewMsg;
                    ret = BuildNewMsgToSvr(
                        hstm, pMsg, pNewMsg );
                    if( ERROR( ret ) )
                        break;
                    msgQue.push_back( pNewMsg );
                    queBuf.pop_front();
                }
                else if( dwPayload < dwSize )
                {
                    pHeadBuf->Append(
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

            auto& msgElem = itr->second;
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

            std::vector< DMsgPtr > vecMsgs;
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
        DispatchData( ( guint64 )hstm );
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

            auto pMgr = this->GetIoMgr();
            pMgr->CompleteIrp( pIrp );

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
            else if( dwCmd == IRP_MN_PNP_STOP )
            {
                SetPreStopStep( pIrp, 1 );
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

template< bool bProxy >
class CFastRpcSkelBase :
    public IFBASE2( bProxy )
{
    InterfPtr m_pParent;

    public:

    typedef typename IFBASE2( bProxy ) super;
    typedef typename super::super _MyVirtBase;

    CFastRpcSkelBase(
        const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
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
    CRpcServices* GetStreamIf() override
    { return m_pParent; }

    InterfPtr& GetParentIf()
    { return m_pParent; };

    void SetParentIf( InterfPtr& pIf )
    { return m_pParent = pIf; };
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
        : _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 NotifyStackReady( PortPtr& pPort );
    inline guint32 GetPendingInvCount() const
    { return m_dwPendingInv; }

    inline void QueueStartTask( TaskletPtr& pTask )
    { m_queStartTasks.push_back( pTask ); }

    gint32 AddAndRunInvTask(
        TaskletPtr& pTask,
        bool bImmediate = false );
    guint32 NotifyInvTaskComplete();
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

    public:
    typedef IFBASE3( false ) super;
    CFastRpcServerBase( const IConfigDb* pCfg ) :
        super( pCfg ) 
    {}

    gint32 GetStmSkel(
        HANDLE hstm, InterfPtr& pIf );

    gint32 AddStmSkel(
        HANDLE hstm, InterfPtr& pIf );

    gint32 RemoveStmSkel(
        HANDLE hstm );

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

    gint32 CheckReqCtx(
        IEventSink* pCallback,
        DMsgPtr& pMsg ) override;

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
};

class CFastRpcProxyBase :
    public virtual IFBASE3( true )
{
    InterfPtr m_pSkelObj;

    public:
    typedef IFBASE3( true ) super;

    CFastRpcProxyBase( const IConfigDb* pCfg ):
        super( pCfg ) 
    {}

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
};

DECLARE_AGGREGATED_SERVER(
     CRpcStreamChannelSvr,
     CRpcStmChanSvr ); 

DECLARE_AGGREGATED_PROXY(
    CRpcStreamChannelCli,
    CRpcStmChanCli );

}
