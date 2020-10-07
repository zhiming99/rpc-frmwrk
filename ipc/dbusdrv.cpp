/*
 * =====================================================================================
 *
 *       Filename:  dbusdrv.cpp
 *
 *    Description:  dbus bus driver implementation
 *
 *        Version:  1.0
 *        Created:  08/21/2016 01:22:49 PM
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

#include <vector>
#include <string>
#include "defines.h"
#include "port.h"
#include "dbusport.h"
#include "emaphelp.h"
// #include "iftasks.h"

using namespace std;

CDBusBusDriver::CDBusBusDriver(
    const IConfigDb* pCfg ): 
    super( pCfg )
{
    SetClassId( clsid( CDBusBusDriver ) );
}

gint32 CDBusBusDriver::Probe(
        IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig )
{
    gint32 ret = 0;

    do{
        // we don't have dynamic bus yet
        CfgPtr pCfg( true );

        // make a copy of the input args
        if( pConfig != nullptr )
            *pCfg = *pConfig;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        ret = oCfg.SetStrProp( propPortClass,
            PORT_CLASS_LOCALDBUS );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetIntProp( propClsid,
            clsid( CDBusBusPort ) );

        // FIXME: we need a better task to receive the
        // notification of the port start events
        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;

        // we don't care the event sink to bind
        ret = oCfg.SetObjPtr( propEventSink,
            ObjPtr( pTask ) );

        if( ERROR( ret ) )
            break;

        ret = CreatePort( pNewPort, pCfg );
        if( ERROR( ret ) )
            break;

        if( pLowerPort != nullptr )
        {
            ret = pNewPort->AttachToPort( pLowerPort );
        }

        if( ERROR( ret ) )
            break;

        ret = NotifyPortAttached( pNewPort );
        if( ret != STATUS_PENDING )
            break;

        // waiting for the start to complete
        CSyncCallback* pSyncTask = pTask;
        if( pSyncTask != nullptr )
        {
            ret = pSyncTask->WaitForComplete();
            if( SUCCEEDED( ret ) )
                ret = pSyncTask->GetError();
        }

    }while( 0 );

    return ret;
}

