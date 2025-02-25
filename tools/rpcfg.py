#!/usr/bin/env python3
#*
#* =====================================================================================
#*
#*       Filename:  rpcfg.py
#*
#*    Description:  implementation of a GUI config tool for rpc-frmwrk
#*
#*        Version:  1.0
#*        Created:  04/26/2021 10:43:00 PM
#*       Revision:  none
#*       Compiler:  gcc
#*
#*         Author:  Ming Zhi( woodhead99@gmail.com )
#*   Organization:
#*
#*      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
#*
#*        License:  Licensed under GPL-3.0. You may not use this file except in
#*                  compliance with the License. You may find a copy of the
#*                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
#*
#* =====================================================================================
#*
import json
import os
import sys 
from copy import deepcopy
from urllib.parse import urlparse
from typing import Tuple
import errno
import tarfile
import time
from updcfg import *
from updwscfg import *
from updk5cfg import *
import re
import platform
import glob
import socket

def vc_changed(stack, gparamstring):
    curTab = stack.get_visible_child_name()
    wnd = stack.get_toplevel()
    #scrolling back to top on switching
    wnd.scrollWin.get_vadjustment().set_value( 0 )
    if  wnd.curTab != "GridMh" :
        wnd.curTab = curTab
        return
    wnd.curTab = curTab
    ret = wnd.UpdateLBGrpPage()
    if ret == -1 :
        wnd.DisplayError( "node name is not valid" )
        stack.set_visible_child( wnd.gridmh )

def SilentRun( strCmd : str ):
    cmdline = "expect -c \"" + strCmd + "\" - <<EOF\n"
    cmdline += "expect {\n"
    cmdline += "\"Sign the certificate? \" { send \"y\\r\";exp_continue}\n" 
    cmdline += "\"requests certified, commit? \" { send \"y\\r\";exp_continue}\n" 
    cmdline += "\"eof\" { exit }\n" 
    cmdline += "\"timeout\" { exit }\n" 
    cmdline += "}\n" 
    cmdline += "EOF\n"
    return rpcf_system( cmdline )

def GenOpenSSLkey( dlg, strPath : str, bServer:bool, cnum : str, snum:str ) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    dir_path += "/opensslkey.sh"
    cmdline = "/bin/bash " + dir_path + " " + strPath + " " + cnum + " " + snum
    bExpect = False
    ret = rpcf_system( "which expect" )
    if ret == 0 :
        bExpect = True
    if bExpect :
        cmdline = "spawn " + cmdline
        ret = SilentRun( cmdline )
    else:
        ret = rpcf_system( cmdline )

    if ret != 0 :
        return

    if bServer :
        strFile = strPath + "/signkey.pem"
        dlg.keyEdit.set_text( strFile )

        strFile = strPath + "/signcert.pem"
        dlg.certEdit.set_text( strFile )

        strFile = strPath + "/certs.pem"
        dlg.cacertEdit.set_text( strFile )
    else:
        strFile = strPath + "/clientkey.pem"
        dlg.keyEdit.set_text( strFile )

        strFile = strPath + "/clientcert.pem"
        dlg.certEdit.set_text( strFile )

        strFile = strPath + "/certs.pem"
        dlg.cacertEdit.set_text( strFile )

    dlg.secretEdit.set_text( "" )

def CheckIpAddr( strIp ):
    ret = False
    try:
        socket.inet_pton(socket.AF_INET, strIp )
        ret = True
    except:
        try:
            socket.inet_pton(socket.AF_INET6, strIp )
            ret = True
        except:
            pass
    return ret

def GenGmSSLkey( dlg, strPath : str, bServer:bool, cnum : str, snum:str ) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    dir_path += "/gmsslkey.sh"
    cmdline = "bash " + dir_path + " " + strPath + " " + cnum + " " + snum
    rpcf_system( cmdline )
    if bServer :
        strFile = strPath + "/signkey.pem"
        dlg.keyEdit.set_text( strFile )

        strFile = strPath + "/signcert.pem"
        dlg.certEdit.set_text( strFile )

        strFile = strPath + "/certs.pem"
        dlg.cacertEdit.set_text( strFile )
    else:
        strFile = strPath + "/clientkey.pem"
        dlg.keyEdit.set_text( strFile )

        strFile = strPath + "/clientcert.pem"
        dlg.certEdit.set_text( strFile )

        strFile = strPath + "/certs.pem"
        dlg.cacertEdit.set_text( strFile )
    dlg.secretEdit.set_text( "" )

def get_instcfg_content()->str :
    content='''#!/bin/bash
if [ -f debian ]; then
    md5sum *.deb
    dpkg -i --force-depends ./*.deb
    apt-get -y --fix-broken install
elif [ -f fedora ]; then
    md5sum *.rpm
    if which dnf; then 
        dnf -y install ./*.rpm
    elif which yum; then
        yum -y install ./*.rpm
    fi
fi
paths=$(echo $PATH | tr ':' ' ' ) 
for i in $paths; do
    a=$i/rpcf/rpcfgnui.py
    if [ -f $a ]; then
        rpcfgnui=$a
        break
    fi
done

if [ "x$rpcfgnui" == "x" ]; then
    $rpcfgnui="/usr/local/bin/rpcf/rpcfgnui.py"
    if [ ! -f $rpcfgnui ]; then
        exit 1
    fi       
fi
if [ -f USESSL ]; then
    if [ "x$1" == "x" ]; then
        echo "Error key index is not specified"
        echo "Usage: $0 <key index>"
        exit 1
    fi
    if grep -q 'UsingGmSSL":."true"' ./initcfg.json; then
        keydir=$HOME/.rpcf/gmssl
    else     
        keydir=$HOME/.rpcf/openssl
    fi       
    if [ ! -d $keydir ]; then
        mkdir -p $keydir || exit 1
    fi   
    updinitcfg=`dirname $rpcfgnui`/updinitcfg.py
    if [ -f clidx ]; then
        clikeyidx=`cat clidx`
        clikeyidx=`expr $clikeyidx + $1`
    elif [ -f svridx ]; then
        svrkeyidx=`cat svridx`
        svrkeyidx=`expr $svrkeyidx + $1`
    else
        echo Error bad installer
    fi
    if [ ! -f $updinitcfg ]; then exit 1; fi
    if [ "x$clikeyidx" != "x" ]; then
        keyfile=clientkeys-$clikeyidx.tar.gz
        option="-c"
    elif [ "x$svrkeyidx" != "x" ]; then
        keyfile=serverkeys-$svrkeyidx.tar.gz
        option=
    fi       
    if [ ! -f $keyfile ]; then
        echo Error cannot find key file $keyfile
    else
        echo Installing $keyfile
    fi
    tar -C $keydir -xf $keyfile || exit 1
    python3 $updinitcfg $option $keydir ./initcfg.json
    chmod 400 $keydir/*.pem
    cat /dev/null > $keyfile
fi

echo setup rpc-frmwrk configurations...
initcfg=$(pwd)/initcfg.json
python3 $rpcfgnui $initcfg
'''
    return content

def get_instscript_content() :
    content = '''#!/bin/bash
unzipdir=$(mktemp -d /tmp/rpcfinst_XXXXX)
GZFILE=`awk '/^__GZFILE__/ {print NR + 1; exit 0; }' $0`
tail -n+$GZFILE $0 | tar -zxv -C $unzipdir > /dev/null 2>&1
if (($?==0)); then
    echo unzip success;
else
    echo unzip failed;
    exit 1;
fi
pushd $unzipdir > /dev/null; bash ./instcfg.sh $1;popd > /dev/null
if (($?==0)); then
    echo install complete;
else
    echo install failed;
fi
rm -rf $unzipdir
exit 0
__GZFILE__
'''
    return content

class InterfaceContext :
    def __init__(self, ifNo ):
        self.ifNo = ifNo
        self.Clear()

    def Clear( self ) :
        self.ipAddr = None
        self.port = None
        self.compress = None
        self.webSock = None
        self.sslCheck = None
        self.authCheck = None
        self.urlEdit = None
        self.rtpathEdit = None
        self.enabled = None
        self.startRow = 0
        self.rowCount = 0
        self.bindTo = None
    
    def IsEmpty( self ) :
        return self.ipAddr is None

class NodeContext :
    def __init__(self, iNo):
        self.iNo = iNo
        self.Clear()
        
    def Clear( self ):
        self.ipAddr = None
        self.port = None
        self.nodeName = None
        self.enabled = None
        self.compress = None
        self.sslCheck = None
        self.webSock = None
        self.urlEdit = None
        self.startRow = 0
        self.rowCount = 0

    def IsEmpty( self ) :
        return self.ipAddr is None
        
class LBGrpContext :
    def __init__(self, iGrpNo ):
        self.iGrpNo = iGrpNo
        self.Clear()

    def Clear( self ) :
        self.grpName = None
        self.textview = None
        self.textbuffer  = None
        self.removeBtn  = None
        self.changeBtn  = None
        self.labelName = None
        self.grpSet = None
        self.startRow = 0
        self.rowCount = 0
    
    def IsEmpty( self ) :
        return self.labelName is None

import gi
gi.require_version("Gtk", "3.0")

from gi.repository import Gtk, Pango

def GetRouterPath( descFile : str ) :
    strPath = '/'
    while True :
        paths = GetTestPaths()
        pathVal = ReadTestCfg( paths, descFile )
        if pathVal is None :
            ret = -errno.ENOENT
            break

        jsonVal = pathVal[ 1 ]
        try:
            svrObjs = jsonVal[ 'Objects' ]
            if svrObjs is None or len( svrObjs ) == 0 :
                ret = -errno.ENOENT
                break
            for svrObj in svrObjs :
                if 'RouterPath' not in svrObj :
                    continue
                strPath = svrObj['RouterPath']
                break

        except :
            ret = -errno.EFAULT

        break

    return strPath

def SetToString( oSet : set ) :
    strText = ""
    count = len( oSet )
    i = 0
    for x in oSet :
        strText += "'" + x + "'"
        if i + 1 < count :
            strText += ', '
            i += 1
    return strText

def SetBtnMarkup( btn, text ):
    elem = btn.get_child()
    if type( elem ) is Gtk.Label :
        elem.set_markup( text )

def SetBtnText( btn, text ):
    elem = btn.get_child()
    if type( elem ) is Gtk.Label :
        elem.set_text( text )

def GetGridRows(grid: Gtk.Grid):
    rows = 0
    for child in grid.get_children():
        x = grid.child_get_property(child, 'top-attach')
        height = grid.child_get_property(child, 'height')
        rows = max(rows, x+height)
    return rows

def GetGridCols(grid: Gtk.Grid):
    cols = 0
    for child in grid.get_children():
        x = grid.child_get_property(child, 'left-attach')
        width = grid.child_get_property(child, 'width')
        cols = max(cols, x+width)
    return cols

