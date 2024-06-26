// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// /usr/local/bin/ridlc -JO. --odesc_url=http://example.com/rpcf ../../stmtest.ridl 
const { randomInt } = require( '/usr/local/etc/rpcf/./jslib/combase/defines' );
const {CoCreateInstance, RegisterFactory}=require( '/usr/local/etc/rpcf/./jslib/combase/factory' );
const {CIoManager} = require( '/usr/local/etc/rpcf/./jslib/ipc/iomgr' );

// Start the iomanager
globalThis.g_iMsgIdx = randomInt( 0xffffffff );
globalThis.CoCreateInstance=CoCreateInstance;
globalThis.RegisterFactory=RegisterFactory;
globalThis.g_oIoMgr = new CIoManager();
globalThis.g_oIoMgr.Start()


const { CConfigDb2 } = require( '/usr/local/etc/rpcf/./jslib/combase/configdb' );
const { messageType } = require( '/usr/local/etc/rpcf/./jslib/dbusmsg/constants' );
const { ERROR, Int32Value, USER_METHOD } = require( '/usr/local/etc/rpcf/./jslib/combase/defines' );
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require( '/usr/local/etc/rpcf/./jslib/combase/enums' );
const {CSerialBase} = require( '/usr/local/etc/rpcf/./jslib/combase/seribase' );
const {CInterfaceProxy} = require( '/usr/local/etc/rpcf/./jslib/ipc/proxy' )
const {Buffer} = require( 'buffer' );
const { DBusIfName, DBusDestination2, DBusObjPath } = require( '/usr/local/etc/rpcf/./jslib/rpc/dmsg' );
// Start the client(s)
const { CStreamTest_CliImpl } = require( './StreamTestcli' )

var oProxy = null;
var strObjDesc = 'http://example.com/rpcf/stmtestdesc.json';
var strAppName = 'stmtest';
var strStreamTestObjName = 'StreamTest';
var oParams0 = globalThis.CoCreateInstance( EnumClsid.CConfigDb2 );
oParams0.SetString( EnumPropId.propObjInstName, 'StreamTest' );
var oStreamTest_cli = new CStreamTest_CliImpl( globalThis.g_oIoMgr,
    strObjDesc, strStreamTestObjName, oParams0 );

 // start the client object
oStreamTest_cli.Start().then((retval)=>{
    if( ERROR( retval ) )
    {
        globalThis.oProxy = null;
        return Promise.reject( retval );
    }
    oProxy = oStreamTest_cli;
    globalThis.oProxy = oProxy;
    /*
    * sample code to open a stream channel, it
    * can be done elsewhere when starting is complete
    */

    return new Promise( (resolve, reject)=>{
        var oStmCtx = new Object()
        oStmCtx.m_oResolve = resolve
        oStmCtx.m_oReject = reject
        return oProxy.Echo( oStmCtx, "hello, World!" )
    }).then((oStmCtx)=>{
        oStmCtx.m_oProxy = oProxy
        return oProxy.m_funcOpenStream( oStmCtx ).then((oCtx)=>{
            console.log( "OpenStream successfully" )
            return Promise.resolve( oStmCtx )
        }).catch((oCtx)=>{
            console.log( "OpenStream failed" )
            return Promise.resolve( oStmCtx )
        })
    }).catch( (oStmCtx )=>{
        console.log( oStmCtx )
        return Promise.resolve( oStmCtx )
    })
}).then((retval)=>{
    setTimeout( ()=>{
        console.log("Stopping the proxy")
        oProxy.Stop()
    }, 60000 )
}).catch((e)=>{
    console.log( 'Start Proxy failed ' + e );
    return Promise.resolve(e);
})

