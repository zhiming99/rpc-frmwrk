/*
 * =====================================================================================
 *
 *       Filename:  propids.h
 *
 *    Description:  definitions of property ids
 *
 *        Version:  1.0
 *        Created:  06/18/2016 09:32:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#pragma once

enum EnumPropId : gint32
{
    // the lower 0x100 for
    // positional parameters
    //
    // Interface related property
    propSerialNumber = 0x1000,
    propSrcDBusName,
    propDestDBusName,
    propSrcUniqName,
    propDestUniqName,
    propIfName,
    propObjPath,
    propMethodName,
    propMatchType,      // type: gint32
    propSrcIpAddr,
    propDestIpAddr,
    propSrcTcpPort,
    propDestTcpPort,

    // Port related property
    propDrvName = 0x2000,
    propBusName,        // type: string
    propBusClass,       // type: string
    propBusId,          // type: guint32 
    propProtocol,       // type: string
    propPortName,
    propPortClass,
    propPortId,         // type: guint32
    propPortState,      // type: guint32
    propFdoName,
    propFdoClass,
    propFdoId,          // type: guint32
    propPdoName,
    propPdoClass,
    propPdoId,          // type: guint32
    propIpAddr,         // type: string
    propPortType,       // type: guint32
    propIsBusPort,      // type: bool
    propModName,
    propRegPath,        // type: string
    propPath1,          // type: string
    propPath2,
    propPath3,
    propPortPtr,        // type: ObjPtr to the port interested
    propUpperPortPtr,   // type: ObjPtr
    propLowerPortPtr,   // type: ObjPtr
    propBusPortPtr,     // type: ObjPtr to the bus port
    propEventMapPtr,    // type: ObjPtr to CStlEventMap
    propMsgPtr,         // type: DMsgPtr to the DBusMessage or CfgPtr,
                        //      depending on the type flag of the container CBuffer

    // connection points
    propStartStopEvent, // type: a connection point for port start/stop event 
    propDBusSysEvent,   // type: a connection point for local dbus daemon online/offline
    propDBusModEvent,   // type: a connection point for local dbus module online/offline
    propRmtSvrEvent,    // type: a connection point for online/offline of the remote server
    propRmtModEvent,    // type: a connection point for online/offline of the remote module
    propAdminEvent,     // type: a connection point for online/offline of the remote module

    // misc
    propRpcSvrPortNum = 0x3000,  // type: integer
    propObjPtr,         // type: ObjPtr of IPort*
    propMatchMap,       // type: ObjPtr of CStdMap<ObjPtr, MatchPtr> and
                        //      ObjPtr is pointer to the interface, which will
                        //      be used as handle for removal

    propIoMgr,          // type: ObjPtr of pointer to CIoManager
    propEventSink,      // type: ObjPtr of pointer to IEventSink
    propConfigPath,     // type: string, for config file path
    propDrvPtr,         // type: ObjPtr of IPortDriver*
    propDBusConn,       // type: guint32
    propParamList,      // type: ObjPtr to CStlIntVector, used to pass parameters in CTasklet::OnEvent
    propParamCount,     // type: guint32 sizeof the propParamList

    propTimerParamList, // type: ObjPtr to CStlIntVector, used to pass parameters in CTasklet::OnEvent for eventTimeout
    propNotifyParamList, // type: ObjPtr to CStlIntVector, used to pass parameters in CTasklet::OnEvent for eventRpcNotify
    propIrpPtr,         // type: ObjPtr to Irp
    propMasterIrp,      // type: ObjPtr to the Master Irp
    propPortPtrs,       // type: ObjPtr to CStlPortVector
    propEventId,        // type: guint32
    propOtherThread,    // type: bool
    propIntervalSec,    // type: guint32 for time interval in second
    propRetries,        // type: guint32 for times to retry
    propRetriesBackup,  // type: guint32 as backup of propRetries
    propTimerId,        // type: guint32 as a timer id
    propParentPtr,      // type: ObjPtr to a generic obj pointer which contains the current object
    propSingleIrp,      // type: bool to tell CRpcPdoPort to complete a event irp in the match map
    propObjId,          // type: guint64 for a unique object id
    propClsid,          // type: guint32 for the class id
    propRefCount,       // type: guint32 for the reference count of the object
    propFuncAddr,       // type: BufPtr for the 32-bit address of a function entry 
    propHandle,         // type: guint32 for a handle
    propIfPtr,          // type: ObjPtr of pointer to a IInterfaceState
    propTaskPtr,        // type: ObjPtr of pointer to a tasklet
    propParentTask,     // type: ObjPtr of pointer to a parent tasklet
    propQueuedReq,      // type: bool value to indicate InvokeMethod to service the req one by one or concurrently
    propMatchPtr,       // type: ObjPtr of pointer to a message match
    propRouterPtr,      // type: ObjPtr of pointer to a CRpcRouter
    propSubmitPdo,      // type: boolean, to indicate whether the request will go direct to pdo instead of the whole port stack

    // if task control bits
    propNotifyClient,   // type: bool value to indicate whether to notify the client
    propContext,        // type: guint32 for a context param
    propRunning,        // type: bool value to indicate a tasklet is running, that is whether the runtask is called
    propNoResched,      // type: bool value to indicate no Reschedule after the sub-task completed
    propCanceling,      // type: bool value to indicate a taskis being canceled
    propCancelTid,      // type: bool value to indicate which thread the cancel method is running on

    propReturnValue,    // type: gint32 value as the return value of a request
    propCallFlags,      // type: guint32 contains flags for the call
    propCallOptions,    // type: guint32 contains flags for the call
    propKeepAliveSec,   // type: guint32 contains the seconds to wait before heartbeat are sent
    propTimeoutSec,     // type: guint32 contains the seconds to wait before the request is timeout
    propTaskId,         // type: guint64 contains the taskid in the call request
    propRmtTaskId,      // type: guint64 contains the taskid for canceling purpose
    propRespPtr,        // type: ObjPtr contains the response CfgPtr 
    propReqPtr,         // type: ObjPtr contains the request CfgPtr 
    propFilePath,       // type: string for file path
    propFilePath1,      // type: string for second file path 
    propFd,             // type: gint32 for file descriptor
    propStreamId,       // type: gint32 for the stream id
    propByteCount,      // type: guint32 for the size to transfer
    propPeerStmId,      // type: gint32 for the stream id from the peer
    propSeqNo,          // type: guint32 for a sequence number
    propSeqNo2,         // type: guint32 for a sequence number
    propCmdId,          // type: guint32 for a command id
    propProtoId,        // type: guint32 for a protocol id
    propReqType,        // type: guint32 for a request type, same as DMsgPtr::GetType()
    propSockPtr,        // type: ObjPtr, contains a pointer to the socket
    propUserName,       // type: ObjPtr, contains a string of user name
    propNonFd,          // type: bool for non file descriptor in SendData/FetchData transfer
    propPasswd,         // type: string, contains a string of password hash
    propThreadId,       // type: gint32, contains a thread id
    propIfStateClass,   // type: class id of the interface-state
    propObjList,        // type: ObjPtr to a CStlObjVector object
    propValList,        // type: ObjPtr to whatever list
    propIid,            // type: an interface id of EnumClsid type, used in CMessageMatch
    propTaskState,      // type: an integer of EnumTaskState type, in CIfParallelTask
    propPausable,       // type: a bool value to indicate the match is for an interface which can be paused
    propSysMethod,      // type: a bool value to indicate if the call is from a built-in interface
    propPauseOnStart,   // type: a bool value to indicate the server to pause on start
    propQueSize,        // type: guint32 for the pending message size for incoming requests
    propStreaming,      // type: a bool value to indicate if current FetchData call is for streaming
    propMsgCount,       // type: a invoke message counter for used by CMessageCounter
    propMsgOutCount,    // type: a send message counter for used by CMessageCounter
    propMsgRespCount,   // type: a resp message counter for used by CMessageCounter
    propDummyMatch,     // type: the match is for state transition only, not for matching purpose
    propObjInstName,    // type: the instance name of an object, used to construct the objpath
    propSvrInstName,    // type: the instance name of a server, used to construct the objpath
    propRouterRole,     // type: gint32, the role of the router instance: 01 as a forwarder, 02 as a server,
                        //       03 or not exist as both forwarder and server, 
    propListenSock,     // type: bool to indicate whether the tcpbusport need to create the listening socket for incoming requests
    propReseredEnd = 0x10000000,
    propInvalid = -1, 
};

#define PropIdFromInt( intid ) ( static_cast< EnumPropId >( intid ) )
#define PropId( propid ) ( static_cast< gint32 >( propid ) )