class ConfigDlg(Gtk.Dialog):

    def RetrieveInfo( self, path : str = None ) :
        confVals = dict()
        jsonFiles = LoadConfigFiles( path )
        if len( jsonFiles ) == 0 :
            return None
        drvVal = jsonFiles[ 0 ][1]
        if 'Ports' in drvVal :
            ports = drvVal['Ports']
        else :
            return None
        if 'rpcfgopt' in drvVal:
            confVals[ 'rpcfgopt' ] = drvVal[ 'rpcfgopt' ]

        bGmSSL = False
        if self.hasGmSSL and self.hasOpenSSL :
            bret = IsUsingGmSSL( drvVal )
            if bret[ 0 ] == 0 :
               bGmSSL = bret[ 1 ]
        else:
            bGmSSL = self.hasGmSSL

        if bGmSSL :
            sslPort = 'RpcGmSSLFido'
            confVals[ 'UsingGmSSL' ] = 'true'
        else:
            sslPort = 'RpcOpenSSLFido'
            confVals[ 'UsingGmSSL' ] = 'false'

        bret = IsVerifyPeer( drvVal, sslPort )
        if bret[ 0 ] == 0 and bret[ 1 ] :
            confVals[ 'VerifyPeer' ] = 'true'
        else:
            confVals[ 'VerifyPeer' ] = 'false'

        for port in ports :
            try:
                if port[ 'PortClass'] == sslPort :
                    sslFiles = port[ 'Parameters']
                    if sslFiles is None :
                        continue

                    if self.bServer :
                        confVals["KeyFile"] = sslFiles[ "KeyFile"]
                        confVals["CertFile"] = sslFiles[ "CertFile"]

                        if 'CACertFile' in sslFiles :
                            confVals['CACertFile'] = sslFiles[ 'CACertFile']
                        else:
                            confVals['CACertFile'] = ''

                        if 'SecretFile' in sslFiles :
                            confVals['SecretFile'] = sslFiles[ 'SecretFile']
                        else:
                            confVals['SecretFile'] = ''
                    else: 
                        keyPath = sslFiles[ "KeyFile" ]
                        dirPath = os.path.dirname( keyPath )
                        keyName = os.path.basename( keyPath )
                        if keyName == "signkey.pem" :
                            '''generated by rpcfg'''
                            confVals[ "KeyFile"] = dirPath + "/clientkey.pem"
                            confVals[ "CertFile"] = dirPath + "/clientcert.pem"
                            confVals[ "CACertFile"] = dirPath + "/certs.pem"
                            confVals[ "SecretFile"] = ""
                        else:
                            confVals["KeyFile"] = sslFiles[ "KeyFile"]
                            confVals["CertFile"] = sslFiles[ "CertFile"]
                            confVals["CACertFile"] = sslFiles[ "CACertFile"]
                            if 'SecretFile' in sslFiles :
                                confVals['SecretFile'] = sslFiles[ 'SecretFile']
                            else:
                                confVals['SecretFile'] = ''
                
                if port[ 'PortClass'] == 'RpcTcpBusPort' :
                    connParams = port[ 'Parameters']
                    if self.bServer :
                        if connParams is None :
                            ifCfg = DefaultIfCfg()
                            confVals[ "connParams"] = [ ifCfg, ]
                        else :
                            confVals[ "connParams"] = [ *connParams ]
                        maxConns = port[ "MaxConnections" ]
                        if maxConns is None :
                            maxConns = "512"
                        confVals[ "MaxConnections" ] = maxConns
            except Exception as err :
                pass

        svrVals = jsonFiles[ 2 ][ 1 ]
        svrObjs = svrVals[ 'Objects']
        if svrObjs is None or len( svrObjs ) == 0 :
            return confVals

        for svrObj in svrObjs :
            try:
                objName = svrObj[ 'ObjectName' ]
                if objName == 'RpcRouterBridgeAuthImpl' :
                    if 'AuthInfo' in svrObj and self.bServer:
                        confVals[ 'AuthInfo'] = svrObj[ 'AuthInfo']
                    else :
                        confVals[ 'AuthInfo' ] = None
                    if 'Nodes' in svrObj :
                        confVals[ 'Nodes'] = svrObj[ 'Nodes']
                    else:
                        confVals[ 'Nodes' ] = []
                    if 'LBGroup' in svrObj : 
                        confVals[ 'LBGroup' ] = svrObj[ 'LBGroup' ]
                    else :
                        confVals[ 'LBGroup' ] = []
                    break

                elif objName == 'RpcReqForwarderAuthImpl' :
                    if 'TaskScheduler' in svrObj :
                        confVals[ 'TaskScheduler' ] = svrObj[ 'TaskScheduler' ]

            except Exception as err :
                pass
        
        apVal = jsonFiles[ 3 ][ 1 ]
        proxies = apVal['Objects']
        if proxies is None :
            return confVals

        for proxy in proxies :
            try:
                if proxy[ 'ObjectName'] == 'KdcRelayServer' :
                    confVals['AuthInfo'][ 'kdcAddr'] = proxy[ 'IpAddress']
                    break
            except Exception as err :
                pass

        # oa2checkdesc.json
        oaVal = jsonFiles[ 4 ][ 1 ]
        proxies = oaVal['Objects']
        if proxies is None :
            return confVals

        for proxy in proxies :
            try:
                if proxy[ 'ObjectName'] == 'OA2proxy' :
                    confVals['AuthInfo'][ 'OA2ChkIp'] = proxy[ 'IpAddress']
                    confVals['AuthInfo'][ 'OA2ChkPort'] = proxy[ 'PortNumber']
                    confVals['AuthInfo'][ 'OA2SSL'] = proxy[ 'EnableSSL']
                    break
            except Exception as err :
                pass

        self.jsonFiles = jsonFiles

        authUser = ""
        # echodesc.json
        pathVal = jsonFiles[ 5 ]
        if pathVal is not None :
            jsonVal = pathVal[ 1 ]
            try:
                svrObjs = jsonVal[ 'Objects' ]
                authInfo = None
                for svrObj in svrObjs :
                    if self.bServer:
                        authInfo = svrObj['AuthInfo']
                        authUser = authInfo[ 'UserName' ]
                        break

                    portClass = svrObj[ "ProxyPortClass"]
                    if  ( portClass == "DBusProxyPdo" ) or (
                        portClass == "DBusProxyPdoLpbk" ) or ( 
                        portClass == "DBusProxyPdoLpbk2" ) :

                        if 'AuthInfo' in svrObj :
                            authInfo = svrObj['AuthInfo']
                            authUser = authInfo[ 'UserName' ]
                            confVals[ 'AuthInfo' ] = authInfo

                        ifCfg0 = DefaultIfCfg()
                        ifCfg0[ 'BindAddr' ] = svrObj[ 'IpAddress' ]
                        ifCfg0[ 'PortNumber' ] = svrObj[ 'PortNumber' ]
                        ifCfg0[ 'Compression' ] = svrObj[ 'Compression' ]
                        ifCfg0[ 'EnableWS' ] = svrObj[ 'EnableWS' ]
                        ifCfg0[ 'EnableSSL' ] = svrObj[ 'EnableSSL' ]

                        if ifCfg0[ 'EnableWS' ] == 'true':
                            ifCfg0[ 'DestURL' ] = svrObj[ 'DestURL' ]

                        if authInfo is not None and self.hasAuth:
                            ifCfg0[ 'HasAuth' ] = 'true'
                        else:
                            ifCfg0[ 'HasAuth' ] = 'false'
                        ifCfg0[ 'RouterPath' ] = svrObj[ 'RouterPath' ]
                        confVals[ 'connParams' ] = [ ifCfg0, ]
                        break

            except Exception as err :
                pass

        if len( authUser ) > 0 and 'AuthInfo' in confVals :
            authInfo = confVals[ 'AuthInfo' ]
            authInfo[ 'UserName' ] = authUser

        return confVals    

    def ReinitDialog( self, path : str ) :
        confVals = self.RetrieveInfo( path )
        self.confVals = confVals
        self.ifctx = []

        self.keyEdit = None
        self.certEdit = None
        self.cacertEdit = None
        self.secretEdit = None
        self.gmsslCheck = None
        self.vfyPeerCheck = None

        self.svcEdit = None
        self.realmEdit = None
        self.userEdit = None
        self.signCombo = None
        self.kdcEdit = None

        try:
            ifCount = len( confVals[ 'connParams'] )
            for i in range( ifCount ) :
                self.ifctx.append( InterfaceContext(i) )
        except Exception as err :
            pass

        gridNet = self.gridNet
        rows = GetGridRows( gridNet )
        for i in range( rows ) :
            gridNet.remove_row( 0 )

        #uncomment the following line for multi-if
        #for interf in self.ifctx :
        for i in range( len( self.ifctx ) ) :
            interf = self.ifctx[ i ]
            row = GetGridRows( gridNet )
            interf.startRow = row
            self.InitNetworkPage( gridNet, 0, row, confVals, i )
            interf.rowCount = GetGridRows( gridNet ) - row
            i += 1

        gridSec = self.gridSec
        rows = GetGridRows( gridSec )
        for i in range( rows ) :
            gridSec.remove_row( 0 )
        self.InitSecurityPage( gridSec, 0, 0, confVals )

        if self.bServer :
            gridmh = self.gridmh
            rows = GetGridRows( gridmh )
            for i in range( rows ) :
                gridmh.remove_row( 0 )
            self.InitMultihopPage( gridmh, 0, 0, confVals )

            gridlb = self.gridlb
            rows = GetGridRows( gridlb )
            for i in range( rows ) :
                gridlb.remove_row( 0 )
            self.InitLBGrpPage( gridlb, 0, 0, confVals )

        self.show_all()

    def __init__(self, bServer):
        self.strCfgPath = ""
        if bServer :
            title = "Config the RPC Router"
        else:
            title = "Config the RPC Proxy Router"

        Gtk.Dialog.__init__(self, title, flags=0)
        self.add_buttons(
            "Load From", Gtk.ResponseType.APPLY,
            "Installer...", Gtk.ResponseType.YES,
            Gtk.STOCK_OK, Gtk.ResponseType.OK,
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL )

        oInstBtn = self.get_widget_for_response(
            Gtk.ResponseType.YES )
        oInstBtn.set_tooltip_text(
            "Create configuration installers for deployment.")
        oInstBtn.set_sensitive( bServer )
        oLoadBtn = self.get_widget_for_response(
            Gtk.ResponseType.APPLY )
        oLoadBtn.set_tooltip_text(
            "Reload settings from another driver.json.")

        self.bServer = bServer
        self.hasGmSSL = IsFeatureEnabled( "gmssl" )
        self.hasOpenSSL = IsFeatureEnabled( "openssl" )
        self.hasSSL = ( self.hasGmSSL or self.hasOpenSSL ) 
        self.hasAuth = ( IsFeatureEnabled( "krb5" ) or
            IsFeatureEnabled( "js" ) )

        confVals = self.RetrieveInfo()
        self.confVals = confVals
        self.ifctx = []
        self.curTab = "GridConn"
        try:
            ifCount = len( confVals[ 'connParams'] )
            for i in range( ifCount ) :
                self.ifctx.append( InterfaceContext(i) )
        except Exception as err :
            pass

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)

        stack = Gtk.Stack()
        stack.set_transition_type(Gtk.StackTransitionType.NONE)
        stack.set_transition_duration(1000)

        gridNet = Gtk.Grid()
        #gridNet.set_row_homogeneous( True )
        gridNet.props.row_spacing = 6
        i = 0

        #uncomment the following line for multi-if
        for i in range( len( self.ifctx ) ) :
            interf = self.ifctx[ i ]
            row = GetGridRows( gridNet )
            interf.startRow = row
            self.InitNetworkPage( gridNet, 0, row, confVals, i )
            interf.rowCount = GetGridRows( gridNet ) - row
            i += 1

        self.gridNet = gridNet
        stack.add_titled(gridNet, "GridConn", "Connection")

        gridSec = Gtk.Grid()
        #gridSec.set_row_homogeneous( True )
        gridSec.props.row_spacing = 6        
        stack.add_titled(gridSec, "GridSec", "Security")
        self.gridSec = gridSec
        self.InitSecurityPage( gridSec, 0, 0, confVals )

        if self.bServer :
            gridmh = Gtk.Grid()
            gridmh.props.row_spacing = 6        
            stack.add_titled(gridmh, "GridMh", "Multihop")
            self.InitMultihopPage( gridmh, 0, 0, confVals )
            self.gridmh = gridmh
            stack.connect("notify::visible-child", vc_changed)

            gridlb = Gtk.Grid()
            gridlb.set_column_homogeneous( True )
            gridlb.props.row_spacing = 6        
            stack.add_titled(gridlb, "GridLB", "Load Balance")
            self.InitLBGrpPage( gridlb, 0, 0, confVals )
            self.gridlb = gridlb

        stack_switcher = Gtk.StackSwitcher()
        stack_switcher.set_stack(stack)
        vbox.pack_start(stack_switcher, False, True, 0)

        scrolledWin = Gtk.ScrolledWindow(hexpand=True, vexpand=True)
        scrolledWin.set_border_width(10)
        scrolledWin.set_policy(
            Gtk.PolicyType.NEVER, Gtk.PolicyType.ALWAYS)
        scrolledWin.add( stack )
        vbox.pack_start(scrolledWin, True, True, 0)
        self.scrollWin = scrolledWin

        self.set_border_width(10)
        self.set_default_size(400, 460)
        box = self.get_content_area()
        box.add(vbox)
        self.show_all()
 
    def InitSecurityPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        row = GetGridRows( grid )
        self.AddSSLCfg( grid, startCol, row, confVals )
        row = GetGridRows( grid )
        self.AddAuthCred( grid, startCol, row, confVals )
        row = GetGridRows( grid )
        self.AddMiscOptions( grid, startCol, row, confVals )
        row = GetGridRows( grid )
        self.AddInstallerOptions( grid, startCol, row, confVals)
        self.ToggleAuthControls(
            self.IsAuthChecked(), self.IsKrb5Enabled() )


    def AddNode( self, grid:Gtk.Grid, i ) :
        try:
            startCol = 0
            startRow = GetGridRows( grid )

            nodeCtx = NodeContext( i )
            nodeCtx.startRow = startRow

            labelNno = Gtk.Label()
            strNno = "<b>Node " + str( i ) + "</b>"
            labelNno.set_markup( strNno )
            labelNno.set_xalign( 0.5 )
            grid.attach(labelNno, startCol + 1, startRow + 0, 1, 1 )
            nodeCfg = self.nodes[ i ]

            labelName = Gtk.Label()
            labelName.set_text( "Node Name: " )
            labelName.set_xalign( 1 )
            grid.attach(labelName, startCol, startRow + 1, 1, 1 )

            nameEntry = Gtk.Entry()
            nameEntry.set_text( nodeCfg[ 'NodeName' ] )
            grid.attach(nameEntry, startCol + 1, startRow + 1, 1, 1 )
            nameEntry.iNo = i
            nodeCtx.nodeName = nameEntry

            labelIpAddr = Gtk.Label()
            labelIpAddr.set_text( "Remote IP Address: " )
            labelIpAddr.set_xalign( 1 )
            grid.attach(labelIpAddr, startCol, startRow + 2, 1, 1 )

            ipAddr = Gtk.Entry()
            ipAddr.set_text( nodeCfg[ 'IpAddress' ] )
            ipAddr.set_tooltip_text( "Enter an ip address or a domain name." )
            grid.attach(ipAddr, startCol + 1, startRow + 2, 2, 1 )
            ipAddr.iNo = i
            nodeCtx.ipAddr = ipAddr

            labelPort = Gtk.Label()
            labelPort.set_text( "Port Number: " )
            labelPort.set_xalign( 1 )
            grid.attach(labelPort, startCol, startRow + 3, 1, 1 )

            portNum = Gtk.Entry()
            portNum.set_text( nodeCfg[ 'PortNumber' ] )
            grid.attach(portNum, startCol + 1, startRow + 3, 2, 1 )
            portNum.iNo = i
            nodeCtx.port = portNum

            labelWebSock = Gtk.Label()
            labelWebSock.set_text("WebSocket: ")
            labelWebSock.set_xalign(1)
            grid.attach(labelWebSock, startCol + 0, startRow + 4, 1, 1 )

            webSockCheck = Gtk.CheckButton(label="")
            webSockCheck.iNo = i
            if nodeCfg[ 'EnableWS' ] == 'false' :
                webSockCheck.props.active = False
            else:
                webSockCheck.props.active = True
            webSockCheck.connect(
                "toggled", self.on_button_toggled, "WebSock2")
            grid.attach( webSockCheck, startCol + 1, startRow + 4, 1, 1)
            nodeCtx.webSock = webSockCheck

            labelUrl = Gtk.Label()
            labelUrl.set_text("WebSocket URL: ")
            labelUrl.set_xalign(1)
            grid.attach(labelUrl, startCol + 0, startRow + 5, 1, 1 )

            strUrl = ""
            if 'DestURL' in nodeCfg:
                strUrl = nodeCfg[ 'DestURL']
            urlEdit = Gtk.Entry()
            urlEdit.set_text( strUrl )
            urlEdit.set_sensitive( webSockCheck.props.active )
            grid.attach(urlEdit, startCol + 1, startRow + 5, 2, 1 )
            nodeCtx.urlEdit = urlEdit
            urlEdit.iNo = i

            enabledCheck = Gtk.CheckButton( label = "Node Enabled" )
            if nodeCfg[ 'Enabled' ] == 'false' :
                enabledCheck.props.active = False
            else :
                enabledCheck.props.active = True
            grid.attach( enabledCheck, startCol + 0, startRow + 6, 1, 1 )
            nodeCtx.enabled = enabledCheck
            enabledCheck.iNo = i

            compress = Gtk.CheckButton( label = "Compression" )
            if nodeCfg[ 'Compression' ] == 'false' :
                compress.props.active = False
            else :
                compress.props.active = True
            grid.attach( compress, startCol + 1, startRow + 6, 1, 1 )
            compress.iNo = i
            nodeCtx.compress = compress

            sslCheck = Gtk.CheckButton( label = "SSL" )
            sslCheck.set_tooltip_text( "Enable OpenSSL or GMSSL" )
            if nodeCfg[ 'EnableSSL' ] == 'false' :
                sslCheck.props.active = False
            else :
                sslCheck.props.active = True
            sslCheck.connect(
                "toggled", self.on_button_toggled, "SSL2")
            grid.attach( sslCheck, startCol + 2, startRow + 6, 1, 1 )
            sslCheck.iNo = i
            nodeCtx.sslCheck = sslCheck

            if not self.hasSSL:
                sslCheck.props.active = False
                sslCheck.set_sensitive( False )
                webSockCheck.props.active = False
                webSockCheck.set_sensitive( False )
                urlEdit.set_sensitive( False )

            removeBtn = Gtk.Button.new_with_label("Remove Node " + str( i ))
            removeBtn.connect("clicked", self.on_remove_node_clicked)
            grid.attach(removeBtn, startCol + 1, startRow + 7, 1, 1 )
            removeBtn.iNo = i

            nodeCtx.rowCount = GetGridRows( grid ) - startRow
            self.nodeCtxs.append( nodeCtx )

        except Exception as err:
            text = "Failed to add node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )

    def InitNodeCfg( self, iNo ) :
        nodeCfg = dict()
        nodeCfg[ "NodeName" ]= "foo" + str(iNo)
        nodeCfg[ "Enabled" ]="true"
        nodeCfg[ "Protocol" ]= "native"
        nodeCfg[ "AddrFormat" ]= "ipv4"
        nodeCfg[ "IpAddress"]= "192.168.0." + str( iNo % 256 )
        nodeCfg[ "PortNumber" ]= "4132"
        nodeCfg[ "Compression" ]= "true"
        nodeCfg[ "EnableWS" ]= "false"
        nodeCfg[ "EnableSSL" ]= "false"
        nodeCfg[ "ConnRecover" ]= "false"
        return nodeCfg

    def InitLBGrpCfg( self, iNo ) :
        grpCfg = dict()
        grpCfg[ "Group" + str(iNo) ] = []
        return grpCfg

    def on_add_node_clicked( self, button ):
        try:
            iNo = len( self.nodeCtxs )
            startRow = GetGridRows( self.gridmh )
            startRow -= 1
            self.gridmh.remove_row( startRow )

            nodeCfg = self.InitNodeCfg( iNo )

            self.nodes.append( nodeCfg )
            self.AddNode( self.gridmh, iNo )
            rows = GetGridRows( self.gridmh )
            addBtn = Gtk.Button.new_with_label("Add Node")
            addBtn.connect("clicked", self.on_add_node_clicked)
            self.gridmh.attach(addBtn, 1, rows, 1, 1 )

            self.gridmh.show_all()
        except Exception as err:
            text = "Failed to add node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )

    def on_remove_node_clicked( self, button ):
        try:
            iNo = button.iNo
            nodeCtx = self.nodeCtxs[ iNo ]
            grid = self.gridmh
            for i in range( nodeCtx.rowCount ) :
                grid.remove_row( nodeCtx.startRow )
            for elem in self.nodeCtxs :
                if elem.iNo > iNo and not elem.IsEmpty():
                    elem.startRow -= nodeCtx.rowCount
            nodeCtx.Clear()

        except Exception as err:
            text = "Failed to remove node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )

    def CreateTextview(self, grid, startCol, startRow, text, iGrpNo):
        scrolledwindow = Gtk.ScrolledWindow()
        scrolledwindow.set_hexpand(True)
        scrolledwindow.set_vexpand(False)
        grid.attach(scrolledwindow, startCol + 0, startRow, 2, 2 )

        grpCtx = self.grpCtxs[ iGrpNo ]
        grpCtx.textview = Gtk.TextView()
        grpCtx.textbuffer = grpCtx.textview.get_buffer()
        grpCtx.textbuffer.set_text( text )
        scrolledwindow.add(grpCtx.textview)
        grpCtx.textview.set_sensitive( False )

    def on_add_lbgrp_clicked( self, button ) :
        try:
            if self.grpCtxs is None :
                iNo = 0
                self.grpCtxs = []
            else :
                iNo = len( self.grpCtxs )
            startRow = GetGridRows( self.gridlb )
            startRow -= 1
            self.gridlb.remove_row( startRow )

            grpCtx = LBGrpContext( iNo )
            self.grpCtxs.append( grpCtx )
            grpCfg = self.InitLBGrpCfg( iNo )
            grpCtx.startRow = startRow
            self.AddLBGrp( self.gridlb, grpCfg, iNo )
            rows = GetGridRows( self.gridlb )
            grpCtx.rowCount = rows - startRow
            addBtn = Gtk.Button.new_with_label("Add Group")
            addBtn.connect("clicked", self.on_add_lbgrp_clicked)
            self.gridlb.attach(addBtn, 1, rows, 1, 1 )

            self.gridlb.show_all()
        except Exception as err:
            text = "Failed to add node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )

    def on_remove_lbgrp_clicked( self, button ) :
        try:
            iNo = button.iGrpNo
            grpCtx = self.grpCtxs[ iNo ]
            grid = self.gridlb
            for i in range( grpCtx.rowCount ) :
                grid.remove_row( grpCtx.startRow )
            for elem in self.grpCtxs :
                if elem.iGrpNo > iNo and not elem.IsEmpty():
                    elem.startRow -= grpCtx.rowCount
            self.nodeSet.update( grpCtx.grpSet )
            grpCtx.Clear()
            return

        except Exception as err:
            text = "Failed to remove node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )


    def on_change_lbgrp_clicked( self, button ) :
        iNo = button.iGrpNo
        dialog = LBGrpEditDlg( parent=self, iGrpNo = iNo )
        response = dialog.run()
        if response == Gtk.ResponseType.CANCEL:
            dialog.destroy() 
            return
        grpCtx = self.grpCtxs[ iNo ]
        strText = SetToString( grpCtx.grpSet )
        grpCtx.textbuffer.set_text( strText )
        grpCtx.grpName = dialog.nameEntry.get_text().strip()
        SetBtnText( grpCtx.removeBtn, "Remove '" + grpCtx.grpName + "'" )
        SetBtnText( grpCtx.changeBtn, "Change '" + grpCtx.grpName + "'" )
        grpCtx.labelName.set_markup( "<b>group '" + grpCtx.grpName + "'</b>" )

        dialog.destroy()

    def AddLBGrp( self, grid : Gtk.Grid, grp:dict, iGrpNo ) :
        try:
            startRow = GetGridRows( grid )
            startCol = 0
            nodeSet = self.nodeSet
            for grpName in grp.keys() :
                labelName = Gtk.Label()
                labelName.set_markup("<b>group '" + grpName + "'</b> ")
                labelName.set_xalign(1)
                grid.attach(labelName, startCol + 1, startRow + 0, 1, 1 )

                grpSet = set()
                members = grp[ grpName ]
                for nodeName in members :
                    if nodeName not in nodeSet :
                        continue
                    grpSet.add( nodeName )
                grpCtx = self.grpCtxs[ iGrpNo ]
                grpCtx.grpSet = grpSet
                grpCtx.labelName = labelName
                grpCtx.grpName = grpName

                strText = SetToString( grpSet )

                self.CreateTextview( grid,
                    startCol, startRow + 1, strText, iGrpNo )

                rows = GetGridRows( grid )
                strLabel = "Remove '" + grpName + "'"
                removeBtn = Gtk.Button.new_with_label( strLabel )
                #SetBtnMarkup( removeBtn, "<b>" + strLabel + "</b>" )
                    
                removeBtn.iGrpNo = iGrpNo
                removeBtn.connect("clicked", self.on_remove_lbgrp_clicked)
                grid.attach(removeBtn, startCol + 0, rows, 1, 1 )
                grpCtx.removeBtn = removeBtn

                strLabel = "Change '" + grpName + "'"
                changeBtn = Gtk.Button.new_with_label(
                    "Change '" + grpName + "'" )
                changeBtn.iGrpNo = iGrpNo
                changeBtn.connect("clicked", self.on_change_lbgrp_clicked)
                grid.attach(changeBtn, startCol + 2, rows, 1, 1 )
                grpCtx.changeBtn = changeBtn
                #SetBtnMarkup( changeBtn, "<b>" + strLabel + "</b>" )


        except Exception as err :
            text = "Failed to remove node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )

    def BuildLBGrpCfg( self ):
        try:
            grpCfgs = []
            for grpCtx in self.grpCtxs:
                grpSet = grpCtx.grpSet
                if len( grpSet ) == 0 :
                    continue
                members = [ *grpSet ]
                cfg = dict()
                cfg[ grpCtx.grpName ] = members
                grpCfgs.append( cfg )

            return grpCfgs
            
        except Exception as err :
            text = "Failed to remove node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return None

    def UpdateLBGrpPage( self ) :
        try:
            ret = self.UpdateNodeNames()
            if ret < 0 :
                return ret
            nodeSet = deepcopy( self.nodeSet )
            for grpCtx in self.grpCtxs:
                grpSet = grpCtx.grpSet
                grpSet.intersection_update( nodeSet )
                nodeSet.difference_update( grpSet )
                strText = SetToString( grpSet )
                grpCtx.textbuffer.set_text( strText )
            return 0
        except Exception as err :
            return -2
        
    def InitLBGrpPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        try:
            bNodes = False
            grps = confVals[ 'LBGroup' ]
            grpCount = len( grps )
            for grp in grps :
                for nameGrp in grp.keys() :
                    if nameGrp in self.nodeSet :
                        strMsg = "'" + nameGrp 
                        strMsg += "' duplicated with a node member's name"
                        raise Exception( strMsg ) 

            if grpCount == 0 :
                self.grpCtxs = None
            else :
                self.grpCtxs = []
                for i in range( grpCount ) :
                    self.grpCtxs.append( LBGrpContext( i ))

            for i in range( grpCount ) :
                grpCtx = self.grpCtxs[ i ]
                startRow = GetGridRows( grid )
                grpCtx.startRow = startRow
                self.AddLBGrp( grid, grps[ i ], i )
                grpCtx.rowCount = GetGridRows( grid ) - startRow

        except Exception as err :
            text = "Failed to remove node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )

        rows = GetGridRows( grid )
        strLabel ="Add Group"
        addBtn = Gtk.Button.new_with_label( strLabel )
        addBtn.connect("clicked", self.on_add_lbgrp_clicked)
        #SetBtnMarkup( addBtn, "<b>" + strLabel + "</b>" )
        grid.attach(addBtn, startCol + 1, rows, 1, 1 )
        return

    def UpdateNodeNames( self ):
        try:
            self.nodeSet.clear()
            for nodeCtx in self.nodeCtxs :
                if nodeCtx.IsEmpty() :
                    continue
                strName = nodeCtx.nodeName.get_text().strip()
                if len( strName ) == 0 :
                    continue
                if not strName.isidentifier() :
                    return -1
                self.nodeSet.add( strName )
            return 0
        except Exception as err :
            text = "Failed to remove node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -1

    def InitMultihopPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        try:
            nodes = confVals[ 'Nodes' ]
            nodeCount = len( nodes )

            self.nodeCtxs = []

            nodeSet = set()
            self.nodeSet = nodeSet
            self.nodes = nodes

            if nodeCount == 0 :
                rows = GetGridRows( grid )
                addBtn = Gtk.Button.new_with_label("Add Node")
                addBtn.connect("clicked", self.on_add_node_clicked)
                grid.attach(addBtn, startCol + 1, rows, 1, 1 )
                return

            for nodeCfg in nodes :
                nodeSet.add( nodeCfg[ 'NodeName' ] )
            
            for i in range( nodeCount ) :
                self.AddNode( grid, i )

            rows = GetGridRows( grid )
            addBtn = Gtk.Button.new_with_label("Add Node")
            addBtn.connect("clicked", self.on_add_node_clicked)
            grid.attach(addBtn, startCol + 1, rows, 1, 1 )

        except Exception as err :
            pass

        return

    def InitNetworkPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        row = GetGridRows( grid )
        if self.bServer :
            labelIfNo = Gtk.Label()
            strIfNo = "<b>Interface " + str( ifNo ) + " :</b>"
            labelIfNo.set_markup( strIfNo )
            grid.attach(labelIfNo, startCol + 1, startRow, 1, 1 )

                #labelNote = Gtk.Label()
                #labelNote.set_markup('<i><small>red options are mandatory</small></i>')
                #grid.attach(labelNote, startCol + 1, startRow, 1, 1 )

            startRow = GetGridRows( grid )
        
        labelIp = Gtk.Label()

        strText = '<span foreground="red">Server Addr: </span>'
        labelIp.set_markup(strText)

        labelIp.set_xalign(.5)
        grid.attach(labelIp, startCol, startRow, 1, 1 )

        ipEditBox = Gtk.Entry()
        ipAddr = ''
        try:
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0 is not None :
                    ipAddr = param0[ 'BindAddr']
        except Exception as err :
            pass
        ipEditBox.set_text(ipAddr)
        ipEditBox.set_tooltip_text( "Enter an IP address or a domain name " +
            "the client will connect to." )
        grid.attach(ipEditBox, startCol + 1, startRow + 0, 2, 1 )
        self.ifctx[ ifNo ].ipAddr = ipEditBox

        if self.bServer:
            checkBindTo = Gtk.CheckButton(label="Bind to")
            grid.attach( checkBindTo, startCol + 3, startRow + 0, 1, 1)
            self.ifctx[ ifNo ].bindTo = checkBindTo
            toolTip = "Uncheck this box will force server to listen to 0.0.0.0 instead of this address. "
            checkBindTo.set_tooltip_text( toolTip )
            checkBindTo.connect(
                "toggled", self.on_button_toggled, "bindTo")
            checkBindTo.iNo = ifNo
            checkBindTo.props.active = True
            if ipAddr == '0.0.0.0' or ipAddr == '::/0' or (
                ipAddr == '0000:0000:0000:0000:0000:0000:0000:0000/0' ) :
                checkBindTo.props.active = False
                if ifNo == 0 :
                    jsonVal = self.jsonFiles[ 5 ][ 1 ]
                elif ifNo == 1:
                    jsonVal = self.jsonFiles[ 6 ][ 1 ]
                elif ifNo == 2:
                    jsonVal = self.jsonFiles[ 7 ][ 1 ]
                try:
                    svrObj = jsonVal[ 'Objects' ][ 0 ]
                    ipAddr = svrObj[ 'IpAddress' ]
                    ipEditBox.set_text( ipAddr )
                except Exception as err:
                    print( err )

        labelPort = Gtk.Label()
        if self.bServer :
            strText = '<span foreground="red">Service Port: </span>'
            labelPort.set_markup( strText )
        else:
            labelPort.set_text("Service Port: ")

        labelPort.set_xalign(1)
        grid.attach(labelPort, startCol + 0, startRow + 1, 1, 1 )

        portNum = 4132
        try:
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0 is not None :
                    portNum = param0[ 'PortNumber']
        except Exception as err :
            pass                
        portEditBox = Gtk.Entry()
        portEditBox.set_text( str( portNum ) )
        grid.attach( portEditBox, startCol + 1, startRow + 1, 2, 1)
        self.ifctx[ ifNo ].port = portEditBox

        rows = GetGridRows( grid )
        self.AddWebSockOptions( grid, startCol + 0, rows, confVals, ifNo)

        rows = GetGridRows( grid )
        self.AddSSLOptions( grid, startCol + 0, rows + 0, confVals, ifNo )

        bActive = True
        try:
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0 is not None :
                    if param0[ "Compression"] == 'false':
                        bActive = False
        except Exception as err :
            pass                

        compressCheck = Gtk.CheckButton(label="Compression")
        compressCheck.props.active = bActive
        compressCheck.connect(
            "toggled", self.on_button_toggled, "Compress")
        compressCheck.ifNo = ifNo
        grid.attach( compressCheck, startCol + 1, rows, 1, 1)
        self.ifctx[ ifNo ].compress = compressCheck

        rows = GetGridRows( grid )
        labelPath = Gtk.Label()
        strText = 'RouterPath: '
        labelPath.set_text( strText )

        labelPath.set_xalign(1)
        grid.attach(labelPath, startCol + 0, rows + 0, 1, 1 )

        strPath = GetRouterPath( "echodesc.json" )
        pathEditBox = Gtk.Entry()
        pathEditBox.set_text( str( strPath ) )
        grid.attach( pathEditBox, startCol + 1, rows + 0, 2, 1)
        self.ifctx[ 0 ].rtpathEdit = pathEditBox

        if ifNo == 0 :
            rows = GetGridRows( grid )
            addBtn = Gtk.Button.new_with_label("Add New Interface")
            addBtn.connect("clicked", self.on_add_if_clicked)
            addBtn.set_tooltip_text(
                "Rpcrouter only, and not work for built-in router apps" )
            grid.attach(addBtn, startCol + 1, rows + 1, 1, 1 )

        else :
            rows = GetGridRows( grid )
            removeBtn = Gtk.Button.new_with_label(
                "Remove interface " + str(ifNo) )
            removeBtn.ifNo = ifNo
            removeBtn.connect("clicked", self.on_remove_if_clicked)
            grid.attach(removeBtn, startCol + 1, rows + 1, 1, 1 )

    def AddWebSockOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        labelWebSock = Gtk.Label()
        labelWebSock.set_text("WebSocket: ")
        labelWebSock.set_xalign(1)
        grid.attach(labelWebSock, startCol + 0, startRow + 0, 1, 1 )

        bActive = False
        try :
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0[ 'EnableWS'] is not None :
                    strVal = param0[ 'EnableWS']
                    if strVal == 'true' :
                        bActive = True
        except Exception as err :
            pass

        webSockCheck = Gtk.CheckButton(label="")
        webSockCheck.ifNo = ifNo
        webSockCheck.props.active = bActive
        webSockCheck.connect(
            "toggled", self.on_button_toggled, "WebSock")
        grid.attach( webSockCheck, startCol + 1, startRow + 0, 1, 1)
        self.ifctx[ ifNo ].webSock = webSockCheck

        labelUrl = Gtk.Label()
        labelUrl.set_text("WebSocket URL: ")
        labelUrl.set_xalign(1)
        grid.attach(labelUrl, startCol + 0, startRow + 1, 1, 1 )

        strUrl = "https://example.com/chat"
        try :
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0[ 'DestURL'] is not None :
                    strUrl = param0[ 'DestURL']
        except Exception as err :
            pass
        urlEdit = Gtk.Entry()
        urlEdit.set_text( strUrl )
        urlEdit.set_sensitive( bActive )
        grid.attach(urlEdit, startCol + 1, startRow + 1, 2, 1 )
        self.ifctx[ ifNo ].urlEdit = urlEdit
        urlEdit.ifNo = ifNo

        if not self.hasSSL:
            webSockCheck.props.active = False
            webSockCheck.set_sensitive( False )
            urlEdit.set_sensitive( False )

    def AddSSLOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        bActive = False
        try:
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0 is not None :
                    strSSL = param0[ 'EnableSSL']
                    if strSSL == 'true' :
                        bActive = True
        except Exception as err :
            pass

        if not bActive :
            bActive = self.ifctx[ ifNo ].webSock.props.active

        sslCheck = Gtk.CheckButton(label="SSL")
        sslCheck.set_active( bActive )
        sslCheck.connect(
            "toggled", self.on_button_toggled, "SSL")
        sslCheck.ifNo = ifNo
        grid.attach( sslCheck, startCol + 2, startRow, 1, 1 )
        self.ifctx[ ifNo ].sslCheck = sslCheck

        if not self.hasSSL:
            sslCheck.set_active( False )
            sslCheck.set_sensitive( False )

        sslCheck.set_tooltip_text( "Enable OpenSSL or GMSSL" )
        bActive = False
        try:
            if not self.bServer: 
                authInfo = confVals[ 'AuthInfo' ]
                if 'UserName' in authInfo :
                    bActive = True
            else:
                param0 = confVals[ 'connParams'][ ifNo ]
                strAuth = param0[ 'HasAuth']
                if strAuth == 'true' :
                    bActive = True
        except Exception as err :
            pass

        authCheck = Gtk.CheckButton(label="Auth")
        authCheck.set_active( bActive )
        authCheck.connect(
            "toggled", self.on_button_toggled, "Auth")
        authCheck.ifNo = ifNo
        grid.attach( authCheck, startCol + 0, startRow, 1, 1 )
        self.ifctx[ ifNo ].authCheck = authCheck

        if not self.hasAuth :
            authCheck.set_active( False )
            authCheck.set_sensitive( False )

    def AddSSLCfg( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        labelSSLfiles = Gtk.Label()
        labelSSLfiles.set_markup("<b>SSL Files</b> ")
        labelSSLfiles.set_xalign(.5)
        grid.attach(labelSSLfiles, startCol + 1, startRow, 1, 1 )
        startRow+=1

        #-----------------------------
        labelKey = Gtk.Label()
        labelKey.set_text("Key File: ")
        labelKey.set_xalign(.5)
        grid.attach(labelKey, startCol + 0, startRow, 1, 1 )

        keyEditBox = Gtk.Entry()
        strKey = confVals.get( 'KeyFile' )
        if strKey is None:
            strKey = ''

        keyEditBox.set_text(strKey)
        grid.attach(keyEditBox, startCol + 1, startRow, 1, 1 )

        keyBtn = Gtk.Button.new_with_label("...")
        keyBtn.connect("clicked", self.on_choose_key_clicked)

        grid.attach(keyBtn, startCol + 2, startRow, 1, 1 )

        #-----------------------------
        labelCert = Gtk.Label()
        labelCert.set_text("Cert File: ")
        labelCert.set_xalign(.5)
        grid.attach(labelCert, startCol + 0, startRow + 1, 1, 1 )

        certEditBox = Gtk.Entry()
        strCert = confVals.get( 'CertFile' )
        if strCert is None:
            strCert = ''
        
        certEditBox.set_text(strCert)
        grid.attach(certEditBox, startCol + 1, startRow + 1, 1, 1 )

        certBtn = Gtk.Button.new_with_label("...")
        certBtn.connect("clicked", self.on_choose_cert_clicked)
        grid.attach(certBtn, startCol + 2, startRow + 1, 1, 1 )

        #-----------------------------
        labelCACert = Gtk.Label()
        labelCACert.set_text("CA cert File: ")
        labelCACert.set_xalign(.5)
        grid.attach(labelCACert, startCol + 0, startRow + 2, 1, 1 )

        cacertEditBox = Gtk.Entry()
        strCert = confVals.get( 'CACertFile' )
        if strCert is None:
            strCert = ''
        
        cacertEditBox.set_text(strCert)
        grid.attach(cacertEditBox, startCol + 1, startRow + 2, 1, 1 )

        cacertBtn = Gtk.Button.new_with_label("...")
        cacertBtn.connect("clicked", self.on_choose_cacert_clicked)
        grid.attach(cacertBtn, startCol + 2, startRow + 2, 1, 1 )

        #-----------------------------
        labelSecret = Gtk.Label()
        labelSecret.set_text("Secret File: ")
        labelSecret.set_xalign(.5)
        grid.attach(labelSecret, startCol + 0, startRow + 3, 1, 1 )

        secretEditBox = Gtk.Entry()
        strCert = confVals.get( 'SecretFile' )
        if strCert is None:
            strCert = ''
        
        secretEditBox.set_text(strCert)
        grid.attach(secretEditBox, startCol + 1, startRow + 3, 1, 1 )

        secretBtn = Gtk.Button.new_with_label("...")
        secretBtn.connect("clicked", self.on_choose_secret_clicked)
        grid.attach(secretBtn, startCol + 2, startRow + 3, 1, 1 )

        #-----------------------------
        labelGmSSL = Gtk.Label()
        labelGmSSL.set_text("Using GmSSL: ")
        labelGmSSL.set_xalign(.5)
        grid.attach(labelGmSSL, startCol + 0, startRow + 4, 1, 1 )
        gmsslCheck = Gtk.CheckButton( label = "" )
        strUsingGmSSL = confVals.get( 'UsingGmSSL' )
        bGmSSL = False
        if strUsingGmSSL is None or strUsingGmSSL == 'false':
            bGmSSL = False
        elif strUsingGmSSL == 'true' :
            bGmSSL = True
        gmsslCheck.props.active = bGmSSL

        gmsslCheck.connect(
            "toggled", self.on_gmssl_toggled, "GmSSL")
        grid.attach( gmsslCheck, startCol + 1, startRow + 4, 1, 1 )
        gmsslCheck.confVals = confVals

        if not ( self.hasGmSSL and self.hasOpenSSL ):
            gmsslCheck.props.active = self.hasGmSSL
            gmsslCheck.set_sensitive( False )

        #-----------------------------
        labelVfyPeer = Gtk.Label()
        labelVfyPeer.set_text("Verify Peer: ")
        labelVfyPeer.set_xalign(.5)
        grid.attach(labelVfyPeer, startCol + 2, startRow + 4, 1, 1 )
        vfyPeerCheck = Gtk.CheckButton( label = "" )
        strVfyPeer = confVals.get( 'VerifyPeer' )
        bVerify = False
        if strVfyPeer is None or strVfyPeer == 'false':
            bVerify = False
        elif strVfyPeer == 'true' :
            bVerify = True

        vfyPeerCheck.props.active = bVerify

        vfyPeerCheck.connect(
            "toggled", self.on_button_toggled, "VerifyPeer")
        grid.attach( vfyPeerCheck, startCol + 3, startRow + 4, 1, 1 )

        if self.bServer :
            genKeyBtn = Gtk.Button.new_with_label("Gen Self-Signed Keys")
            genKeyBtn.connect("clicked", self.on_choose_key_dir_clicked)
            grid.attach( genKeyBtn, startCol + 1, startRow + 5, 1, 1 )

        #-----------------------------
        self.keyEdit = keyEditBox
        self.certEdit = certEditBox
        self.cacertEdit = cacertEditBox
        self.secretEdit = secretEditBox
        self.gmsslCheck = gmsslCheck
        self.vfyPeerCheck = vfyPeerCheck

        keyBtn.editBox = keyEditBox
        certBtn.editBox = certEditBox
        cacertBtn.editBox = cacertEditBox
        secretBtn.editBox = secretEditBox

    def switch_between_ssl_gmssl( self, sslPort : str ):
        try:
            drvCfg = self.jsonFiles[ 0 ][ 1 ]
            for port in drvCfg[ 'Ports' ]:
                if port[ 'PortClass' ] != sslPort:
                    continue

                sslFiles = port.get( 'Parameters' )
                if sslFiles is None:
                    break

                keyFile = sslFiles.get( "KeyFile" )
                if keyFile is None:
                    keyFile = ''
                self.keyEdit.set_text( keyFile )

                certFile = sslFiles.get( "CertFile" )
                if certFile is None:
                    certFile = ''
                self.certEdit.set_text( certFile )

                cacertFile = sslFiles.get( 'CACertFile' )
                if cacertFile is None:
                    cacertFile = ''
                self.cacertEdit.set_text( cacertFile )

                secretFile = sslFiles.get( 'SecretFile' )
                if secretFile is None:
                    secretFile = ''
                self.secretEdit.set_text( secretFile )
                bret = IsVerifyPeer( drvCfg, sslPort )
                if bret[ 0 ] == 0 and bret[ 1 ]:
                    self.vfyPeerCheck.props.active = True
                elif bret[ 0 ] == 0 and not bret[ 1 ]:
                    self.vfyPeerCheck.props.active = False

        except Exception as err:
            pass

    def on_gmssl_toggled( self, button, name ):
        if name != 'GmSSL':
            return
        #load settings from RpcOpenSSLFido or RpcGmSSLFido
        if button.props.active :
            sslPort = 'RpcGmSSLFido'
        else:
            sslPort = 'RpcOpenSSLFido'

        self.switch_between_ssl_gmssl( sslPort )

    def on_button_toggled( self, button, name ):
        print( name )
        bActive = button.props.active
        if name == 'SSL' :
            ifNo = button.ifNo
            if self.ifctx[ ifNo ].webSock.props.active and not bActive :
                button.props.active = True
                return
        elif name == 'Auth' :
            if not hasattr( self, "mechCombo" ):
                return
            it = self.mechCombo.get_active_iter()
            if it is not None:
                model = self.mechCombo.get_model()
                row_id, name = model[it][:2]
                if name == 'Kerberos' and IsFeatureEnabled( "krb5" ):
                    self.ToggleAuthControls( bActive, True )
                elif name == 'OAuth2' and IsFeatureEnabled( "js"):
                    self.ToggleAuthControls( bActive, False )
            return
        elif name == 'GmSSL' or name == 'VerifyPeer':
            pass
        elif name == 'WebSock' :
            ifNo = button.ifNo
            self.ifctx[ ifNo ].urlEdit.set_sensitive( bActive )
            if not self.ifctx[ ifNo ].webSock.props.active :
                return
            self.ifctx[ ifNo ].sslCheck.set_active( bActive )
            self.switch_between_ssl_gmssl( "RpcOpenSSLFido" )
            self.gmsslCheck.props.active = False
            self.gmsslCheck.set_sensitive( False )
            self.vfyPeerCheck.set_sensitive( False )
        elif name == 'WebSock2' :
            iNo = button.iNo
            self.nodeCtxs[ iNo ].urlEdit.set_sensitive( bActive )
            if not self.nodeCtxs[ iNo ].webSock.props.active :
                return
            self.nodeCtxs[ iNo ].sslCheck.set_active( bActive )
            self.switch_between_ssl_gmssl( "RpcOpenSSLFido" )
            self.gmsslCheck.props.active = False
            self.vfyPeerCheck.set_sensitive( False )
        elif name == 'SSL2' :
            iNo = button.iNo
            if self.nodeCtxs[ iNo ].webSock.props.active and not bActive  :
                button.props.active = True
                return
        elif name == 'bindTo' :
            iNo = button.iNo
            return

    def on_selection_changed(self, widget):
        if not ( IsFeatureEnabled( "krb5" ) 
            or IsFeatureEnabled( "js" ) ):
            return

        it = widget.get_active_iter()
        if it is not None:
            model = widget.get_model()
            row_id, name = model[it][:2]
            bActive = self.IsAuthChecked()
            if name == 'Kerberos':
                self.ToggleAuthControls( bActive, True )
            else:
                self.ToggleAuthControls( bActive, False )
        return

    def get_hint_for_filechooser( self, btnName, button ) :
        if button.editBox is None:
            return None
        strHint = None 
        while True:
            if btnName != "key" :
                strHint = self.keyEdit.get_text().strip() 
                if strHint is not None and len( strHint ) > 0:
                    break

            if btnName != "cert" :
                strHint = self.certEdit.get_text().strip() 
                if strHint is not None and len( strHint ) > 0:
                    break

            if btnName != "cacert" :
                strHint = self.cacertEdit.get_text().strip() 
                if strHint is not None and len( strHint ) > 0:
                    break

            if btnName != "secret" :
                strHint = self.cacertEdit.get_text().strip() 
                if strHint is not None and len( strHint ) > 0:
                    break

            return None

        strHint = os.path.dirname( strHint ) + "/*"
        return strHint

    def on_choose_key_clicked( self, button ) :
        strHint = self.get_hint_for_filechooser(
            "key", button )
        self.on_choose_file_clicked(
            button, True, strHint )

    def on_choose_cert_clicked( self, button ) :
        strHint = self.get_hint_for_filechooser(
            "cert", button )
        self.on_choose_file_clicked(
            button, False, strHint )

    def on_choose_cacert_clicked( self, button ) :
        strHint = self.get_hint_for_filechooser(
            "cacert", button )
        self.on_choose_file_clicked(
            button, False, strHint )

    def on_choose_secret_clicked( self, button ) :
        strHint = self.get_hint_for_filechooser(
            "secret", button )
        self.on_choose_file_clicked(
            button, False, strHint )

    def on_remove_if_clicked( self, button ) :
        ifNo = button.ifNo
        interf = self.ifctx[ ifNo ]
        for i in range( interf.rowCount ) :
            self.gridNet.remove_row( interf.startRow )
        for elem in self.ifctx :
            if elem.ifNo > ifNo and not elem.IsEmpty():
                elem.startRow -= interf.rowCount
        interf.Clear()
        self.ToggleAuthControls(
            self.IsAuthChecked(), self.IsKrb5Enabled() )

    def on_add_if_clicked( self, button ) :
        interf = InterfaceContext( len( self.ifctx) )
        self.ifctx.append( interf )
        startRow = GetGridRows( self.gridNet )
        interf.startRow = startRow        
        self.InitNetworkPage( self.gridNet,
            0, startRow, self.confVals, interf.ifNo )
        interf.rowCount = GetGridRows( self.gridNet ) - startRow
        print( "%d, %d" % (interf.rowCount, interf.startRow) )
        self.gridNet.show_all()

    def on_choose_file_clicked(
        self, button, bKey:bool, strHint=None ) :
        if not button.editBox is None:
            strPath = button.editBox.get_text()
        dialog = Gtk.FileChooserDialog(
            title="Please choose a file",
            parent=self, action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN,
            Gtk.ResponseType.OK,
        )
        if strPath is not None and len( strPath ) > 0:
            dialog.set_filename( strPath )

        elif strHint is not None:
            dialog.set_filename( strHint )

        #self.add_filters(dialog, bKey)

        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            button.editBox.set_text( dialog.get_filename() )

        dialog.destroy()
        
    def on_choose_pkgdir_clicked( self, button ) :
        while True: 
            dialog = Gtk.FileChooserDialog(
                title="Please choose a directory",
                parent=self,
                action=Gtk.FileChooserAction.SELECT_FOLDER)
            dialog.add_buttons(
                Gtk.STOCK_CANCEL,
                Gtk.ResponseType.CANCEL,
                Gtk.STOCK_OPEN,
                Gtk.ResponseType.OK,
            )
            response = dialog.run()
            path = None 
            if response == Gtk.ResponseType.OK:
                path = dialog.get_filename()
            dialog.destroy()
            if path is None:
                break
            button.editBox.set_text( path )
            break

    def on_choose_key_dir_clicked( self, button ) :
        dialog = SSLNumKeyDialog( self )

        response = dialog.run()
        if response != Gtk.ResponseType.OK:
            dialog.destroy()
            return
        cnum = dialog.cnumEdit.get_text().strip()
        snum = dialog.snumEdit.get_text().strip()

        strCurPath = os.path.expanduser( "~" ) + "/.rpcf"
        bGmSSL = self.gmsslCheck.props.active
        if bGmSSL :
            strCurPath += "/gmssl"
            if not os.path.isdir( strCurPath ):
                os.makedirs( strCurPath )
            GenGmSSLkey( self, strCurPath, self.bServer, cnum, snum )
        else:
            strCurPath += "/openssl"
            if not os.path.isdir( strCurPath ):
                os.makedirs( strCurPath )
            GenOpenSSLkey( self, strCurPath, self.bServer, cnum, snum )
        dialog.destroy()

    def GetTestKdcIp( self ) -> str:
        kdcIp = ""
        try:
            kdcIp = self.kdcEdit.get_text().strip().lower()
            if kdcIp != '' and IsLocalIpAddr( kdcIp ) :
                return kdcIp
            for ctx in self.ifctx :
                if ctx.authCheck.props.active :
                    return ctx.ipAddr.get_text().strip()
        except:
            kdcIp = ''
        return kdcIp

    def GetTestRealm( self ) -> str:
        strRealm = self.realmEdit.get_text().strip()
        if len( strRealm ) == 0:
            strRealm = 'RPCF.ORG'

        return strRealm

    # GSSAPI form of the ServiceName
    def GetTestKdcSvcHostName( self ) -> str:
        strSvc = self.svcEdit.get_text().strip()
        if len( strSvc ) > 0:
            components = strSvc.split( '@' )
            if len( components ) == 1:
                strSvc += '@' + platform.node()
            elif len( components ) > 2:
                raise Exception( "Error service name '%s' is valid" % strSvc )
        else:
            strSvc = 'rpcrouter' + "@" + platform.node()
        return strSvc

    # principal for server usage
    def GetTestKdcHostSvcPrinc( self ) -> str:
        strSvc = self.GetTestKdcSvcHostName()
        components = strSvc.split( '@' )
        return components[ 0 ] + "/" + components[ 1 ].lower() + \
            '@' + self.GetTestRealm()

    # principal for admin usage
    def GetTestKdcAdminSvcPrinc( self ) -> str:
        strSvc = self.GetTestKdcSvcHostName()
        components = strSvc.split( '@' )
        return components[ 0 ] + "/admin@" + self.GetTestRealm()

    def GetTestKdcUser( self ) -> str:
        strUser = self.userEdit.get_text().strip()
        if len( strUser ) == 0:
            strUser = os.getlogin()

        if strUser.find( '@' ) == -1:
            strRealm = self.GetTestRealm()
            strUser += '@' + strRealm 
        return strUser

    def GetKadmAcl( self ) -> str:
        strAcl = '''{ServiceAdm} aic
{User}  i
'''
        strSvc = self.GetTestKdcAdminSvcPrinc()
        strUser = self.GetTestKdcUser()
        return strAcl.format(
            ServiceAdm=strSvc,
            User=strUser )

    def GetKdcConf( self ) -> str:
        strKdcConf='''[realms]
{Realm} = {{
    acl_file = {KdcConfPath}/kadm5.acl
    max_renewable_life = 7d 0h 0m 0s
    supported_enctypes = aes256-cts:normal aes128-cts:normal
    default_principal_flags = +preauth
    key_stash_file = {KdcConfPath}/stash
}}
'''
        try:
            strRealm = self.GetTestRealm()
            strRet = strKdcConf.format(
                Realm=strRealm,
                KdcConfPath=GetKdcConfPath() )
            return strRet
                
        except Exception as err:
            print( err )
            return ""

    def GenKrb5InstFiles( self,
        destPath : str,
        bServer : bool ) -> int:
        ret = 0
        try:
            if not self.IsKrb5Enabled(): 
                print( "Warning 'krb5' is not selected and no need " + \
                    "to generate krb5 files for installer" )
                return 1

            if not IsTestKdcSet():
                print( "Warning local KDC is not setup " )
                return 2

            krbConf = self.GetKrb5Conf()
            if krbConf == "" :
                print( "Warning 'krb5.conf' is not generated" )
                return 3

            if not self.checkCfgKrb5.props.active:
                print( "Warning " )
                return 4

            strSrcPath = os.path.dirname(
                GetTestKeytabPath() )

            if bServer :
                krbConf = re.sub( "^default_client_.*$", "", krbConf, flags=re.MULTILINE )
                destKeytab = destPath + "/krb5.keytab"
                srcKeytab = strSrcPath + "/krb5adm.keytab"
            else:
                krbConf = re.sub( "^default_keytab_.*$", "", krbConf, flags=re.MULTILINE)
                destKeytab = destPath + "/krb5cli.keytab"
                srcKeytab = strSrcPath + "/krb5cli.keytab"

            destConf = destPath + "/krb5.conf"
            fp = open( destConf, "w" )
            fp.write( krbConf )
            fp.close()

            cmdline = "rm -f " + destKeytab + " > /dev/null 2>&1"
            cmdline += ";"
            cmdline += 'cp ' + srcKeytab + ' ' + destKeytab

            #print( cmdline )
            ret = rpcf_system( cmdline )

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT
        return ret

    def GenAuthInstFiles( self,
        destPath : str,
        bServer : bool ) -> int:
        ret = self.GenKrb5InstFiles(
            destPath, bServer )
        return ret

    def GetKrb5Conf( self ) -> str:
        strKrb5Conf = '''[logging]
default = FILE:/var/log/krb5libs.log
kdc = FILE:/var/log/krb5kdc.log
admin_server = FILE:/var/log/kadmind.log

[libdefaults]
default_realm = {Realm}
dns_lookup_realm = false
dns_lookup_kdc = false
ticket_lifetime = 24h
renew_lifetime = 7d
forwardable = true
allow_weak_crypto = true
default_keytab_name=FILE:{DefKeytab}
default_client_keytab_name=FILE:{DefCliKeytab}
rdns = false
qualify_shortname = false

[realms]
{Realm} = {{
kdc = {KdcServer}
admin_server = {KdcServer}
default_domain = {DomainName}
}}

[domain_realm]
.{DomainName} = {Realm}
{DomainName} = {Realm}

'''
        try:
            if not self.IsKrb5Enabled(): 
                raise Exception( "krb5 not enabled" )

            kdcIp = self.GetTestKdcIp()
            if len( kdcIp ) == 0:
                raise Exception( "Unable to determine kdc address, " + \
                "and at lease one interface should be auth enabled" )

            strRealm = self.GetTestRealm()
            strKeytab = GetTestKeytabPath()
            strCliKeytab = os.path.dirname( strKeytab ) + "/krb5cli.keytab"
            strRet = strKrb5Conf.format(
                KdcServer=kdcIp,
                DomainName=strRealm.lower(),
                Realm=strRealm,
                DefKeytab=strKeytab,
                DefCliKeytab=strCliKeytab
            )
            return strRet
                
        except Exception as err:
            print( err )
            return ""

    def ElevatePrivilege( self ) -> int:
        ret = rpcf_system( "sudo -n echo updating... 2>/dev/null" )
        if ret == 0 :
            return ret
        passDlg = PasswordDialog( self )
        ret, passwd = passDlg.runDlg()
        if ret == -errno.EINVAL:
            self.DisplayError( "Error invalid password")
        if ret < 0:
            return ret
        ret = rpcf_system( "echo '" + passwd + "'| sudo -S echo updating..." )
        passwd = None
        return ret

    def SetupTestKdc( self )->int:
        tempKrb = None
        tempKdc = None
        tempAcl = None
        tempNewRealm = None
        tempInit = None
        ret = 0
        try:
            strConf = self.GetKrb5Conf()
            if len( strConf ) == 0:
                return -errno.EFAULT

            tempKrb = tempname()
            if len(tempKrb) == 0:
                ret = -errno.EEXIST
                return ret
            fp = open( tempKrb, "w" )
            fp.write( strConf )
            fp.close()

            tempKdc = tempname()
            strKdc = self.GetKdcConf()
            if len(tempKdc) == 0:
                ret = -errno.EEXIST
                return ret
            fp = open( tempKdc, "w" )
            fp.write( strKdc )
            fp.close()

            tempAcl = tempname()
            strAcl = self.GetKadmAcl()
            if len(tempAcl) == 0:
                ret = -errno.EEXIST
                return ret
            fp = open( tempAcl, "w" )
            fp.write( strAcl )
            fp.close()

            tempNewRealm = tempname()
            fp = open( tempNewRealm, "w" )
            fp.write( '''
krb5_newrealm <<EOF
123456
123456
EOF
''' )
            fp.close()

            cmdline = '{sudo} systemctl stop {KdcSvc};'
            cmdline += '{sudo} systemctl stop ' + GetKadminSvcName() + ";"
            cmdline += "{sudo} install -bm 644 " + tempKrb
            cmdline += " /etc/krb5.conf;"
            cmdline += "{sudo} install -bm 644 " + tempKdc
            cmdline += " {KdcConfPath}/kdc.conf;"
            cmdline += "{sudo} install -bm 644 " + tempAcl
            cmdline += " {KdcConfPath}/kadm5.acl;"

            strDomain = self.GetTestRealm()
            strIpAddr = self.GetTestKdcIp()

            strSvcHost = self.GetTestKdcSvcHostName()
            comps = strSvcHost.split( "@" )
            strNames = comps[ 1 ] + " " + \
                strDomain + " kdc." + strDomain

            if not IsNameRegistered( strIpAddr, comps[ 1 ] ):
                strHostEntry = AddEntryToHosts(
                    strIpAddr, strNames )
                if strHostEntry != "":
                    cmdline += strHostEntry + ";"

            cmdline += "{sudo} bash " + tempNewRealm

            #add principal for the client user
            strUser = self.GetTestKdcUser()
            if not CheckPrincipal( strUser, strDomain ):
                ret = -errno.EINVAL 
                return ret

            cmdline += ";"
            cmdline += DeletePrincipal( strUser )
            cmdline += ";"
            cmdline += AddPrincipal( strUser, "" )

            #add principal for the server service
            strSvc = self.GetTestKdcSvcHostName()
            cmdline += ";"

            strHostPrinc = self.GetTestKdcHostSvcPrinc()
            cmdline += DeletePrincipal( strHostPrinc )
            cmdline += ";"
            cmdline += AddPrincipal( strHostPrinc, "" )
            cmdline += ";"

            strAdminPrinc = self.GetTestKdcAdminSvcPrinc()
            cmdline += DeletePrincipal( strAdminPrinc )
            cmdline += ";"
            cmdline += AddPrincipal( strAdminPrinc, "" )
            cmdline += ";"


            #add svc to keytable
            strKeytab = GetTestKeytabPath()
            cmdline += "rm -rf " + strKeytab + ">/dev/null 2>&1;"
            strAdmKt = GetTestAdminKeytabPath()
            cmdline += "rm -rf " + strAdmKt + ">/dev/null 2>&1;"

            cmdline += AddToKeytab(
                strHostPrinc, strKeytab )
            cmdline += ";"
            cmdline += ChangeKeytabOwner( strKeytab )
            cmdline += ";"
            cmdline += AddToKeytab(
                strAdminPrinc, strAdmKt )
            cmdline += ";"
            cmdline += ChangeKeytabOwner( strAdmKt )

            #add user to client keytable
            strCliKeytab = os.path.dirname( strKeytab ) + \
                "/krb5cli.keytab"
            cmdline += ";"
            cmdline += "rm -rf " + strCliKeytab + ">/dev/null 2>&1"
            cmdline += ";"
            cmdline += AddToKeytab(
                strUser, strCliKeytab )
            cmdline += ";"
            cmdline += ChangeKeytabOwner( strCliKeytab )

            cmdline += ";"

            #add a tag file
            cmdline += "touch " + os.path.dirname( strKeytab ) + "/kdcinited;"

            cmdline += '{sudo} systemctl restart {KdcSvc};'
            cmdline += '{sudo} systemctl restart ' + GetKadminSvcName() + ";"

            if os.geteuid() == 0:
                actCmd = cmdline.format( sudo = '',
                    KdcSvc = GetKdcSvcName(),
                    KdcConfPath = GetKdcConfPath() )
            elif IsSudoAvailable():
                ret = self.ElevatePrivilege()
                if ret < 0:
                    return ret
                actCmd = cmdline.format(
                    sudo = 'sudo',
                    KdcSvc = GetKdcSvcName(),
                    KdcConfPath = GetKdcConfPath() )
            else:
                actCmd = "su -c '" + cmdline.format(
                    sudo = "",
                    KdcSvc = GetKdcSvcName(),
                    KdcConfPath = GetKdcConfPath() ) + "'"

            #print( actCmd )
            ret = rpcf_system( actCmd )
            if ret < 0:
                return ret

            if self.realmEdit is not None:
                self.realmEdit.set_text( strDomain )
                self.svcEdit.set_text( strSvc )
                self.userEdit.set_text( strUser )
                self.kdcEdit.set_text( strIpAddr ) 

            if not self.checkNoUpdRpc.props.active:
                tempInit = tempname()
                ret = self.Export_InitCfg( tempInit )
                if ret < 0 :
                    return ret
                return  Update_InitCfg( tempInit, None )
                
        except Exception as err:
            print( err )
        finally:
            if tempKrb is not None:
                os.unlink( tempKrb )
            if tempKdc is not None:
                os.unlink( tempKdc )
            if tempAcl is not None:
                os.unlink( tempAcl )
            if tempNewRealm is not None:
                os.unlink( tempNewRealm )
            if tempInit is not None:
                os.unlink( tempInit )

    def UpdateAuthSettings( self ) -> int:
        ret = 0
        strTmpConf = None
        tempInit = None
        try:
            bServer = self.bServer
            if not self.IsKrb5Enabled() :
                return ret

            if self.realmEdit is None :
                return -errno.ENOENT

            bChangeUser = False
            bChangeSvc = False
            bChangeKdc = False
            bChangeRealm = False

            authInfo = self.confVals[ 'AuthInfo']
            strNewRealm = self.realmEdit.get_text().strip()
            strNewSvc = self.svcEdit.get_text().strip()
            strNewUser = self.userEdit.get_text().strip()
            strNewKdcIp = self.kdcEdit.get_text().strip()

            if strNewRealm == "" :
                raise Exception( "Realm is empty" )
            if strNewSvc == "":
                raise Exception( "Service is empty" )
            if strNewUser == "":
                raise Exception( "User is empty" )
            if strNewKdcIp == "":
                raise Exception( "KDC address is empty" )

            if not CheckPrincipal( strNewUser, strNewRealm ):
                raise Exception( "bad principal '%s'" % strNewUser )

            if bServer and IsLocalIpAddr( strNewKdcIp ):
                bChangeSvc = True
                bChangeUser = True
            else:
                ## change through installer
                bChangeSvc = False
                bChangeUser = False

            if authInfo[ 'Realm' ] !=  strNewRealm :
                bChangeRealm = True
            if self.confVals['AuthInfo'][ 'kdcAddr' ] != strNewKdcIp :
                bChangeKdc = True

            if not bChangeKdc and not bChangeUser and \
                not bChangeSvc and not bChangeRealm :
                return ret
            
            if IsSudoAvailable():
                ret = self.ElevatePrivilege()
                if ret < 0:
                    return ret

            cmdline = ""
            cmdline += '{sudo} systemctl stop {KdcSvc}'
            if bChangeSvc :
                cmdline += ";"

                strHostPrinc = self.GetTestKdcHostSvcPrinc()
                cmdline += DeletePrincipal( strHostPrinc )
                cmdline += ";"
                cmdline += AddPrincipal( strHostPrinc, "" )
                cmdline += ";"

                strAdminPrinc = self.GetTestKdcAdminSvcPrinc()
                cmdline += DeletePrincipal( strAdminPrinc )
                cmdline += ";"
                cmdline += AddPrincipal( strAdminPrinc, "" )
                cmdline += ";"

                strKeytab = GetTestKeytabPath()
                cmdline += "rm -rf " + strKeytab + ">/dev/null 2>&1;"
                strAdmKt = GetTestAdminKeytabPath()
                cmdline += "rm -rf " + strAdmKt + ">/dev/null 2>&1;"
                cmdline += AddToKeytab(
                    strHostPrinc, strKeytab )
                cmdline += ";"
                cmdline += ChangeKeytabOwner( strKeytab )
                cmdline += ";"
                cmdline += AddToKeytab(
                    strAdminPrinc, strAdmKt )
                cmdline += ";"
                cmdline += ChangeKeytabOwner( strAdmKt )
                cmdline += ";"

            #add user to client keytable
            if bChangeUser :
                if strKeytab is None:
                    strKeytab = GetTestKeytabPath()
                strCliKeytab = os.path.dirname( strKeytab ) + \
                    "/krb5cli.keytab"
                cmdline += "rm -rf " + strCliKeytab + ">/dev/null 2>&1"
                cmdline += ";"
                cmdline += DeletePrincipal( strNewUser )
                cmdline += ";"
                cmdline += AddPrincipal( strNewUser, "" )
                cmdline += ";"
                cmdline += AddToKeytab(
                    strNewUser, strCliKeytab )
                cmdline += ";"
                cmdline += ChangeKeytabOwner( strCliKeytab )
                cmdline += ";"
                aclFile = "{KdcConfPath}/kadm5.acl"
                cmdline += "if ! {sudo} grep '" + strNewUser + "' " + aclFile + "; then "
                cmdline += "{sudo} echo >> " + aclFile + ";"
                cmdline += "{sudo} echo '" + strNewUser + " i' >> "
                cmdline += aclFile + "; fi"

            #update krb5.conf
            strTmpConf =tempname()
            if bChangeKdc or bChangeRealm:
                ret, node = ParseKrb5Conf( '/etc/krb5.conf' )
                if ret == 0:
                    ret = UpdateKrb5Cfg( node, strNewRealm,
                        strNewKdcIp, strTmpConf, False )
                    node.RemoveChildren()
                    if ret == 0:
                        cmdline += ";"
                        cmdline += "{sudo} install -bm 644 " + strTmpConf + \
                            " /etc/krb5.conf"

                strSvcHost = self.GetTestKdcSvcHostName()
                comps = strSvcHost.split( "@" )
                strNames = comps[ 1 ] + " " + \
                    strNewRealm.lower()+ " kdc." + \
                    strNewRealm.lower()

                if not IsNameRegistered( strNewKdcIp, comps[ 1 ] ):
                    strCmd = AddEntryToHosts(
                        strNewKdcIp, strNames )
                    if strCmd != "" :
                        cmdline += ";" + strCmd

            if len( cmdline ) > 0:
                cmdline += ";"
                cmdline += '{sudo} systemctl restart {KdcSvc}'
                if os.geteuid() == 0:
                    actCmd = cmdline.format( sudo = '',
                        KdcConfPath=GetKdcConfPath(),
                        KdcSvc = GetKdcSvcName() )
                elif IsSudoAvailable():
                    actCmd = cmdline.format(
                        sudo = 'sudo',
                        KdcConfPath=GetKdcConfPath(),
                        KdcSvc = GetKdcSvcName() )
                else:
                    actCmd = "su -c '" + cmdline.format(
                        sudo = "",
                        KdcConfPath=GetKdcConfPath(),
                        KdcSvc = GetKdcSvcName() ) + "'"

                #print( actCmd )
                ret = rpcf_system( actCmd )
                if ret < 0:
                    return ret

            if not self.checkNoUpdRpc.props.active:
                tempInit = tempname()
                ret = self.Export_InitCfg( tempInit )
                if ret < 0 :
                    return ret
                return  Update_InitCfg( tempInit, None )

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT

        finally:
            if strTmpConf is not None:
                os.unlink( strTmpConf )
            if tempInit is not None:
                os.unlink( tempInit )
        return ret

    def on_update_ws_settings( self, button ):
        if not IsFeatureEnabled( "openssl" ):
            return ret

        error = self.VerifyInput()
        if error != 'success' :
            text = "Error occurs : " + error
            self.DisplayError( text )
            return -1
                    
        try:
            initFile = tempname()
            ret = self.Export_InitCfg( initFile )
            if ret < 0 :
                return ret

            if IsSudoAvailable():
                #make the following sudo password free
                ret = self.ElevatePrivilege()
                if ret < 0:
                    return ret

            fp = open( initFile, 'r' )
            cfgVal = json.load( fp )
            fp.close()

            miscOpts = cfgVal[ 'Security' ][ 'misc' ]
            miscOpts[ 'ConfigWebServer' ] = 'true'

            ret = ConfigWebServer2( cfgVal )
            if ret < 0:
                print( "Error failed to config web server " + str(ret))
                ret = 0
            else:
                print( "WebServer is configured for rpc-frmwrk successfully" )
            return ret

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT
            return ret
        finally:
            if initFile is not None:
                os.unlink( initFile )

    def on_enable_kinit_proxy( self, button ):
        ret = self.ElevatePrivilege()
        if ret < 0:
            return ret
        bEnabled = IsKinitProxyEnabled()
        EnableKinitProxy( not bEnabled )
        if IsKinitProxyEnabled() :
            button.set_label( "Disable KProxy")
        else:
            button.set_label( "Enable KProxy")

    def on_update_auth_settings( self, button ):
        self.UpdateAuthSettings()

    def on_init_kdc_settings( self, button ):
        self.SetupTestKdc()

    def add_filters(self, dialog, bKey ):
        filter_text = Gtk.FileFilter()
        if bKey :
            strFile = "key files"
        else :
            strFile = "cert files"
        filter_text.set_name( strFile )
        filter_text.add_mime_type("text/plain")
        dialog.add_filter(filter_text)

        filter_any = Gtk.FileFilter()
        filter_any.set_name("Any files")
        filter_any.add_pattern("*")
        dialog.add_filter(filter_any)

    def IsOAuth2Enabled( self )->bool :
        try:
            if not IsFeatureEnabled( "js" ):
                return False
            bChecked = self.IsAuthChecked()
            if not bChecked :
                return False
            it = self.mechCombo.get_active_iter()
            if it is not None:
                model = self.mechCombo.get_model()
                row_id, name = model[it][:2]
                if name == 'OAuth2':
                    return True
        except Exception as err:
            print( err )
            return False

    def IsKrb5Enabled( self )->bool :
        try:
            if not IsFeatureEnabled( "krb5" ):
                return False
            bChecked = self.IsAuthChecked()
            if not bChecked :
                return False
            it = self.mechCombo.get_active_iter()
            if it is not None:
                model = self.mechCombo.get_model()
                row_id, name = model[it][:2]
                if name == 'Kerberos':
                    return True
        except Exception as err:
            print( err )
            return False

    def ToggleAuthControls( self, bActive : bool, bKrb5 ):
        bSensitive = bActive and bKrb5
        self.svcEdit.set_sensitive( bSensitive )
        self.realmEdit.set_sensitive( bSensitive )
        self.signCombo.set_sensitive( bSensitive )
        self.kdcEdit.set_sensitive( bSensitive )
        self.userEdit.set_sensitive( bSensitive )
        self.btnEnaKProxy.set_sensitive( bSensitive )

        if self.bServer:
            self.updKrb5Btn.set_sensitive( bSensitive )
            self.initKrb5Btn.set_sensitive( bSensitive )
            self.checkCfgKrb5.set_sensitive( bSensitive )
            self.checkKProxy.set_sensitive( bSensitive )
            self.labelCfgKrb5.set_sensitive( bSensitive )
            self.labelKProxy.set_sensitive( bSensitive )
            self.checkNoUpdRpc.set_sensitive( bSensitive )
            self.labelNoUpdRpc.set_sensitive( bSensitive )

        grid = self.gridSec
        if not bActive :
            self.oa2cEdit.set_sensitive( False )
            self.oa2cpEdit.set_sensitive( False )
            self.authUrlEdit.set_sensitive( False )
            self.oa2sslCheck.set_sensitive( False )

            it = self.mechCombo.get_active_iter()
            if it is not None:
                model = self.mechCombo.get_model()
                row_id, name = model[it][:2]
                if name == 'Kerberos':
                    self.labelSvc.set_text("Service Name: ")
                    self.labelRealm.set_text( "Realm: ")
                    self.labelSign.set_text("Sign/Encrypt: ")
                    self.labelUser.set_text("User Name: ")
                    self.labelKdc.set_text("KDC Address: ")

                    self.labelOa2c.set_text("")
                    self.labelOa2cp.set_text("")
                    self.labelAuthUrl.set_text("")
                    self.labelOA2ssl.set_text("")
                elif name == 'OAuth2':
                    self.labelSvc.set_text("")
                    self.labelRealm.set_text("")
                    self.labelSign.set_text("")
                    self.labelUser.set_text("")
                    self.labelKdc.set_text("")

                    self.labelOa2c.set_text("Checker IP: ")
                    self.labelOa2cp.set_text("Checker Port: ")
                    self.labelAuthUrl.set_text("Auth URL: ")
                    self.labelOA2ssl.set_text("Enable SSL: ")
            self.mechCombo.set_sensitive( False )
            return
        else:
            self.mechCombo.set_sensitive( True )

        if bKrb5:
            self.labelOa2c.set_text("")
            self.labelOa2cp.set_text("")
            self.oa2cEdit.set_sensitive( False )
            self.oa2cpEdit.set_sensitive( False )
            self.authUrlEdit.set_sensitive( False )
            self.oa2sslCheck.set_sensitive( False )

            grid.remove( self.oa2cEdit )
            grid.remove( self.oa2cpEdit )
            grid.remove( self.authUrlEdit )
            grid.remove( self.oa2sslCheck )

            self.labelOa2c.set_text("")
            self.labelOa2cp.set_text("")
            self.labelAuthUrl.set_text("")
            self.labelOA2ssl.set_text("")

            grid.attach( self.svcEdit,
                self.svcEdit.cpos, self.svcEdit.rpos, 2, 1 )
            grid.attach( self.realmEdit,
                self.realmEdit.cpos, self.realmEdit.rpos, 2, 1 )
            grid.attach( self.signCombo,
                self.signCombo.cpos, self.signCombo.rpos, 1, 1 )
            grid.attach( self.kdcEdit,
                self.kdcEdit.cpos, self.kdcEdit.rpos, 2, 1 )
            grid.attach( self.userEdit,
                self.userEdit.cpos, self.userEdit.rpos, 2, 1 )
            grid.attach( self.btnEnaKProxy,
                self.btnEnaKProxy.cpos, self.btnEnaKProxy.rpos, 2, 1 )
        
            self.svcEdit.show()
            self.realmEdit.show()
            self.signCombo.show()
            self.kdcEdit.show()
            self.userEdit.show()
            self.btnEnaKProxy.show()

            self.svcEdit.set_sensitive( True )
            self.realmEdit.set_sensitive( True )
            self.kdcEdit.set_sensitive( True )
            self.userEdit.set_sensitive( True )
            self.signCombo.set_sensitive( True )
            self.btnEnaKProxy.set_sensitive(True)
            self.svcEdit.set_sensitive( True )

            self.labelSvc.set_text("Service Name: ")
            self.labelRealm.set_text( "Realm: ")
            self.labelSign.set_text("Sign/Encrypt: ")
            self.labelUser.set_text("User Name: ")
            self.labelKdc.set_text("KDC Address: ")
        else:
            self.labelSvc.set_text("")
            self.labelRealm.set_text("")
            self.svcEdit.set_sensitive( False )
            self.realmEdit.set_sensitive( False )
            self.kdcEdit.set_sensitive( False )
            self.signCombo.set_sensitive( False )
            self.userEdit.set_sensitive( False )
            self.btnEnaKProxy.set_sensitive( False )

            grid.remove( self.svcEdit )
            grid.remove( self.realmEdit )
            grid.remove( self.signCombo )
            grid.remove( self.userEdit )
            grid.remove( self.kdcEdit )
            grid.remove( self.btnEnaKProxy )

            self.labelSvc.set_text("")
            self.labelRealm.set_text("")
            self.labelSign.set_text("")
            self.labelUser.set_text("")
            self.labelKdc.set_text("")

            grid.attach( self.oa2cEdit,
                self.oa2cEdit.cpos, self.oa2cEdit.rpos, 2, 1 )
            grid.attach( self.oa2cpEdit,
                self.oa2cpEdit.cpos, self.oa2cpEdit.rpos, 2, 1 )
            grid.attach( self.authUrlEdit,
                self.authUrlEdit.cpos, self.authUrlEdit.rpos, 2, 1 )
            grid.attach( self.oa2sslCheck,
                self.oa2sslCheck.cpos, self.oa2sslCheck.rpos, 2, 1 )

            self.oa2cEdit.show()
            self.oa2cpEdit.show()
            self.authUrlEdit.show()
            self.oa2sslCheck.show()

            self.oa2cEdit.set_sensitive( True )
            self.oa2cpEdit.set_sensitive( True )
            self.authUrlEdit.set_sensitive( True )
            self.oa2sslCheck.set_sensitive( True )

            self.labelOa2c.set_text("Checker IP: ")
            self.labelOa2cp.set_text("Checker Port: ")
            self.labelAuthUrl.set_text("Auth URL: ")
            self.labelOA2ssl.set_text("Enable SSL: ")

    def AddAuthCred( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        labelAuthCred = Gtk.Label()
        labelAuthCred.set_markup("<b>Auth Information</b>")
        labelAuthCred.set_xalign(.5)
        grid.attach(labelAuthCred, startCol + 1, startRow + 0, 1, 1 )

        authInfo = None
        try:
            if 'AuthInfo' in confVals :
                authInfo = confVals[ 'AuthInfo']
        except Exception as err :
            pass

        labelMech = Gtk.Label()
        labelMech.set_text("Auth Mech: ")
        labelMech.set_xalign(.5)
        grid.attach(labelMech, startCol + 0, startRow + 1, 1, 1 )

        mechList = Gtk.ListStore()
        mechList = Gtk.ListStore(int, str)
        mechList.append([1, "Kerberos"] )
        mechList.append( [2, "OAuth2"] )
    
        # ComboBox for Auth Mechanism
        strMech = "krb5"
        if authInfo is not None and "AuthMech" in authInfo :
            strMech = authInfo[ "AuthMech" ]
        mechCombo = Gtk.ComboBox.new_with_model_and_entry(mechList)
        mechCombo.set_entry_text_column(1)
        if strMech == "krb5" :
            mechCombo.set_active( 0 )
        else:
            mechCombo.set_active( 1 )

        mechCombo.set_sensitive( True )
        mechCombo.connect('changed', self.on_selection_changed )
        grid.attach(mechCombo, startCol + 1, startRow + 1, 1, 1 )

        # krb5 service name
        labelSvc = Gtk.Label()
        labelSvc.set_text("Service Name: ")
        labelSvc.set_xalign(.5)
        grid.attach(labelSvc, startCol + 0, startRow + 2, 1, 1 )

        strSvc = ""
        if authInfo is not None and 'ServiceName' in authInfo :
            strSvc = authInfo['ServiceName']

        svcEditBox = Gtk.Entry()
        svcEditBox.set_text(strSvc)
        svcEditBox.set_tooltip_text( "Host-based service name in " + \
            "the form 'service@hostname'" )
        grid.attach(svcEditBox, startCol + 1, startRow + 2, 2, 1 )
        svcEditBox.cpos = startCol + 1
        svcEditBox.rpos = startRow + 2

        # OAuth2 Checker IP addr, leave empty to use local IPC
        labelOa2c = Gtk.Label()
        labelOa2c.set_text("OAuth2 Checker IP: ")
        labelOa2c.set_xalign(.5)
        grid.attach(labelOa2c, startCol + 0, startRow + 2, 1, 1 )

        strIp = ""
        if authInfo is not None and 'OA2ChkIp' in authInfo :
            strIp = authInfo['OA2ChkIp']

        oa2cEditBox = Gtk.Entry()
        oa2cEditBox.set_text(strIp)
        oa2cEditBox.set_tooltip_text( "OAuth2 Checker IP " + \
            "address. Assumes local when empty" )
        grid.attach(oa2cEditBox, startCol + 1, startRow + 2, 2, 1 )
        oa2cEditBox.cpos = startCol + 1
        oa2cEditBox.rpos = startRow + 2

        # krb5 realm
        labelRealm = Gtk.Label()
        labelRealm.set_text("Realm: ")
        labelRealm.set_xalign(.5)
        grid.attach(labelRealm, startCol + 0, startRow + 3, 1, 1 )

        strRealm = ""
        if authInfo is not None and 'Realm' in authInfo :
            strRealm = authInfo['Realm']

        realmEditBox = Gtk.Entry()
        realmEditBox.set_text(strRealm)
        grid.attach(realmEditBox, startCol + 1, startRow + 3, 2, 1 )
        realmEditBox.cpos = startCol + 1
        realmEditBox.rpos = startRow + 3

        # OAuth2 oa2check ip addr
        labelOa2cp = Gtk.Label()
        labelOa2cp.set_text("Port Number: ")
        labelOa2cp.set_xalign(.5)
        grid.attach(labelOa2cp, startCol + 0, startRow + 3, 1, 1 )

        strPortNum = ""
        if authInfo is not None and 'OA2ChkPort' in authInfo :
            strPortNum = authInfo['OA2ChkPort']

        oa2cpEditBox = Gtk.Entry()
        oa2cpEditBox.set_text(strPortNum)
        oa2cpEditBox.set_tooltip_text( "OAuth2 checker's port number" )
        grid.attach(oa2cpEditBox, startCol + 1, startRow + 3, 2, 1 )
        oa2cpEditBox.cpos = startCol + 1
        oa2cpEditBox.rpos = startRow + 3

        # krb5 sign/encrypt message
        labelSign = Gtk.Label()
        labelSign.set_text("Sign/Encrypt: ")
        labelSign.set_xalign(.5)
        grid.attach(labelSign, startCol + 0, startRow + 4, 1, 1 )

        idx = 1
        if authInfo is not None and 'SignMessage' in authInfo :
            strSign = authInfo[ 'SignMessage']
            if strSign == 'false' :
                idx = 0

        signMethod = Gtk.ListStore()
        signMethod = Gtk.ListStore(int, str)
        signMethod.append([1, "Encrypt Messages"])
        signMethod.append([2, "Sign Messages"])

        signCombo = Gtk.ComboBox.new_with_model_and_entry(signMethod)
        signCombo.connect("changed", self.on_sign_msg_changed)
        signCombo.set_entry_text_column(1)
        signCombo.set_active(idx)

        grid.attach( signCombo, startCol + 1, startRow + 4, 1, 1 )
        signCombo.cpos = startCol + 1
        signCombo.rpos = startRow + 4

        # OAuth2 OA2ssl
        labelOA2ssl = Gtk.Label()
        labelOA2ssl.set_text("Enable SSL: ")
        labelOA2ssl.set_xalign(.5)
        grid.attach(labelOA2ssl, startCol + 0, startRow + 4, 1, 1 )
        labelOA2ssl.cpos = startCol
        labelOA2ssl.rpos = startRow + 4

        oa2sslCheck = Gtk.CheckButton()
        oa2sslCheck.set_tooltip_text(
            "Enable SSL between the oa2checker server and the bridge" )

        if authInfo is not None and 'OA2SSL' in authInfo :
            if authInfo['OA2SSL'] == 'true':
                oa2sslCheck.props.active = True
            else:
                oa2sslCheck.props.active = False

        grid.attach(oa2sslCheck, startCol + 1, startRow + 4, 1, 1 )
        oa2sslCheck.cpos = startCol + 1
        oa2sslCheck.rpos = startRow + 4

        # krb5 kdc ip address
        labelKdc = Gtk.Label()
        labelKdc.set_text("KDC Address: ")
        labelKdc.set_xalign(.5)
        grid.attach(labelKdc, startCol + 0, startRow + 5, 1, 1 )

        strKdcIp = ""
        try:
            strKdcIp = authInfo[ 'kdcAddr']
        except Exception as err :
            pass

        kdcEditBox = Gtk.Entry()
        kdcEditBox.set_text(strKdcIp)
        grid.attach(kdcEditBox, startCol + 1, startRow + 5, 2, 1 )
        kdcEditBox.cpos = startCol + 1
        kdcEditBox.rpos = startRow + 5

        # OAuth2 AuthUrl
        labelAuthUrl = Gtk.Label()
        labelAuthUrl.set_text("Auth URL: ")
        labelAuthUrl.set_xalign(.5)
        grid.attach(labelAuthUrl, startCol + 0, startRow + 5, 1, 1 )
        labelAuthUrl.cpos = startCol
        labelAuthUrl.rpos = startRow + 5

        strAuthUrl = ""
        try:
            strAuthUrl = authInfo[ 'AuthUrl']
        except Exception as err :
            pass

        authUrlEditBox = Gtk.Entry()
        authUrlEditBox.set_text(strAuthUrl)
        grid.attach(authUrlEditBox, startCol + 1, startRow + 5, 2, 1 )
        authUrlEditBox.cpos = startCol + 1
        authUrlEditBox.rpos = startRow + 5

        # krb5 user name
        labelUser = Gtk.Label()
        labelUser.set_text("User Name: ")
        labelUser.set_xalign(.5)
        grid.attach(labelUser, startCol + 0, startRow + 6, 1, 1 )

        strUser = ""
        try:
            strUser = authInfo[ 'UserName']
        except Exception as err :
            pass

        userEditBox = Gtk.Entry()
        userEditBox.set_text(strUser)
        grid.attach(userEditBox, startCol + 1, startRow + 6, 2, 1 )
        userEditBox.cpos = startCol + 1
        userEditBox.rpos = startRow + 6

        # krb5 Disable/Enable KProxy
        if IsKinitProxyEnabled():
            btnText = "Disable KProxy"
        else:
            btnText = "Enable KProxy"
        btnEnaKProxy = Gtk.Button.new_with_label( btnText )
        btnEnaKProxy.connect("clicked", self.on_enable_kinit_proxy )
        btnEnaKProxy.set_tooltip_text( "Enable/Disable kinit proxy on this host" )
        grid.attach( btnEnaKProxy , startCol + 1, startRow + 7, 2, 1 )
        self.btnEnaKProxy = btnEnaKProxy
        btnEnaKProxy.cpos = startCol + 1
        btnEnaKProxy.rpos = startRow + 7

        if self.bServer:

            toolTip = "Initialize a KDC server on this host"
            toolTip2 = "Update the local Kerberos settings with " +\
                "the above 'Auth Information'"

            updKrb5Btn = Gtk.Button.new_with_label("Update Auth Settings")
            updKrb5Btn.connect("clicked", self.on_update_auth_settings )
            updKrb5Btn.set_tooltip_text( toolTip2 )
            grid.attach( updKrb5Btn, startCol + 1, startRow + 8, 2, 1 )
            self.updKrb5Btn = updKrb5Btn

            initKrb5Btn = Gtk.Button.new_with_label("Initialize KDC")
            initKrb5Btn.connect("clicked", self.on_init_kdc_settings )
            initKrb5Btn.set_tooltip_text( toolTip )
            grid.attach( initKrb5Btn, startCol + 1, startRow + 9, 2, 1 )
            self.initKrb5Btn = initKrb5Btn

            labelNoUpdRpc = Gtk.Label()
            labelNoUpdRpc.set_text("No Change To RPC")
            labelNoUpdRpc.set_xalign(.5)
            grid.attach(labelNoUpdRpc, startCol + 0, startRow + 10, 1, 1 )
            self.labelNoUpdRpc = labelNoUpdRpc

            checkNoUpdRpc = Gtk.CheckButton(label="")
            checkNoUpdRpc.props.active = False
            checkNoUpdRpc.connect(
                "toggled", self.on_button_toggled, "NoUpdRpc")
            grid.attach( checkNoUpdRpc, startCol + 1, startRow + 10, 1, 1)
            toolTip3 = "Don't update the settings for rpc-frmwrk after " + \
                "Initialized KDC or Updated Auth Settings"
            checkNoUpdRpc.set_tooltip_text( toolTip3 )
            self.checkNoUpdRpc = checkNoUpdRpc

        self.mechCombo = mechCombo

        self.svcEdit = svcEditBox
        self.labelSvc = labelSvc
        self.realmEdit = realmEditBox
        self.labelRealm = labelRealm
        self.signCombo = signCombo
        self.labelSign = labelSign
        self.kdcEdit = kdcEditBox
        self.labelKdc = labelKdc
        self.userEdit = userEditBox
        self.labelUser = labelUser

        self.oa2cEdit = oa2cEditBox
        self.labelOa2c = labelOa2c
        self.oa2cpEdit = oa2cpEditBox
        self.labelOa2cp = labelOa2cp
        self.authUrlEdit = authUrlEditBox
        self.labelAuthUrl = labelAuthUrl
        self.oa2sslCheck = oa2sslCheck
        self.labelOA2ssl = labelOA2ssl

    def AddMiscOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        labelMisc = Gtk.Label()
        labelMisc.set_markup("<b>Misc Options</b>")
        labelMisc.set_xalign(.5)
        grid.attach(labelMisc, startCol + 1, startRow + 0, 1, 1 )

        labelMaxConns = Gtk.Label()
        labelMaxConns.set_text("Max Connections: ")
        labelMaxConns.set_xalign(.5)
        grid.attach( labelMaxConns, startCol + 0, startRow + 1, 1, 1 )

        strMaxConns = ""
        if 'MaxConnections' in confVals :
            strMaxConns = confVals[ 'MaxConnections']
        else :
            strMaxConns = str( 512 )

        maxconnEdit = Gtk.Entry()
        maxconnEdit.set_text(strMaxConns)
        grid.attach( maxconnEdit , startCol + 1, startRow + 1, 1, 1 )
        self.maxconnEdit = maxconnEdit
    
        labelTs = Gtk.Label()
        labelTs.set_text("Task Scheduler ")
        labelTs.set_xalign(.5)
        grid.attach( labelTs, startCol + 0, startRow + 2, 1, 1 )

        tsCheck = Gtk.CheckButton(label="")
        if 'TaskScheduler' in confVals :
            tsCheck.props.active = True
        else:
            tsCheck.props.active = False

        tsCheck.connect(
            "toggled", self.on_button_toggled, "TaskSched")
        grid.attach( tsCheck, startCol + 1, startRow + 2, 1, 1)
        self.tsCheck = tsCheck
 
        btnText = "Config WebServer"
        if not IsNginxInstalled() and \
            not IsApacheInstalled() :
            return

        updWsBtn = Gtk.Button.new_with_label( btnText )
        updWsBtn.connect("clicked", self.on_update_ws_settings )
        updWsBtn.set_tooltip_text( "Config the local WebServer" )
        grid.attach( updWsBtn, startCol + 1, startRow + 3, 2, 1 )

        return

    def AddInstallerOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        labelInstaller = Gtk.Label()
        labelInstaller.set_markup("<b>Installer Options</b>")
        labelInstaller.set_xalign(.5)
        grid.attach(labelInstaller, startCol + 1, startRow + 0, 1, 1 )

        labelCfgWs = Gtk.Label()
        labelCfgWs.set_text("WS Installer")
        labelCfgWs.set_xalign(.5)
        grid.attach( labelCfgWs, startCol + 0, startRow + 1, 1, 1 )

        checkCfgWs = Gtk.CheckButton(label="")
        checkCfgWs.props.active = False
        checkCfgWs.set_tooltip_text( 
            "The installer will setup WebServer for rpc-frmwrk" )

        checkCfgWs.connect(
            "toggled", self.on_button_toggled, "CfgWs")
        grid.attach( checkCfgWs, startCol + 1, startRow + 1, 1, 1)
        self.checkCfgWs = checkCfgWs

        labelCfgKrb5 = Gtk.Label()
        labelCfgKrb5.set_text("Krb5 Installer")
        labelCfgKrb5.set_xalign(.5)
        grid.attach( labelCfgKrb5, startCol + 0, startRow + 2, 1, 1 )

        checkCfgKrb5 = Gtk.CheckButton(label="")
        checkCfgKrb5.props.active = False
        checkCfgKrb5.set_tooltip_text(
            "The installer will setup Kerberos for rpc-frmwrk" )
        self.labelCfgKrb5 = labelCfgKrb5

        checkCfgKrb5.connect(
            "toggled", self.on_button_toggled, "CfgAuth")
        grid.attach( checkCfgKrb5, startCol + 1, startRow + 2, 1, 1)
        self.checkCfgKrb5 = checkCfgKrb5

        labelKProxy = Gtk.Label()
        labelKProxy.set_text("KProxy Installer")
        labelKProxy.set_xalign(.5)
        grid.attach( labelKProxy, startCol + 0, startRow + 3, 1, 1 )
        checkKProxy = Gtk.CheckButton(label="")
        checkKProxy.props.active = False
        checkKProxy.set_tooltip_text(
            "The installer will enable kinit proxy for rpc-frmwrk" )
        self.labelKProxy = labelKProxy

        checkKProxy.connect(
            "toggled", self.on_button_toggled, "KProxy")
        grid.attach( checkKProxy, startCol + 1, startRow + 3, 1, 1)
        self.checkKProxy = checkKProxy

        labelKey = Gtk.Label()
        strDist = GetDistName()
        if strDist == 'fedora' :
            lblstr = 'rpm package'
        else :
            lblstr = 'deb package'
        labelKey.set_text(lblstr)

        labelKey.set_xalign(.5)
        grid.attach(labelKey, startCol + 0, startRow + 4, 1, 1 )

        pkgEditBox = Gtk.Entry()
        grid.attach(pkgEditBox, startCol + 1, startRow + 4 , 1, 1 )
        self.pkgEditBox = pkgEditBox

        pkgBtn = Gtk.Button.new_with_label("...")
        pkgBtn.connect("clicked", self.on_choose_pkgdir_clicked)
        pkgBtn.editBox = pkgEditBox

        pkgEditBox.set_tooltip_text( "path to the {}".format( lblstr ) )
        grid.attach(pkgBtn, startCol + 2, startRow + 4, 1, 1 )
        if not 'rpcfgopt' in confVals :
            return

        rpcfgOpt = confVals[ 'rpcfgopt' ]
        if 'PackagePath' in rpcfgOpt:
            pkgEditBox.set_text( rpcfgOpt[ 'PackagePath' ] )

        return

    def on_sign_msg_changed(self, combo) :
        tree_iter = combo.get_active_iter()
        if tree_iter is not None:
            model = combo.get_model()
            row_id, name = model[tree_iter][:2]
            print("Selected: ID=%d, name=%s" % (row_id, name))
        else:
            entry = combo.get_child()
            print("Entered: %s" % entry.get_text())

    def VerifyInput( self ) :
        try:
            addrSet = set()
            strNode = ""
            for interf in self.ifctx :
                if interf.IsEmpty() :
                    continue
                if interf.sslCheck.props.active :
                    if len( self.keyEdit.get_text().strip() ) == 0 :
                        return "SSL enabled, key file is empty"
                    if len( self.certEdit.get_text().strip() ) == 0 :
                        return "SSL enabled, cert file is empty"
                if interf.authCheck.props.active and self.IsKrb5Enabled():
                    if len( self.realmEdit.get_text().strip() ) == 0 :
                        return "Kerberos enabled, but realm is empty"
                    if len( self.svcEdit.get_text().strip() ) == 0 and self.bServer :
                        return "Kerberos enabled, but service is empty"
                    if len( self.kdcEdit.get_text().strip() ) == 0 and self.bServer :
                        return "Kerberos enabled, but kdc address is empty"
                    if len( self.userEdit.get_text().strip() ) == 0 and not self.bServer:
                        return "Kerberos enabled, but user name is empty"
                elif interf.authCheck.props.active and self.IsOAuth2Enabled():
                    strIp = self.oa2cEdit.get_text().strip()
                    if len( strIp  ) > 0 and CheckIpAddr( strIp ):
                        pass
                    elif len( strIp ) > 0:
                        return "OAuth2 checker ip address is not valid"
                    strPort = self.oa2cpEdit.get_text().strip()
                    if len( strIp ) >0 and len( strPort  ) > 0 :
                        dwPort = int( strPort )
                        if dwPort > 65535:
                            return "OAuth2 checker port number is not valid"

                if len( interf.ipAddr.get_text().strip() ) == 0 :
                    return "Ip address is empty"
                else :
                    strIp = interf.ipAddr.get_text().strip()

                if len( interf.port.get_text().strip() ) == 0 :
                    return "Port number is empty"
                else:
                    strPort = interf.port.get_text().strip()

                if (strIp, strPort) not in addrSet :
                    addrSet.add((strIp, strPort))
                else :
                    return "Identical IP and Port pair found between interfaces"

                if ( interf.ipAddr.get_text().strip() == '0.0.0.0' or  
                    interf.ipAddr.get_text().strip() == "::" ) :
                    return "Ip address is '0.0.0.0'"

                if interf.rtpathEdit is not None :
                    strPath = interf.rtpathEdit.get_text().strip()
                    if strPath[ 0 ] != '/' :
                        return "RouterPath is not a valid path"
                    try:
                        strNode = strPath[1:].split('/')[ 0 ]
                    except:
                        return "RouterPath is not a valid path"

                if interf.webSock.props.active :
                    if len( interf.urlEdit.get_text().strip() ) == 0 :
                        return "WebSocket enabled, but dest url is empty"

            if not self.bServer :
                return 'success'

            addrSet.clear()
            bValidPath = False
            if strNode == "" :
                bValidPath = True

            for nodeCtx in self.nodeCtxs :
                if nodeCtx.IsEmpty() :
                    continue
                if nodeCtx.sslCheck.props.active :
                    if len( self.keyEdit.get_text().strip() ) == 0 :
                        return "SSL enabled, but key file is empty"
                    if len( self.certEdit.get_text().strip() ) == 0 :
                        return "SSL enabled, but cert file is empty"
                if len( nodeCtx.ipAddr.get_text().strip() ) == 0 :
                    return "Ip address is empty"
                if len( nodeCtx.port.get_text().strip() ) == 0 :
                    return "Port number is empty"
                else :
                    strPort = nodeCtx.port.get_text().strip()

                if nodeCtx.ipAddr.get_text().strip() == '0.0.0.0' :
                    return "ip address is '0.0.0.0'"
                else :
                    strIp = nodeCtx.ipAddr.get_text().strip()

                if (strIp, strPort) not in addrSet :
                    addrSet.add((strIp, strPort))
                else :
                    return "Identical IP and Port pair found between nodes"

                if nodeCtx.webSock.props.active :
                    if len( interf.urlEdit.get_text().strip() ) == 0 :
                        return "WebSocket enabled, but dest url is empty"
                
                strName = nodeCtx.nodeName.get_text().strip()
                if len( strName ) == 0:
                    return "Multihop node name cannot be empty"

                if strNode == strName:
                    bValidPath = True

                if not strName.isidentifier() :
                    return "Multihop node name '%s' is not a valid identifier" % strName

            if not bValidPath and self.bServer:
                return "RouterPath is not valid because the node to forward is not in the multihop node list"

        except Exception as err:
            text = "Verify input failed:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return text

        return "success"

    def DisplayError( self, text, second_text = None ):
        dialog = Gtk.MessageDialog(
            parent=self, flags=0,
            message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK,
            text=text )
        if second_text is not None and len( second_text ) > 0 :
            dialog.format_secondary_text( second_text )                     
        dialog.run()
        dialog.destroy()

    def ExportNodeCtx( self, nodeCtx, nodeCfg ):
        try:
            strVal = nodeCtx.ipAddr.get_text().strip() 
            if len( strVal ) == 0 :
                raise Exception("'IP address' cannot be empty") 
            nodeCfg[ 'IpAddress' ] = strVal

            strVal = nodeCtx.port.get_text().strip()
            if len( strVal ) == 0 :
                strVal = str( 4132 )
            nodeCfg[ 'PortNumber' ] = strVal

            strVal = nodeCtx.nodeName.get_text().strip()
            if len( strVal ) == 0 :
                raise Exception("'Node Name' cannot be empty") 
            nodeCfg[ 'NodeName' ] = strVal

            if nodeCtx.enabled.props.active :
                nodeCfg[ "Enabled" ] = "true"
            else :
                nodeCfg[ "Enabled" ] = "false"

            if nodeCtx.compress.props.active:
                nodeCfg[ "Compression" ] = "true"
            else:
                nodeCfg[ "Compression" ] = "false"

            if nodeCtx.sslCheck.props.active:
                nodeCfg[ "EnableSSL" ] = "true"
            else:
                nodeCfg[ "EnableSSL" ] = "false"

            strVal = nodeCtx.urlEdit.get_text().strip()
            if nodeCtx.webSock.props.active :
                if len( strVal ) == 0 :
                    raise Exception("'WebSocket URL' cannot be empty") 
                nodeCfg[ "EnableWS" ] = "true"
                nodeCfg[ "DestURL" ] = strVal
            else :
                nodeCfg[ "EnableWS" ] = "false"

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -1

        return 0

    def Export_Conns( self, jsonVal ) -> int :
        ret = 0
        try:
            elemList = []
            jsonVal[ 'Connections' ] = elemList
            for i in range( len( self.ifctx ) ) :
                curVals = self.ifctx[ i ]
                if curVals.IsEmpty() :
                    continue
                elem = dict()
                strIp = curVals.ipAddr.get_text().strip()
                elem[ 'IpAddress' ] = strIp
                if self.bServer and curVals.bindTo.props.active :
                    elem[ 'BindTo' ] = 'true'
                elif self.bServer:
                    elem[ 'BindTo' ] = 'false'
                strPort = curVals.port.get_text().strip()
                elem[ 'PortNumber' ] = curVals.port.get_text().strip()
                if curVals.compress.props.active :
                    elem[ 'Compression' ] = 'true'
                else:
                    elem[ 'Compression' ] = 'false'

                if curVals.sslCheck.props.active :
                    elem[ 'EnableSSL' ] = 'true'
                else:
                    elem[ 'EnableSSL' ] = 'false'

                if curVals.authCheck.props.active :
                    elem[ 'HasAuth' ] = 'true'
                else:
                    elem[ 'HasAuth' ] = 'false'

                if curVals.webSock.props.active :
                    elem[ 'EnableWS' ] = 'true'
                    elem[ 'DestURL' ] = curVals.urlEdit.get_text().strip()
                else:
                    elem[ 'EnableWS' ] = 'false'

                if i == 0 :
                    strPath = curVals.rtpathEdit.get_text().strip()
                    if len( strPath ) == 0 :
                        strPath = '/'
                    elem[ 'RouterPath' ] = strPath

                elemList.append( elem )
                
        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            ret = False

        return ret

    def IsSSLChecked( self ) -> bool:
        ret = 0
        try:
            for i in range( len( self.ifctx ) ) :
                curVals = self.ifctx[ i ]
                if curVals.IsEmpty() :
                    continue
                if curVals.sslCheck.props.active :
                    return True
            ret = False

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            ret = False

        return ret

    def IsAuthChecked( self ) -> bool:
        try:
            for i in range( len( self.ifctx ) ) :
                curVals = self.ifctx[ i ]
                if curVals.IsEmpty() :
                    continue
                if curVals.authCheck.props.active :
                    return True
        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
        return False
        
    def Export_Security( self, jsonVal ) -> int :
        ret = 0
        try:
            elemSecs = dict()
            jsonVal[ 'Security' ] = elemSecs

            if self.IsSSLChecked() :
                sslFiles = dict()
                elemSecs[ 'SSLCred' ] = sslFiles
                sslFiles[ "KeyFile"] = self.keyEdit.get_text().strip()
                sslFiles[ "CertFile"] = self.certEdit.get_text().strip()
                sslFiles[ "CACertFile"] = self.cacertEdit.get_text().strip()
                sslFiles[ "SecretFile"] = self.secretEdit.get_text().strip()
                if self.gmsslCheck.props.active:
                    sslFiles[ "UsingGmSSL" ] = 'true'
                else:
                    sslFiles[ "UsingGmSSL" ] = 'false'

                if self.vfyPeerCheck.props.active:
                    sslFiles[ "VerifyPeer" ] = 'true'
                else:
                    sslFiles[ "VerifyPeer" ] = 'false'

            authInfo = dict()
            bKrb5 = self.IsKrb5Enabled()
            if bKrb5 :
                elemSecs[ 'AuthInfo' ] = authInfo

                authInfo[ 'Realm' ] = self.realmEdit.get_text().strip()
                authInfo[ 'ServiceName' ] = self.GetTestKdcSvcHostName()
                authInfo[ 'AuthMech' ] = 'krb5'
                authInfo[ 'UserName' ] = self.userEdit.get_text().strip()
                authInfo[ 'KdcIp' ] = self.kdcEdit.get_text().strip()

                tree_iter = self.signCombo.get_active_iter()
                if tree_iter is not None:
                    model = self.signCombo.get_model()
                    row_id, name = model[tree_iter][:2]
                if row_id == 1 :
                    authInfo[ 'SignMessage' ] = 'false'
                else:
                    authInfo[ 'SignMessage' ] = 'true'
            elif self.IsOAuth2Enabled() :
                elemSecs[ 'AuthInfo' ] = authInfo
                authInfo[ 'AuthMech' ] = 'OAuth2'
                strIp = self.oa2cEdit.get_text()
                strPort = self.oa2cpEdit.get_text()
                if len( strIp ) > 0 :
                    authInfo[ 'OA2ChkIp'] = strIp
                    if len( strPort ) == 0:
                        authInfo[ 'OA2ChkPort'] = "4132"
                    else:
                        authInfo[ 'OA2ChkPort'] = strPort
                strAuthUrl = self.authUrlEdit.get_text()
                if len( strAuthUrl ):
                    authInfo[ 'AuthUrl' ] = strAuthUrl
                if self.oa2sslCheck.props.active :
                    authInfo[ 'OA2SSL' ] = "true"
                else:
                    authInfo[ 'OA2SSL' ] = "false"

            elif self.IsAuthChecked():
                raise Exception( "unsupported auth mech")

            miscOpts = dict()
            elemSecs[ 'misc' ] = miscOpts
            try:
                iVal = int( self.maxconnEdit.get_text().strip() )
                if iVal > 60000 :
                    iVal = 60000
            except Exception as err :
                iVal = 512
            miscOpts[ 'MaxConnections' ] = str( iVal )

            if self.tsCheck.props.active :
                miscOpts[ 'TaskScheduler' ] = "RR"
            if self.checkCfgWs.props.active and self.bServer:
                miscOpts[ 'ConfigWebServer' ] = 'true'
            if bKrb5 and self.checkCfgKrb5.props.active and self.bServer:
                miscOpts[ 'ConfigKrb5' ] = 'true'
            if bKrb5 and self.checkKProxy.props.active:
                miscOpts[ 'KinitProxy' ] = 'true'

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -errno.EFAULT
        return ret

    def Export_Multihop( self, jsonVal ) -> int :
        ret = 0
        if not self.bServer :
            return 0
        try:
            nodes = list()
            jsonVal[ 'Multihop' ] = nodes

            nodeCtxs = self.nodeCtxs
            nodeCtxsFin = []
            if nodeCtxs is not None :
                for nodeCtx in nodeCtxs :
                    if nodeCtx.IsEmpty() :
                        continue
                    nodeCtxsFin.append( nodeCtx )

            numCtx = len( nodeCtxsFin )
            if numCtx == 0 :
                return 0

            for i in range( numCtx ):
                node = self.InitNodeCfg( 0 )
                ret = self.ExportNodeCtx(
                    nodeCtxsFin[ i ], node )
                if ret < 0 :
                    break
                nodes.append( node )

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -errno.EFAULT
        return ret

    def Export_LBGrp( self, jsonVal ) -> int :
        ret = 0
        if not self.bServer :
            return 0
        try:
            groups = list()
            jsonVal[ 'LBGroup' ] = groups

            if self.grpCtxs is None:
                return ret

            for grpCtx in self.grpCtxs:
                if grpCtx.IsEmpty():
                    continue
                grpSet = grpCtx.grpSet
                if len( grpSet ) == 0 :
                    continue
                members = [ *grpSet ]
                cfg = dict()
                cfg[ grpCtx.grpName ] = members
                groups.append( cfg )

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -errno.EFAULT

        return ret

    def GetNewerFile( self, strPath, pattern, bRPM )->str:
        try:
            curdir = os.getcwd()
            os.chdir( strPath )
            files = glob.glob( pattern )
            if len( files ) == 0 :
                return ""
            if len( files ) == 1 :
                return files[ 0 ]
            if bRPM :
                files = [s for s in files if not 'src.rpm' in s]
            files.sort(key=lambda x: os.path.getmtime(os.path.join('', x)))
            return files[-1]
        finally:
            os.chdir( curdir )

    def AddInstallPackages( self, destPkg )->str :
        cmdline = "" 
        try:
            strPath = self.pkgEditBox.get_text()
            if len( strPath ) == 0:
                raise Exception( "no package specified" )
            
            if not os.access( strPath, os.X_OK ):
                raise Exception( 'Package path is not valid' )
            strDist = GetDistName()
            strCmd = "touch " + strDist + ";"
            strCmd += "tar rf " + destPkg + " " + strDist + ";"
            strCmd += "tar rf " + destPkg + " -C " + strPath + " "
            if strDist == 'debian' :
                 mainPkg = self.GetNewerFile(
                     strPath, 'rpcf_*.deb', False )
                 devPkg = self.GetNewerFile(
                    strPath, 'rpcf-dev_*.deb', False )
            elif strDist == 'fedora' :     
                 devPkg = self.GetNewerFile(
                    strPath, 'rpcf-devel-[0-9]*.rpm', True )
                 mainPkg = self.GetNewerFile(
                    strPath, 'rpcf-[0-9]*.rpm', True )

            if len( mainPkg ) == 0 or len( devPkg ) == 0:
                return cmdline

            strCmd += mainPkg + " " + devPkg + ";"
            strCmd += ( "md5sum " + strPath + "/" + mainPkg + " "
                + strPath + "/" + devPkg + ";")
            strCmd += "rm " + strDist + ";"
            cmdline = strCmd
        except :
            pass

        return cmdline

    # create installer for keys generated outside rpcfg.py
    def CreateInstaller( self, initCfg : object,
        cfgPath : str, destPath : str,
        bServer : bool, bInstKeys : bool ) -> int:
        ret = 0
        keyPkg = None
        destPkg = None
        try:
            if bServer :
                destPkg = destPath + "/instsvr.tar"
                installer = destPath + "/instsvr"
            else:
                destPkg = destPath + "/instcli.tar"
                installer = destPath + "/instcli"

            if bInstKeys :
                bGmSSL = False
                sslFiles = initCfg[ 'Security' ][ 'SSLCred' ]
                if "UsingGmSSL" in sslFiles:
                    if sslFiles[ "UsingGmSSL" ] == "true": 
                        bGmSSL = True
                files = [ sslFiles[ 'KeyFile' ],
                    sslFiles[ 'CertFile' ] ]

                if 'CACertFile' in sslFiles:
                    files.append( sslFiles[ 'CACertFile' ] )

                if 'SecretFile' in sslFiles:
                    oSecret = sslFiles[ 'SecretFile' ]
                    if oSecret == "" or oSecret == "1234":
                       pass 
                    elif os.access( oSecret, os.R_OK ):
                        files.append( oSecret )

                if bServer :
                    keyPkg = destPath + "/serverkeys-0.tar.gz"
                else:
                    keyPkg = destPath + "/clientkeys-1.tar.gz"

                dirPath = os.path.dirname( files[ 0 ] )
                fileName = os.path.basename( files[ 0 ] )
                cmdLine = 'tar cf ' + keyPkg + " -C " + dirPath + " " + fileName
                ret = rpcf_system( cmdLine )
                if ret != 0:
                    raise Exception( "Error create tar file " + files[ 0 ] )
                files.pop(0)
                for i in files:
                    dirPath = os.path.dirname( i )
                    fileName = os.path.basename( i )
                    cmdLine = 'tar rf ' + keyPkg + " -C " + dirPath + " " +  fileName
                    ret = rpcf_system( cmdLine )
                    if ret != 0:
                        raise Exception( "Error appending to tar file " + i )

                cmdLine = "touch " + destPath + "/USESSL"
                rpcf_system( cmdLine )

            fp = open( destPath + '/instcfg.sh', 'w' )
            fp.write( get_instcfg_content() )
            fp.close()

            cmdLine = "cd " + destPath + ";" 
            suffix = ".sh"

            if bInstKeys:
                if bServer :
                    cmdLine += "echo 0 > svridx;"
                    cmdLine += "echo 1 > endidx;"
                    cmdLine += "tar cf " + destPkg
                    cmdLine += " svridx endidx "
                    suffix = "-0-1.sh"
                else:
                    cmdLine += "echo 1 > clidx;"
                    cmdLine += "echo 2 > endidx;"
                    cmdLine += "tar cf " + destPkg
                    cmdLine += " clidx endidx "
                    suffix = "-1-1.sh"
                cmdLine += os.path.basename( keyPkg ) + " USESSL"
            else:
                cmdLine += "tar cf " + destPkg
            cmdLine += " instcfg.sh;"
            cmdLine += "rm instcfg.sh || true;"
            if bInstKeys:
                if bServer:
                    cmdLine += "rm svridx || true;"
                else:
                    cmdLine += "rm clidx || true;"
            cmdLine += "rm -rf " + destPkg + ".gz"
            if bInstKeys:
               cmdLine += " USESSL "+ keyPkg

            ret = rpcf_system( cmdLine )
            if ret != 0:
                raise Exception( "Error creating install package" )

            
            keyPkg = None
            # add instcfg to destPkg
            cfgDir = os.path.dirname( cfgPath )
            cmdLine = 'tar rf ' + destPkg + " -C " + cfgDir + " initcfg.json;"

            cmdLine += self.AddInstallPackages( destPkg )

            bAuthFile = False
            ret = self.GenAuthInstFiles(
                destPath, bServer )
            if ret == 0:
                bAuthFile = True
                cmdLine += 'tar rf ' + destPkg + " -C " + destPath + \
                    " krb5.conf "
                if bServer:
                    cmdLine += 'krb5.keytab;'
                else:
                    cmdLine += 'krb5cli.keytab;'
            elif ret < 0:
                raise Exception( "Error generating auth files " + \
                    "when creating install package" )

            cmdLine += "gzip " + destPkg
            ret = rpcf_system( cmdLine )
            if ret != 0:
                raise Exception( "Error creating install package" )
            destPkg += ".gz"

            # create the installer
            curDate = time.strftime('-%Y-%m-%d')
            if bInstKeys:
                if bGmSSL:
                    curDate = "-g" + curDate
                else:
                    curDate = "-o" + curDate
            installer += curDate + suffix
            fp = open( installer, "w" )
            fp.write( get_instscript_content() )
            fp.close()

            cmdLine = "cd " + destPath + ";" 
            cmdLine += "cat " + destPkg + " >> " + installer + ";"
            cmdLine += "chmod 700 " + installer + ";"
            cmdLine += "rm -rf " + destPkg
            if bInstKeys:
                cmdLine += " USESSL endidx "
                if bServer :
                    cmdLine += "svridx"
                else:
                    cmdLine += "clidx"
            if bAuthFile :
                cmdline += " krb5.conf krb5.keytab krb5cli.keytab"
            rpcf_system( cmdLine )

        except Exception as err:
            try:
                if keyPkg is not None:
                    os.unlink( keyPkg )
                if destPkg is not None:
                    os.unlink( destPkg )
            except:
                pass
            if ret == 0:
                ret = -errno.EFAULT

        return ret


    def CopyInstPkg( self, keyPath : str, destPath: str )->int:
        ret = 0
        files = [ 'instsvr.tar', 'instcli.tar' ]
        try:
            idxFiles = [ keyPath + "/svridx",
                keyPath + "/clidx" ]
            sf = keyPath + "/rpcf_serial.old"
            if os.access( sf, os.R_OK ):
                idxFiles.append( sf )

            indexes = []
            for i in idxFiles:
                fp = open( i, "r" )
                idx = int( fp.read( 4 ) )
                fp.close()
                indexes.append( idx )

            if len( indexes ) == 2:
                indexes.append( 0 )
            
            '''
            if indexes[ 0 ] < indexes[ 2 ]:
                files.remove('instsvr.tar')
            if indexes[ 1 ] < indexes[ 0 ]:
                files.remove('instcli.tar')
            if len( files ) == 0 :                
                raise Exception( "InstPkgs too old to copy" )
            '''

            for i in files:
                cmdLine = "cp " + keyPath + "/" + i + " " + destPath
                ret = rpcf_system( cmdLine )
                if ret != 0:
                    raise Exception( "failed to copy " + i  )
        except Exception as err:
            if ret == 0:
                ret = -errno.ENOENT
        return ret
    
    class InstPkg :
        def __init__( self ):
            self.pkgName = ""
            self.startIdx = ""
            self.isServer = True
            self.instName = ""

    def BuildInstallers( self, initCfg : object, cfgPath : str )->int :
        ret = 0
        try:
            bSSL = self.hasSSL
            strKeyPath = os.path.expanduser( "~" ) + "/.rpcf"
            bGmSSL = False
            try:
                sslFiles = initCfg[ 'Security' ][ 'SSLCred' ]
                if 'UsingGmSSL' in sslFiles: 
                    if sslFiles[ 'UsingGmSSL' ] == 'true':
                        bGmSSL = True
            except Exception as err:
                bSSL = False

            if bGmSSL:
                strKeyPath += "/gmssl"
            else:
                strKeyPath += "/openssl"

            curDir = os.path.dirname( cfgPath )
            svrPkg = curDir + "/instsvr.tar"
            cliPkg = curDir + "/instcli.tar"

            bSSL2 = False
            if 'Connections' in initCfg:
                oConn = initCfg[ 'Connections']
                for oElem in oConn :
                    if 'EnableSSL' in oElem :
                        if oElem[ 'EnableSSL' ] == 'true' :
                            bSSL2 = True
                            break

            ret = self.CopyInstPkg( strKeyPath, curDir )
            if ret < 0:
                ret = self.CreateInstaller(
                    initCfg, cfgPath, curDir, self.bServer, bSSL and bSSL2 )
                if ret < 0:
                    raise Exception( "Error CreateInstaller in " + curDir + " with " + cfgPath )

                if bSSL and bSSL2 :
                    return ret

                #non ssl situation, create both
                ret = self.CreateInstaller(
                    initCfg, cfgPath, curDir, not self.bServer, False )
                return ret

            if os.access( cliPkg, os.W_OK ):
                bCliPkg = True
            else:
                bCliPkg = False

            if os.access( svrPkg, os.W_OK ):
                bSvrPkg = True
            else:
                bSvrPkg = False

            if not bSSL or not bSSL2:
                #removing the keys if not necessary
                if bCliPkg:
                    cmdline = "keyfiles=`tar tf " + cliPkg + " | grep '.*keys.*tar' | tr '\n' ' '`;"
                    cmdline += "for i in $keyfiles; do tar --delete -f " + cliPkg + " $i;done"
                    rpcf_system( cmdline )

                if bSvrPkg:
                    cmdline = "keyfiles=`tar tf " + svrPkg + " | grep '.*keys.*tar' | tr '\n' ' '`;"
                    cmdline += "for i in $keyfiles; do tar --delete -f " + svrPkg + " $i;done"
                    rpcf_system( cmdline )

            # generate the master install package
            instScript=get_instscript_content()

            objs = []
            svrObj = self.InstPkg()
            if bSvrPkg:
                svrObj.pkgName = svrPkg
                svrObj.startIdx = "svridx"
            svrObj.instName = "instsvr"
            svrObj.isServer = True
            objs.append( svrObj )

            cliObj = self.InstPkg()
            if bCliPkg:
                cliObj.pkgName = cliPkg
                cliObj.startIdx = "clidx"
            cliObj.instName = "instcli"
            cliObj.isServer = False
            objs.append( cliObj )

            for obj in objs :
                if obj.pkgName == "" :
                    ret = self.CreateInstaller( initCfg, cfgPath,
                        curDir, self.bServer, False )
                    if ret < 0:
                        raise Exception( "Error CreateInstaller in " + \
                            curDir + " with " + cfgPath )
                    continue

                if not obj.isServer and not self.bServer:
                    continue

                fp = open( curDir + '/instcfg.sh', 'w' )
                fp.write( get_instcfg_content() )
                fp.close()

                cmdline = "tar rf " + obj.pkgName + " -C " + curDir + " initcfg.json instcfg.sh;"
                cmdline += self.AddInstallPackages( obj.pkgName )
                bHasKey = False
                if bSSL and bSSL2:
                    cmdline += "touch " + curDir + "/USESSL;"
                    cmdline += "tar rf " + obj.pkgName + " -C " + curDir + " USESSL;"
                    bHasKey = True

                ret = self.GenAuthInstFiles(
                    curDir, obj.isServer )
                if ret == 0:
                    cmdline += 'tar rf ' + obj.pkgName + " -C " + curDir + \
                        " krb5.conf "
                    if obj.isServer:
                        cmdline += 'krb5.keytab;'
                    else:
                        cmdline += 'krb5cli.keytab;'
                elif ret < 0:
                    raise Exception( "Error generating auth files " + \
                        "when creating install package" )

                cmdline += "rm -f " + obj.pkgName + ".gz || true;"
                cmdline += "gzip " + obj.pkgName + ";"
                ret = rpcf_system( cmdline )
                if ret != 0 :
                    if obj.isServer :
                        strMsg = "error create server installer"
                    else:
                        strMsg = "error create client installer"
                    raise Exception( strMsg )

                obj.pkgName += ".gz"
                installer = curDir + "/" + obj.instName + ".sh"
                fp = open( installer, "w" )
                fp.write( instScript )
                fp.close()
                cmdline = "cat " + obj.pkgName + " >> " + installer
                rpcf_system( cmdline )
                cmdline = "chmod 700 " + installer + ";"
                rpcf_system( cmdline )

                # generate a more intuitive name
                curDate = time.strftime('%Y-%m-%d')
                if bHasKey :
                    if bGmSSL:
                        indicator= '-g-'
                    else:
                        indicator= '-o-'
                    newName = curDir + '/' + \
                        obj.instName + indicator + curDate
                    try:
                        tf = tarfile.open( obj.pkgName, "r:gz")
                        ti = tf.getmember(obj.startIdx)
                        startFp = tf.extractfile( ti )
                        startIdx = int( startFp.read(4) )
                        startFp.close()
                        ti = tf.getmember( 'endidx' )
                        endFp = tf.extractfile( ti)
                        endIdx = int( endFp.read(4) )
                        count = endIdx - startIdx
                        endFp.close()
                        if count < 0 :
                            raise Exception( "bad index file" )
                        newName += "-" + str( startIdx ) + "-" + str( count ) + ".sh"
                    except Exception as err:     
                        newName += ".sh"
                    finally:
                        tf.close()
                else:
                    newName = curDir + '/' + \
                        obj.instName + '-' + curDate + ".sh"

                rpcf_system( "mv " + installer + " " + newName )

            cmdline = "cd " + curDir + " && ( rm -f ./USESSL > /dev/null 2>&1;" + \
                "rm -f *inst*.gz krb5.conf krb5.keytab krb5cli.keytab instcfg.sh > /dev/null 2>&1);"
            #cmdline += "rm " + curDir + "/initcfg.json || true;"
            rpcf_system( cmdline )

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT

        return ret

    def Export_InitCfg( self, path ) -> int :
        jsonVal = dict()
        ret = self.Export_Conns( jsonVal )
        if ret < 0 :
            return ret

        ret = self.Export_Security( jsonVal )
        if ret < 0 :
            return ret

        ret = self.Export_Multihop( jsonVal )
        if ret < 0 :
            return ret

        ret = self.Export_LBGrp( jsonVal )
        if ret < 0 :
            return ret

        if self.bServer :
            jsonVal[ 'IsServer' ] = "true"
        else:
            jsonVal[ 'IsServer' ] = "false"

        try:
            fp = open(path, "w")
            json.dump( jsonVal, fp, indent=4)
            fp.close()
        except Exception as err:
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            ret = -errno.EFAULT

        return ret

    def Export_Installer( self, destPath : str, bServer: bool ) -> int :
        error = self.VerifyInput()
        if error != 'success' :
            text = "Error occurs : " + error
            self.DisplayError( text )
            return -1
        ret = 0
        try:
            initFile = None
            if destPath is None :
                raise Exception( 'Error dest path is none' )
            if not os.access( destPath, os.X_OK ):
                raise Exception( 'Error dest path is not valid' )
                
            initFile = destPath + '/initcfg.json'
            ret = self.Export_InitCfg( initFile )
            if ret < 0:
                raise Exception( 'Error export initcfg.json' )
            fp = open( initFile, 'r' )
            cfgVal = json.load( fp )
            fp.close()
            ret = self.BuildInstallers( cfgVal, initFile )

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT

        return ret

    def SaveRpcfgOptions( self ):
        try:
            jsonFiles = LoadConfigFiles( None )
            strPath = self.pkgEditBox.get_text().strip()
            if len( strPath ) == 0:
                return
            if not os.access( strPath, os.X_OK ):
                raise Exception( 'Warning Package path is not valid' )
            jsonDrv = jsonFiles[ 0 ][ 1 ]
            if 'rpcfgopt' not in jsonDrv:
                rpcfgOpt = dict()
            else:
                rpcfgOpt = jsonDrv[ 'rpcfgopt' ]
            rpcfgOpt[ 'PackagePath' ] = strPath
            jsonDrv[ 'rpcfgopt' ] = rpcfgOpt
            fp = open(jsonFiles[0][0], "w")
            json.dump( jsonDrv, fp, indent=4)
            fp.close()
        except Exception as err:
            pass
        
    def Export_Files( self, destPath : str, bServer: bool ) -> int :
        error = self.VerifyInput()
        if error != 'success' :
            text = "Error occurs : " + error
            self.DisplayError( text )
            return -1
                    
        try:
            initFile = tempname()
            ret = self.Export_InitCfg( initFile )
            if ret < 0 :
                return ret

            if IsInDevTree():
                ret = Update_InitCfg( initFile, None )
                self.SaveRpcfgOptions()
                return ret

            if IsSudoAvailable():
                #make the following sudo password free
                ret = self.ElevatePrivilege()
                if ret < 0:
                    return ret

            ret = Update_InitCfg( initFile, None )
            if ret < 0:
                return ret

            self.SaveRpcfgOptions()

            if not IsFeatureEnabled( "openssl" ):
                return ret

            ret = ConfigWebServer( initFile )
            if ret < 0:
                print( "Error failed to config web server " + str(ret))
                ret = 0
        finally:
            if initFile is not None:
                os.unlink( initFile )
            return ret

    def UpdateConfig( self ) :
        return self.Export_Files( None, self.bServer )

class LBGrpEditDlg(Gtk.Dialog):
    def __init__(self, parent, iGrpNo):
        Gtk.Dialog.__init__(self, flags=0,
        transient_for = parent )
        self.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OK, Gtk.ResponseType.OK )

        grid = Gtk.Grid()
        grid.set_column_homogeneous( True )
        grid.props.row_spacing = 6
        grid.props.column_spacing = 6

        self.iGrpNo = iGrpNo
        self.grpCtxs = parent.grpCtxs
        self.parent = parent

        nodeSet = deepcopy( parent.nodeSet )
        for grpCtx in parent.grpCtxs :
            nodeSet.difference_update( grpCtx.grpSet )

        grpCtx = parent.grpCtxs[ iGrpNo ]
        grpSet = grpCtx.grpSet
        self.grpSet = grpSet

        self.nodeSet = nodeSet
        self.listBoxes = []

        labelName = Gtk.Label()
        labelName.set_text( "Group Name: " )
        labelName.set_xalign( 1 )
        grid.attach(labelName, 0, 0, 1, 1 )

        nameEntry = Gtk.Entry()
        nameEntry.set_text( grpCtx.grpName  )
        grid.attach(nameEntry, 1, 0, 1, 1 )
        self.nameEntry = nameEntry

        listAvail = self.BuildListBox( nodeSet, "Available Nodes" ) 
        grid.attach( listAvail, 0, 1, 1, 8 )

        listGrp = self.BuildListBox( grpSet, "Group Members" ) 
        grid.attach( listGrp, 2, 1, 1, 8 )

        rows = GetGridRows( grid )
        if rows <= 3 :
            rowAdd = 0
            rowRm = 1
        else :
            btnSpc = rows / 3
            rowAdd = btnSpc
            rowRm = 2 * btnSpc

        toMemberBtn = Gtk.Button.new_with_label(">>")
        toMemberBtn .connect("clicked", self.on_alloc_lbgrp_clicked )
        grid.attach(toMemberBtn, 1, rowAdd, 1, 1 )
        if len( nodeSet ) == 0 :
            toMemberBtn.set_sensitive( False )

        rmMemberBtn = Gtk.Button.new_with_label("<<")
        rmMemberBtn.connect("clicked", self.on_release_lbgrp_clicked )
        grid.attach(rmMemberBtn, 1, rowRm, 1, 1 )
        if len( grpSet ) == 0 :
            rmMemberBtn.set_sensitive( False )

        self.rmMemberBtn = rmMemberBtn
        self.toMemberBtn = toMemberBtn

        self.set_border_width(10)
        #self.set_default_size(400, 460)
        box = self.get_content_area()
        box.add(grid)
        self.show_all()

    def BuildListBox( self, nodeSet, title ) :
        member_store = Gtk.ListStore(str)
        sortList = sorted( nodeSet )
        for node in sortList :
            member_store.append([node])

        tree_members = Gtk.TreeView()
        tree_members.set_model(member_store)

        # The view can support a whole tree, but we want just a list.
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn(title, renderer, text=0)
        tree_members.append_column(column)

        # This is the part that enables multiple selection.
        tree_members.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE)

        scrolled_tree = Gtk.ScrolledWindow(hexpand=True, vexpand=True)

        scrolled_tree.set_policy(
            Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.ALWAYS)

        scrolled_tree.add_with_viewport(tree_members)
        self.listBoxes.append( tree_members )

        return scrolled_tree

    def on_alloc_lbgrp_clicked( self, button ) :
        listAvail = self.listBoxes[ 0 ]
        listMember = self.listBoxes[ 1 ]
        selection = listAvail.get_selection()
        if selection.count_selected_rows() == 0 :
            return
        model, listPaths = selection.get_selected_rows()
        for path in listPaths :
            itr = model.get_iter( path )
            nodeName = model.get_value( itr , 0)
            listMember.get_model().append( [nodeName ] )
            self.nodeSet.remove( nodeName )
            self.grpSet.add( nodeName )
            model.remove( itr )
        if len( self.nodeSet ) == 0 :
            self.toMemberBtn.set_sensitive( False )
        self.rmMemberBtn.set_sensitive( True )


    def on_release_lbgrp_clicked( self, button ) :
        listAvail = self.listBoxes[ 0 ]
        listMember = self.listBoxes[ 1 ]
        selection = listMember.get_selection()
        if selection.count_selected_rows() == 0 :
            return
        model, listPaths = selection.get_selected_rows()
        for path in listPaths :
            itr = model.get_iter( path )
            nodeName = model.get_value( itr , 0)
            listAvail.get_model().append( [nodeName ] )
            self.grpSet.remove( nodeName )
            self.nodeSet.add( nodeName )
            model.remove( itr )
        if len( self.grpSet ) == 0 :
            self.rmMemberBtn.set_sensitive( False )
        self.toMemberBtn.set_sensitive( True )

