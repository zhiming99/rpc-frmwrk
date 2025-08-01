// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2025  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// /usr/local/bin/ridlc -JO. --odesc_url=https://example.com/rpcf ../../actcancel.ridl 
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
const { CActiveCancel_CliImpl } = require( './ActiveCancelcli' )

var oProxy = null;
var strObjDesc = 'https://example.com/rpcf/actcanceldesc.json';
var strAppName = 'actcancel';
var strActiveCancelObjName = 'ActiveCancel';
var oParams0 = globalThis.CoCreateInstance( EnumClsid.CConfigDb2 );
oParams0.SetString( EnumPropId.propObjInstName, 'ActiveCancel' );
var oActiveCancel_cli = new CActiveCancel_CliImpl( globalThis.g_oIoMgr,
    strObjDesc, strActiveCancelObjName, oParams0 );

 // start the client object
oActiveCancel_cli.Start().then((retval)=>{
    if( ERROR( retval ) )
    {
        globalThis.oProxy = null;
        return Promise.resolve( retval );
    }
    oProxy = oActiveCancel_cli;
    globalThis.oProxy = oProxy;
    if( globalThis.onRpcReady !== undefined )
        globalThis.onRpcReady();
}).catch((e)=>{
    console.log( 'Start Proxy failed ' + e );
    return Promise.resolve(e);
})

