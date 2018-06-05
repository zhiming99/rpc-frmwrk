/*
 * =====================================================================================
 *
 *       Filename:  clsids.cpp
 *
 *    Description:  a map of class id to class name
 *
 *        Version:  1.0
 *        Created:  08/07/2016 01:18:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */
#include <glib.h>
#include <map>
#include <vector>
#include "clsids.h"
#include "defines.h"
#include "stlcont.h"
#include "tasklets.h"
#include "utils.h"
#include "proxy.h"
#include "port.h"
#include "frmwrk.h"
#include "dbusport.h"

#include <dlfcn.h>

using namespace std;

FctryVecPtr g_pFactories;

std::map< gint32, FUNC_MAP > g_mapFuncs;
std::map< gint32, PROXY_MAP > g_mapProxyFuncs;


// c++11 required

static FactoryPtr InitClassFactory()
{

    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRY( CIrpCompThread );
    INIT_MAP_ENTRY( IoRequestPacket );
    INIT_MAP_ENTRY( IRP_CONTEXT );
    INIT_MAP_ENTRY( CTaskQueue );
    INIT_MAP_ENTRY( CMainIoLoop );
    INIT_MAP_ENTRY( CStlIrpVector );
    INIT_MAP_ENTRY( CStlIrpVector2 );
    INIT_MAP_ENTRY( CStlPortVector );
    INIT_MAP_ENTRY( CTaskThread );
    INIT_MAP_ENTRY( COneshotTaskThread );
    INIT_MAP_ENTRY( CTaskWrapper );

    INIT_MAP_ENTRYCFG( CCancelIrpsTask );
    INIT_MAP_ENTRYCFG( CDBusBusDriver );
    INIT_MAP_ENTRYCFG( CDBusBusPort );
    INIT_MAP_ENTRYCFG( CDBusBusPortCreatePdoTask );
    INIT_MAP_ENTRYCFG( CDBusBusPortDBusOfflineTask );
    INIT_MAP_ENTRYCFG( CDBusDisconnMatch );
    INIT_MAP_ENTRYCFG( CDBusLocalPdo );
    INIT_MAP_ENTRYCFG( CDBusLoopbackMatch );
    INIT_MAP_ENTRYCFG( CDBusLoopbackPdo );
    INIT_MAP_ENTRYCFG( CDBusPdoListenDBEventTask );
    INIT_MAP_ENTRYCFG( CDBusSysMatch );
    INIT_MAP_ENTRYCFG( CDBusTransLpbkMsgTask );
    INIT_MAP_ENTRYCFG( CDriverManager );
    INIT_MAP_ENTRYCFG( CGenBusPortStopChildTask );
    INIT_MAP_ENTRYCFG( CIfCleanupTask );
    INIT_MAP_ENTRYCFG( CIfCpEventTask );
    INIT_MAP_ENTRYCFG( CIfDummyTask );
    INIT_MAP_ENTRYCFG( CIfEnableEventTask );
    INIT_MAP_ENTRYCFG( CIfInvokeMethodTask );
    INIT_MAP_ENTRYCFG( CIfIoReqTask );
    INIT_MAP_ENTRYCFG( CIfParallelTaskGrp );
    INIT_MAP_ENTRYCFG( CIfPauseResumeTask );
    INIT_MAP_ENTRYCFG( CIfRootTaskGroup );
    INIT_MAP_ENTRYCFG( CIfServiceNextMsgTask );
    INIT_MAP_ENTRYCFG( CIfStartRecvMsgTask );
    INIT_MAP_ENTRYCFG( CIfStopRecvMsgTask );
    INIT_MAP_ENTRYCFG( CIfStopTask );
    INIT_MAP_ENTRYCFG( CIfTaskGroup );
    INIT_MAP_ENTRYCFG( CIfFetchDataTask );
    INIT_MAP_ENTRYCFG( CIoManager );
    INIT_MAP_ENTRYCFG( CIoMgrIrpCompleteTask );
    INIT_MAP_ENTRYCFG( CIoMgrPostStartTask );
    INIT_MAP_ENTRYCFG( CIoMgrStopTask );
    INIT_MAP_ENTRYCFG( CIrpThreadPool );
    INIT_MAP_ENTRYCFG( CLocalProxyState );
    INIT_MAP_ENTRYCFG( CMessageMatch );
    INIT_MAP_ENTRYCFG( CPnpManager );
    INIT_MAP_ENTRYCFG( CPnpMgrDoStopNoMasterIrp );
    INIT_MAP_ENTRYCFG( CPnpMgrQueryStopCompletion );
    INIT_MAP_ENTRYCFG( CPnpMgrStartPortCompletionTask );
    INIT_MAP_ENTRYCFG( CPnpMgrStopPortAndDestroyTask );
    INIT_MAP_ENTRYCFG( CPnpMgrStopPortStackTask );
    INIT_MAP_ENTRYCFG( CPortAttachedNotifTask );
    INIT_MAP_ENTRYCFG( CPortStartStopNotifTask );
    INIT_MAP_ENTRYCFG( CPortState );
    INIT_MAP_ENTRYCFG( CPortStateResumeSubmitTask );
    INIT_MAP_ENTRYCFG( CProxyMsgMatch );
    INIT_MAP_ENTRYCFG( CRemoteProxyState );
    INIT_MAP_ENTRYCFG( CSyncCallback );
    INIT_MAP_ENTRYCFG( CTaskThreadPool );
    INIT_MAP_ENTRYCFG( CTimerService );
    INIT_MAP_ENTRYCFG( CUtilities );
    INIT_MAP_ENTRYCFG( CWorkitemManager );

    END_FACTORY_MAPS;
};

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}
