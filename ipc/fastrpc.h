/*
 * =====================================================================================
 *
 *       Filename:  fastrpc.h
 *
 *    Description:  declarations of fast-rpc related classes 
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

template< bool bProxy >
class CRpcReqStreamBase :
    public virtual IFBASE( bProxy ) 
{
    timespec m_tsStartTime;
    std::hashmap< HANDLE, stdstr > m_mapStm2Sess;
    std::map< stdstr, guint32 > m_mapSessRefs;

    using BUFQUE=std::deque< BufPtr >;
    std::map< HANDLE, DMsgPtr > m_mapMsgRead;
    std::hashmap< HANDLE, BufPtr > m_mapBufReading;
    guint32 m_dwNumReqs = 0;
    std::deque< HANDLE > m_queStreams;
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
        HANDLE hStream, stdstr& strSess )
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
            iRet = oCtx.GetStrProp(
                propSessHash, strSess );
            if( ERROR( iRet ) )
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

            m_mapStm2Sess[ hStream ] = strSess;

        }while( 0 );

        return ret;
    }

    gint32 OnStmReady( HANDLE hChannel ) override
    {
        gint32 ret = 0;
        do{
            ret = super::OnStreamReady( hChannel );
            CStdRMutex oLock( this->GetLock() );
            m_queStreams.push_back( hChannel );

        }while( 0 );

        return ret;
    }

    gint32 AddListeningReq()
    {
        gint32 ret = 0;
        do{
            CStdRMutex oLock( this->GetLock() );
            m_dwNumReqs++;
            if( m_queBufRead.empty())
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

    gint32 OnStmClosing( HANDLE hChannel ) override
    {
        gint32 ret = 0;
        do{
            ret = super::OnStmClosing( hChannel );

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

            m_mapMsgRead.erase( hChannel );
            m_mapBufReading.erase( hChannel );
            oLock.Unlock();

        }while( 0 );

        return ret;
    }

    bool HasOutQueLimit() const override
    { return true; }

    gint32 OnReadStreamComplete(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx ) override
    {
        return 0;
    }

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

    gint32 FillReadBuf(
        HANDLE hChannel, BufPtr& pBuf ) 
    {
        gint32 ret = 0;
        CStdRMutex oLock( this->GetLock() );
        do{
            auto itr1 = m_mapMsgRead.find( hChannel );
            if( itr1 != m_mapMsgRead.end() )
                break;

            BufPtr pBufReady;
            auto itr =
                m_mapBufReading.find( hChannel );

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
                    strSender += std::to_string( hChannel )
                    pMsg.SetSender( strSender );
                    m_mapMsgRead[ hChannel ]= pMsg;
                }
                else if( dwPayload >= dwSize )
                {
                    ret = -EBADMSG;
                    break;
                }
                m_mapBufReading[ hChannel ] = pBuf;
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
                    strSender += std::to_string( hChannel )
                    pMsg.SetSender( strSender );
                    m_mapMsgRead[ hChannel ]= pMsg;
                    m_mapBufReading.erase( hChannel );
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
            while( itr == m_mapMsgRead.end() )
            {
                oLock.Unlock();
                BufPtr pBuf;
                ret = this->ReadStreamNoWait(
                    hstm, pBuf );

                if( ERROR( ret ) )
                    break;

                ret = FillReadBuf( hstm, pBuf );
                if( ERROR( ret ) )
                    break;

                oLock.Lock();
                itr = m_mapMsgRead.find( hstm );
            }

        }while( 0 );

        return ret;
    }

    gint32 DispatchData( HANDLE hChannel )
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

            guint32 dwCount = std::min(
                ( guint32 )m_mapMsgRead.size(),
                m_dwNumReqs );

            vecMsgs.insert( vecMsgs.begin(),
                m_mapMsgRead.begin(),
                m_mapMsgRead.begin() + dwCount );

            m_mapMsgRead.erase(
                m_mapMsgRead.begin(),
                m_mapMsgRead.begin() + dwCount );

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

    gint32 OnStmRecv( HANDLE hChannel,
        BufPtr& pBuf ) override
    {
        gint32 ret = 0;
        ret = super::OnStmRecv( hChannel, pBuf );
        if( ERROR( ret ) )
            return ret;

        FillReadBuf( hChannel, pBuf );
        DispatchData();
        return ret;
    }
};

class CRpcReqStreamProxy :
    public CRpcReqStreamBase< true >
{
};

class CRpcReqStreamServer :
    public CRpcReqStreamBase< false >
{
};

}
