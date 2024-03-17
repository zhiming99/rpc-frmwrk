// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// /usr/local/bin/ridlc -JO. --odesc_url=http://example.com/rpcf ../../sftest.ridl 
const { CConfigDb2 } = require( '/usr/local/etc/rpcf/./jslib/combase/configdb' );
const { messageType } = require( '/usr/local/etc/rpcf/./jslib/dbusmsg/constants' );
const { randomInt, ERROR, Int32Value, USER_METHOD } = require( '/usr/local/etc/rpcf/./jslib/combase/defines' );
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require( '/usr/local/etc/rpcf/./jslib/combase/enums' );
const {CSerialBase} = require( '/usr/local/etc/rpcf/./jslib/combase/seribase' );
const {CInterfaceProxy} = require( '/usr/local/etc/rpcf/./jslib/ipc/proxy' )
const {Buffer} = require( 'buffer' );
const { DBusIfName, DBusDestination2, DBusObjPath } = require( '/usr/local/etc/rpcf/./jslib/rpc/dmsg' );
const { FileInfo, } = require( './sfteststructs' );
const { CFileTransferclibase } = require ( './FileTransferclibase' )
class CFileTransfer_CliImpl extends CFileTransferclibase
{
    constructor( oIoMgr, strObjDescPath, strObjName, oParams )
    {
        super( oIoMgr, strObjDescPath, strObjName, oParams );
        this.OnDataReceived = this.OnDataReceivedImpl;
        this.OnStmClosed = this.OnStmClosedImpl;
    }

    // The following are callbacks and event handlers
    // which need to be filled out by you
    
    // IFileTransfer::StartUploadCallback
    // request callback
    StartUploadCallback( oContext, ret )
    {
        // add code here
        // 'oContext' is the parameter passed to this.StartUpload
        // 'ret' is the status code of the request
        // if ret == errno.STATUS_SUCCESS, the following parameters are valid response from the server
        // if ret < 0, the following parameters are undefined
    }

    // IFileTransfer::StartDownloadCallback
    // request callback
    StartDownloadCallback( oContext, ret )
    {
        // add code here
        // 'oContext' is the parameter passed to this.StartDownload
        // 'ret' is the status code of the request
        // if ret == errno.STATUS_SUCCESS, the following parameters are valid response from the server
        // if ret < 0, the following parameters are undefined
    }

    // IFileTransfer::GetFileInfoCallback
    // request callback
    GetFileInfoCallback( oContext, ret,
        fi // FileInfo
    )
    {
        // add code here
        // 'oContext' is the parameter passed to this.GetFileInfo
        // 'ret' is the status code of the request
        // if ret == errno.STATUS_SUCCESS, the following parameters are valid response from the server
        // if ret < 0, the following parameters are undefined
    }

    // IFileTransfer::RemoveFileCallback
    // request callback
    RemoveFileCallback( oContext, ret )
    {
        // add code here
        // 'oContext' is the parameter passed to this.RemoveFile
        // 'ret' is the status code of the request
        // if ret == errno.STATUS_SUCCESS, the following parameters are valid response from the server
        // if ret < 0, the following parameters are undefined
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
        if( globalThis.OnStreamDataReceived)
            globalThis.OnStreamDataReceived( hStream, oBuf );
    }

    /**
    * callback called when the stream channel `hStream` is closed
    * @param {number} hStream the stream channel closed 
    */
    OnStmClosedImpl( hStream )
    {
        // add code here
        // to do some cleanup work if necessary
    }
}

exports.CFileTransfer_CliImpl = CFileTransfer_CliImpl;
