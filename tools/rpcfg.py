import gi
import json
import os
import sys 
from shutil import move
from copy import deepcopy

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

def LoadConfigFiles( path : str) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    paths = []
    if path is None:
        curDir = dir_path;
        paths.append( curDir + "/../../etc/rpcf" )
        paths.append( "/etc/rpcf")

        paths.append( curDir + "/../ipc" )
        paths.append( curDir + "/../rpc/router" )
        paths.append( curDir + "/../rpc/security" )
    else:
        paths.append( path )
    
    jsonvals = [ None ] * 4
    for strPath in paths :
        try:
            if jsonvals[ 0 ] is None :
                drvfile = strPath + "/driver.json"
                fp = open( drvfile, "r" )
                jsonvals[ 0 ] = [drvfile, json.load(fp) ]
                fp.close()
        except OSError as err :
            pass
        try:
            if jsonvals[ 1 ] is None :
                rtfile = strPath + "/router.json"
                fp = open( rtfile, "r" )
                jsonvals[ 1 ] = [rtfile, json.load(fp)]
                fp.close()
        except OSError as err :
            pass

        try:
            if jsonvals[ 2 ] is None :
                rtaufile = strPath + "/rtauth.json"
                fp = open( rtaufile, "r" )
                jsonvals[ 2 ] = [rtaufile, json.load(fp)]
                fp.close()
        except OSError as err :
            pass
        
        try:
            if jsonvals[ 3 ] is None :
                auprxyfile = strPath + "/authprxy.json"
                fp = open( auprxyfile, "r" )
                jsonvals[ 3 ] = [auprxyfile, json.load(fp)]
                fp.close()
        except OSError as err :
            pass            

    return jsonvals


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
        
