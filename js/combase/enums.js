module.exports = {
    constval :{
      PAGE_SIZE : 4096,
      BUF_MAX_SIZE: (512*1024*1024),
      MAX_BYTES_PER_TRANSFER: (1024*1024),
      MAX_BYTES_PER_BUFFER: (16*1024*1024),
      CFGDB_MAX_ITEMS: 1024,
      CFGDB_MAX_SIZE: (16*1024*1024),
      DBUS_NAME_PREFIX: "org.rpcf.",
      IFNAME_MIN_BRIDGE: "CRpcMinBridge",
      IFNAME_TCP_BRIDGE: "CRpcTcpBridge",
      OBJNAME_TCP_BRIDGE: "RpcTcpBridgeImpl",
      OBJNAME_ROUTER: "RpcRouterManagerImpl",
      OBJNAME_ROUTER_BRIDGE: "RpcRouterBridgeImpl",
      BRIDGE_PROXY_GREETINGS: "rpcf-bridge-proxy",
      BRIDGE_GREETINGS: "rpcf-bridge", 
      IFSTATE_OPENPORT_INTERVAL : 120,
      IFSTATE_DEFAULT_IOREQ_TIMEOUT:120,
      IFSTATE_OPENPORT_RETRIES: 3,
      INVALID_HANDLE : 0,
      CTRLCODE_OPEN_STREAM_PDO: 0x15,
      CTRLCODE_CLOSE_STREAM_PDO: 0x16,
      STM_MAX_PENDING_WRITE:(1*1024*1024),
      STM_MAX_PACKETS_REPORT: 32,
      STM_MAX_BYTES_PER_BUF: (63*1024),
    },

    EnumTypes:{
      DataTypeMem : 0,
      DataTypeObjPtr: 1,
      DataTypeMsgPtr: 2,
    },

    EnumTypeId: {
        typeNone : 0,
        typeByte : 1,
        typeUInt16 : 2,
        typeUInt32 : 3,
        typeUInt64 : 4,
        typeFloat : 5,
        typeDouble : 6,
        typeString : 7,
        typeDMsg : 8,
        typeObj : 9,
        typeByteArr : 10,
    },

    EnumClsid: {
      Invalid: -1,
      CBuffer: 0x825,
      CConfigDb2: 0x917,
      CStlStrVector: 0x851,
      CMessageMatch: 0x85e,
      CStlObjVector: 0x8ab,
      CClassFactory: 0x8a3,
      CStlQwordVector: 0x900,
      IStream:0x10000003,
    },

    EnumPropId: {
        propSrcDBusName: 4097,
        propDestDBusName: 4098,
        propIfName: 4101,
        propObjPath: 4102,
        propMethodName: 4103,
        propSrcIpAddr: 4105,
        propDestIpAddr: 4106,
        propSrcTcpPort: 4107,
        propDestTcpPort: 4108,
        propPortId: 8199,
        propPath2: 8213,
        propMsgPtr: 8220,
        propConfigPath: 12293,
        propParamCount:12297,
        propMatchPtr: 12320,
        propSubmitPdo: 12322,
        propParentPtr: 12309,
        propContext: 12324,
        propReturnValue: 12329,
        propCallFlags: 12330,
        propCallOptions: 12331,
        propKeepAliveSec: 12332,
        propTimeoutSec: 12333,
        propTaskId: 12334,
        propRespPtr: 12336,
        propReqPtr: 12337,
        propStreamId: 12341,
        propSeqNo: 12344,
        propNonFd: 12351,
        propIid: 12357,
        propSysMethod: 12360,
        //propStreaming: 12363,
        propObjInstName:12368,
        propSvrInstName:12369,
        propRxBytes:12376,
        propTxBytes:12377,
        propRxPkts:12378,
        propTxPkts:12379,
        propDataDesc:12382,
        propRouterName:12384,
        propCompress:12388,
        propEnableSSL:12389,
        propEnableWebSock:12390,
        propDestUrl: 12391,
        propConnParams: 12393,
        propConnHandle: 12394,
        propTransCtx: 12395,
        propRouterPath: 12397,
        propSessHash: 12401,
        propTimestamp: 12407,
        propContinue: 12411,
        propPeerObjId:12419,
        propSeriProto:12420,
        propNoReply: 12421,
    },

    EnumMatchType: {
        matchServer : 0,
        matchClient  : 1,
        matchInvalid : 2,
    },

    EnumProtoId: {
        protoDBusRelay : 0,
        protoStream: 1,
        protoControl: 2,
    },

    EnumStmId : {
        TCP_CONN_DEFAULT_CMD : 0,
        TCP_CONN_DEFAULT_STM : 1,
    },

    EnumStmToken : {
        tokInvalid : 255,
        tokData : 1,
        tokPing : 2,
        tokPong : 3,
        tokClose : 4,
        tokProgress : 5,
        tokFlowCtrl : 6,
        tokLift : 7,
        tokError : 8,
    },

    EnumPktFlag :{
        flagCompress : 1,
    },

    errno : {
        EPERM : 1,
        ENOENT : 2,
        ESRCH : 3,
        EINTR : 4,
        EIO : 5,
        ENXIO : 6,
        E2BIG : 7,
        ENOEXEC : 8,
        EBADF : 9,
        ECHILD : 10,
        EAGAIN : 11,
        ENOMEM : 12,
        EACCES : 13,
        EFAULT : 14,
        ENOTBLK : 15,
        EBUSY : 16,
        EEXIST : 17,
        EXDEV : 18,
        ENODEV : 19,
        ENOTDIR : 20,
        EISDIR : 21,
        EINVAL : 22,
        ENFILE : 23,
        EMFILE : 24,
        ENOTTY : 25,
        ETXTBSY : 26,
        EFBIG : 27,
        ENOSPC : 28,
        ESPIPE : 29,
        EROFS : 30,
        EMLINK : 31,
        EPIPE : 32,
        EDOM : 33,
        ERANGE : 34,
        EDEADLK : 35,
        ENAMETOOLONG : 36,
        ENOLCK : 37,
        ENOSYS : 38,
        ENOTEMPTY : 39,
        ELOOP : 40,
        EWOULDBLOCK : 11,
        ENOMSG : 42,
        EIDRM : 43,
        ECHRNG : 44,
        EL2NSYNC : 45,
        EL3HLT : 46,
        EL3RST : 47,
        ELNRNG : 48,
        EUNATCH : 49,
        ENOCSI : 50,
        EL2HLT : 51,
        EBADE : 52,
        EBADR : 53,
        EXFULL : 54,
        ENOANO : 55,
        EBADRQC : 56,
        EBADSLT : 57,
        EDEADLOCK : 35,
        EBFONT : 59,
        ENOSTR : 60,
        ENODATA : 61,
        ETIME : 62,
        ENOSR : 63,
        ENONET : 64,
        ENOPKG : 65,
        EREMOTE : 66,
        ENOLINK : 67,
        EADV : 68,
        ESRMNT : 69,
        ECOMM : 70,
        EPROTO : 71,
        EMULTIHOP : 72,
        EDOTDOT : 73,
        EBADMSG : 74,
        EOVERFLOW : 75,
        ENOTUNIQ : 76,
        EBADFD : 77,
        EREMCHG : 78,
        ELIBACC : 79,
        ELIBBAD : 80,
        ELIBSCN : 81,
        ELIBMAX : 82,
        ELIBEXEC : 83,
        EILSEQ : 84,
        ERESTART : 85,
        ESTRPIPE : 86,
        EUSERS : 87,
        ENOTSOCK : 88,
        EDESTADDRREQ : 89,
        EMSGSIZE : 90,
        EPROTOTYPE : 91,
        ENOPROTOOPT : 92,
        EPROTONOSUPPORT : 93,
        ESOCKTNOSUPPORT : 94,
        EOPNOTSUPP : 95,
        EPFNOSUPPORT : 96,
        EAFNOSUPPORT : 97,
        EADDRINUSE : 98,
        EADDRNOTAVAIL : 99,
        ENETDOWN : 100,
        ENETUNREACH : 101,
        ENETRESET : 102,
        ECONNABORTED : 103,
        ECONNRESET : 104,
        ENOBUFS : 105,
        EISCONN : 106,
        ENOTCONN : 107,
        ESHUTDOWN : 108,
        ETOOMANYREFS : 109,
        ETIMEDOUT : 110,
        ECONNREFUSED : 111,
        EHOSTDOWN : 112,
        EHOSTUNREACH : 113,
        EALREADY : 114,
        EINPROGRESS : 115,
        ESTALE : 116,
        EUCLEAN : 117,
        ENOTNAM : 118,
        ENAVAIL : 119,
        EISNAM : 120,
        EREMOTEIO : 121,
        EDQUOT : 122,
        ENOMEDIUM : 123,
        EMEDIUMTYPE : 124,
        ECANCELED : 125,
        ENOKEY : 126,
        EKEYEXPIRED : 127,
        EKEYREVOKED : 128,
        EKEYREJECTED : 129,
        EOWNERDEAD : 130,
        ENOTRECOVERABLE : 131,
        ERFKILL : 132,
        EHWPOISON : 133,
        ENOTSUP : 95,

        STATUS_PENDING : 0x10001,
        STATUS_MORE_PROCESS_NEEDED : 0x10002,
        STATUS_CHECK_RESP : 0x10003,
        STATUS_SUCCESS : 0,
        ERROR_FAIL : 0x80000001,
        ERROR_TIMEOUT : -110,
        ERROR_CANCEL : -125,
        ERROR_ADDRESS : 0x80010002,
        ERROR_STATE : 0x80010003,
        ERROR_WRONG_THREAD : 0x80010004,
        ERROR_CANNOT_CANCEL : 0x80010005,
        ERROR_PORT_STOPPED : 0x80010006,
        ERROR_FALSE : 0x80010007,
        ERROR_REPEAT : 0x80010008,
        ERROR_PREMATURE : 0x80010009,
        ERROR_NOT_HANDLED : 0x8001000a,
        ERROR_CANNOT_COMP : 0x8001000b,
        ERROR_USER_CANCEL : 0x8001000c,
        ERROR_PAUSED : 0x8001000d,
        ERROR_CANCEL_INSTEAD : 0x8001000f,
        ERROR_NOT_IMPL : 0x80010010,
        ERROR_DUPLICATED : 0x80010011,
        ERROR_KILLED_BYSCHED : 0x80010012,
        ERROR_QUEUE_FULL : 0x8001000e,
    },

    EnumCallFlags : {
      CF_ASYNC_CALL : 0x08,
      CF_WITH_REPLY : 0x10,
      CF_KEEP_ALIVE : 0x20,
      CF_NON_DBUS : 0x40,
    },

    EnumIfState : {
        stateStopped : 0,
        stateStarting: 1,
        stateStarted: 2,
        stateConnected:3,
        stateStopping: 4,
        stateStopped : 5,
        stateStartFailed: 6,
    },

    EnumSeriProto : {
        seriNone : 0,
        seriRidl : 1,
        seriPython : 2,
        seriJava : 3,
        seriInvalid : 4,
    },

    EnumFCState: {
        fcsKeep : 0,
        fcsFlowCtrl : 1,
        fcsLift : 2,
        fcsReport : 3,
    },
}
