/*
 * =====================================================================================
 *
 *       Filename:  loopool.cpp
 *
 *    Description:  implementations of CLoopPools and CLoopPool 
 *
 *        Version:  1.0
 *        Created:  06/15/2022 07:10:28 PM
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
#include "configdb.h"
#include "frmwrk.h"
#include "loopool.h"
#include <limits.h>

namespace rpcf{

CLoopPool::CLoopPool( const IConfigDb* pCfg )
    : super()
{
    const CCfgOpener oParams( pCfg );
    guint32 dwVal = 0;
    gint32 ret = 0;

    m_dwTag = oParams[ 0 ];
    m_dwMaxLoops = oParams[ 1 ];
    m_strPrefix = (stdstr&)oParams[ 2 ];

    oParams.GetIntProp( 3, m_dwLowMark );
    oParams.GetBoolProp( 4, m_bAutoClean );
    oParams.GetPointer( propIoMgr, m_pMgr );

    SetClassId( clsid( CLoopPool ) );
}

gint32 CLoopPool::DestroyPool()
{
    gint32 ret = 0; 
    do{
        for( auto elem : m_mapLoops )
        {
            CMainIoLoop* pLoop = elem.first;
            gint32 iRet = pLoop->GetError();
            if( iRet != STATUS_PENDING )
                continue;
            // don't call pLoop->Stop, which is
            // to stop the dbus stuffs, and we are
            // non-dbus loops
            pLoop->WakeupLoop( aevtStop, iRet );
            pLoop->WaitForQuit();
        }
        m_mapLoops.clear();

    }while( 0 );

    return ret;
}

gint32 CLoopPool::CreateLoop(
    MloopPtr& pLoop )
{
    gint32 ret = 0;
    do{
        int i = m_mapLoops.size();
        CParamList oCfg;
        oCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        std::string strName(
            m_strPrefix + std::to_string( i ) );

        // thread name
        oCfg.Push( strName );

        // start new thread
        oCfg.Push( true );

        ret = pLoop.NewObj(
            clsid( CMainIoLoop ),
            oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pLoop->Start();
        if( ERROR( ret ) )
            break;

        m_mapLoops[ pLoop ] = 0;

    }while( 0 );

    return ret;
}

gint32 CLoopPool::AllocMainLoop(
    MloopPtr& pLoop )
{
    gint32 ret = STATUS_SUCCESS;
    do{
        LOOPMAP_ITR itr = m_mapLoops.begin();
        LOOPMAP_ITR itrRet = itr;

        guint32 dwCount =
            std::numeric_limits< guint32 >::max();
        for( ; itr != m_mapLoops.end(); ++itr )
        {
            if( dwCount > itr->second )
            {
                itrRet = itr;
                dwCount = itr->second;
            }
        }

        if( m_mapLoops.size() == m_dwMaxLoops )
        {
            // all the loops have been created
            ++itrRet->second;
            pLoop = itrRet->first;
            break;
        }

        if( dwCount < m_dwLowMark )
        {
            ++itrRet->second;
            pLoop = itrRet->first;
            break;
        }

        ret = CreateLoop( pLoop );
        if( ERROR( ret ) )
            break;

        m_mapLoops[ pLoop ] = 1;

    }while( 0 );

    return ret;
}
    
gint32 CLoopPool::ReleaseMainLoop(
    MloopPtr& pLoop )
{
    if( pLoop.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        LOOPMAP_ITR itr =
            m_mapLoops.find( pLoop );
        if( itr == m_mapLoops.end() )
            break;
        --itr->second;

    }while( 0 );

    return ret;
}

CLoopPools::CLoopPools(
    const IConfigDb* pCfg ) :
    super()
{
    SetClassId( clsid( CLoopPools ) );
    gint32 ret = 0;
    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CCfgOpener oParams( pCfg );
        ret = oParams.GetPointer(
            propIoMgr, m_pMgr );

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            DebugMsg( ret, "Error occurs "
                "in CLoopPool ctor" ) );
    }
}

bool CLoopPools::IsPool( guint32 dwTag ) const
{
    CStdRMutex oLock( GetLock() );
    if( m_mapLPools.find( dwTag ) ==
        m_mapLPools.end() )
        return false;
    return true;
}

gint32 CLoopPools::CreatePool(
    guint32 dwTag,
    const stdstr& strPrefix, 
    guint32 dwMaxLoops,
    guint32 dwLowMark,
    bool bAutoClean )
{
    CStdRMutex oLock( GetLock() );

    if( IsPool( dwTag ) )
        return 0;

    CIoManager* pMgr = GetIoMgr();
    if( dwMaxLoops == 0 )
        dwMaxLoops = pMgr->GetNumCores(); 

    LPoolPtr pPool;
    CParamList oParams;
    oParams.Push( dwTag );
    oParams.Push( dwMaxLoops );
    oParams.Push( strPrefix );
    oParams.Push( dwLowMark );
    oParams.Push( bAutoClean );
    oParams.SetPointer(
        propIoMgr, pMgr );

    gint32 ret = pPool.NewObj(
        clsid( CLoopPool ),
        oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    m_mapLPools[ dwTag ] = pPool;
    return ret;
}

gint32 CLoopPools::DestroyPool(
    guint32 dwTag )
{
    CStdRMutex oLock( GetLock() );

    if( !IsPool( dwTag ) )
        return -ENOENT;

    LPoolPtr pPool = m_mapLPools[ dwTag ];
    m_mapLPools.erase( dwTag );
    oLock.Unlock();
    pPool->DestroyPool();
    return STATUS_SUCCESS;
}

gint32 CLoopPools::AllocMainLoop(
    guint32 dwTag, 
    MloopPtr& pLoop )
{
    CStdRMutex oLock( GetLock() );
    if( !IsPool( dwTag ) )
        return -ENOENT;
    LPoolPtr pPool = m_mapLPools[ dwTag ];
    return pPool->AllocMainLoop( pLoop );
}
    
gint32 CLoopPools::ReleaseMainLoop(
    guint32 dwTag, 
    MloopPtr& pLoop )
{
    CStdRMutex oLock( GetLock() );
    if( !IsPool( dwTag ) )
        return -ENOENT;
    LPoolPtr& pPool = m_mapLPools[ dwTag ];
    return pPool->ReleaseMainLoop( pLoop );
}

gint32 CLoopPools::Start()
{ return 0; }

gint32 CLoopPools::Stop()
{
    std::vector< LPoolPtr > vecPools;
    CStdRMutex oLock( GetLock() );
    for( auto elem : m_mapLPools )
        vecPools.push_back( elem.second );

    m_mapLPools.clear();
    oLock.Unlock();
    for( auto& elem : vecPools )
        elem->DestroyPool();

    return 0;
}

}
