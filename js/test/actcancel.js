const { CConfigDb2 } = require("../combase/configdb")
const { messageType } = require( "../dbusmsg/constants")
const { randomInt, ERROR, Int32Value, USER_METHOD, Pair } = require("../combase/defines")
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require("../combase/enums")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("../combase/factory")
const {CSerialBase} = require("../combase/seribase")
globalThis.CoCreateInstance=CoCreateInstance
const {CIoManager} = require( "../ipc/iomgr")
const {CInterfaceProxy} = require( "../ipc/proxy")
const {CBuffer} = require("../combase/cbuffer")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
var g_oIoMgr = new CIoManager()
g_oIoMgr.Start()

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString( EnumPropId.propObjInstName, strObjName)

var strObjDesc = "http://example.com/rpcf/actcanceldesc.json"
var strObjName = "ActiveCancel"
var strAppName = "actcancel"

class CActiveCancelCli extends CInterfaceProxy
{
    constructor( oIoMgr,
        strObjDesc, strObjName, oParams )
    {
        super( oIoMgr, strObjDesc, strObjName )
    }
    LongWaitCallback(oContext, ret, strResp)
    {
        var qwTaskId = oContext.m_qwTaskId
        if( ret === errno.ERROR_USER_CANCEL)
        {
            console.log( `request ${qwTaskId} is canceled ${Int32Value(ret)}`)
            return
        }
        else if( ERROR( ret) )
        {
            console.log( `error occurs ${Int32Value(ret)}`)
            return
        }
        console.log( `server returns ${strResp}`)
    }

    LongWait( oContext, strText, oCallback=( oContext, oResp )=>{
        try{
            var ret = oResp.GetProperty(
                EnumPropId.propReturnValue)
            if( ERROR( ret ) )
            {
                this.LongWaitCallback( oContext, ret )
                return
            }
            var args = [ oContext, ret ]
            var ridlBuf = Buffer.from( oResp.GetProperty( 0 ) )
            var osb = new CSerialBase( false, this )
            var ret = osb.DeserialString( ridlBuf, 0 )
            args.push( ret[0])
            this.LongWaitCallback.apply( this, args )
        }
        catch( e )
        {
            console.log(e)
        }
    })
    {
        var oReq = new CConfigDb2()
        var osb = new CSerialBase(true, this )
        osb.SerialString( strText ) 
        var ridlBuf = osb.GetRidlBuf()
        oReq.Push( new Pair( {t: EnumTypeId.typeByteArr, v:ridlBuf} ) )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( "IActiveCancel") )
        oReq.SetString( EnumPropId.propObjPath,
            DBusObjPath( strAppName, strObjName))
        oReq.SetString( EnumPropId.propSrcDBusName,
            this.m_strSender)
        oReq.SetUint32( EnumPropId.propSeriProto,
            EnumSeriProto.seriRidl )
        oReq.SetString( EnumPropId.propDestDBusName,
            DBusDestination2(strAppName, strObjName ))
        oReq.SetString( EnumPropId.propMethodName, 
            USER_METHOD("LongWait"))
        var oCallOpts = new CConfigDb2()
        oCallOpts.SetUint32(
            EnumPropId.propTimeoutsec, 97)
        oCallOpts.SetUint32(
            EnumPropId.propKeepAliveSec, 48 )
        oCallOpts.SetUint32( EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )
        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oCallOpts)
        var ret =  this.m_funcForwardRequest(
            oReq, oCallback, oContext )
        console.log( `taskid to cancel is ${oContext.m_qwTaskId}`)
        return ret
    }
}

var oProxy = new CActiveCancelCli(
    g_oIoMgr, strObjDesc, strObjName, oParams )

oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    oContext = new Object()
    oProxy.LongWait( oContext, "hello, World!" )
    if( oContext.m_qwTaskId !== undefined )
    {
        setTimeout( ()=>{
            oProxy.m_funcCancelRequest(
                oContext.m_qwTaskId,
                (ret, qwTaskId)=>{
                    console.log( `Canceling request ${qwTaskId} completed with status ${Int32Value(ret)}` )
                })
        }, 5000)
    }

}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})


