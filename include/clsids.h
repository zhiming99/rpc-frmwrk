/*
 * =====================================================================================
 *
 *       Filename:  clsids.h
 *
 *    Description:  definitions of class ids
 *
 *        Version:  1.0
 *        Created:  06/18/2016 09:31:16 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */
#pragma once

#define DECL_CLSID( __classname__ ) \
    Clsid_ ## __classname__

#define clsid( classname ) \
    DECL_CLSID( classname )

/**
* @name Class id declarations
* @{ */
/** Please don't forget to add an entry to the
 * map from Class id to class name in the clsids.cpp
 * the order must be kept the same in favor of
 * binary search @} */

typedef enum 
{
    DECL_CLSID( MinClsid ) = 0x0823,
    DECL_CLSID( CObjBase ),
    DECL_CLSID( CBuffer ),
    DECL_CLSID( CMsgQueue ),
    DECL_CLSID( CPort ),
    DECL_CLSID( CBusPort ),
    DECL_CLSID( CDriverManager ),
    DECL_CLSID( CMainIoLoop ),
    DECL_CLSID( CUtilities ),
    DECL_CLSID( CTimerService ),
    DECL_CLSID( CWorkitemManager ),
    DECL_CLSID( CRegistry ),
    DECL_CLSID( CConfigDb ),
    DECL_CLSID( IoRequestPacket ),
    DECL_CLSID( CInterfaceServer ),
    DECL_CLSID( CRpcLocalReqServer ), 
    DECL_CLSID( CIrpCompThread ),
    DECL_CLSID( CIrpThreadPool ),
    DECL_CLSID( CPnpManager ),
    DECL_CLSID( CDBusLocalPdo ),
    DECL_CLSID( CDBusProxyPdo ),
    DECL_CLSID( CDBusProxyFdo ),
    DECL_CLSID( IRP_CONTEXT ),
    DECL_CLSID( CIoManager ),
    DECL_CLSID( CDBusBusPort ),
    DECL_CLSID( CDBusBusDriver ),
    DECL_CLSID( CIrpSlaveMap ),
    DECL_CLSID( CTaskThread ),
    DECL_CLSID( COneshotTaskThread ),
    DECL_CLSID( CTaskThreadPool ),
    DECL_CLSID( CIoMgrStopTask ),
    DECL_CLSID( CIoMgrPostStartTask ),
    DECL_CLSID( CTaskQueue ),
    DECL_CLSID( CStlEventMap ),
    DECL_CLSID( CStlIntMap ),
    DECL_CLSID( CStlIntVector ),
    DECL_CLSID( CStlIrpVector ),
    DECL_CLSID( CStlIrpVector2 ),
    DECL_CLSID( CStlIntQueue ),
    DECL_CLSID( CStlPortVector ),
    DECL_CLSID( CStlBufVector ),
    DECL_CLSID( CStlObjSet ),
    DECL_CLSID( CClassFactories ),
    DECL_CLSID( CPnpMgrQueryStopCompletion ),
    DECL_CLSID( CPnpMgrDoStopNoMasterIrp ),
    DECL_CLSID( CPnpMgrStartPortCompletionTask ),
    DECL_CLSID( CPnpMgrStopPortStackTask ),
    DECL_CLSID( CPnpMgrStopPortAndDestroyTask ),
    DECL_CLSID( CGenBusPortStopChildTask ),
    DECL_CLSID( CPortStartStopNotifTask ),
    DECL_CLSID( CPortAttachedNotifTask ),
    DECL_CLSID( CDBusBusPortCreatePdoTask ),
    DECL_CLSID( CIoMgrIrpCompleteTask ),
    DECL_CLSID( CProxyPdoConnectTask ),
    DECL_CLSID( CProxyPdoDisconnectTask ),
    DECL_CLSID( CRpcReqForwarder ),
    DECL_CLSID( CRpcReqForwarderProxy ),
    DECL_CLSID( CRpcTcpBridgeProxy ),
    DECL_CLSID( CRpcTcpBridge ),
    DECL_CLSID( CMessageMatch ),
    DECL_CLSID( CProxyMsgMatch ),
    DECL_CLSID( CDBusDisconnMatch ),
    DECL_CLSID( CDBusSysMatch ),
    DECL_CLSID( CDBusPdoListenDBEventTask ),
    DECL_CLSID( CPortStateResumeSubmitTask ),
    DECL_CLSID( CDBusBusPortDBusOfflineTask ),
    DECL_CLSID( CRpcBasePortModOnOfflineTask ),
    DECL_CLSID( CProxyFdoModOnOfflineTask ),
    DECL_CLSID( CProxyFdoRmtDBusOfflineTask ),
    DECL_CLSID( CProxyFdoListenTask ),
    DECL_CLSID( CProxyFdoDispEvtTask ),
    DECL_CLSID( CPortState ),
    DECL_CLSID( CLocalProxyState ),
    DECL_CLSID( CRemoteProxyState ),
    DECL_CLSID( CIfServerState ),
    DECL_CLSID( CIfEnableEventTask ),
    DECL_CLSID( CIfOpenPortTask ),
    DECL_CLSID( CIfStartRecvMsgTask ),
    DECL_CLSID( CIfTaskGroup ),
    DECL_CLSID( CIfRootTaskGroup ),
    DECL_CLSID( CIfStopTask ),
    DECL_CLSID( CIfStopRecvMsgTask ),
    DECL_CLSID( CIfPauseResumeTask ),
    DECL_CLSID( CIfCpEventTask ),
    DECL_CLSID( CIfDummyTask ),
    DECL_CLSID( CIfCleanupTask ),
    DECL_CLSID( CIfParallelTaskGrp ),
    DECL_CLSID( CIfIoReqTask ),
    DECL_CLSID( CIfInvokeMethodTask ),
    DECL_CLSID( CIfServiceNextMsgTask ),
    DECL_CLSID( CIfFetchDataTask ),
    DECL_CLSID( CDBusLoopbackPdo ),
    DECL_CLSID( CDBusLoopbackMatch ),
    DECL_CLSID( CDBusTransLpbkMsgTask ),
    DECL_CLSID( CRpcTcpFido ),
    DECL_CLSID( CTcpStreamPdo ),
    DECL_CLSID( CRpcStreamSock ),
    DECL_CLSID( CRpcListeningSock ),
    DECL_CLSID( CRpcStream ),
    DECL_CLSID( CRpcControlStream ),
    DECL_CLSID( CCancelIrpsTask ),
    DECL_CLSID( CStmSockCompleteIrpsTask ),
    DECL_CLSID( COutgoingPacket ),
    DECL_CLSID( CIncomingPacket ),
    DECL_CLSID( CRpcTcpBusPort ),
    DECL_CLSID( CRpcTcpBusDriver ),
    DECL_CLSID( CStmSockConnectTask ),
    DECL_CLSID( CStmSockInvalStmNotifyTask ),
    DECL_CLSID( CStmPdoSvrOfflineNotifyTask ),
    DECL_CLSID( CFidoRecvDataTask ),
    DECL_CLSID( CReqFwdrOpenRmtPortTask ),
    DECL_CLSID( CReqFwdrEnableRmtEventTask ),
    DECL_CLSID( CBridgeAddRemoteMatchTask ),
    DECL_CLSID( CRouterOpenRmtPortTask ),
    DECL_CLSID( CRegisteredModule ),
    DECL_CLSID( CRouterLocalMatch ),
    DECL_CLSID( CRouterRemoteMatch ),
    DECL_CLSID( CRouterEnableEventRelayTask ),
    DECL_CLSID( CReqFwdrForwardRequestTask ),
    DECL_CLSID( CReqFwdrSendDataTask ),
    DECL_CLSID( CReqFwdrFetchDataTask ),
    DECL_CLSID( CRpcRfpForwardEventTask ),
    DECL_CLSID( CBdgeProxyReadWriteComplete ),
    DECL_CLSID( CBdgeProxyOpenStreamTask ),
    DECL_CLSID( CBdgeProxyStartSendTask ),
    DECL_CLSID( CBdgeStartRecvDataTask ),
    DECL_CLSID( CBdgeStartFetchDataTask ),
    DECL_CLSID( CSyncCallback ),
    DECL_CLSID( CTaskWrapper ),
    DECL_CLSID( CClassFactory ),
    DECL_CLSID( MaxClsid ),
    DECL_CLSID( ReservedClsidEnd ) = 0x0FFFFFFF,
    DECL_CLSID( UserClsidStart ) = 0x10000000,

    DECL_CLSID( Invalid ) = 0x7FFFFFFF

}EnumClsid;