class SSLNumKeyDialog(Gtk.Dialog):
    def __init__(self, parent ):
        Gtk.Dialog.__init__(self, flags=0,
        transient_for = parent )
        self.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OK, Gtk.ResponseType.OK )

        grid = Gtk.Grid()
        grid.set_column_homogeneous( True )
        grid.props.row_spacing = 6
        grid.props.column_spacing = 6

        startCol = 0
        startRow = 0

        labelNumCli = Gtk.Label()
        labelNumCli.set_text("Number of Client Keys: ")
        labelNumCli.set_xalign(.5)
        grid.attach(labelNumCli, startCol + 0, startRow, 1, 1 )

        cnumEditBox = Gtk.Entry()
        cnumEditBox.set_text( "1" )

        grid.attach(cnumEditBox, startCol + 1, startRow, 1, 1 )

        labelNumSvr = Gtk.Label()
        labelNumSvr.set_text("Number of Server Keys: ")
        labelNumSvr.set_xalign(.5)
        grid.attach(labelNumSvr, startCol + 0, startRow + 1, 1, 1 )

        snumEditBox = Gtk.Entry()
        snumEditBox.set_text( "0" )
        grid.attach(snumEditBox, startCol + 1, startRow + 1, 1, 1 )

        self.cnumEdit = cnumEditBox
        self.snumEdit = snumEditBox

        self.set_border_width(10)
        #self.set_default_size(400, 460)
        box = self.get_content_area()
        box.add(grid)
        self.show_all()

