import gi
import json
import os 

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

def LoadConfigFiles() :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    paths = list()
    curDir = dir_path;
    paths.append( curDir + "/../../etc/rpcf" )
    paths.append( "/etc/rpcf")

    paths.append( curDir + "/../ipc" )
    paths.append( curDir + "/../rpc/router" )
    paths.append( curDir + "/../rpc/security" )
    
    jsonvals = list()
    for strPath in paths :
        try:
            drvfile = strPath + "/driver.json"
            fp = open( drvfile, "r" )
            jsonvals.append( [drvfile, json.load(fp) ] )
            fp.close()
        except OSError as err :
            pass
        try:
            rtfile = strPath + "/router.json"
            fp = open( rtfile, "r" )
            jsonvals.append( [rtfile, json.load(fp)] )
            fp.close()
        except OSError as err :
            pass

        try:
            rtaufile = strPath + "/rtauth.json"
            fp = open( rtaufile, "r" )
            jsonvals.append( [rtaufile, json.load(fp)])
            fp.close()
        except OSError as err :
            pass
        
        try:
            auprxyfile = strPath + "/authprxy.json"
            fp = open( auprxyfile, "r" )
            jsonvals.append( [auprxyfile, json.load(fp)] )
            fp.close()
        except OSError as err :
            pass            


    return jsonvals

def RetrieveInfo() :
    confVals = dict()
    jsonFiles = LoadConfigFiles()
    if len( jsonFiles ) == 0 :
        return
    drvVal = jsonFiles[ 0 ][1]
    ports = drvVal['Ports']
    for port in ports :
        if port[ 'PortClass'] == 'RpcOpenSSLFido' :
            sslFiles = port[ 'Parameters']
            if sslFiles is None :
                continue
            confVals["keyFile"] = sslFiles[ "KeyFile"]
            confVals["certFile"] = sslFiles[ "CertFile"]
        
        if port[ 'PortClass'] == 'RpcTcpBusPort' :
            connParams = port[ 'Parameters']
            if connParams is None :
                continue
            confVals[ "connParams"] = [ *connParams ]

    apVal = jsonFiles[ 3 ][ 1 ]
    proxies = apVal['Objects']
    if proxies is None :
        return confVals

    for proxy in proxies :
        if proxy[ 'ObjectName'] == 'KdcRelayServer' :
            try:
                if proxy[ 'IpAddress'] is not None :
                    confVals[ 'kdcAddr'] = proxy[ 'IpAddress']
            except Exception as err :
                pass
        if proxy[ 'ObjectName'] == 'KdcChannel' :
            try:
                authInfo = proxy[ 'AuthInfo']
                confVals[ 'authInfo'] = authInfo
            except Exception as err :
                pass
    
    return confVals    

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
        self.startRow = 0
        self.rowCount = 0
        self.Clear()

    def Clear( self ) :
        self.ipAddr = None
        self.port = None
        self.compress = None
        self.webSock = None
        self.urlEdit = None
        self.certEdit = None
        self.keyEdit = None
        self.sslCheck = None
        self.svcEdit = None
        self.realmEdit = None
        self.signCombo = None
        self.kdcEdit = None
        self.startRow = 0
        self.rowCount = 0
    
    def IsEmpty( self ) :
        return self.ipAddr is None
        
