module.exports = {
    constval :{
      PAGE_SIZE : 4096,
      BUF_MAX_SIZE: (512*1024*1024),
      MAX_BYTES_PER_TRANSFER: (1024*1024),
      CFGDB_MAX_ITEMS: 1024,
      CFGDB_MAX_SIZE: (16*1024*1024)
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
    },

    EnumPropId: {
        propSrcDBusName: 4097,
        propDestDBusName: 4098,
        propIfName: 4101,
        propObjPath: 4102,
        propMethodName: 4103,
        propPath2: 8213,
        propMsgPtr: 8220,
        propConfigPath: 12293,
        propParamCount:12297,
        propReturnValue: 12329,
        propCallFlags: 12331,
        propCallOptions: 12331,
        propKeepAliveSec: 12332,
        propTimeoutsec: 12333,
        propTaskId: 12334,
        propRespPtr: 12336,
        propReqPtr: 12337,
        propSysMethod: 12360,
        propRouterPath: 12397,
        propConnHandle: 12394,
        propSessHash: 12401,
        propTimestamp: 12407,
        propContinue: 12411,
        propSeriProto:12420,
        propNoReply: 12421,
    },

  EnumMatchType: {
      matchServer : 0,
      matchClient  : 1,
      matchInvalid : 2,
  }
}