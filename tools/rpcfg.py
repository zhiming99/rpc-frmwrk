#!/usr/bin/env python3
import json
import os
import sys 
from shutil import move
from copy import deepcopy
from urllib.parse import urlparse
from typing import Tuple
import errno
from updcfg import *

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
    return os.system( cmdline )

def GenOpenSSLkey( dlg, strPath : str, bServer:bool, cnum : str, snum:str ) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    dir_path += "/opensslkey.sh"
    cmdline = "/bin/bash " + dir_path + " " + strPath + " " + cnum + " " + snum
    bExpect = False
    ret = os.system( "which expect" )
    if ret == 0 :
        bExpect = True
    if bExpect :
        cmdline = "spawn " + cmdline
        ret = SilentRun( cmdline )
    else:
        ret = os.system( cmdline )

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

def GenGmSSLkey( dlg, strPath : str, bServer:bool, cnum : str, snum:str ) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    dir_path += "/gmsslkey.sh"
    cmdline = "bash " + dir_path + " " + strPath + " " + cnum + " " + snum
    os.system( cmdline )
    if bServer :
        strFile = strPath + "/signkey.pem"
        dlg.keyEdit.set_text( strFile )

        strFile = strPath + "/certs.pem"
        dlg.certEdit.set_text( strFile )

        strFile = strPath + "/cacert.pem"
        dlg.cacertEdit.set_text( strFile )
    else:
        strFile = strPath + "/clientkey.pem"
        dlg.keyEdit.set_text( strFile )

        strFile = strPath + "/clientcert.pem"
        dlg.certEdit.set_text( strFile )

        strFile = strPath + "/rootcacert.pem"
        dlg.cacertEdit.set_text( strFile )
    dlg.secretEdit.set_text( "" )

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

        bGmSSL = False
        bret = IsUsingGmSSL( drvVal )
        if bret[ 0 ] == 0 and bret[ 1 ] :
            sslPort = 'RpcGmSSLFido'
            bGmSSL = True
        else:
            sslPort = 'RpcOpenSSLFido'

        if bGmSSL :
            confVals[ 'UsingGmSSL' ] = 'true'
        else:
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
                            if bGmSSL:
                                sslFiles[ "CACertFile"] = dirPath + "/rootcacert.pem"
                            else:
                                sslFiles[ "CACertFile"] = dirPath + "/certs.pem"
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
                    confVals[ 'kdcAddr'] = proxy[ 'IpAddress']
                    break
            except Exception as err :
                pass
        self.jsonFiles = jsonFiles

        authUser = ""
        paths = GetTestPaths()
        pathVal = ReadTestCfg( paths, "echodesc.json" )
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

                    if not self.bServer :
                        if svrObj[ "ProxyPortClass"] != "DBusProxyPdo" :
                            continue

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

                        if authInfo is not None:
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
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OK, Gtk.ResponseType.OK,
            Gtk.STOCK_SAVE_AS, Gtk.ResponseType.YES,
            Gtk.STOCK_OPEN, Gtk.ResponseType.APPLY )

        self.bServer = bServer

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
        self.InitSecurityPage( gridSec, 0, 0, confVals )
        self.gridSec = gridSec

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
            grid.attach(ipAddr, startCol + 1, startRow + 2, 1, 1 )
            ipAddr.iNo = i
            nodeCtx.ipAddr = ipAddr

            labelPort = Gtk.Label()
            labelPort.set_text( "Port Number: " )
            labelPort.set_xalign( 1 )
            grid.attach(labelPort, startCol, startRow + 3, 1, 1 )

            portNum = Gtk.Entry()
            portNum.set_text( nodeCfg[ 'PortNumber' ] )
            grid.attach(portNum, startCol + 1, startRow + 3, 1, 1 )
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
            grid.attach(urlEdit, startCol + 1, startRow + 5, 1, 1 )
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
            if nodeCfg[ 'EnableSSL' ] == 'false' :
                sslCheck.props.active = False
            else :
                sslCheck.props.active = True
            sslCheck.connect(
                "toggled", self.on_button_toggled, "SSL2")
            grid.attach( sslCheck, startCol + 2, startRow + 6, 1, 1 )
            sslCheck.iNo = i
            nodeCtx.sslCheck = sslCheck

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
        grpCtx.grpName = dialog.nameEntry.get_text()
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
                strName = nodeCtx.nodeName.get_text()
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
        if self.bServer :
            strText = '<span foreground="red">Binding IP: </span>'
            labelIp.set_markup(strText)
            #labelIp.set_markup("<mark>Binding IP: </mark>")
        else :    
            labelIp.set_text("Server IP: ")

        labelIp.set_xalign(1)
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
        grid.attach(ipEditBox, startCol + 1, startRow + 0, 2, 1 )
        self.ifctx[ ifNo ].ipAddr = ipEditBox

        labelPort = Gtk.Label()
        if self.bServer :
            strText = '<span foreground="red">Listening Port: </span>'
            labelPort.set_markup( strText )
        else:
            labelPort.set_text("Server Port: ")

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
        grid.attach( portEditBox, startCol + 1, startRow + 1, 1, 1)
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

        if ifNo == 0 :
            rows = GetGridRows( grid )
            labelPath = Gtk.Label()
            strText = 'RouterPath: '
            labelPath.set_text( strText )

            labelPath.set_xalign(1)
            grid.attach(labelPath, startCol + 0, rows + 0, 1, 1 )

            strPath = GetRouterPath( "echodesc.json" )
            pathEditBox = Gtk.Entry()
            pathEditBox.set_text( str( strPath ) )
            grid.attach( pathEditBox, startCol + 1, rows + 0, 1, 1)
            self.ifctx[ 0 ].rtpathEdit = pathEditBox

            rows = GetGridRows( grid )
            addBtn = Gtk.Button.new_with_label("Add interface")
            addBtn.connect("clicked", self.on_add_if_clicked)
            grid.attach(addBtn, startCol + 1, rows + 1, 1, 1 )

        else :
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

        strUrl = "https://example.com"
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
        grid.attach(urlEdit, startCol + 1, startRow + 1, 1, 1 )
        self.ifctx[ ifNo ].urlEdit = urlEdit
        urlEdit.ifNo = ifNo

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
            genKeyBtn = Gtk.Button.new_with_label("Gen Demo Key")
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

    def on_gmssl_toggled( self, button, name ):
        if name != 'GmSSL':
            return
        #load settings from RpcOpenSSLFido or RpcGmSSLFido
        drvCfg = self.jsonFiles[ 0 ][ 1 ]
        if button.props.active :
            sslPort = 'RpcGmSSLFido'
        else:
            sslPort = 'RpcOpenSSLFido'
        try:
            for port in drvCfg[ 'Ports' ]:
                if port[ 'PortClass' ] != sslPort:
                    continue

                sslFiles = port.get( 'Parameters' )
                if sslFiles is None:
                    break;

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

    def on_button_toggled( self, button, name ):
        print( name )
        if name == 'SSL' :
            ifNo = button.ifNo
            if self.ifctx[ ifNo ].webSock.props.active and not button.props.active :
                button.props.active = True
                return
        elif name == 'Auth' or name == 'GmSSL' or name == 'VerifyPeer':
            pass
        elif name == 'WebSock' :
            ifNo = button.ifNo
            self.ifctx[ ifNo ].urlEdit.set_sensitive( button.props.active )
            if not self.ifctx[ ifNo ].webSock.props.active :
                return
            self.ifctx[ ifNo ].sslCheck.set_active(button.props.active)
        elif name == 'WebSock2' :
            iNo = button.iNo
            self.nodeCtxs[ iNo ].urlEdit.set_sensitive( button.props.active )
            if not self.nodeCtxs[ iNo ].webSock.props.active :
                return
            self.nodeCtxs[ iNo ].sslCheck.set_active(button.props.active)
        elif name == 'SSL2' :
            iNo = button.iNo
            if self.nodeCtxs[ iNo ].webSock.props.active and not button.props.active :
                button.props.active = True
                return

    def on_choose_key_clicked( self, button ) :
        self.on_choose_file_clicked( button, True )

    def on_choose_cert_clicked( self, button ) :
        self.on_choose_file_clicked( button, False )

    def on_choose_cacert_clicked( self, button ) :
        self.on_choose_file_clicked( button, False )

    def on_choose_secret_clicked( self, button ) :
        self.on_choose_file_clicked( button, False )

    def on_remove_if_clicked( self, button ) :
        ifNo = button.ifNo
        interf = self.ifctx[ ifNo ]
        for i in range( interf.rowCount ) :
            self.gridNet.remove_row( interf.startRow )
        for elem in self.ifctx :
            if elem.ifNo > ifNo and not elem.IsEmpty():
                elem.startRow -= interf.rowCount
        interf.Clear()

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

    def on_choose_file_clicked( self, button, bKey:bool ) :
        if not button.editBox is None:
            strPath = button.editBox.get_text()
        dialog = Gtk.FileChooserDialog(
            title="Please choose a file", parent=self, action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN,
            Gtk.ResponseType.OK,
        )
        if strPath is not None:
            dialog.set_filename( strPath )

        #self.add_filters(dialog, bKey)

        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            button.editBox.set_text( dialog.get_filename() )

        dialog.destroy()
        
    def on_choose_key_dir_clicked( self, button ) :
        dialog = SSLNumKeyDialog( self )

        response = dialog.run()
        if response != Gtk.ResponseType.OK:
            dialog.destroy()
            return
        cnum = dialog.cnumEdit.get_text()
        snum = dialog.snumEdit.get_text()

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

    def AddAuthCred( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        labelAuthCred = Gtk.Label()
        labelAuthCred.set_markup("<b>Auth Information</b>")
        labelAuthCred.set_xalign(.5)
        grid.attach(labelAuthCred, startCol + 1, startRow + 0, 1, 1 )

        labelMech = Gtk.Label()
        labelMech.set_text("Auth Mech: ")
        labelMech.set_xalign(.5)
        grid.attach(labelMech, startCol + 0, startRow + 1, 1, 1 )

        mechList = Gtk.ListStore()
        mechList = Gtk.ListStore(int, str)
        mechList.append([1, "Kerberos"])
    
        mechCombo = Gtk.ComboBox.new_with_model_and_entry(mechList)
        mechCombo.set_entry_text_column(1)
        mechCombo.set_active( 0 )
        mechCombo.set_sensitive( False )
        grid.attach(mechCombo, startCol + 1, startRow + 1, 1, 1 )

        labelSvc = Gtk.Label()
        labelSvc.set_text("Service Name: ")
        labelSvc.set_xalign(.5)
        grid.attach(labelSvc, startCol + 0, startRow + 2, 1, 1 )

        strSvc = ""
        try:
            if 'AuthInfo' in confVals :
                authInfo = confVals[ 'AuthInfo']
                if 'ServiceName' in authInfo :
                    strSvc = authInfo[ 'ServiceName']
        except Exception as err :
            pass

        svcEditBox = Gtk.Entry()
        svcEditBox.set_text(strSvc)
        grid.attach(svcEditBox, startCol + 1, startRow + 2, 2, 1 )

        labelRealm = Gtk.Label()
        labelRealm.set_text("Realm: ")
        labelRealm.set_xalign(.5)
        grid.attach(labelRealm, startCol + 0, startRow + 3, 1, 1 )

        strRealm = ""
        try:
            if 'AuthInfo' in confVals :
                authInfo = confVals[ 'AuthInfo']
                if 'Realm' in authInfo :
                    strRealm = authInfo[ 'Realm']
        except Exception as err :
            pass
        realmEditBox = Gtk.Entry()
        realmEditBox.set_text(strRealm)
        grid.attach(realmEditBox, startCol + 1, startRow + 3, 2, 1 )

        labelSign = Gtk.Label()
        labelSign.set_text("Sign/Encrypt: ")
        labelSign.set_xalign(.5)
        grid.attach(labelSign, startCol + 0, startRow + 4, 1, 1 )

        idx = 1
        try:
            if 'AuthInfo' in confVals :
                authInfo = confVals[ 'AuthInfo']
                if 'SignMessage' in authInfo :
                    strSign = authInfo[ 'SignMessage']
                    if strSign == 'false' :
                        idx = 0
        except Exception as err :
            pass

        signMethod = Gtk.ListStore()
        signMethod = Gtk.ListStore(int, str)
        signMethod.append([1, "Encrypt Messages"])
        signMethod.append([2, "Sign Messages"])

        signCombo = Gtk.ComboBox.new_with_model_and_entry(signMethod)
        signCombo.connect("changed", self.on_sign_msg_changed)
        signCombo.set_entry_text_column(1)
        signCombo.set_active(idx)

        grid.attach( signCombo, startCol + 1, startRow + 4, 1, 1 )

        labelKdc = Gtk.Label()
        labelKdc.set_text("KDC IP address: ")
        labelKdc.set_xalign(.5)
        grid.attach(labelKdc, startCol + 0, startRow + 5, 1, 1 )

        strKdcIp = ""
        try:
            if 'kdcAddr' in confVals :
                strKdcIp = confVals[ 'kdcAddr']
        except Exception as err :
            pass

        kdcEditBox = Gtk.Entry()
        kdcEditBox.set_text(strKdcIp)
        grid.attach(kdcEditBox, startCol + 1, startRow + 5, 2, 1 )

        labelUser = Gtk.Label()
        labelUser.set_text("User Name: ")
        labelUser.set_xalign(.5)
        grid.attach(labelUser, startCol + 0, startRow + 6, 1, 1 )

        strUser = ""
        try:
            authInfo = confVals[ 'AuthInfo']
            strUser = authInfo[ 'UserName']
        except Exception as err :
            pass

        userEditBox = Gtk.Entry()
        userEditBox.set_text(strUser)
        grid.attach(userEditBox, startCol + 1, startRow + 6, 2, 1 )

        self.svcEdit = svcEditBox
        self.realmEdit = realmEditBox
        self.signCombo = signCombo
        self.kdcEdit = kdcEditBox
        self.userEdit = userEditBox

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
    
        labelTs = Gtk.Label()
        labelTs.set_text("Task Scheduler ")
        labelTs.set_xalign(.5)
        grid.attach( labelTs, startCol + 0, startRow + 2, 1, 1 )
        self.maxconnEdit = maxconnEdit

        tsCheck = Gtk.CheckButton(label="")
        if 'TaskScheduler' in confVals :
            tsCheck.props.active = True
        else:
            tsCheck.props.active = False

        tsCheck.connect(
            "toggled", self.on_button_toggled, "TaskSched")
        grid.attach( tsCheck, startCol + 1, startRow + 2, 1, 1)
        self.tsCheck = tsCheck
 
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
                    if len( self.keyEdit.get_text() ) == 0 :
                        return "SSL enabled, key file is empty"
                    if len( self.certEdit.get_text() ) == 0 :
                        return "SSL enabled, cert file is empty"
                if interf.authCheck.props.active :
                    if len( self.realmEdit.get_text() ) == 0 :
                        return "Auth enabled, but realm is empty"
                    if len( self.svcEdit.get_text() ) == 0 and self.bServer :
                        return "Auth enabled, but service is empty"
                    if len( self.kdcEdit.get_text() ) == 0 and self.bServer :
                        return "Auth enabled, but kdc address is empty"
                    if len( self.userEdit.get_text() ) == 0 and not self.bServer:
                        return "Auth enabled, but user name is empty"
                if len( interf.ipAddr.get_text() ) == 0 :
                    return "Ip address is empty"
                else :
                    strIp = interf.ipAddr.get_text()

                if len( interf.port.get_text() ) == 0 :
                    return "Port number is empty"
                else:
                    strPort = interf.port.get_text()

                if (strIp, strPort) not in addrSet :
                    addrSet.add((strIp, strPort))
                else :
                    return "Identical IP and Port pair found between interfaces"

                if interf.ipAddr.get_text() == '0.0.0.0' :
                    return "Ip address is '0.0.0.0'"

                if interf.rtpathEdit is not None :
                    strPath = interf.rtpathEdit.get_text()
                    if strPath[ 0 ] != '/' :
                        return "RouterPath is not a valid path"
                    try:
                        strNode = strPath[1:].split('/')[ 0 ]
                    except:
                        return "RouterPath is not a valid path"

                if interf.webSock.props.active :
                    if len( interf.urlEdit.get_text() ) == 0 :
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
                    if len( self.keyEdit.get_text() ) == 0 :
                        return "SSL enabled, but key file is empty"
                    if len( self.certEdit.get_text() ) == 0 :
                        return "SSL enabled, but cert file is empty"
                if len( nodeCtx.ipAddr.get_text() ) == 0 :
                    return "Ip address is empty"
                if len( nodeCtx.port.get_text() ) == 0 :
                    return "Port number is empty"
                else :
                    strPort = nodeCtx.port.get_text()

                if nodeCtx.ipAddr.get_text() == '0.0.0.0' :
                    return "ip address is '0.0.0.0'"
                else :
                    strIp = nodeCtx.ipAddr.get_text()

                if (strIp, strPort) not in addrSet :
                    addrSet.add((strIp, strPort))
                else :
                    return "Identical IP and Port pair found between nodes"

                if nodeCtx.webSock.props.active :
                    if len( interf.urlEdit.get_text() ) == 0 :
                        return "WebSocket enabled, but dest url is empty"
                
                strName = nodeCtx.nodeName.get_text()
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
            strVal = nodeCtx.ipAddr.get_text() 
            if len( strVal ) == 0 :
                raise Exception("'IP address' cannot be empty") 
            nodeCfg[ 'IpAddress' ] = strVal

            strVal = nodeCtx.port.get_text()
            if len( strVal ) == 0 :
                strVal = str( 4132 )
            nodeCfg[ 'PortNumber' ] = strVal

            strVal = nodeCtx.nodeName.get_text()
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

            strVal = nodeCtx.urlEdit.get_text()
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
                strIp = curVals.ipAddr.get_text()
                elem[ 'IpAddress' ] = strIp
                strPort = curVals.port.get_text()
                elem[ 'PortNumber' ] = curVals.port.get_text()
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
                    elem[ 'DestURL' ] = curVals.urlEdit.get_text()
                else:
                    elem[ 'EnableWS' ] = 'false'

                if i == 0 :
                    strPath = curVals.rtpathEdit.get_text()
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
            return -errno.EFAULT

        return ret
        
    def Export_Security( self, jsonVal ) -> int :
        ret = 0
        try:
            elemSecs = dict()
            jsonVal[ 'Security' ] = elemSecs
            sslFiles = dict()
            elemSecs[ 'SSLCred' ] = sslFiles
            sslFiles[ "KeyFile"] = self.keyEdit.get_text()
            sslFiles[ "CertFile"] = self.certEdit.get_text()
            sslFiles[ "CACertFile"] = self.cacertEdit.get_text()
            sslFiles[ "SecretFile"] = self.secretEdit.get_text()
            if self.gmsslCheck.props.active:
                sslFiles[ "UsingGmSSL" ] = 'true'
            else:
                sslFiles[ "UsingGmSSL" ] = 'false'

            if self.vfyPeerCheck.props.active:
                sslFiles[ "VerifyPeer" ] = 'true'
            else:
                sslFiles[ "VerifyPeer" ] = 'false'

            authInfo = dict()
            elemSecs[ 'AuthInfo' ] = authInfo

            authInfo[ 'Realm' ] = self.realmEdit.get_text()
            authInfo[ 'ServiceName' ] = self.svcEdit.get_text()
            authInfo[ 'AuthMech' ] = 'krb5'
            authInfo[ 'UserName' ] = self.userEdit.get_text()
            if self.bServer :
                authInfo[ 'KdcIp' ] = self.kdcEdit.get_text()

            tree_iter = self.signCombo.get_active_iter()
            if tree_iter is not None:
                model = self.signCombo.get_model()
                row_id, name = model[tree_iter][:2]
            if row_id == 1 :
                authInfo[ 'SignMessage' ] = 'false'
            else:
                authInfo[ 'SignMessage' ] = 'true'

            miscOpts = dict()
            elemSecs[ 'misc' ] = miscOpts
            try:
                iVal = int( self.maxconnEdit.get_text() )
                if iVal > 60000 :
                    iVal = 60000
            except Exception as err :
                iVal = 512
            miscOpts[ 'MaxConnections' ] = str( iVal )

            if self.tsCheck.props.active :
                miscOpts[ 'TaskScheduler' ] = "RR"

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

    def Export_Files( self, destPath : str, bServer: bool ) -> int :
        error = self.VerifyInput()
        if error != 'success' :
            text = "Error occurs : " + error
            self.DisplayError( text )
            return -1
                
        initFile = '/tmp/initcfg.json'
        if destPath is not None :
            initFile = destPath + '/initcfg.json'
            return self.Export_InitCfg( initFile )

        ret = self.Export_InitCfg( initFile )
        if ret < 0 :
            return ret

        passDlg = PasswordDialog( self )
        ret = Update_InitCfg( initFile, destPath, passDlg )
        os.remove( initFile )
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
        labelNumSvr.set_text("Number of Client Keys: ")
        labelNumSvr.set_xalign(.5)
        grid.attach(labelNumSvr, startCol + 0, startRow + 1, 1, 1 )

        snumEditBox = Gtk.Entry()
        snumEditBox.set_text( "1" )
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

        labelEmpty = Gtk.Label()
        labelEmpty.set_xalign(.5)
        grid.attach(labelEmpty, startCol + 1, startRow + 1, 2, 1 )

        self.passEdit = passEditBox
        self.set_border_width(10)
        box = self.get_content_area()
        box.add(grid)
        self.show_all()

    def runDlg( self )->Tuple[ int, int ]:
        ret = 0
        response = self.run()
        passwd = None
        if response != Gtk.ResponseType.OK:
            self.destroy()
            ret = -errno.EINVAL
        else:
            passwd = self.passEdit.get_text()
            self.destroy()
            ret = 0
        return ( ret, passwd )

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
            win.Export_Files(path, win.bServer)
            continue
        elif response == Gtk.ResponseType.APPLY:
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
            path = '.'
            if response == Gtk.ResponseType.OK:
                path = dialog.get_filename()
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
                dialog.destroy()
                continue

            dialog.destroy()
            win.strCfgPath = path
            win.ReinitDialog( path )
            continue

        break
        
    win.destroy()

if __name__ == "__main__":
    main()
