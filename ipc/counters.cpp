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
    gint32 ret = m_pCfg.NewObj();
    if( ERROR( ret ) )
        return ret;

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
    m_pCfg.Clear();
    m_pMsgFilter.Clear();
    return 0;
}

// business logics
gint32 CStatCountersServer::IncMsgCount(
    EnumPropId iProp )
{
    gint32 ret = 0;
    guint32 dwCount = 1;
    CStdRMutex oIfLock( GetLock() );
    CCfgOpener oCounters(
        ( IConfigDb* ) m_pCfg );
    ret = oCounters.GetIntProp(
        iProp, dwCount );
    if( ret == -ENOENT )
    {
        oCounters.SetIntProp(
            iProp, dwCount );
    }
    else if( ERROR( ret ) )
    {
        return ret;
    }

    return oCounters.SetIntProp(
        iProp, dwCount + 1 );
}

// interface implementations
gint32 CStatCountersServer::GetCounters(
    IEventSink* pCallback,
    CfgPtr& pCfg )
{
    CStdRMutex oIfLock( GetLock() );
    *pCfg = *m_pCfg;
    return 0;
}

gint32 CStatCountersServer::GetCounter(
    IEventSink* pCallback,
    guint32 iPropId,
    BufPtr& pBuf  )
{
    if( m_pCfg.IsEmpty() )
        return -EFAULT;

    if( pBuf.IsEmpty() )
        pBuf.NewObj();

    CCfgOpener oCfg( ( IConfigDb* )m_pCfg );
    CStdRMutex oIfLock( GetLock() );

    // make a copy
    return oCfg.GetProperty( iPropId, *pBuf );
}

}
