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
#include <map>
#include <vector>
#include "clsids.h"
#include "defines.h"
#include "stlcont.h"
#include <dlfcn.h>
#include "frmwrk.h"
#include "prxyport.h"
#include "rpcroute.h"
#include "tcpport.h"
#include "objfctry.h"
#include "fdodrv.h"


using namespace std;

// c++11 required

static FactoryPtr InitClassFactory()
{

    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRY( COutgoingPacket );
    INIT_MAP_ENTRY( CIncomingPacket );

    INIT_MAP_ENTRYCFG( CDBusProxyFdo );
    INIT_MAP_ENTRYCFG( CDBusProxyPdo );
    INIT_MAP_ENTRYCFG( CFidoRecvDataTask );
    INIT_MAP_ENTRYCFG( CProxyFdoDispEvtTask );
    INIT_MAP_ENTRYCFG( CProxyFdoListenTask );
    INIT_MAP_ENTRYCFG( CProxyFdoModOnOfflineTask );
    INIT_MAP_ENTRYCFG( CProxyFdoRmtDBusOfflineTask );
    INIT_MAP_ENTRYCFG( CProxyMsgMatch );
    INIT_MAP_ENTRYCFG( CProxyPdoConnectTask );
    INIT_MAP_ENTRYCFG( CProxyPdoDisconnectTask );
    INIT_MAP_ENTRYCFG( CRegisteredObject );
    INIT_MAP_ENTRYCFG( CRemoteProxyState );
    INIT_MAP_ENTRYCFG( CReqFwdrEnableRmtEventTask );
    INIT_MAP_ENTRYCFG( CReqFwdrForwardRequestTask );
    INIT_MAP_ENTRYCFG( CReqFwdrSendDataTask );
    INIT_MAP_ENTRYCFG( CReqFwdrFetchDataTask );
    INIT_MAP_ENTRYCFG( CReqFwdrOpenRmtPortTask );
    INIT_MAP_ENTRYCFG( CReqFwdrCloseRmtPortTask );
    INIT_MAP_ENTRYCFG( CRouterEnableEventRelayTask );
    INIT_MAP_ENTRYCFG( CRouterLocalMatch );
    INIT_MAP_ENTRYCFG( CRouterOpenRmtPortTask );
    INIT_MAP_ENTRYCFG( CRouterRemoteMatch );
    INIT_MAP_ENTRYCFG( CRpcControlStream );
    INIT_MAP_ENTRYCFG( CRpcListeningSock );
    INIT_MAP_ENTRYCFG( CRpcReqForwarderImpl );
    INIT_MAP_ENTRYCFG( CRpcReqForwarderProxyImpl );
    INIT_MAP_ENTRYCFG( CRpcRfpForwardEventTask );
    INIT_MAP_ENTRYCFG( CRpcStream );
    INIT_MAP_ENTRYCFG( CRpcStreamSock );
    INIT_MAP_ENTRYCFG( CRpcTcpBridgeImpl );
    INIT_MAP_ENTRYCFG( CRpcTcpBridgeProxyImpl );
    INIT_MAP_ENTRYCFG( CRpcTcpBusDriver );
    INIT_MAP_ENTRYCFG( CRpcTcpBusPort );
    INIT_MAP_ENTRYCFG( CRpcTcpFido );
    INIT_MAP_ENTRYCFG( CStmPdoSvrOfflineNotifyTask );
    INIT_MAP_ENTRYCFG( CStmSockCompleteIrpsTask );
    INIT_MAP_ENTRYCFG( CStmSockConnectTask );
    INIT_MAP_ENTRYCFG( CStmSockInvalStmNotifyTask );
    INIT_MAP_ENTRYCFG( CTcpStreamPdo );
    INIT_MAP_ENTRYCFG( CBdgeProxyReadWriteComplete );
    INIT_MAP_ENTRYCFG( CBdgeProxyOpenStreamTask );
    INIT_MAP_ENTRYCFG( CBdgeProxyStartSendTask );
    INIT_MAP_ENTRYCFG( CBdgeStartRecvDataTask );
    INIT_MAP_ENTRYCFG( CBdgeStartFetchDataTask );
    INIT_MAP_ENTRYCFG( CRpcSockWatchCallback );
    INIT_MAP_ENTRYCFG( CProxyFdoDriver );
    INIT_MAP_ENTRYCFG( CRpcRouterImpl );
    INIT_MAP_ENTRYCFG( CRouterStartReqFwdrProxyTask );
    INIT_MAP_ENTRYCFG( CRouterStopBridgeProxyTask );
    INIT_MAP_ENTRYCFG( CRouterStopBridgeTask );
    INIT_MAP_ENTRYCFG( CRouterStartRecvTask );
    INIT_MAP_ENTRYCFG( CRouterAddRemoteMatchTask );
    INIT_MAP_ENTRYCFG( CRouterEventRelayRespTask );
    INIT_MAP_ENTRYCFG( CRpcTcpFidoDrv );
    INIT_MAP_ENTRYCFG( CIfRouterState );
    INIT_MAP_ENTRYCFG( CTcpFidoListenTask );
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
