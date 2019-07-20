/*
 * =====================================================================================
 *
 *       Filename:  mainloop.cpp
 *
 *    Description:  implementation of CMainIoLoop
 *
 *        Version:  1.0
 *        Created:  10/03/2016 11:50:47 AM
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

#include <stdio.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <functional>
#include  <sys/syscall.h>
#include "configdb.h"
#include "frmwrk.h"
#include "mainloop.h"

gint32 CSchedTaskCallback::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );
        CIoManager* pMgr = nullptr;

        ret = oCfg.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        MloopPtr pLoop =
            pMgr->GetMainIoLoop();

        CMainIoLoop* pMainLoop = pLoop;
        if( pMainLoop == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = CMainIoLoop::TaskCallback(
            ( gpointer )pMainLoop );

    }while( 0 );

#ifndef _USE_LIBEV
    // for glib, this is an idle source, and it
    // must be removed to aovid running
    // repeatedly without tasks.
    return SetError( G_SOURCE_REMOVE );
#else
    // for libev, this is an async source, and
    // unless the loop is signaled, it will not be
    // run repeatedly
    return SetError( G_SOURCE_CONTINUE );
#endif
}

template<>
gint32 CMainIoLoop::InstallTaskSource()
{
    gint32 ret = 0;
    do{
        TaskletPtr pTask;

        if( m_pSchedCb.IsEmpty() )
        {
            CParamList oCfg;
            ret = oCfg.SetPointer(
               propIoMgr, this->GetIoMgr() );

            if( ERROR( ret ) )
                break;

            // to start immediately
            oCfg.Push( true ); 
            ret = m_pSchedCb.NewObj(
                clsid( CSchedTaskCallback ),
                oCfg.GetCfg() );
        
            if( ERROR( ret ) )
                break;

#ifndef _USE_LIBEV
        }

        // m_pSchedCb is already active, no
        // need to install it again
        if( !IsTaskDone() )
            break;
        SetTaskDone( false );
        ret = this->AddIdleWatch(
            m_pSchedCb, m_hTaskWatch );

#else
            // nothing to do here
        }

        if( !IsTaskDone() )
            break;
        SetTaskDone( false );
        this->WakeupLoop( aevtRunSched );
#endif
    }while( 0 );

    return ret;
}
