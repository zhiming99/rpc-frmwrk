// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2025  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// /usr/local/bin/ridlc -JO. -Lcn --odesc_url=https://example.com/rpcf ../../hellowld.ridl 
const { randomInt } = require( '/usr/local/lib/rpcf/jslib/combase/defines' );
const {CoCreateInstance, RegisterFactory}=require( '/usr/local/lib/rpcf/jslib/combase/factory' );
const {CIoManager} = require( '/usr/local/lib/rpcf/jslib/ipc/iomgr' );

// Start the iomanager
globalThis.g_iMsgIdx = randomInt( 0xffffffff );
globalThis.CoCreateInstance=CoCreateInstance;
globalThis.RegisterFactory=RegisterFactory;
globalThis.g_oIoMgr = new CIoManager();
globalThis.g_oIoMgr.Start()


const { CConfigDb2 } = require( '/usr/local/lib/rpcf/jslib/combase/configdb' );
const { messageType } = require( '/usr/local/lib/rpcf/jslib/dbusmsg/constants' );
const { ERROR, Int32Value, USER_METHOD } = require( '/usr/local/lib/rpcf/jslib/combase/defines' );
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require( '/usr/local/lib/rpcf/jslib/combase/enums' );
const {CSerialBase, Variant} = require( '/usr/local/lib/rpcf/jslib/combase/seribase' );
const {CInterfaceProxy} = require( '/usr/local/lib/rpcf/jslib/ipc/proxy' )
const {Buffer} = require( 'buffer' );
const { DBusIfName, DBusDestination2, DBusObjPath } = require( '/usr/local/lib/rpcf/jslib/rpc/dmsg' );
// Start the client(s)
const { CHelloWorldSvc_CliImpl } = require( './HelloWorldSvccli' )

var oProxy = null;
var strObjDesc = 'https://example.com/rpcf/HelloWorlddesc.json';
var strAppName = 'HelloWorld';
var strHelloWorldSvcObjName = 'HelloWorldSvc';
var oParams0 = globalThis.CoCreateInstance( EnumClsid.CConfigDb2 );
oParams0.SetString( EnumPropId.propObjInstName, 'HelloWorldSvc' );
var oHelloWorldSvc_cli = new CHelloWorldSvc_CliImpl( globalThis.g_oIoMgr,
    strObjDesc, strHelloWorldSvcObjName, oParams0 );

 // start the client object
oHelloWorldSvc_cli.Start().then((retval)=>{
    if( ERROR( retval ) )
    {
        globalThis.oProxy = null;
        return Promise.resolve( retval );
    }
    oProxy = oHelloWorldSvc_cli;

    /*
    * sample code to make a request
    oHelloWorldSvc_cli.Echo( strText );
    * and the response goes to 'oHelloWorldSvc_cli.EchoCallback'
    */
    
    /*
    * sample code to make a request with promise
    return new Promise( ( resolve, reject )=>{
        var oContext = new Object();
        oContext.m_oResolve = resolve;
        oContext.m_oReject = reject;
        return oHelloWorldSvc_cli.Echo( strText );
    }).then(( oContext)=>{
        console.log( 'request Echo is done with status ' + oContext.m_iRet );
    }).catch((e)=>{
        console.log(e);
    })
    */
    globalThis.oProxy = oProxy;
    if( globalThis.onRpcReady !== undefined )
        globalThis.onRpcReady();
}).catch((e)=>{
    console.log( 'Start Proxy failed ' + e );
    return Promise.resolve(e);
})

