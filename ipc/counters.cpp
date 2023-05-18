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
    CMessageCounterTask* pFilter = m_pMsgFilter;
    if( pFilter != nullptr )
        pFilter->Stop();
    m_pMsgFilter.Clear();
    return 0;
}

// allocate resources
gint32 CStatCountersProxy::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    CParamList oParams;
    oParams[ propIfPtr ] = ObjPtr( this );
    ret = m_pMsgFilter.NewObj(
        clsid( CMessageCounterTaskProxy ),
        oParams.GetCfg() );

    if( ERROR( ret ) )
        return ret;

    return RegisterFilter( m_pMsgFilter );
}

// deallocate resources
gint32 CStatCountersProxy::OnPostStop(
    IEventSink* pCallback )
{
    UnregisterFilter( m_pMsgFilter );
    CMessageCounterTaskProxy* pFilter =
        m_pMsgFilter;

    if( pFilter != nullptr )
        pFilter->Stop();

    m_pMsgFilter.Clear();
    return 0;
}

gint32 IStatCounters::IncCounter(
    EnumPropId iProp, guint32 dwVal )
{
    return IncCounter( iProp, dwVal, false );
}

gint32 IStatCounters::DecCounter(
    EnumPropId iProp, guint32 dwVal )
{
    return IncCounter( iProp, dwVal, true );
}

gint32 IStatCounters::SetCounter(
    EnumPropId iProp, guint32 dwVal )
{
    CStdRMutex oIfLock( m_oStatLock );
    m_mapCounters[ iProp ] = dwVal;
    return 0;
}

gint32 IStatCounters::SetCounter(
    EnumPropId iProp, guint64 qwVal )
{
    CStdRMutex oIfLock( m_oStatLock );
    m_mapCounters[ iProp ] = qwVal;
    return 0;
}

// business logics
gint32 IStatCounters::IncCounter(
    EnumPropId iProp,
    guint32 dwVal,
    bool bNegative )
{
    gint32 ret = 0;
    do{
        CStdRMutex oIfLock( m_oStatLock );
        std::hashmap< gint32, Variant >::iterator
            itr = m_mapCounters.find( iProp );

        if( itr == m_mapCounters.end() )
        {
            if( bNegative )
                m_mapCounters[ iProp ] =
                    ( guint32 )0;
            else
                m_mapCounters[ iProp ] =
                    ( guint32 )dwVal;
            break;
        }

        gint32 iTypeId =
            itr->second.GetTypeId();
        if( iTypeId != typeUInt32 &&
            iTypeId != typeUInt64 )
        {
            ret = -EINVAL;
            break;
        }

        if( iTypeId == typeUInt32 )
        {
            guint32& dwCount = itr->second;
            if( bNegative && dwCount == 0 )
            {
                ret = -ERANGE ;
                break;
            }

            if( bNegative )
                dwCount -= dwVal;
            else
                dwCount += dwVal;
        }
        else 
        {
            guint64& qwCount = itr->second;
            if( bNegative && qwCount == 0 )
            {
                ret = -ERANGE ;
                break;
            }

            if( bNegative )
                qwCount -= dwVal;
            else
                qwCount += dwVal;
        }

    }while( 0 );

    return ret;
}

gint32 IStatCounters::GetCounter2( 
    guint32 iProp, guint32& dwVal  )
{
    CStdRMutex oIfLock( m_oStatLock );
    std::hashmap< gint32, Variant >::iterator
        itr = m_mapCounters.find( iProp );
    if( itr == m_mapCounters.end() )
        return -ENOENT;

    dwVal = itr->second;
    return STATUS_SUCCESS;
}

gint32 IStatCounters::GetCounter2( 
    guint32 iProp, guint64& qwVal  )
{
    CStdRMutex oIfLock( m_oStatLock );
    std::hashmap< gint32, Variant >::iterator
        itr = m_mapCounters.find( iProp );
    if( itr == m_mapCounters.end() )
        return -ENOENT;

    qwVal = itr->second;
    return STATUS_SUCCESS;
}

// interface implementations
gint32 IStatCounters::GetCounters(
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

gint32 IStatCounters::GetCounter(
    guint32 iPropId,
    BufPtr& pBuf  )
{
    CStdRMutex oIfLock( m_oStatLock );
    if( m_mapCounters.empty() )
        return -EFAULT;

    // make a copy
    std::hashmap< gint32, Variant >::iterator
        itr = m_mapCounters.find( iPropId );
    if( itr == m_mapCounters.end() )
        return -ENOENT;
    pBuf = itr->second.ToBuf();
    return STATUS_SUCCESS;
}

}
