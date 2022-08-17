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

#define IFBASE( _bProxy ) std::conditional< \
    _bProxy, CStreamProxyAsync, CStreamServerAsync>::type

namespace rpc{

using SESS_INFO=std::pair< stdstr, stdstr >;

template< bool bProxy >
class CRpcReqStreamBase :
    public virtual IFBASE( bProxy ) 
{
    timespec m_tsStartTime;
    std::hashmap< HANDLE, SESS_INFO > m_mapStm2Sess;
    std::map< stdstr, guint32 > m_mapSessRefs;

    using BUFQUE=std::deque< BufPtr >;
    std::map< HANDLE, DMsgPtr > m_mapMsgRead;
    std::hashmap< HANDLE, BufPtr > m_mapBufReading;
    guint32 m_dwNumReqs = 0;
    std::set< HANDLE > m_setStreams;
    std::list< HANDLE > m_lstStreams;
    std::list< HANDLE > m_lstStmSched;
    IPort* m_pPort = nullptr;

    public:
    typedef typename IFBASE( bProxy ) super;

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
            CStdRMutex oLock( this->GetLock() );
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
            CStdRMutex oLock( this->GetLock() );
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
                
            CStdRMutex oLock( this->GetLock() );

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

    gint32 OnStmReady( HANDLE hstm ) override
    {
        gint32 ret = 0;
        do{
            ret = super::OnStreamReady( hstm );
            CStdRMutex oLock( this->GetLock() );
            m_setStreams.insert( hstm );
            m_lstStmSched.push_back( hstm );

        }while( 0 );

        return ret;
    }

    gint32 AddRecvReq()
    {
        gint32 ret = 0;
        do{
            FillReadBuf();

            CStdRMutex oLock( this->GetLock() );
            m_dwNumReqs++;
            if( m_mapMsgRead.empty())
                break;

            TaskletPtr pTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, this,
                &CRpcReqStreamBase::DispatchData, 
                INVALID_HANDLE );
            
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
            ret = super::OnStmClosing( hstm );

            CStdRMutex oLock( this->GetLock() );
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
            auto itr = m_lstStreams.begin();
            while( itr != m_lstStreams.end() )
            {
                if( *itr == hstm )
                {
                    itr = m_lstStreams.erase( itr );
                    break;
                }
                else
                    itr++;
            }
            auto itr = m_lstStmSched.begin();
            while( itr != m_lstmSched.end() )
            {
                if( *itr == hstm )
                {
                    itr = m_lstStmSched.erase( itr );
                    break;
                }
                else
                    itr++;
            }
            oLock.Unlock();

        }while( 0 );

        return ret;
    }

    bool HasOutQueLimit() const override
    { return true; }

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

            oReqCtx.SetIntPtr( propStreamId,
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
        CStdRMutex oLock( this->GetLock() );
        do{
            auto itr1 = m_mapMsgRead.find( hstm );
            if( itr1 != m_mapMsgRead.end() )
            {
                ret = -EEXIST;
                break;
            }

            BufPtr pBufReady;
            auto itr =
                m_mapBufReading.find( hstm );

            if( itr == m_mapBufReading.end() )
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
                    m_mapMsgRead[ hstm ]= pNewMsg;
                }
                else if( dwPayload < dwSize )
                {
                    m_mapBufReading[ hstm ] = pBuf;
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
                BufPtr& pHeadBuf = itr->second;
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
                    m_mapMsgRead[ hstm ]= pNewMsg;
                    m_mapBufReading.erase( hstm );
                }
                else if( dwPayload < dwSize )
                {
                    pHeadBuf->append(
                        pBuf->ptr(), pBuf->size() );
                }
                else
                {
                    ret = -EBADMSG;
                }
            }

        }while( 0 );

        if( SUCCEEDED( ret ) )
            m_lstStreams.push_back( hstm );

        return ret;
    }

    gint32 FillReadBuf( HANDLE hstm )
    {
        if( hstm == INVALID_HANDLE )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oLock( this->GetLock() );
            auto itr = m_mapMsgRead.find( hstm );
            if( itr != m_mapMsgRead.end() )
            {
                ret = -EEXIST;
                break;
            }
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

    gint32 FillReadBuf()
    {
        gint32 ret = 0;
        do{
            CStdRMutex oLock( this->GetLock() );
            guint32 dwCount = m_lstStmSched.size();
            for( guint32 i = 0; i < dwCount; ++i )
            {
                HANDLE hstm = m_lstStmSched.front();
                m_lstStmSched.pop_front();
                m_lstStmSched.push_back( hstm );
                oLock.Unlock();

                FillReadBuf( hstm );
                oLock.Lock();
                dwCount = m_lstStmSched.size();
            }

        }while( 0 );

        return ret;
    }

    gint32 DispatchData()
    {
        gint32 ret = 0;    
        do{
            std::vector< DMsgPtr > vecMsgs;
            CStdRMutex oLock( this->GetLock() );
            if( this->GetState() != stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }

            if( m_mapMsgRead.empty() ||
                m_dwNumReqs == 0 )
                break;

            guint32 dwCount = ( gint32 )std::min(
                ( guint32 )m_lstStreams.size(),
                m_dwNumReqs );

            gint32 iCount = ( gint32 )dwCount;
            while( iCount > 0 )
            {
                --iCount;
                HANDLE hstm = m_lstStreams.front();
                m_lstStreams.pop_front();
                auto itr = m_mapMsgRead.find( hstm );
                if( itr == m_mapMsgRead.end() )
                    continue;

                vecMsgs.push_back( *itr )
                m_mapMsgRead.erase( itr );
            }

            m_dwNumReqs -= dwCount;
            oLock.Unlock();

            BufPtr pBuf( true );
            for( auto& elem : vecMsgs )
            {
                *pBuf = elem;
                auto ptr = ( CBuffer* )pBuf;
                m_pPort->OnEvent(
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
        DispatchData();
        return ret;
    }

    gint32 DoRmtModEvent(
        EnumEventId iEvent,
        const std::string& strModule,
        IConfigDb* pEvtCtx ) override
    {
        gint32 ret = super::DoRmtModEvent(
             iEvent, strModule, pEvtCtx );
        if( SUCCEEDED( ret ) )
        {
            if( iEvent != eventRmtModOffline )
                break;
        
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

            CDBusStreamPdo* pPort = nullptr;
            ret = oReqCtx.GetPointer(
                    propPortPtr, pPort );
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oReqCfg( pIoReq );
            IConfigDb* pResp;
            gint32 iRet = oReqCfg.GetPointer(
                propRespPtr, pResp );
            
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
};

class CRpcReqStreamProxy :
    public CRpcReqStreamBase< true >
{
    public:
    typedef CRpcReqStreamBase< true > super;
    CRpcReqStreamProxy( const IConfigDb* pCfg )
        : super( pCfg )
    {}

};

class CRpcReqStreamServer :
    public CRpcReqStreamBase< false >
{
    public:
    typedef CRpcReqStreamBase< false > super;
    CRpcReqStreamServer( const IConfigDb* pCfg )
        : super( pCfg )
    {}
};

DECLARE_AGGREGATED_SERVER(
     CRpcStreamChannelSvr,
     CRpcReqStreamProxy ); 

DECLARE_AGGREGATED_PROXY(
    CRpcStreamChannelCli,
    CRpcReqStreamServer );

}