class PasswordDialog(Gtk.Dialog):
    def __init__(self, parent ):
        super().__init__(
        title = "Requiring Password to Update config files",
        flags=0, transient_for = parent )
        self.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OK, Gtk.ResponseType.OK )

        grid = Gtk.Grid()
        grid.set_column_homogeneous( True )
        grid.props.row_spacing = 6
        grid.props.column_spacing = 6

        startCol = 0
        startRow = 0

        labelPass = Gtk.Label()
        labelPass.set_text("Password: ")
        labelPass.set_xalign(.5)
        grid.attach(labelPass, startCol + 0, startRow, 1, 1 )

        passEditBox = Gtk.Entry()
        passEditBox.set_text( "" )
        passEditBox.set_visibility( False )
        passEditBox.props.input_purpose = Gtk.InputPurpose.PASSWORD
        grid.attach(passEditBox, startCol + 1, startRow, 1, 1 )
        passEditBox.connect( 'activate', self.submit)

        labelEmpty = Gtk.Label()
        labelEmpty.set_xalign(.5)
        grid.attach(labelEmpty, startCol + 1, startRow + 1, 2, 1 )

        self.passEdit = passEditBox
        self.set_border_width(10)
        box = self.get_content_area()
        box.add(grid)
        self.show_all()

    def runDlg( self )->Tuple[ int, str ]:
        ret = 0
        response = self.run()
        passwd = None
        if response != Gtk.ResponseType.OK:
            self.destroy()
            return ( -errno.ECANCELED, None )
        passwd = self.passEdit.get_text()
        self.destroy()
        if not IsValidPassword( passwd ):
            return ( -errno.EINVAL, None )
        return ( ret, passwd )

    def submit( self, entry ):
        self.response( Gtk.ResponseType.OK )

