const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR, InvalFunc } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumMatchType } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")
const { DBusIfName, DBusDestination, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
const { CMessageMatch } = require( "../combase/msgmatch")
const { ForwardRequestLocal } = require("./fwrdreq")
const { ForwardEventLocal } = require("./fwrdevt")
const { CInterfaceProxy } = require("./proxy")
const { OpenStreamLocal, CloseStreamLocal } = require("./openstm")
const { OnDataReceivedLocal, OnStreamClosedLocal, NotifyDataConsumed} = require( "./stmevt")
const { StreamWriteLocal } = require( "./stmwrite")

class CFastRpcChanProxy extends CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        var oOwner = oParams.GetPropery(
            EnumPropId.propParentPtr)
        if( !oOwner )
            throw new Error( "Error parent object is not given")
        this.m_oOwner = oOwner;
        this.OnDataReceived = function( hStream, oBuf )
        {

        }
        this.OnStreamClosed = function( hStream )
        {}
    }

    SendRequest( oMsg )
    {

    }

    OnRespReceived( hStream, oBuf )
    {

    }

    OnEventReceived( hStream, oBuf )
    {

    }

    ForwardRequest( oReq, oCallback, oContext )
    {}
}

class CFastRpcProxy extends CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        super( oMgr, strObjDesc, strObjName, oParams );
        var strChannName = strObjName + "_ChannelSvr";
        this.m_oChanProxy = new CFastRpcChanProxy(
            oMgr, strObjDesc, strChannName, oParams );
    }

    Start()
    {
        // start the channel client first
        var oChanCb = function( retVal )
        {
            if( ERROR( retVal ) )
                return retVal;
            
            var oStmCtx;
            oStmCtx.m_oProxy = this;
            return oFileTransfer_cli.m_funcOpenStream( oStmCtx).then((oCtx)=>{
                this.m_hStream = oCtx.m_hStream;
                return Promise.resolve( oCtx );
            }).catch((oCtx)=>{
                console.log( 'OpenStream failed with error=' + oCtx.m_iRet );
                return Promise.resolve( oCtx );
            })
        }
        this.m_oChanProxy.Start().then(
            oChanCb.bind( this.m_oChanProxy ))
    }
    PostMessage( oMsg )
    {}
}

exports.CFastRpcProxy = CFastRpcProxy