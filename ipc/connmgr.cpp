/*
 * =====================================================================================
 *
 *       Filename:  connmgr.cpp
 *
 *    Description:  Implementation of the CIfSvrConnMgr. the class is to
 *                  track the modules this server will have to respond. it's
 *                  purpose is to stop the server from sending to the client
 *                  module if it is already down.
 *
 *        Version:  1.0
 *        Created:  10/25/2018 08:35:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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

#include <deque>
#include <vector>
#include <string>
#include <semaphore.h>
#include "defines.h"
#include "frmwrk.h"
#include "emaphelp.h"
#include "connhelp.h"
#include "proxy.h"

CIfSvrConnMgr::CIfSvrConnMgr(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;

    do{
        SetClassId( clsid( CIfSvrConnMgr ) );
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        m_pIfSvr = pObj;
        if( m_pIfSvr == nullptr )
            ret = -EFAULT;

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIfSvrConnMgr ctor" );
        throw std::runtime_error( strMsg );
    }
}

CIfSvrConnMgr::~CIfSvrConnMgr()
{;}

// a new invoke task comes
gint32 CIfSvrConnMgr::OnInvokeMethod(
    HANDLE hTask, const std::string& strSrc )
{
    gint32 ret = 0;

    if( m_pIfSvr == nullptr )
        return ERROR_STATE;

    CStdRMutex oIfLock( m_pIfSvr->GetLock() );
    if( hTask == INVALID_HANDLE ||
        strSrc.empty() )
        return -EINVAL;
    do{
        m_mapTaskToAddr[ hTask ] = strSrc;
        m_mapTaskStates[ hTask ] = true;
        m_mapAddrToTask.emplace( strSrc, hTask );

    }while( 0 );

    return ret;
}

// the invoke task is done, and can be removed
gint32 CIfSvrConnMgr::OnInvokeComplete(
    HANDLE hTask )
{
    gint32 ret = 0;

    if( hTask == INVALID_HANDLE )
        return -EINVAL;

    if( m_pIfSvr == nullptr )
        return ERROR_STATE;

    CStdRMutex oIfLock( m_pIfSvr->GetLock() );

    do{
        if( m_mapTaskToAddr.find( hTask ) == 
            m_mapTaskToAddr.end() )
            return -ENOENT;

        std::string strSrc =
            m_mapTaskToAddr[ hTask ];

        m_mapTaskStates.erase( hTask );
        m_mapTaskToAddr.erase( hTask );

        AddrRange oRange =
            m_mapAddrToTask.equal_range( strSrc );
        if( oRange.first == oRange.second )
            break;

        while( oRange.first != oRange.second )
        {
            if( oRange.first->second == hTask )
            {
                m_mapAddrToTask.erase( oRange.first );
                break;
            }
            oRange.first++;
        }

    }while( 0 );

    return ret;
}
// received a disconnection event
gint32 CIfSvrConnMgr::OnDisconn(
    const std::string& strDest )
{
    gint32 ret = 0;
    if( strDest.empty() )
        return -EINVAL;

    if( m_pIfSvr == nullptr )
        return ERROR_STATE;

    CStdRMutex oIfLock( m_pIfSvr->GetLock() );
    do{

        AddrRange oRange =
            m_mapAddrToTask.equal_range( strDest );
        if( oRange.first == oRange.second )
            break;
        while( oRange.first != oRange.second )
        {
            HANDLE hTask = oRange.first->second;
            if( m_mapTaskStates.find( hTask ) !=
                m_mapTaskStates.end() )
            {
                m_mapTaskStates[ hTask ] = false;
            }
            oRange.first++;
        }
    }while( 0 );

    return ret;
}

// test if the task can send a response to the
// client
gint32 CIfSvrConnMgr::CanResponse(
    HANDLE hTask )
{
    gint32 ret = ERROR_FALSE;
    if( unlikely( hTask == INVALID_HANDLE ) )
        return ret;

    if( unlikely( m_pIfSvr == nullptr ) )
        return ret;

    CStdRMutex oIfLock( m_pIfSvr->GetLock() );
    do{
        if( m_mapTaskStates.find( hTask ) !=
            m_mapTaskStates.end() )
        {
            if( m_mapTaskStates[ hTask ] == true )
                ret = 0;
        }
        else
        {
            ret = -ENOENT;
        }

    }while( 0 );

    return ret;
}

// test if the task can send a response to the
// client
gint32 CIfSvrConnMgr::CanResponse(
    const std::string& strSrc )
{
    gint32 ret = ERROR_FALSE;

    if( unlikely( m_pIfSvr == nullptr ) )
        return ret;

    CStdRMutex oIfLock( m_pIfSvr->GetLock() );
    do{
        AddrRange oRange;
        oRange.first =
            m_mapAddrToTask.find( strSrc );

        if( oRange.first !=
            m_mapAddrToTask.end() )
        {
            HANDLE hTask = oRange.first->second;
            if( m_mapTaskStates.find( hTask ) ==
                m_mapTaskStates.end() )
                break;

            if( m_mapTaskStates[ hTask ] == true )
                ret = 0;
        }
        else
        {
            ret = -ENOENT;
        }

    }while( 0 );

    return ret;
}
