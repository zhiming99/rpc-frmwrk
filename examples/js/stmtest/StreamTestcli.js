// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// /usr/local/bin/ridlc -JO. --odesc_url=http://example.com/rpcf ../../stmtest.ridl 
const { CConfigDb2 } = require( '/usr/local/etc/rpcf/./jslib/combase/configdb' );
const { messageType } = require( '/usr/local/etc/rpcf/./jslib/dbusmsg/constants' );
const { randomInt, ERROR, Int32Value, USER_METHOD } = require( '/usr/local/etc/rpcf/./jslib/combase/defines' );
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require( '/usr/local/etc/rpcf/./jslib/combase/enums' );
const {CSerialBase} = require( '/usr/local/etc/rpcf/./jslib/combase/seribase' );
const {CInterfaceProxy} = require( '/usr/local/etc/rpcf/./jslib/ipc/proxy' )
const {Buffer} = require( 'buffer' );
const { DBusIfName, DBusDestination2, DBusObjPath } = require( '/usr/local/etc/rpcf/./jslib/rpc/dmsg' );
const { CStreamTestclibase } = require ( './StreamTestclibase' )
class CStreamTest_CliImpl extends CStreamTestclibase
{
    constructor( oIoMgr, strObjDescPath, strObjName, oParams )
    {
        super( oIoMgr, strObjDescPath, strObjName, oParams );
        this.OnDataReceived = this.OnDataReceivedImpl.bind( this );
        this.OnStmClosed = this.OnStmClosedImpl.bind( this );
    }

    // The following are callbacks and event handlers
    // which need to be filled out by you
    
    // IStreamTest::EchoCallback
    // request callback
    EchoCallback( oContext, ret,
        i0r // String
    )
    {
        // add code here
        // 'oContext' is the parameter passed to this.Echo
        // 'ret' is the status code of the request
        // if ret == errno.STATUS_SUCCESS, the following parameters are valid response from the server
        // if ret < 0, the following parameters are undefined
        if( ERROR( ret) )
        {
            this.DebugPrint( `error occurs ${Int32Value(ret)}`)
            return
        }
        this.DebugPrint( `server returns ${i0r}`)
    }

    /**
    * callback called when data arrives from the stream channel `hStream`
    * @param {number} hStream the stream channel the data comes in 
    * @param {Buffer} oBuf the payload in Uint8Array or Buffer
    */
    OnDataReceivedImpl( hStream, oBuf )
    {
        // add code here
        // to handle the incoming byte stream
        const decoder = new TextDecoder();
        var strResp = decoder.decode( oBuf )
        strResp = strResp.replace('\0', '')
        this.DebugPrint( `stream@${hStream} received: ` + strResp )
        const encoder = new TextEncoder()
        var oNewMsge = encoder.encode( Date.now() + " proxy: Hello, world!")
        this.m_funcStreamWrite( hStream, oNewMsge ).then((e)=>{
            if( ERROR( e ))
                this.DebugPrint( `Error ${e}`)
        });
    }

    /**
    * callback called when the stream channel `hStream` is closed
    * @param {number} hStream the stream channel closed 
    */
    OnStmClosedImpl( hStream )
    {
        // add code here
        // to do some cleanup work if necessary
        this.DebugPrint( `stream@${hStream} closed ` + oMsg)
    }
}

exports.CStreamTest_CliImpl = CStreamTest_CliImpl;