import getopt
def usage():
    print( "Usage: python3 rpcfg.py [-hc]" )
    print( "\t-c: to config a client host." )
    print( "\t\tOtherwise it is for a server host" )

def main() :
    bServer = True
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hc" )
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(-errno.EINVAL)

    output = None
    verbose = False
    for o, a in opts:
        if o == "-h" :
            usage()
            sys.exit( 0 )
        elif o == "-c":
            bServer = False
        else:
            assert False, "unhandled option"
 
    win = ConfigDlg(bServer)
    win.connect("close", Gtk.main_quit)
    while True :
        response = win.run()
        if response == Gtk.ResponseType.OK:
            ret = win.UpdateConfig()
            if ret < 0 :
                continue
        elif response == Gtk.ResponseType.YES:
            #Save As
            dialog = Gtk.FileChooserDialog(
                title="Please choose a directory",
                parent=win,
                action=Gtk.FileChooserAction.SELECT_FOLDER)
            dialog.add_buttons(
                Gtk.STOCK_CANCEL,
                Gtk.ResponseType.CANCEL,
                Gtk.STOCK_OPEN,
                Gtk.ResponseType.OK,
            )
            response = dialog.run()
            path = None 
            if response == Gtk.ResponseType.OK:
                path = dialog.get_filename()
            dialog.destroy()
            if path is None:
                continue
            win.strCfgPath = path
            win.Export_Installer(path, win.bServer)
            continue
        elif response == Gtk.ResponseType.APPLY:
            #Load Another Config File
            dialog = Gtk.FileChooserDialog(
                title="Please choose a directory",
                parent=win,
                action=Gtk.FileChooserAction.SELECT_FOLDER)
            dialog.add_buttons(
                Gtk.STOCK_CANCEL,
                Gtk.ResponseType.CANCEL,
                Gtk.STOCK_OPEN,
                Gtk.ResponseType.OK,
            )
            response = dialog.run()
            path = None 
            if response == Gtk.ResponseType.OK:
                path = dialog.get_filename()
            dialog.destroy()
            dialog = None
            if path is None:
                continue
            drvCfgPath = path + "/" + "driver.json"
            try:
                fp = open( drvCfgPath, "r" )
                fp.close()
            except OSError as err:
                text = "Failed to load config files:" + str( err )
                exc_type, exc_obj, exc_tb = sys.exc_info()
                fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
                second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
                win.DisplayError( text, second_text )
                if dialog is not None:
                    dialog.destroy()
                continue

            win.strCfgPath = path
            win.ReinitDialog( path )
            continue

        break
        
    win.destroy()

if __name__ == "__main__":
    main()
