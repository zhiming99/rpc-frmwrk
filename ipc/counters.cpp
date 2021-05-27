/*
 * =====================================================================================
 *
 *       Filename:  counters.cpp
 *
 *    Description:  implementation of CStatCounters proxy/server
 *
 *        Version:  1.0
 *        Created:  11/18/2018 02:35:10 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */


#include <rpc.h>
#include <unistd.h>
#include "frmwrk.h"
#include "ifhelper.h"
#include "counters.h"

namespace rpcf
{

gint32 CStatCountersProxy::InitUserFuncs()
{
    // don't call super::InitUserFuncs here
    BEGIN_IFPROXY_MAP( CStatCounters, false );

    ADD_USER_PROXY_METHOD_EX( 0,
        CStatCountersProxy::GetCounters,
        METHOD_GetCounters );

    ADD_USER_PROXY_METHOD_EX( 1,
        CStatCountersProxy::GetCounter,
        METHOD_GetCounter );

    END_IFPROXY_MAP;

    return 0;
}

gint32 CStatCountersProxy::GetCounters(
    CfgPtr& pCfg )
{
    return FORWARD_IF_CALL( iid( CStatCounters ),
        0, METHOD_GetCounters, pCfg );
}

gint32 CStatCountersProxy::GetCounter(
    guint32 iPropId, BufPtr& pBuf  )
{
    return FORWARD_IF_CALL( iid( CStatCounters ),
        1, METHOD_GetCounter, iPropId, pBuf );
}

// init the functions
gint32 CStatCountersServer::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CStatCounters );

    ADD_USER_SERVICE_HANDLER_EX( 0,
        CStatCountersServer::GetCounters,
        METHOD_GetCounters );

    ADD_USER_SERVICE_HANDLER_EX( 1,
        CStatCountersServer::GetCounter,
        METHOD_GetCounter );

    END_IFHANDLER_MAP;

    return 0;
}

// allocate resources
gint32 CStatCountersServer::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    CParamList oParams;
    oParams[ propIfPtr ] = ObjPtr( this );
    ret = m_pMsgFilter.NewObj(
        clsid( CMessageCounterTask ),
        oParams.GetCfg() );

    if( ERROR( ret ) )
        return ret;

    return RegisterFilter( m_pMsgFilter );
}

// deallocate resources
gint32 CStatCountersServer::OnPostStop(
    IEventSink* pCallback )
{
    UnregisterFilter( m_pMsgFilter );
    m_mapCounters.clear();
    m_pMsgFilter.Clear();
    return 0;
}

gint32 CStatCountersServer::IncCounter(
    EnumPropId iProp )
{
    return IncCounter( iProp, false );
}

gint32 CStatCountersServer::DecCounter(
    EnumPropId iProp )
{
    return IncCounter( iProp, true );
}

gint32 CStatCountersServer::SetCounter(
    EnumPropId iProp, guint32 dwVal )
{
    CStdRMutex oIfLock( m_oStatLock );
    m_mapCounters[ iProp ] = dwVal;
    return 0;
}
// business logics
gint32 CStatCountersServer::IncCounter(
    EnumPropId iProp, bool bNegative )
{
    gint32 ret = 0;
    guint32 dwCount = 0;
    CStdRMutex oIfLock( m_oStatLock );
    std::hashmap< gint32, guint32 >::iterator
        itr = m_mapCounters.find( iProp );
    if( itr != m_mapCounters.end() )
        dwCount = itr->second;

    if( bNegative && dwCount == 0 )
        return -ERANGE ;

    if( bNegative )
        dwCount -= 1;
    else
        dwCount += 1;

    if( itr != m_mapCounters.end() )
        itr->second = dwCount;
    else
        m_mapCounters[ iProp ] = dwCount;

    return dwCount;
}

gint32 CStatCountersServer::GetCounter2( 
    EnumPropId iProp, guint32& dwVal  )
{
    CStdRMutex oIfLock( m_oStatLock );
    std::hashmap< gint32, guint32 >::iterator
        itr = m_mapCounters.find( iProp );
    if( itr == m_mapCounters.end() )
        return -ENOENT;

    dwVal = itr->second;
    
    return STATUS_SUCCESS;
}

// interface implementations
gint32 CStatCountersServer::GetCounters(
    IEventSink* pCallback,
    CfgPtr& pCfg )
{
    if( pCfg.IsEmpty() )
        pCfg.NewObj();

    CStdRMutex oIfLock( m_oStatLock );
    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    for( auto elem : m_mapCounters )
        oCfg[ elem.first ] = elem.second;

    return 0;
}

gint32 CStatCountersServer::GetCounter(
    IEventSink* pCallback,
    guint32 iPropId,
    BufPtr& pBuf  )
{
    if( pBuf.IsEmpty() )
        pBuf.NewObj();

    CStdRMutex oIfLock( m_oStatLock );
    if( m_mapCounters.empty() )
        return -EFAULT;

    // make a copy
    std::hashmap< gint32, guint32 >::iterator
        itr = m_mapCounters.find( iPropId );
    if( itr == m_mapCounters.end() )
        return -ENOENT;
    *pBuf = itr->second;
    return STATUS_SUCCESS;
}

gint32 CMessageCounterTask::FilterMsgOutgoing(
    IEventSink* pReqTask, DBusMessage* pRawMsg )
{
    gint32 ret = 0;
    if( pRawMsg == nullptr ||
        pReqTask == nullptr )
        return STATUS_SUCCESS;

    do{
        DMsgPtr pMsg( pRawMsg );
        CStatCountersServer* pIf = nullptr;

        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );    

        gint32 ret = oTaskCfg.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) )
            break;

        CIfIoReqTask* pReq = ObjPtr( pReqTask );
        if( pReq != nullptr )
        {
            ret = pIf->IncCounter(
                propMsgOutCount );
            break;
        }

    }while( 0 );

    return ret;
}

}