class ConfigDlg(Gtk.Dialog):
    def __init__(self, bServer):
        Gtk.Dialog.__init__(self, title="Config RPC Server Router", flags=0)
        self.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK )
        self.bServer = bServer
        if not bServer :
            self.get_header_bar().set_title( "Config RPC Proxy Router" )

        confVals = RetrieveInfo()
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
        stack.set_transition_type(Gtk.StackTransitionType.SLIDE_LEFT_RIGHT)
        stack.set_transition_duration(1000)

        gridNet = Gtk.Grid();
        gridNet.set_row_homogeneous( True )
        gridNet.props.row_spacing = 6
        i = 0
        for interf in self.ifctx :
            row = GetGridRows( gridNet )
            interf.startRow = row
            self.InitNetworkPage( gridNet, 0, row, confVals, i )
            interf.rowCount = GetGridRows( gridNet ) - row
            i += 1
        self.gridNet = gridNet

        stack.add_titled(gridNet, "GridConn", "Connection")

        label = Gtk.Label()
        label.set_text("Multihop Downstream Nodes")
        gridMisc = Gtk.Grid();
        gridMisc.add(label)
        stack.add_titled(gridMisc, "GridMisc", "Misc")

        stack_switcher = Gtk.StackSwitcher()
        stack_switcher.set_stack(stack)
        vbox.pack_start(stack_switcher, False, True, 0)

        scrolledWin = Gtk.ScrolledWindow(hexpand=True, vexpand=True)
        scrolledWin.set_border_width(10)
        scrolledWin.set_policy(
            Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.ALWAYS)
        scrolledWin.add_with_viewport( stack )
        vbox.pack_start(scrolledWin, True, True, 0)

        self.set_border_width(10)
        self.set_default_size(600, 740)
        box = self.get_content_area()
        box.add(vbox)
        self.show_all()
 

    def InitNetworkPage( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        row = GetGridRows( grid )
        if self.bServer :
            labelIfNo = Gtk.Label()
            strIfNo = "Interface " + str( ifNo ) + " :"
            labelIfNo.set_text( strIfNo )
            grid.attach(labelIfNo, startCol, startRow, 1, 1 )
            if ifNo > 0 :
                removeBtn = Gtk.Button.new_with_label("Remove interface")
                removeBtn.ifNo = ifNo
                removeBtn.connect("clicked", self.on_remove_if_clicked)
                grid.attach(removeBtn, startCol + 2, startRow, 1, 1 )
            else :
                addBtn = Gtk.Button.new_with_label("Add interface")
                addBtn.connect("clicked", self.on_add_if_clicked)
                grid.attach(addBtn, startCol + 2, startRow, 1, 1 )

            startRow = GetGridRows( grid )
        
        labelIp = Gtk.Label()
        if self.bServer :
            labelIp.set_text("Binding IP: ")
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
            labelPort.set_text("Listening Port: ")
        else:
            labelPort.set_text("Server Port: ")

        labelPort.set_xalign(1)
        grid.attach(labelPort, startCol + 0, startRow + 1, 1, 1 )

        portNum = 4312
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

        self.AddSSLOptions( grid, startCol + 0, startRow + 2, confVals, ifNo )
        self.AddAuthOptions( grid, startCol + 0, startRow + 5, confVals, ifNo )

        rows = GetGridRows( grid )
        self.AddWebSockOptions( grid, startCol + 0, rows, confVals, ifNo)

        rows = GetGridRows( grid )
        labelCompress = Gtk.Label()
        labelCompress.set_text("Compress: ")
        labelCompress.set_xalign(1)
        grid.attach(labelCompress, startCol + 0, rows, 1, 1 )

        compressCheck = Gtk.CheckButton(label="")
        compressCheck.connect(
            "toggled", self.on_button_toggled, "Compress")
        compressCheck.ifNo = ifNo
        grid.attach( compressCheck, startCol + 1, rows, 1, 1)
        self.ifctx[ ifNo ].compress = compressCheck

    def AddWebSockOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        labelWebSock = Gtk.Label()
        labelWebSock.set_text("WebSocket: ")
        labelWebSock.set_xalign(1)
        grid.attach(labelWebSock, startCol + 0, startRow + 0, 1, 1 )

        bActive = True
        try :
            if confVals[ 'connParams'] is not None :
                param0 = confVals[ 'connParams'][ ifNo ]
                if param0[ 'EnableWS'] is not None :
                    strUrl = param0[ 'EnableWS']
                    if strUrl == 'false' :
                        bActive = False
        except Exception as err :
            pass

        webSockCheck = Gtk.CheckButton(label="")
        webSockCheck.connect(
            "toggled", self.on_button_toggled, "WebSock")
        webSockCheck.ifNo = ifNo
        grid.attach( webSockCheck, startCol + 1, startRow + 0, 1, 1)
        self.ifctx[ ifNo ].webSock = webSockCheck

        labelUrl = Gtk.Label()
        labelUrl.set_text("WebSocket URL: ")
        labelUrl.set_xalign(1)
        grid.attach(labelUrl, startCol + 1, startRow + 1, 1, 1 )

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
        grid.attach(urlEdit, startCol + 2, startRow + 1, 1, 1 )

        self.ifctx[ ifNo ].urlEdit = urlEdit

    def AddSSLOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        labelSSL = Gtk.Label()
        labelSSL.set_text("SSL: ")
        labelSSL.set_xalign(1)
        grid.attach(labelSSL, startCol, startRow, 1, 1 )

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

        sslCheck = Gtk.CheckButton(label="")
        sslCheck.set_active( bActive )
        sslCheck.connect(
            "toggled", self.on_button_toggled, "SSL")
        sslCheck.ifNo = ifNo
        grid.attach( sslCheck, startCol + 1, startRow, 1, 1 )

        labelKey = Gtk.Label()
        labelKey.set_text("Key File: ")
        labelKey.set_xalign(.618)
        grid.attach(labelKey, startCol + 1, startRow + 1, 1, 1 )

        keyEditBox = Gtk.Entry()
        strKey = ""
        try:
            if confVals[ 'keyFile'] is not None :
                strKey = confVals[ 'keyFile']
        except Exception as err :
            pass            
        keyEditBox.set_text(strKey)
        grid.attach(keyEditBox, startCol + 2, startRow + 1, 1, 1 )

        keyBtn = Gtk.Button.new_with_label("...")
        keyBtn.connect("clicked", self.on_choose_key_clicked)
        keyBtn.ifNo = ifNo
        grid.attach(keyBtn, startCol + 3, startRow + 1, 1, 1 )

        labelCert = Gtk.Label()
        labelCert.set_text("Cert File: ")
        labelCert.set_xalign(.618)
        grid.attach(labelCert, startCol + 1, startRow + 2, 1, 1 )

        certEditBox = Gtk.Entry()
        strCert = ""
        
        try:
            if confVals[ 'certFile'] is not None :
                strCert = confVals[ 'certFile']    
        except Exception as err :
            pass    
        certEditBox.set_text(strCert)
        grid.attach(certEditBox, startCol + 2, startRow + 2, 1, 1 )

        certBtn = Gtk.Button.new_with_label("...")
        certBtn.connect("clicked", self.on_choose_cert_clicked)
        certBtn.ifNo = ifNo
        grid.attach(certBtn, startCol + 3, startRow + 2, 1, 1 )

        keyEditBox.set_sensitive( bActive )
        certEditBox.set_sensitive( bActive )
        keyBtn.set_sensitive( bActive )
        certBtn.set_sensitive( bActive )

        self.ifctx[ ifNo ].certEdit = certEditBox
        self.ifctx[ ifNo ].keyEdit = keyEditBox
        self.ifctx[ ifNo ].sslCheck = sslCheck
        sslCheck.keyBtn = keyBtn
        sslCheck.certBtn = certBtn
        keyBtn.editBox = keyEditBox
        certBtn.editBox = certEditBox


    def on_button_toggled( self, button, name ):
        print( name )
        ifNo = button.ifNo
        if name == 'SSL' :
            if self.ifctx[ ifNo ].webSock.props.active and not button.props.active :
                button.props.active = True
                return
            self.ifctx[ ifNo ].certEdit.set_sensitive( button.props.active )
            self.ifctx[ ifNo ].keyEdit.set_sensitive( button.props.active )
            button.certBtn.set_sensitive( button.props.active )
            button.keyBtn.set_sensitive( button.props.active )
        elif name == 'Auth' :
            self.ifctx[ ifNo ].svcEdit.set_sensitive( button.props.active )
            self.ifctx[ ifNo ].realmEdit.set_sensitive( button.props.active )
            self.ifctx[ ifNo ].signCombo.set_sensitive( button.props.active )
            self.ifctx[ ifNo ].kdcEdit.set_sensitive( button.props.active )
        elif name == 'WebSock' :
            self.ifctx[ ifNo ].urlEdit.set_sensitive( button.props.active )
            if not self.ifctx[ ifNo ].webSock.props.active :
                return
            self.ifctx[ ifNo ].certEdit.set_sensitive( button.props.active )
            self.ifctx[ ifNo ].keyEdit.set_sensitive( button.props.active )
            self.ifctx[ ifNo ].sslCheck.set_active(button.props.active)
            button.certBtn.set_sensitive( button.props.active )
            button.keyBtn.set_sensitive( button.props.active )

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

    def AddAuthOptions( self, grid:Gtk.Grid, startCol, startRow, confVals : dict, ifNo = 0 ) :
        labelAuth = Gtk.Label()
        labelAuth.set_text("Auth: ")
        labelAuth.set_xalign(1)
        grid.attach(labelAuth, startCol, startRow, 1, 1 )

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

        authCheck = Gtk.CheckButton(label="")
        authCheck.set_active( bActive )
        authCheck.connect(
            "toggled", self.on_button_toggled, "Auth")
        authCheck.ifNo = ifNo
        grid.attach( authCheck, startCol + 1, startRow, 1, 1 )

        labelMech = Gtk.Label()
        labelMech.set_text("Auth Mech: ")
        labelMech.set_xalign(.618)
        grid.attach(labelMech, startCol + 1, startRow + 2, 1, 1 )

        mechList = Gtk.ListStore()
        mechList = Gtk.ListStore(int, str)
        mechList.append([1, "Kerberos"])
    
        mechCombo = Gtk.ComboBox.new_with_model_and_entry(mechList)
        mechCombo.set_entry_text_column(1)
        mechCombo.set_active( 0 )
        mechCombo.set_sensitive( False )
        grid.attach(mechCombo, startCol + 2, startRow + 2, 1, 1 )

        labelSvc = Gtk.Label()
        labelSvc.set_text("Service Name: ")
        labelSvc.set_xalign(.618)
        grid.attach(labelSvc, startCol + 1, startRow + 3, 1, 1 )

        strSvc = ""
        try:
            if confVals[ 'authInfo'] is not None :
                authInfo = confVals[ 'authInfo']
                if authInfo[ 'ServiceName'] is not None :
                    strSvc = authInfo[ 'ServiceName']
        except Exception as err :
            pass

        svcEditBox = Gtk.Entry()
        svcEditBox.set_text(strSvc)
        grid.attach(svcEditBox, startCol + 2, startRow + 3, 2, 1 )

        labelRealm = Gtk.Label()
        labelRealm.set_text("Realm: ")
        labelRealm.set_xalign(.618)
        grid.attach(labelRealm, startCol + 1, startRow + 4, 1, 1 )

        strRealm = ""
        try:
            if confVals[ 'authInfo'] is not None :
                authInfo = confVals[ 'authInfo']
                if authInfo[ 'Realm'] is not None :
                    strRealm = authInfo[ 'Realm']
        except Exception as err :
            pass
        realmEditBox = Gtk.Entry()
        realmEditBox.set_text(strRealm)
        grid.attach(realmEditBox, startCol + 2, startRow + 4, 2, 1 )

        labelSign = Gtk.Label()
        labelSign.set_text("Sign/Encrypt: ")
        labelSign.set_xalign(.618)
        grid.attach(labelSign, startCol + 1, startRow + 5, 1, 1 )

        idx = 1
        try:
            if confVals[ 'authInfo'] is not None :
                authInfo = confVals[ 'authInfo']
                if authInfo[ 'SignMessage'] is not None :
                    strSign = authInfo[ 'ServiceName']
                    if strSign == 'true' :
                        idx = 0
        except Exception as err :
            pass

        signMethod = Gtk.ListStore()
        signMethod = Gtk.ListStore(int, str)
        signMethod.append([1, "Encrypt Messages"])
        signMethod.append([2, "Sign Messages"])

        signCombo = Gtk.ComboBox.new_with_model_and_entry(signMethod)
        signCombo.connect("changed", self.on_sign_msg_changed)
        signCombo.ifNo = ifNo
        signCombo.set_entry_text_column(1)
        signCombo.set_active(idx)

        grid.attach( signCombo, startCol + 2, startRow + 5, 1, 1 )

        labelKdc = Gtk.Label()
        labelKdc.set_text("KDC IP address: ")
        labelKdc.set_xalign(.618)
        grid.attach(labelKdc, startCol + 1, startRow + 6, 1, 1 )

        strKdcIp = ""
        try:
            if confVals[ 'kdcAddr'] is not None :
                strKdcIp = confVals[ 'kdcAddr']
        except Exception as err :
            pass

        kdcEditBox = Gtk.Entry()
        kdcEditBox.set_text(strKdcIp)
        grid.attach(kdcEditBox, startCol + 2, startRow + 6, 2, 1 )

        self.ifctx[ ifNo ].svcEdit = svcEditBox
        self.ifctx[ ifNo ].realmEdit = realmEditBox
        self.ifctx[ ifNo ].signCombo = signCombo
        self.ifctx[ ifNo ].kdcEdit = kdcEditBox

        svcEditBox.set_sensitive( authCheck.props.active )
        realmEditBox.set_sensitive( authCheck.props.active )
        signCombo.set_sensitive( authCheck.props.active )

    def on_sign_msg_changed(self, combo) :
        tree_iter = combo.get_active_iter()
        if tree_iter is not None:
            model = combo.get_model()
            row_id, name = model[tree_iter][:2]
            print("Selected: ID=%d, name=%s" % (row_id, name))
        else:
            entry = combo.get_child()
            print("Entered: %s" % entry.get_text())


win = ConfigDlg(True)
win.connect("close", Gtk.main_quit)
response = win.run()
if response == Gtk.ResponseType.OK:
    print("The OK button was clicked")
win.destroy()