class ConfigDlg(Gtk.Dialog):

    def RetrieveInfo( self, path : str = None ) :
        confVals = dict()
        jsonFiles = LoadConfigFiles( path )
        if len( jsonFiles ) == 0 :
            return None
        drvVal = jsonFiles[ 0 ][1]
        ports = drvVal['Ports']
        for port in ports :
            try:
                if port[ 'PortClass'] == 'RpcOpenSSLFido' :
                    sslFiles = port[ 'Parameters']
                    if sslFiles is None :
                        continue
                    confVals["keyFile"] = sslFiles[ "KeyFile"]
                    confVals["certFile"] = sslFiles[ "CertFile"]
                
                if port[ 'PortClass'] == 'RpcTcpBusPort' :
                    connParams = port[ 'Parameters']
                    if connParams is None :
                        ifCfg = dict()
                        ifCfg[ "AddrFormat" ]= "ipv4";
                        ifCfg[ "Protocol" ]= "native";
                        ifCfg[ "PortNumber" ]= "4132";
                        ifCfg[ "BindAddr"] = "127.0.0.1" ;
                        ifCfg[ "PdoClass"] = "TcpStreamPdo2",;
                        ifCfg[ "Compression" ]= "true";
                        ifCfg[ "EnableWS" ]= "false";
                        ifCfg[ "EnableSSL" ]= "false";
                        ifCfg[ "ConnRecover" ]= "false";
                        ifCfg[ "HasAuth" ]= "false";
                        connparams = [ ifCfg, ]
                    confVals[ "connParams"] = [ *connParams ]
            except Exception as err :
                pass

        svrVals = jsonFiles[ 2 ][ 1 ]
        svrObjs = svrVals[ 'Objects']
        if svrObjs is None or len( svrObjs ) == 0 :
            return confVals

        for svrObj in svrObjs :
            try:
                if svrObj[ 'ObjectName'] == 'RpcRouterBridgeAuthImpl' :
                    confVals[ 'AuthInfo'] = svrObj[ 'AuthInfo']
                    confVals[ 'Nodes'] = svrObj[ 'Nodes']
                    break
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
        return confVals    

    def ReinitDialog( self, path : str ) :
        confVals = self.RetrieveInfo( path )
        self.confVals = confVals
        self.ifctx = []

        self.keyEdit = None
        self.certEdit = None
        self.svcEdit = None
        self.realmEdit = None
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
            if interf.IsEmpty():
                continue
            row = GetGridRows( gridNet )
            interf.startRow = row
            self.InitNetworkPage( gridNet, 0, row, confVals, i )
            interf.rowCount = GetGridRows( gridNet ) - row
            i += 1

        gridCred = self.gridCred
        rows = GetGridRows( gridCred )
        for i in range( rows ) :
            gridCred.remove_row( 0 )
        self.InitCredPage( gridCred, 0, 0, confVals )

        gridmh = self.gridmh
        rows = GetGridRows( gridmh )
        for i in range( rows ) :
            gridmh.remove_row( 0 )
        self.InitMultihopPage( gridmh, 0, 0, confVals )
        self.show_all()

    def __init__(self, bServer):
        Gtk.Dialog.__init__(self, title="Config the RPC Router", flags=0)
        self.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OK, Gtk.ResponseType.OK,
            Gtk.STOCK_SAVE_AS, Gtk.ResponseType.YES,
            Gtk.STOCK_OPEN, Gtk.ResponseType.APPLY )
        self.bServer = bServer
        if not bServer :
            self.get_header_bar().set_title( "Config RPC Proxy Router" )

        confVals = self.RetrieveInfo()
        self.confVals = confVals
        self.ifctx = []
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

        gridCred = Gtk.Grid()
        #gridCred.set_row_homogeneous( True )
        gridCred.props.row_spacing = 6        
        stack.add_titled(gridCred, "GridCred", "Security Info")
        self.InitCredPage( gridCred, 0, 0, confVals )
        self.gridCred = gridCred

        gridmh = Gtk.Grid()
        gridmh.props.row_spacing = 6        
        stack.add_titled(gridmh, "GridMh", "Multihop")
        self.InitMultihopPage( gridmh, 0, 0, confVals )
        self.gridmh = gridmh

        stack_switcher = Gtk.StackSwitcher()
        stack_switcher.set_stack(stack)
        vbox.pack_start(stack_switcher, False, True, 0)

        scrolledWin = Gtk.ScrolledWindow(hexpand=True, vexpand=True)
        scrolledWin.set_border_width(10)
        scrolledWin.set_policy(
            Gtk.PolicyType.NEVER, Gtk.PolicyType.ALWAYS)
        scrolledWin.add( stack )
        vbox.pack_start(scrolledWin, True, True, 0)

        self.set_border_width(10)
        self.set_default_size(400, 460)
        box = self.get_content_area()
        box.add(vbox)
        self.show_all()
 
    def InitCredPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        row = GetGridRows( grid )
        self.AddSSLCred( grid, startCol, row, confVals )
        row = GetGridRows( grid )
        self.AddAuthCred( grid, startCol, row, confVals )

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
            nodeCtx.nodeName = nameEntry;

            labelIpAddr = Gtk.Label()
            labelIpAddr.set_text( "Remote IP Address: " )
            labelIpAddr.set_xalign( 1 )
            grid.attach(labelIpAddr, startCol, startRow + 2, 1, 1 )

            ipAddr = Gtk.Entry()
            ipAddr.set_text( nodeCfg[ 'IpAddress' ] )
            grid.attach(ipAddr, startCol + 1, startRow + 2, 1, 1 )
            ipAddr.iNo = i
            nodeCtx.ipAddr = ipAddr;

            labelPort = Gtk.Label()
            labelPort.set_text( "Port Number: " )
            labelPort.set_xalign( 1 )
            grid.attach(labelPort, startCol, startRow + 3, 1, 1 )

            portNum = Gtk.Entry()
            portNum.set_text( nodeCfg[ 'PortNumber' ] )
            grid.attach(portNum, startCol + 1, startRow + 3, 1, 1 )
            portNum.iNo = i
            nodeCtx.port = portNum;

            labelWebSock = Gtk.Label()
            labelWebSock.set_text("WebSocket: ")
            labelWebSock.set_xalign(1)
            grid.attach(labelWebSock, startCol + 0, startRow + 4, 1, 1 )

            webSockCheck = Gtk.CheckButton(label="")
            webSockCheck.connect(
                "toggled", self.on_button_toggled, "WebSock2")
            webSockCheck.iNo = i
            if nodeCfg[ 'EnableWS' ] == 'false' :
                webSockCheck.props.active = False
            else:
                webSockCheck.props.active = True

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

    def InitMultihopPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        try:
            nodes = confVals[ 'Nodes' ]
            nodeCount = len( nodes )

            self.nodeCtxs = []

            if nodeCount == 0 :
                return

            self.nodes = nodes
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
        compressCheck.connect(
            "toggled", self.on_button_toggled, "Compress")
        compressCheck.props.active = bActive
        compressCheck.ifNo = ifNo
        grid.attach( compressCheck, startCol + 1, rows, 1, 1)
        self.ifctx[ ifNo ].compress = compressCheck

        if ifNo == 0 :
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
        webSockCheck.connect(
            "toggled", self.on_button_toggled, "WebSock")
        webSockCheck.ifNo = ifNo
        webSockCheck.props.active = bActive
        grid.attach( webSockCheck, startCol + 1, startRow + 0, 1, 1)
        self.ifctx[ ifNo ].webSock = webSockCheck

        labelUrl = Gtk.Label()
        labelUrl.set_text("WebSocket URL: ")
        labelUrl.set_xalign(1)
        grid.attach(labelUrl, startCol + 0, startRow + 1, 1, 1 )

        strUrl = "https://example.com";
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
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0 is not None :
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

    def AddSSLCred( self, grid:Gtk.Grid, startCol, startRow, confVals : dict ) :
        labelSSLfiles = Gtk.Label()
        labelSSLfiles.set_markup("<b>SSL Files</b> ")
        labelSSLfiles.set_xalign(.5)
        grid.attach(labelSSLfiles, startCol + 1, startRow, 1, 1 )
        
        startRow+=1
        labelKey = Gtk.Label()
        labelKey.set_text("Key File: ")
        labelKey.set_xalign(.5)
        grid.attach(labelKey, startCol + 0, startRow, 1, 1 )

        keyEditBox = Gtk.Entry()
        strKey = ""
        try:
            if confVals[ 'keyFile'] is not None :
                strKey = confVals[ 'keyFile']
        except Exception as err :
            pass

        keyEditBox.set_text(strKey)
        grid.attach(keyEditBox, startCol + 1, startRow, 1, 1 )

        keyBtn = Gtk.Button.new_with_label("...")
        keyBtn.connect("clicked", self.on_choose_key_clicked)

        grid.attach(keyBtn, startCol + 2, startRow, 1, 1 )

        labelCert = Gtk.Label()
        labelCert.set_text("Cert File: ")
        labelCert.set_xalign(.5)
        grid.attach(labelCert, startCol + 0, startRow + 1, 1, 1 )

        certEditBox = Gtk.Entry()
        strCert = ""
        
        try:
            if confVals[ 'certFile'] is not None :
                strCert = confVals[ 'certFile']    
        except Exception as err :
            pass    
        certEditBox.set_text(strCert)
        grid.attach(certEditBox, startCol + 1, startRow + 1, 1, 1 )

        certBtn = Gtk.Button.new_with_label("...")
        certBtn.connect("clicked", self.on_choose_cert_clicked)
        grid.attach(certBtn, startCol + 2, startRow + 1, 1, 1 )

        self.keyEdit = keyEditBox
        self.certEdit = certEditBox

        keyBtn.editBox = keyEditBox
        certBtn.editBox = certEditBox

    def on_button_toggled( self, button, name ):
        print( name )
        if name == 'SSL' :
            ifNo = button.ifNo
            if self.ifctx[ ifNo ].webSock.props.active and not button.props.active :
                button.props.active = True
                return
        elif name == 'Auth' :
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
        if name == 'SSL2' :
            iNo = button.iNo
            if self.nodeCtxs[ iNo ].webSock.props.active and not button.props.active :
                button.props.active = True
                return

    def on_choose_key_clicked( self, button ) :
        self.on_choose_file_clicked( button, True )

    def on_choose_cert_clicked( self, button ) :
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
        dialog = Gtk.FileChooserDialog(
            title="Please choose a file", parent=self, action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN,
            Gtk.ResponseType.OK,
        )

        self.add_filters(dialog, bKey)

        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            button.editBox.set_text( dialog.get_filename() )

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
            if confVals[ 'AuthInfo'] is not None :
                authInfo = confVals[ 'AuthInfo']
                if authInfo[ 'ServiceName'] is not None :
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
            if confVals[ 'AuthInfo'] is not None :
                authInfo = confVals[ 'AuthInfo']
                if authInfo[ 'Realm'] is not None :
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
            if confVals[ 'AuthInfo'] is not None :
                authInfo = confVals[ 'AuthInfo']
                if authInfo[ 'SignMessage'] is not None :
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
            if confVals[ 'kdcAddr'] is not None :
                strKdcIp = confVals[ 'kdcAddr']
        except Exception as err :
            pass

        kdcEditBox = Gtk.Entry()
        kdcEditBox.set_text(strKdcIp)
        grid.attach(kdcEditBox, startCol + 1, startRow + 5, 2, 1 )

        self.svcEdit = svcEditBox
        self.realmEdit = realmEditBox
        self.signCombo = signCombo
        self.kdcEdit = kdcEditBox

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
                    if len( self.svcEdit.get_text() ) == 0 :
                        return "Auth enabled, but service is empty"
                    if len( self.kdcEdit.get_text() ) == 0 :
                        return "Auth enabled, but kdc address is empty"
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
                    return "Ip address is '0.0.0.0'";
                if interf.webSock.props.active :
                    if len( interf.urlEdit.get_text() ) == 0 :
                        return "WebSocket enabled, but dest url is empty"
            addrSet.clear()
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
            self, 0,
            Gtk.MessageType.ERROR,
            Gtk.ButtonsType.OK,
            text )
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

            if nodeCtx.enabled.props.active :
                nodeCfg[ "Enabled" ] = "true"
            else :
                nodeCfg[ "Enabled" ] = "false"

            if nodeCtx.compress.props.active:
                nodeCfg[ "Compression" ] = "true"
            else:
                nodeCfg[ "Compression" ] = "false"

            if nodeCtx.sslCheck.props.active:
                nodeCfg[ "EnalbeSSL" ] = "true"
            else:
                nodeCfg[ "EnalbeSSL" ] = "false"

            strVal = nodeCtx.urlEdit.get_text()
            if nodeCtx.webSock.props.active :
                if len( strVal ) == 0 :
                    raise Exception("'WebSocket URL' cannot be empty") 
                nodeCfg[ "EnableWS" ] = "true"
                nodeCfg[ "DestURL" ] = strVal
            else :
                nodeCfg[ "EnableWS" ] = "false"
                nodeCfg[ "DestURL" ] = strVal

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -1

        return 0

    def ExportNodeCtxs( self ):
        try:
            nodeCtxs = self.nodeCtxs
            nodeCtxsFin = []
            if nodeCtxs is not None :
                for nodeCtx in nodeCtxs :
                    if nodeCtx.IsEmpty() :
                        continue
                    nodeCtxsFin.append( nodeCtx )

            numCtx = len( nodeCtxsFin )
            if numCtx == 0 :
                self.nodes.clear()
                return 0

            if self.nodes is not None :
                numCfg = len( self.nodes )
            else :
                numCfg = numCtx
                self.nodes = [ InitNodeCfg( 0 ) ] * numCtx

            diff = numCtx - numCfg
            if diff < 0 :
                for i in range( -diff ) :
                    self.nodes.pop()
            elif diff > 0 :
                self.nodes += [ InitNodeCfg( 0 ) ] * diff

            for i in range( numCtx ):
                ret = self.ExportNodeCtx(
                    nodeCtxsFin[ i ], self.nodes[ i ] )
                if ret < 0 :
                    return ret

        except Exception as err :
            text = "Failed to export node:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -1

        return 0

    def ExportFiles( self, path : str) :
        error = self.VerifyInput()
        if error != 'success' :
            text = "Error occurs : " + error
            self.DisplayError( text )
            return -1
        try:
            jsonFiles = self.jsonFiles;
            drvVal = jsonFiles[ 0 ][ 1 ]
            if len( self.keyEdit.get_text() ) > 0 :
                ports = drvVal['Ports']

            confVals = None
            for port in ports :
                if port[ 'PortClass'] == 'RpcOpenSSLFido' :
                    sslFiles = port[ 'Parameters']
                    if sslFiles is None :
                        continue
                    sslFiles[ "KeyFile"] = self.keyEdit.get_text()
                    sslFiles[ "CertFile"] = self.certEdit.get_text()

                if port[ 'PortClass'] == 'RpcTcpBusPort' :
                    confVals = port[ 'Parameters']

            ifs = []
            for elem in self.ifctx :
                if not elem.IsEmpty() :
                    ifs.append( elem )

            if ifs is None:
                raise Exception("no valid configurations") 

            diff = len( ifs ) - len( confVals )
            if diff < 0 :
                for i in range( -diff ):
                    confVals.pop()
            elif diff > 0 :
                for i in range( diff ):
                    confVals.append( deepcopy( confVals[ 0 ] ) )

            for i in range( len( ifs ) ):  
                curVals = ifs[ i ]
                elem = confVals[ i ]
                elem[ 'BindAddr' ] = curVals.ipAddr.get_text()
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
                else:
                    elem[ 'EnableWS' ] = 'false'

            drvPath = path + "/driver.json"
            fp = open(drvPath, "w")
            json.dump( drvVal, fp, indent=4)
            fp.close()

            rtDesc = jsonFiles[ 2 ][ 1 ]
            svrObjs = rtDesc[ 'Objects' ]
            if svrObjs is not None and len( svrObjs ) > 0 :
                for svrObj in svrObjs :
                    if svrObj[ 'ObjectName'] == 'RpcRouterBridgeAuthImpl' :
                        if len( self.realmEdit.get_text() ) == 0:
                            break;
                        authInfo = dict();
                        authInfo[ 'Realm' ] = self.realmEdit.get_text()
                        authInfo[ 'ServiceName' ] = self.svcEdit.get_text()
                        authInfo[ 'AuthMech' ] = 'krb5'
                        tree_iter = self.signCombo.get_active_iter()
                        if tree_iter is not None:
                            model = self.signCombo.get_model()
                            row_id, name = model[tree_iter][:2]
                        if row_id == 1 :
                            authInfo[ 'SignMessage' ] = 'false'
                        else:
                            authInfo[ 'SignMessage' ] = 'true'

                        svrObj[ 'AuthInfo'] = authInfo;
                        self.ExportNodeCtxs();
                        break

            rtauPath = path + '/rtauth.json'
            fp = open(rtauPath, "w")
            json.dump( rtDesc, fp, indent = 4 )
            fp.close()

            rtDesc = jsonFiles[ 1 ][ 1 ]
            svrObjs = rtDesc[ 'Objects' ]
            if svrObjs is not None and len( svrObjs ) > 0 :
                for svrObj in svrObjs :
                    if svrObj[ 'ObjectName'] == 'RpcRouterBridgeImpl' :
                        svrObj[ 'Nodes' ] = self.nodes;
                        break
            rtPath = path + '/router.json'
            fp = open(rtPath, "w")
            json.dump( rtDesc, fp, indent = 4 )
            fp.close()

            apVal = jsonFiles[ 3 ][ 1 ]
            proxies = apVal['Objects']
            if proxies is None or len( proxies ) == 0:
                raise Exception("bad content in authprxy.json") 

            if0 = ifs[ 0]
            for proxy in proxies :
                objName = proxy[ 'ObjectName' ]
                if objName == 'K5AuthServer' or objName == 'KdcChannel':
                    elem = proxy
                    elem[ 'IpAddress' ] = if0.ipAddr.get_text()
                    elem[ 'PortNumber' ] = if0.port.get_text()
                    if if0.compress.props.active :
                        elem[ 'Compression' ] = 'true'
                    else:
                        elem[ 'Compression' ] = 'false'

                    if if0.sslCheck.props.active :
                        elem[ 'EnableSSL' ] = 'true'
                    else:
                        elem[ 'EnableSSL' ] = 'false'

                    if objName == 'K5AuthServer' :
                        if if0.authCheck.props.active :
                            elem[ 'HasAuth' ] = 'true'
                        else:
                            elem[ 'HasAuth' ] = 'false'
                    else :
                        if 'AuthInfo' in elem :
                            authInfo = elem[ 'AuthInfo' ]
                        else:
                            authInfo = dict()

                        if len( self.realmEdit.get_text() ) != 0:
                            authInfo[ 'Realm' ] = self.realmEdit.get_text()
                            authInfo[ 'ServiceName' ] = self.svcEdit.get_text()
                            authInfo[ 'AuthMech' ] = 'krb5'
                            tree_iter = self.signCombo.get_active_iter()
                            if tree_iter is not None:
                                model = self.signCombo.get_model()
                                row_id, name = model[tree_iter][:2]
                            if row_id == 1 :
                                authInfo[ 'SignMessage' ] = 'false'
                            else :
                                authInfo[ 'SignMessage' ] = 'true'

                    if if0.webSock.props.active :
                        elem[ 'EnableWS' ] = 'true'
                        elem[ 'DestURL'] = if0.urlEdit.get_text()
                    else:
                        elem[ 'EnableWS' ] = 'false'

                elif objName == 'KdcRelayServer' :
                    if len( self.kdcEdit.get_text() ) != 0:
                        proxy[ 'IpAddress' ] = self.kdcEdit.get_text()
                    break

            apPath = path + "/authprxy.json"
            fp = open( apPath, "w" )
            json.dump( apVal, fp, indent = 4  )
            fp.close()

            rtPath = path + "/router.json"
            fp = open( rtPath, 'w')
            json.dump( jsonFiles[1][1], fp, indent = 4  )
            fp.close()

        except Exception as err :
            text = "Failed to export files:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -1

        return 0

    def UpdateConfig( self ) :
        path = '/tmp'
        err = self.ExportFiles( path )
        if err < 0 :
            return err
        try:
            move( path + '/driver.json', self.jsonFiles[ 0 ][ 0 ] )
            move( path + '/router.json', self.jsonFiles[ 1 ][ 0 ] )
            move( path + '/rtauth.json', self.jsonFiles[ 2 ][ 0 ] )
            move( path + '/authprxy.json', self.jsonFiles[ 3 ][ 0 ] )
        except Exception as err :
            text = "Failed to export files:" + str( err )
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
            self.DisplayError( text, second_text )
            return -1
        return 0
        

win = ConfigDlg(True)
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
        path = '.'
        if response == Gtk.ResponseType.OK:
            path = dialog.get_filename()
        dialog.destroy()
        win.ExportFiles(path)
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
        dialog.destroy()
        win.ReinitDialog( path )
        continue

    break
    
win.destroy()
