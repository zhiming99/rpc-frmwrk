// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -JO. -Lcn --odesc_url=http://example.com/rpcf ../../katest.ridl 
const { CConfigDb2 } = require( '/usr/local/etc/rpcf/./jslib/combase/configdb' );
const { messageType } = require( '/usr/local/etc/rpcf/./jslib/dbusmsg/constants' );
const { randomInt, ERROR, Int32Value, USER_METHOD } = require( '/usr/local/etc/rpcf/./jslib/combase/defines' );
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require( '/usr/local/etc/rpcf/./jslib/combase/enums' );
const {CSerialBase} = require( '/usr/local/etc/rpcf/./jslib/combase/seribase' );
const {CInterfaceProxy} = require( '/usr/local/etc/rpcf/./jslib/ipc/proxy' )
const {Buffer} = require( 'buffer' );
const { DBusIfName, DBusDestination2, DBusObjPath } = require( '/usr/local/etc/rpcf/./jslib/rpc/dmsg' );
const { CKeepAliveclibase } = require ( './KeepAliveclibase' )
class CKeepAlive_CliImpl extends CKeepAliveclibase
{
    constructor( oIoMgr, strObjDescPath, strObjName, oParams )
    {
        super( oIoMgr, strObjDescPath, strObjName, oParams );
    }

    // The following are callbacks and event handlers
    // which need to be filled out by you
    
    // IKeepAlive::LongWaitCallback
    // request callback
    LongWaitCallback( oContext, ret,
        i0r // String
    )
    {
        // add code here
        // 'oContext' is the parameter passed to this.LongWait
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
}

exports.CKeepAlive_CliImpl = CKeepAlive_CliImpl;