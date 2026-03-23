import json
import re

import urwid
import gettext
import os
import errno
import getopt
import sys
from urwid.widget.constants import BOX_SYMBOLS
from updcfg import CheckIpAddr, IsFeatureEnabled, tempname
from updwscfg import rpcf_system, GetDistName
from updk5cfg import *
from functools import partial
import subprocess
import platform
import locale

from pathlib import Path

# Initialize gettext
try:
    domain = 'rpcfgtui'
    paths = [ "/usr/local/share/locale", "/usr/share/locale" ]
    bFound = False
    curLang, codeset=locale.getlocale()
    if codeset.upper() != 'UTF-8':
        raise Exception( f"Warnig '{codeset}' is not supported yet" )

    for path in paths:
        if os.access( f"{path}/{curLang}/LC_MESSAGES/{domain}.mo", os.R_OK ):
            bFound = True
            break
    if bFound:
        gettext.bindtextdomain(domain, path,)
        gettext.textdomain(domain)
        lang = gettext.translation(domain, path, languages=['zh_CN'], fallback=True, )
        lang.install()
    else:
        raise Exception( f"Warnig cannot find locale file '{domain}.mo' for {curLang}.{codeset}" )
except Exception as err:
    print(err)
    pass

_ = gettext.gettext

def is_valid_domain(domain):
    # check if the domain matches the pattern for valid domain names (e.g., example.com, sub.example.co.uk)
    pattern = re.compile(
        r'^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\.)+[A-Za-z]{2,6}$'
    )
    ret = pattern.match(domain)
    return ret is not None
import urwid

bw_pallete = [
    ('body', 'light gray', 'black', 'standout'),  # Yellow text on blue background
    ('header', 'white', 'black', 'bold'),    # White text on blue background
    ('button normal', 'light gray', 'black', 'standout'),  # Light gray text on blue background
    ('button select', 'black', 'light gray'),    # Yellow text on red background
    ('button focus', 'black', 'light gray', 'standout'),    # Yellow text on red background
    ('divider', 'light gray', 'light gray'),      # Divider color
    ('radio normal', 'light gray', 'black'),  # Light gray text on blue background
    ('radio select', 'light gray', 'dark gray'),      # Yellow text on blue background
]

bolland_palette = [
    ('body', 'yellow', 'dark blue', 'standout'),  # Yellow text on blue background
    ('header', 'white', 'dark blue', 'bold'),    # White text on blue background
    ('button normal', 'light gray', 'dark blue', 'standout'),  # Light gray text on blue background
    ('button select', 'yellow', 'dark red'),    # Yellow text on red background
    ('button focus', 'yellow', 'dark red', 'standout'),    # Yellow text on red background
    ('divider', 'light gray', 'light gray'),      # Divider color
    ('radio normal', 'light gray', 'dark blue'),  # Light gray text on blue background
    ('radio select', 'light gray', 'dark green'),      # Yellow text on blue background
]
palette = bw_pallete

class PasswordDialog:
    def __init__(self, parent ):
        self.password = None
        self.create_dialog()
        self.parent = parent 
        self.callback = None
    
    def create_dialog(self):
        self.password_edit = urwid.Edit(caption=_("Password: "), mask='*')
        
        ok_button = urwid.Button(_("OK"), align='center', on_press=self.on_ok)
        cancel_button = urwid.Button(_("Cancel"), align='center', on_press=self.on_cancel)
        
        pile = urwid.Pile([
            urwid.Divider(),
            urwid.GridFlow( 
                [ self.password_edit, ],
                cell_width=50,  # Adjust button width
                h_sep=2,        # Horizontal space between buttons
                v_sep=0,        # No vertical space
                align='center'  # Center-align buttons
            ),
            urwid.Divider(),
            urwid.GridFlow(
                [ urwid.AttrMap(ok_button, 'button normal',  focus_map='button focus'),
                  urwid.AttrMap(cancel_button, 'button normal',  focus_map='button focus')
                ],
                cell_width=10,  # Adjust button width
                h_sep=2,        # Horizontal space between buttons
                v_sep=0,        # No vertical space
                align='center'  # Center-align buttons
            )
        ])
        
        lineBox = createDoubleLineBox( pile, _("Input Password"))
        lineBox = urwid.Padding( lineBox, align='center', width=("relative", 61.8))
        dlg = urwid.AttrMap( lineBox, 'body')
 
        self.dialog = urwid.Padding(
            urwid.AttrMap( urwid.Filler(dlg, valign='middle', ), 'body' ),
            align='center',
            ) 
        
    def on_ok(self, button):
        self.password = self.password_edit.get_edit_text()
        if self.password == "":
            self.password = None
        elif self.parent:
            self.parent.main_loop.set_alarm_in(0, lambda loop, data: self.callback(self.password))
        if self.parent:
            self.parent.go_back(button)
            self.parent.main_loop.unhandled_input = self.parent.unhandled_input
    
    def on_cancel(self, button):
        self.password = None
        if self.parent:
            self.parent.go_back(button)
            self.parent.main_loop.unhandled_input = self.parent.unhandled_input

    def runDlg(self):
        if self.parent:
            self.parent.menu_stack.append(
                self.parent.main_widget.body)
            self.parent.main_widget.body = self.dialog
            self.parent.main_loop.unhandled_input = self.unhandled_input

    def unhandled_input(self, key):
        if key == 'esc' or key == 'q':
            self.on_cancel(None)  # Treat ESC as cancel
        elif key == 'enter':
            self.on_ok(None)  # Treat Enter as OK
    
def createDoubleLineBox( widget, strTitle ):
    return urwid.LineBox(widget,
        title=strTitle,
        tline=BOX_SYMBOLS.DOUBLE.HORIZONTAL,
        tlcorner=BOX_SYMBOLS.DOUBLE.TOP_LEFT,
        trcorner=BOX_SYMBOLS.DOUBLE.TOP_RIGHT,
        lline= BOX_SYMBOLS.DOUBLE.VERTICAL,
        blcorner=BOX_SYMBOLS.DOUBLE.BOTTOM_LEFT,
        rline= BOX_SYMBOLS.DOUBLE.VERTICAL,
        bline=BOX_SYMBOLS.DOUBLE.HORIZONTAL,
        brcorner=BOX_SYMBOLS.DOUBLE.BOTTOM_RIGHT,
        )
class MenuDialog:
    def __init__(self):
        self.main_loop = None
        self.menu_stack = []  # stack to track widget z-order
        self.setup_ui()
        self.initCfg = json.loads(os.popen('rpcfctl geninitcfg').read())   
        self.oConns = []  # widgets for each connection configuration, used for updating config and reverting changes if user cancels
        self.oSecWidgets = []  # widgets on security settings page, used for reverting changes if user cancels
        self.authMechs = []
        self.bServer = True
        self.key_dialog = None
        self.pkgPath = None  # package path where to find the DEB package or RPM package
        self.destPath = None # destination path to store the installer
        self.oNodes = [] # widgets for each node configuration, used for updating config and reverting changes if user cancels
        self.iNodeIdx = len( self.initCfg.get("Multihop", 0))

    def setup_ui(self):
        # main menu
        menu_items = [
            urwid.Divider(' '),
            urwid.AttrWrap( urwid.Button(_("Network Settings"), on_press=self.showNetworkSettings), "button normal", "button focus" ),
            urwid.Divider(' '),
            urwid.AttrWrap( urwid.Button(_("Security Settings"), on_press=self.showSecuritySettings), "button normal", "button focus" ),
            urwid.Divider(' '),
            urwid.AttrWrap( urwid.Button(_("Multihop Settings"), on_press=self.showMultihopSettings), "button normal", "button focus" ),
            urwid.Divider(' '),
            urwid.AttrWrap( urwid.Button(_("Configuration List"), on_press=self.config_list), "button normal", "button focus" ),
            urwid.Divider(' '),
            urwid.AttrWrap( urwid.Button(_("Exit"), on_press=self.exit_program), "button normal", "button focus" )
        ]
        
        # create the boarder frame
        list_walker = urwid.SimpleFocusListWalker(menu_items)
        list_box = urwid.ListBox(list_walker)
        line_box = createDoubleLineBox(list_box,
            _("Configure Rpc-Framework") )
        
        #header = urwid.Text(_("Configure Rpc-Framework"), align='center')
        #header = urwid.AttrWrap(header, 'header')
        
        self.main_widget = urwid.Frame(
            urwid.AttrWrap(line_box, 'body'),
            #header=header,
            footer=urwid.Text(_("Use arrow keys to navigate, Enter to confirm"), align='center')
        )
    def remove_connection(self, button, idx):    
        # remove the connection configuration at the specified index
        oConnParams = self.initCfg.get('Connections', [])
        if 0 <= idx < len(oConnParams):
            for idx2, oConn in enumerate(self.oConns):
                if self.getConnWidgetIndex( oConn ) == idx:
                    del self.oConns[idx2]
                # refresh the network settings page to reflect the removed connection
            oConn = oConnParams[idx] if idx < len(oConnParams) else None
            if oConn:
                oConn["deleted"] = True  # Mark the connection as deleted in the config
            self.showNetworkSettings(button)

    def add_connection(self, button):
        # add a new connection configuration (default values)
        oConnParams = self.initCfg.get('Connections', [])
        oConnParams.append({
            'IpAddress': '127.0.0.1',
            'BindTo': 'false',
            'AddrFormat': 'ipv4',
            'PortNumber': '',
            'PdoClass': 'TcpStreamPdo2',
            'Protocol': 'native',
            'ConnRecover': 'true',
            'RouterPath': '/',
            'Compression': 'false',
            'EnableSSL': 'false',
            'EnableWS': 'false',
            'HasAuth': 'false',
            'EnableBPS': 'true',
            'added': True  # Mark the connection as newly added in the config
        })
        # refresh the network settings page to show the new connection
        self.showNetworkSettings(button)

    def getConnWidgetIndex( self, oConn ):
        for widget in oConn:
            if isinstance( widget, urwid.Text ):
                label, style = widget.get_text()
                words = label.split()
                return int(words[-1]) - 1
        return None 

    def update_network_config(self, button):
        # walk through the widgets in oConns and update the initCfg with the new values
        try:
            oConnParams = self.initCfg.get('Connections', [])
            for conn_widgets in self.oConns:
                idx = self.getConnWidgetIndex( conn_widgets )
                if idx is None:
                    raise Exception(_("Error internal error"))
                if idx < 0 or idx >= len(oConnParams):
                    continue
                conn_params = oConnParams[idx]
                if conn_params.get("deleted", False):
                    continue
                for widget in conn_widgets:
                    if isinstance(widget, urwid.Edit):
                        label = widget.caption.strip()
                        if label == _("IP Address:"):
                            ipAddr = widget.edit_text.strip()
                            if CheckIpAddr(ipAddr) is None and not is_valid_domain(ipAddr):
                                raise ValueError(_("Invalid IP address")+": {}".format(ipAddr))
                            conn_params['IpAddress'] = widget.edit_text.strip()
                        elif label == _("Port Number:"):
                            conn_params['PortNumber'] = widget.edit_text.strip()
                        elif label == _("Router Path:"):
                            conn_params['RouterPath'] = widget.edit_text.strip()
                        elif label == _("WebSocket URL:"):
                            conn_params['DestURL'] = widget.edit_text.strip()
                    elif isinstance(widget, urwid.CheckBox):
                        label = widget.get_label().strip()
                        state = 'true' if widget.get_state() else 'false'
                        if label == _("Server bind to this Interface"):
                            conn_params['BindTo'] = state
                        elif label == _("Enable Compression"):
                            conn_params['Compression'] = state
                        elif label == _("Enable SSL"):
                            conn_params['EnableSSL'] = state
                        elif label == _("Enable WebSockets"):
                            conn_params['EnableWS'] = state
                        elif label == _("Enable Authentication"):
                            conn_params['HasAuth'] = state
                        elif label == _("Enable Flow Control"):
                            conn_params['EnableBPS'] = state
            for conn in oConnParams[:]:
                if conn.get( 'deleted', False):
                    oConnParams.remove(conn)  # remove the newly added connection from config
            for conn in oConnParams[:]:
                if conn.get( 'added', False):
                    conn.pop('added')  # remove the added mark for display purposes
        except Exception as e:
            self.show_message(_("Failed to update network configuration") + ": {}".format(str(e)))
            return
        self.oConns.clear()  # clear current widgets
        self.go_back( button )  # return to previous menu after updating config

    def revertToNetworkSettings(self, network_widgets):
        # revert the network settings page to the previous state (before changes)
        oConnParams = self.initCfg.get('Connections', [])
        self.oConns.clear()  # clear current widgets
        oConns = self.oConns
        for conn in oConnParams[:]:
            if conn.get( 'added', False):
                oConnParams.remove(conn)  # remove the newly added connection from config
        for idx, conn in enumerate(oConnParams):
            if conn.get('deleted', False):
               del conn['deleted']  # remove the deleted mark for display purposes
            conn_widgets = [
                urwid.Text(_("Network Connection") + " {}".format(idx + 1), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("IP Address: "), edit_text=conn.get('IpAddress', '')),
                urwid.CheckBox(_("Server bind to this Interface"), state=(conn.get('BindTo', False) == 'true')),
                urwid.Edit(_("Port Number: "), edit_text=conn.get('PortNumber', '')),
                urwid.Edit(_("Router Path: "), edit_text=conn.get('RouterPath', '/')),
                urwid.CheckBox(_("Enable Compression"), state=(conn.get('Compression', False) == 'true')),
                urwid.CheckBox(_("Enable SSL"), state=(conn.get('EnableSSL', False) == 'true')),
                urwid.CheckBox(_("Enable WebSockets"), state=(conn.get('EnableWS', False) == 'true')),
                urwid.Edit(_("WebSocket URL: "), edit_text=conn.get('DestURL', '')),
                urwid.CheckBox(_("Enable Authentication"), state=(conn.get('HasAuth', False) == 'true')),
                urwid.CheckBox(_("Enable Flow Control"), state=(conn.get('EnableBPS', False) == 'true')),
                urwid.AttrWrap( urwid.Button( _("Remove This Connection"), on_press=self.remove_connection, user_data=idx ), "button normal", "button focus" )
            ]
            oConns.append(conn_widgets)

    def showNetworkSettings(self, button):
        # Save the current menu state to the stack
        action = button.get_label() 
        if button and action == _("Network Settings") : 
            self.menu_stack.append(self.main_widget.body)
        # Create widgets for each set of network_items in oConns
        network_widgets = []
        oConnParams = self.initCfg.get('Connections', [])
        oConns = self.oConns
        if action == _("Network Settings") and not oConns:
            for idx, conn in enumerate(oConnParams):
                conn_widgets = [
                    urwid.Text((_("Network Connection") + " {}".format(idx + 1)), align='center'),
                    urwid.Divider('-'),
                    urwid.Edit(_("IP Address: "), edit_text=conn.get('IpAddress', '')),
                    urwid.CheckBox(_("Server bind to this Interface"), state=(conn.get('BindTo', False) == 'true')),
                    urwid.Edit(_("Port Number: "), edit_text=conn.get('PortNumber', '')),
                    urwid.Edit(_("Router Path: "), edit_text=conn.get('RouterPath', '/')),
                    urwid.CheckBox(_("Enable Compression"), state=(conn.get('Compression', False) == 'true')),
                    urwid.CheckBox(_("Enable SSL"), state=(conn.get('EnableSSL', False) == 'true')),
                    urwid.CheckBox(_("Enable WebSockets"), state=(conn.get('EnableWS', False) == 'true')),
                    urwid.Edit(_("WebSocket URL: "), edit_text=conn.get('DestURL', '')),
                    urwid.CheckBox(_("Enable Authentication"), state=(conn.get('HasAuth', False) == 'true')),
                    urwid.CheckBox(_("Enable Flow Control"), state=(conn.get('EnableBPS', False) == 'true')),
                    urwid.AttrWrap( urwid.Button( _("Remove This Connection"), on_press=self.remove_connection, user_data=idx ),  "button normal", "button focus", )
                ]
                oConns.append(conn_widgets)
        elif action == _("Add New Connection"):
            # Create a widget for the new connection
            conn = oConnParams[-1]
            idx = len(oConns)  # index of the new connection
            conn_widgets = [
                urwid.Text(_("Network Connection") + " {}".format(idx + 1), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("IP Address: "), edit_text=conn.get('IpAddress', '')),
                urwid.CheckBox(_("Server bind to this Interface"), state=(conn.get('BindTo', False) == 'true')),
                urwid.Edit(_("Port Number: "), edit_text=conn.get('PortNumber', '')),
                urwid.Edit(_("Router Path: "), edit_text=conn.get('RouterPath', '/')),
                urwid.CheckBox(_("Enable Compression"), state=(conn.get('Compression', False) == 'true')),
                urwid.CheckBox(_("Enable SSL"), state=(conn.get('EnableSSL', False) == 'true')),
                urwid.CheckBox(_("Enable WebSockets"), state=(conn.get('EnableWS', False) == 'true')),
                urwid.Edit(_("WebSocket URL: "), edit_text=conn.get('DestURL', '')),
                urwid.CheckBox(_("Enable Authentication"), state=(conn.get('HasAuth', False) == 'true')),
                urwid.CheckBox(_("Enable Flow Control"), state=(conn.get('EnableBPS', False) == 'true')),
                urwid.AttrWrap( urwid.Button( _("Remove This Connection"), on_press=self.remove_connection, user_data=idx ),  "button normal", "button focus", )
            ]
            oConns.append(conn_widgets)
        elif action == _("Remove This Connection"):
            pass

        # Create a list walker for the current set of network items
        widgetList = []
        for conn in oConns:
            widgetList.extend( conn )
        
        widgetList.append(urwid.Divider('-'))  # Add a divider between sets

        addConnButton = urwid.AttrWrap( urwid.Button(_("Add New Connection"), on_press=self.add_connection),  "button normal", "button focus", )
        returnButton = urwid.AttrWrap( urwid.Button(_("Return to Previous Level"), on_press=self.update_network_config) ,  "button normal", "button focus", ) # 修改为通用返回方法
        widgetList.extend( [ addConnButton, returnButton ] )
        list_walker = urwid.SimpleFocusListWalker( widgetList )
        list_box = urwid.ListBox(list_walker)
        network_widgets.append(urwid.AttrWrap(list_box, 'body'))
        # Create the Pile with the correct focus_item
        combined_pile = urwid.Pile(network_widgets)
        combined_pile.revert_func = lambda: self.revertToNetworkSettings(network_widgets)  # Store the current state for reverting

        # Set the combined pile as the body of the main widget
        self.main_widget.body = combined_pile

    def show_message(self, message):
        # display message dialog
        text = urwid.Text(message, align='center')
        ok_button = urwid.Padding(
            urwid.AttrMap(urwid.Button(_("OK"), align='center', on_press=self.close_message), 'button normal', focus_map='button focus'),
            align='center',
            width=15  # Set the width of the button
        )
        pile = urwid.Pile([text, urwid.Divider(), ok_button])

        self.menu_stack.append(self.main_widget.body)
        self.main_widget.body = urwid.AttrWrap(
            urwid.Filler(pile, valign='middle'), 'body'
        )

    def show_output( self, strCmd ):
        pass

    def ask_file_path(self):
        # Create input field and buttons
        input_edit = urwid.Edit(_("Enter file path: "), edit_text="initcfg_exported.json", align='center')
        ok_button = urwid.Button(_("OK"), align='center', on_press=self.confirm_file_path, user_data=input_edit)
        cancel_button = urwid.Button(_("Cancel"), on_press=self.close_message)

        # Arrange buttons horizontally
        buttons = urwid.GridFlow(
            [
                urwid.AttrMap(ok_button, 'button normal', focus_map='button focus'),
                urwid.AttrMap(cancel_button, 'button normal', focus_map='button focus')
            ],
            cell_width=10,  # Adjust button width
            h_sep=2,        # Horizontal space between buttons
            v_sep=0,        # No vertical space
            align='center'  # Center-align buttons
        )

        # Create dialog layout
        pile = urwid.Pile([input_edit, urwid.Divider(), buttons])

        # Save current state and display the dialog
        self.main_widget.body = urwid.AttrWrap(
            urwid.Filler(pile, valign='middle'), 'body'
        )

    def confirm_file_path(self, button, input_edit):
        # Get the entered file path
        file_path = input_edit.get_edit_text()
        # Perform actions with the file path (e.g., validation, saving, etc.)
        try:
            with open(file_path, 'w') as f:
                json.dump(self.initCfg, f, indent=4)
            self.go_back( None ) 
            self.show_message(_("Successfully export configuration to" ) + " {}".format(file_path))
        except Exception as e:
            self.show_message(_("Failed to export configuration" ) + ": {}".format(str(e)))

    def export_config(self, button): 
        # export to a JSON file for external use, such as debugging or manual editing; save current menu state to stack before showing message
        self.ask_file_path()

    def config_list(self, button):
        self.menu_stack.append(self.main_widget.body)
        
        initcfg = json.dumps(self.initCfg, indent=4)
        info_text = urwid.Text( initcfg , align='left')
        export_button1 = urwid.Button(_("Export Configuration"), align='center', on_press=self.export_config )
        export_button = urwid.Padding(
            urwid.AttrMap(export_button1, 'button normal', focus_map='button focus'),
            align='left',
            width='pack',
        )
        back_button1 = urwid.Button(_("Return to Previous Level"), align='center', on_press=self.go_back)
        back_button = urwid.Padding(
            urwid.AttrMap(back_button1, 'button normal', focus_map='button focus'),
            align='left',
            width='pack',
        )
        buttons = urwid.GridFlow(
            [export_button, back_button],
            cell_width=max(len( export_button1.get_label().encode() ), len( back_button1.get_label().encode() )),  # Adjust the width of each button
            h_sep=2,        # Horizontal space between buttons
            v_sep=0,        # Vertical space between rows (not needed here)
            align='left'  # Left-align the buttons
        )
        list_walker = urwid.SimpleFocusListWalker([info_text, urwid.Filler(urwid.Divider()), buttons])
        list_box = urwid.ListBox(list_walker)
        pile = urwid.Pile([urwid.AttrWrap(list_box, 'body')])
        self.main_widget.body = pile

    def showOutputDlg(self, strCommand, callback = None ):
        process = subprocess.Popen( [ "bash", "-c", strCommand ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT )
        stdout, stderr = process.communicate()
        ret = process.returncode
        message = stdout.decode().replace( "\r", "")
        if len( message ) == 0:
            if ret == 0:
                message = "Successfully completed the command"
            else:
                message = f"The command failed with {ret}"
        text = urwid.Text(message, align='left')
        if callback:
            close_dlg = callback
        else:
            close_dlg = self.close_message
        okBtn = urwid.Button(_("OK"), align='center', on_press=close_dlg)
        okBtn.returncode = ret
        ok_button = urwid.Padding(
            urwid.AttrMap( okBtn, 'button normal', focus_map='button focus'),
            align='center',
            width=15  # Set the width of the button
        )
        pile = urwid.Pile([urwid.Divider(), text, ok_button])

        list_walker = urwid.SimpleFocusListWalker([pile])
        list_box = urwid.ListBox(list_walker)

        lineBox = createDoubleLineBox( list_box, _("Output"))
        lineBox = urwid.Padding( lineBox, align='center', width=("relative", 61.8))
        dlg = urwid.AttrMap( lineBox, 'body')
        #dlg = urwid.AttrMap( urwid.Filler(dlg, valign='middle', ), 'body' )
        #dlg = urwid.Filler(dlg, valign='middle', height="pack" )

        self.menu_stack.append(self.main_widget.body)
        self.main_widget.body = urwid.AttrWrap( dlg, 'body')

    def askForKeyNumbers( self, bGmSSL ):
        server_key_edit = urwid.Edit(caption=_("#Server Keys: "))
        client_key_edit = urwid.Edit(caption=_("#Client Keys: "))

        def on_ok(dlg, button):
            if dlg:
                strsnum = server_key_edit.edit_text.strip()
                strcnum = client_key_edit.edit_text.strip()
                if strsnum:
                    dlg.sknum = int( strsnum )
                else:
                    dlg.sknum = 0
                if strcnum:
                    dlg.cknum = int( strcnum )
                else:
                    dlg.cknum = 0
            self.go_back( button )
            sslType =  "gmssl" if bGmSSL else 'openssl'
            taskToRun =  f"rpcfctl genkeys {sslType} {dlg.cknum} {dlg.sknum}"
            def updateKeyCb( self, button ):
                self.close_message( button )
                for widget in self.oSecWidgets:
                    if not isinstance( widget, urwid.Edit ):
                        continue
                    homeDir = os.path.expanduser("~") + "/.rpcf"
                    if bGmSSL:
                        homeDir += "/gmssl/"
                    else:
                        homeDir += "/openssl/"
                    label = widget.caption.strip()
                    if label == _("Key File :"):
                        if self.bServer: 
                            widget.edit_text = homeDir + "signkey.pem"
                        else:
                            widget.edit_text = homeDir + "clientkey.pem"
                    elif label == _("Cert File :"):
                        if self.bServer: 
                            widget.edit_text = homeDir + "signcert.pem"
                        else:
                            widget.edit_text = homeDir + "clientcert.pem"
                    elif label == _("CACert File :"):
                            widget.edit_text = homeDir + "certs.pem"

            callback = partial( updateKeyCb, self )
            self.main_loop.set_alarm_in(0, lambda loop, data: self.showOutputDlg( taskToRun, callback ) )

        def on_cancel( dlg, button):
            self.go_back( button )

        ok_button = urwid.Button(_("OK"), align='center', on_press=lambda button: on_ok( self.key_dialog, button))
        cancel_button = urwid.Button(_("Cancel"), align='center', on_press=lambda button: on_cancel(self.key_dialog, button))
        
        pile = urwid.Pile([
            urwid.Divider(),
            server_key_edit,
            client_key_edit,
            urwid.Divider(),
            urwid.GridFlow(
                [ urwid.AttrMap(ok_button, 'button normal',  focus_map='button focus'),
                  urwid.AttrMap(cancel_button, 'button normal',  focus_map='button focus')
                ],
                cell_width=10,  # Adjust button width
                h_sep=2,        # Horizontal space between buttons
                v_sep=0,        # No vertical space
                align='center'  # Center-align buttons
            )
        ])
        
        lineBox = createDoubleLineBox( pile, _("Input Password"))
        lineBox = urwid.Padding( lineBox, align='center', width=("relative", 61.8))
        dlg = urwid.AttrMap( lineBox, 'body')
 
        self.key_dialog = urwid.Padding(
            urwid.AttrMap( urwid.Filler(dlg, valign='middle', ), 'body' ),
            align='center',
            )
        self.menu_stack.append( self.main_widget.body )
        self.main_widget.body = self.key_dialog

    def genSelfSignedCerts(self, button):
        bGmSSL = False
        for widget in self.oSecWidgets:
            if isinstance(widget, urwid.CheckBox ):
                if widget.get_label().strip() == _("Using GmSSL"):
                    bGmSSL = widget.get_state()
                    break
        self.askForKeyNumbers( bGmSSL )

    def buildAuthWidgets( self, authInfo )->list:
        widgetPile = []
        authMech = authInfo.get( "AuthMech", None )
        if authMech == 'SimpAuth':
            widgetPile.append(urwid.Edit(_("User Name : "), edit_text=authInfo.get('UserName', '')))
        elif authMech == 'OAuth2':
            widgetPile.append(urwid.Edit(_("  OA2Checker Ip : "), edit_text=authInfo.get('OA2ChkIp', '')))
            widgetPile.append(urwid.Edit(_("OA2Checker Port : "), edit_text=authInfo.get('OA2ChkPort', '')))
            widgetPile.append(urwid.Edit(_("       Auth URL : "), edit_text=authInfo.get('AuthUrl', '')))
            widgetPile.append(urwid.CheckBox(_("Enable SSL"), state=(authInfo.get('OA2SSL', False) == 'true')))
        elif authMech == 'krb5' and IsFeatureEnabled('krb5'):
            widgetPile.append(urwid.Edit(_(" Service Name : "), edit_text=authInfo.get('ServiceName', '')))
            widgetPile.append(urwid.Edit(_("        Realm : "), edit_text=authInfo.get('Realm', '')))
            widgetPile.append(urwid.Edit(_("    User Name : "), edit_text=authInfo.get('UserName', '')))
            widgetPile.append(urwid.Edit(_("KDC IpAddress : "), edit_text=authInfo.get('KdcIp', '')))
            widgetPile.append(urwid.CheckBox(_("Sign Message"), state=(authInfo.get('SignMessage', False) == 'true')))

            checkBoxes = []
            if not self.bServer:
                if IsKinitProxyEnabled(): 
                    btnLabel = _("Disable kinit proxy")
                else:
                    btnLabel = _("Enable kinit proxy")
                kinitProxy = urwid.Button(btnLabel, align='center', on_press=self.enableKinitProxy )
                oKinitButton = urwid.Padding(
                    urwid.AttrMap(kinitProxy, 'button normal', focus_map='button focus'),
                    align='left',
                    width='pack',
                )
                checkBoxes.append( oKinitButton )

            if self.bServer:
                initKdc = urwid.Button(_("Initialize KDC"), align='center', on_press=self.initKdc )
                oInitKdcButton = urwid.Padding(
                    urwid.AttrMap(initKdc, 'button normal', focus_map='button focus'),
                    align='left',
                    width='pack',
                )
                checkBoxes.append( oInitKdcButton )

            updateAuthSettings = urwid.Button(_("Update Auth Settings"),
                align='center', on_press=self.updateAuthSettngsKrb5 )
            oUpdAuthSettings = urwid.Padding(
                urwid.AttrMap(updateAuthSettings, 'button normal', focus_map='button focus'),
                align='left',
                width='pack',
            )
            checkBoxes.append( oUpdAuthSettings )
            if self.bServer:
                cellWidth = max( 
                    len( initKdc.get_label().encode() ),
                    len( updateAuthSettings.get_label().encode() ))  # Adjust the width of each button
            else:
                cellWidth = len( kinitProxy.get_label() )
            buttons = urwid.GridFlow(
                checkBoxes,
                cellWidth,
                h_sep=2,        # Horizontal space between buttons
                v_sep=0,        # Vertical space between rows (not needed here)
                align='left'  # Left-align the buttons
            )
            widgetPile.append(buttons)
        return widgetPile

    def on_choice_changed(self, button, new_state):
        if new_state:  # Only update the security settings when a radio button is selected
            label = button.get_label()  # Get the label of the selected radio button
            oSec = self.initCfg.get('Security', {})
            authMech = None
            if label == _("SimpAuth"):
                authMech = 'SimpAuth'
            elif label == _("Kerberos"):
                authMech = 'krb5'
            elif label == _("OAuth2"):
                authMech = 'OAuth2'

            divCount = 0
            insPos = 0
            endPos = 0
            for idx, widget in enumerate(self.oSecWidgets):
                if isinstance(widget, urwid.Divider ):
                    divCount += 1
                    if divCount == 3:  # the new widgets should be inserted before the second divider
                        insPos = idx + 2
                        focus_pos = idx + 1
                        continue
                    if divCount == 4:
                        endPos = idx
                        break
            authInfo = oSec.get('AuthInfo', {})
            newAuthInfo = {"AuthMech": authMech}
            if authInfo.get('AuthMech', "") == authMech:
                newAuthInfo = authInfo
            widgetPile = self.buildAuthWidgets( newAuthInfo )
            del self.oSecWidgets[insPos:endPos]  # remove the old widgets between the two dividers
            for widget in widgetPile:
                self.oSecWidgets.insert(insPos, widget)  # insert the new widgets between the two dividers
                insPos += 1

            list_walker = urwid.SimpleFocusListWalker(self.oSecWidgets)
            list_box = urwid.ListBox(list_walker)
            if 'focus_pos' in locals():
                list_walker.set_focus(focus_pos)  # restore the focus position after updating the widgets
            self.main_widget.body = urwid.AttrWrap(list_box, 'body')
            self.main_widget.body.revert_func = lambda: self.revert_to_security_settings()  # Store the current state for reverting

    def revert_to_security_settings(self):
        self.oSecWidgets.clear()  # clear current widgets
        self.authMechs.clear()

    def updateSecCfg( self ):
        oSecurity = self.initCfg.get('Security', {})
        sslFiles = oSecurity.get('SSLCred', {})
        authInfo = oSecurity.get('AuthInfo', {})
        secItems = self.oSecWidgets
        oMisc = oSecurity.get( 'misc', {} )
        for widget in secItems:
            if isinstance(widget, urwid.Edit):
                label = widget.caption.strip()
                if label == _("Key File :"):
                    keyFile = widget.edit_text.strip()
                    if not os.access( keyFile, os.R_OK ):
                        raise Exception( _("Error key file is not accessible") )
                    sslFiles['KeyFile'] = keyFile
                elif label == _("Cert File :"):
                    certFile = widget.edit_text.strip()
                    if not os.access( certFile, os.R_OK ):
                        raise Exception( _("Error cert file is not accessible") )
                    sslFiles['CertFile'] = certFile
                elif label == _("CACert File :"):
                    certsFile = widget.edit_text.strip()
                    if len( certsFile ) > 0 and not os.access( certsFile, os.R_OK ):
                        raise Exception( _("Error CA Cert file is not accessible") )
                    sslFiles['CACertFile'] = certsFile
                elif label == _("OA2Checker Ip :"):
                    authInfo['OA2ChkIp'] = widget.edit_text.strip()
                elif label == _("OA2Checker Port :"):
                    authInfo['OA2ChkPort'] = widget.edit_text.strip()
                elif label == _("Auth URL :"):
                    authInfo['AuthUrl'] = widget.edit_text.strip()
                elif label == _("Service Name :"):
                    authInfo['ServiceName'] = widget.edit_text.strip()
                elif label == _("Realm :"):
                    authInfo['Realm'] = widget.edit_text.strip()
                elif label == _("KDC IpAddress :"):
                    authInfo['KdcIp'] = widget.edit_text.strip()
                elif label == _("User Name :"):
                    authInfo['UserName'] = widget.edit_text.strip()
                elif label == _("Max Connections :"):
                    strNum = widget.edit_text.strip()
                    connNum = 512 if len( strNum ) == 0 else int(strNum)
                    if connNum < 0 :
                        raise Exception( _("Invalid 'Max Connections'"))
                    oMisc['MaxConnections'] = connNum
                elif label == _("DEB Package Path :") or label == _("RPM Package Path :"):
                    self.pkgPath = widget.edit_text.strip() 
                    if len( self.pkgPath ) == 0:
                        self.pkgPath = None
                    else:
                        strPath = self.pkgPath
                        process = subprocess.Popen(["bash", "-c", f"ls {strPath} > /dev/null"])
                        process.communicate()
                        ret = process.returncode
                        if ret != 0:
                            self.pkgPath = None
                            raise Exception(_("Error cannot access the package path ") + f"'{strPath}'")
                elif label == _("Destination Directory :"):
                    self.destPath = widget.edit_text.strip()
                    if len( self.destPath ) == 0:
                        raise Exception( _("Error the destion path is empty"))
                    else:
                        strPath = self.destPath
                        process = subprocess.Popen(["bash", "-c", f"ls {strPath} > /dev/null"])
                        process.communicate()
                        ret = process.returncode
                        if ret != 0:
                            self.destPath = None
                            raise Exception( _("Error cannot access the destPath path ") + f"'{strPath}'")

            elif isinstance(widget, urwid.CheckBox):
                label = widget.get_label().strip()
                state = 'true' if widget.get_state() else 'false'
                if label == _("Using GmSSL"):
                    sslFiles['UsingGmSSL'] = state
                elif label == _("Verify Peer"):
                    sslFiles['VerifyPeer'] = state
                elif label == _("Enable SSL") and "OA2ChkIp" in authInfo:
                    authInfo['OA2SSL'] = state
                elif label == _("Sign Message") and "ServiceName" in authInfo:
                    authInfo['SignMessage'] = state
                elif label == _("Task Scheduler") :
                    oMisc['TaskScheduler'] = "RR" if widget.get_state() else None
                elif label == _("Configure WebSever on installation"):
                    oMisc['ConfigWebServer' ] = "true" if widget.get_state() else "false"
                elif label == _("Configure Kerberos on installation"):
                    oMisc['ConfigKrb5' ] = "true" if widget.get_state() else "false"
                elif label == _("Enable kinit proxy on installation (client)"):
                    oMisc['KinitProxy' ] = "true" if widget.get_state() else "false"

        # Determine AuthMech based on which fields are filled or which checkboxes are checked
        for authMech in self.authMechs:
            if authMech.base_widget.get_state():
                label = authMech.base_widget.get_label().strip()
                if label == _("SimpAuth"):
                    authInfo['AuthMech'] = 'SimpAuth'
                elif label == _("Kerberos"):
                    authInfo['AuthMech'] = 'krb5'
                elif label == _("OAuth2"): 
                    authInfo['AuthMech'] = 'OAuth2'
                break

    def update_security_config(self, button):
        if button.get_label() != _("Return to Previous Level"):
            self.revert_to_security_settings()
            self.go_back( button )  # return to previous menu after updating config
            return
        try:
            self.updateSecCfg()
        except Exception as err:
            self.show_message(_("Failed to update security configuration") +": {}".format(str(err)))
            return
        self.oSecWidgets.clear()  # clear current widgets
        self.authMechs.clear()
        self.go_back( button )  # return to previous menu after updating config
        
    def HasAuth( self ):
        oConns = self.oConns
        if len(oConns) == 0:
            oConnParams = self.initCfg.get('Connections', [])
            for conn in oConnParams:
                if conn.get('HasAuth', 'false') == 'true':
                    return True
            return False
        for oWidgets in oConns:
            for widget in oWidgets:
                if isinstance(widget, urwid.CheckBox):
                    label = widget.get_label().strip()
                    if label == _("Enable Authentication") and widget.get_state():
                        return True
        return False
        return False

    def HasSSL( self ):
        if not ( IsFeatureEnabled('openssl') or IsFeatureEnabled('gmssl') ):
            return False
        oConns = self.oConns
        if len(oConns) == 0:
            oConnParams = self.initCfg.get('Connections', [])
            for conn in oConnParams:
                if conn.get('EnableSSL', 'false') == 'true':
                    return True
            return False
        for oWidgets in oConns:
            for widget in oWidgets:
                if isinstance(widget, urwid.CheckBox):
                    label = widget.get_label().strip()
                    if label == _("Enable SSL") and widget.get_state():
                        return True
        return False

    def enableKinitProxy(self, button):    
        def doEnableKinitProxy( self, button ):
            bEnable = not IsKinitProxyEnabled()
            strCmd = GetEnableKinitProxyCmd( bEnable )
            def toggleKinitButton( self, button, button2 ):
                ret = 0
                if button2.returncode is not None:
                    ret = button2.returncode
                if ret == 0 and button:
                    if bEnable:
                        button.set_label( _("Disable kinit proxy" ) )
                    else:
                        button.set_label( _("Enable kinit proxy" ) )
                self.close_message( button2 )
            self.showOutputDlg( strCmd, partial( toggleKinitButton, self, button ) )
        self.ElevatePrivilege2( lambda : doEnableKinitProxy( self, button ) )

    def initKdc(self, button):    
        try:
            self.updateSecCfg()
            self.ElevatePrivilege2( self.SetupTestKdc )
        except Exception as err:
            self.show_message(_("Failed to setup test KDC") + ": {}".format(str(err)))
            return

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
            if not IsFeatureEnabled( "krb5" ): 
                print( "Warning 'krb5' is not selected and no need " + \
                    "to generate krb5 files for installer",
                    file=sys.stderr )
                return 1

            if not IsTestKdcSet():
                print( "Warning local KDC is not setup " )
                return 2

            krbConf = self.GetKrb5Conf()
            if krbConf == "" :
                print( "Warning 'krb5.conf' is not generated",
                    file=sys.stderr )
                return 3

            if not self.checkCfgKrb5.props.active:
                print( "Warning configuring krb5 is not selected",
                    file=sys.stderr )
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
        strKrb5Conf = GetKrb5ConfTempl()
        try:
            if not IsFeatureEnabled( "krb5" ): 
                raise Exception( _("krb5 not enabled" ))

            kdcIp = self.GetTestKdcIp()
            if len( kdcIp ) == 0:
                raise Exception( _("Unable to determine kdc address, and at lease one interface should be auth enabled") )

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

    def GetTestKdcIp( self ) -> str:
        kdcIp = ""
        try:
            for widget in self.oSecWidgets:
                if isinstance(widget, urwid.Edit):
                    label = widget.caption.strip()
                    if label == _("KdcIp :"):
                        kdcIp = widget.edit_text.strip()
                        break
            if kdcIp != '' and IsLocalIpAddr( kdcIp ) :
                return kdcIp

            oConns = self.initCfg.get( "Connections", None ) 
            if oConns is None:
                raise Exception( _("Error no connection defined in initcfg.json") )
            if not self.bServer:
                return ''
            for oConn in oConns :
                if oConn.get( "HasAuth", False ) == 'true' and \
                    oConn.get( "AuthMech", None ) == 'krb5': 
                    kdcIp = oConn.get( "IpAddress", "" )
                    if kdcIp != "":
                        break
        except:
            kdcIp = ''
        return kdcIp

    def GetTestRealm( self ) -> str:
        for widget in self.oSecWidgets:
            if isinstance(widget, urwid.Edit):
                label = widget.caption.strip()
                if label == _("Realm :"):
                    strRealm = widget.edit_text.strip()
                    break
        if len( strRealm ) == 0:
            strRealm = 'RPCF.ORG'
        return strRealm

    def GetTestKdcUser( self ) -> str:
        for widget in self.oSecWidgets:
            if isinstance(widget, urwid.Edit):
                label = widget.caption.strip()
                if label == _("User Name :"):
                    strUser = widget.edit_text.strip()
                    break
        if len( strUser ) == 0:
            strUser = os.getlogin()

        if strUser.find( '@' ) == -1:
            strRealm = self.GetTestRealm()
            strUser += '@' + strRealm 
        return strUser

    # GSSAPI form of the ServiceName
    def GetTestKdcSvcHostName( self ) -> str:
        for widget in self.oSecWidgets:
            if isinstance(widget, urwid.Edit):
                label = widget.caption.strip()
                if label == _("Service Name :"):
                    strSvc = widget.edit_text.strip()
                    break
        if len( strSvc ) > 0:
            components = strSvc.split( '@' )
            if len( components ) == 1:
                strSvc += '@' + platform.node()
            elif len( components ) > 2:
                raise Exception( _("Error bad service name ") + f"'{strSvc}'" )
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

    def UpdateAuthSettingsKrb5( self ) -> int:
        ret = 0
        strTmpConf = None
        tempInit = None
        try:
            bServer = self.bServer

            bChangeUser = False
            bChangeSvc = False
            bChangeKdc = False
            bChangeRealm = False

            oSec = self.initCfg.get( "Security", None )
            if oSec is None:
                raise Exception( _("Error unable to find 'Security' in initcfg.json") )
            authInfo = oSec.get( 'AuthInfo', None )
            if authInfo is None:
                raise Exception( _("Error unable to find 'AuthInfo' in initcfg.json") )

            strNewKdcIp = strNewRealm = strNewSvc = strNewUser = ""
            for widget in self.oSecWidgets:
                if isinstance(widget, urwid.Edit):
                    label = widget.caption.strip()
                    if label == _("Service Name :"):
                        strNewSvc = widget.edit_text.strip()
                    elif label == _("Realm :"):
                        strNewRealm = widget.edit_text.strip()
                    elif label == _("KDC IpAddress :"):
                        strNewKdcIp = widget.edit_text.strip()
                    elif label == _("User Name :"):
                        strNewUser = widget.edit_text.strip()

            if strNewRealm == "" :
                raise Exception( _("Error Realm is empty") )
            if strNewSvc == "":
                raise Exception( _("Error Service is empty") )
            if strNewUser == "":
                raise Exception( _("Error User is empty") )
            if strNewKdcIp == "":
                raise Exception( _("Error KDC address is empty") )

            if not CheckPrincipal( strNewUser, strNewRealm ):
                raise Exception( _("Bad principal ") + f"'{strNewUser}'" )

            if bServer and IsLocalIpAddr( strNewKdcIp ):
                bChangeSvc = True
                bChangeUser = True
            else:
                ## change through installer
                bChangeSvc = False
                bChangeUser = False

            if authInfo.get( 'Realm', None ) !=  strNewRealm :
                bChangeRealm = True

            if authInfo.get( 'KdcIp', None ) != strNewKdcIp :
                bChangeKdc = True

            if not bChangeKdc and not bChangeUser and \
                not bChangeSvc and not bChangeRealm :
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

            if len( cmdline ) == 0:
                raise Exception( _("Error building update commands") )

            cmdline += ";"
            cmdline += '{sudo} systemctl restart {KdcSvc};'

            tempInit = tempname()
            with open(tempInit, 'w') as f:
                json.dump(self.initCfg, f, indent=4)

            cmdline += f'rpcfctl updcfg {tempInit};'
            cmdline += f'rm {tempInit} {strTmpConf};'
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
            self.showOutputDlg( actCmd )

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT

        return ret

    def updateAuthSettngsKrb5(self, button):    
        try:
            self.updateSecCfg()
            self.ElevatePrivilege2( self.UpdateAuthSettingsKrb5  )

        except Exception as err:
            self.show_message(_("Failed to update security configuration" ) + ": {}".format(str(err)))
            return

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

            tempInit = tempname()
            with open(tempInit, 'w') as f:
                json.dump(self.initCfg, f, indent=4)

            cmdline += f'rpcfctl updcfg {tempInit};'
            cmdline += f'rm -f {tempInit} {tempKrb} {tempKdc} {tempAcl} {tempNewRealm};'

            if os.geteuid() == 0:
                actCmd = cmdline.format( sudo = '',
                    KdcSvc = GetKdcSvcName(),
                    KdcConfPath = GetKdcConfPath() )
            elif IsSudoAvailable():
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
            self.showOutputDlg( actCmd )

            #if self.realmEdit is not None:
            #    self.realmEdit.set_text( strDomain )
            #    self.svcEdit.set_text( strSvc )
            #    self.userEdit.set_text( strUser )
            #    self.kdcEdit.set_text( strIpAddr ) 
                
        except Exception as err:
            print( err )

    def configWebServer( self, button ):
        try:
            self.updateSecCfg()
        except Exception as err:
            self.show_message(_("Failed to update security configuration") + ": {}".format(str(err)))
            return
        self.showOutputDlg( f"rpcfctl cfgweb" )

    def genInstaller( self, button ):
        try:
            self.updateSecCfg()
            initFile = tempname()
            with open(initFile, 'w') as f:
                json.dump( self.initCfg, f, indent=4 )
            pkgPath = self.pkgPath if self.pkgPath is not None else ""
            strCmd = f"rpcfctl geninstaller {initFile} {self.destPath} {pkgPath}"
            self.showOutputDlg( strCmd )
            os.unlink( initFile )
        except Exception as err:
            self.show_message(_("Failed to update security configuration")+": {}".format(str(err)))
            return

    def buildSecuritySettings(self):
            
        if len( self.oSecWidgets ) == 0:
            oSecurity = self.initCfg.get('Security', [])
            sslFiles = oSecurity.get('SSLCred', {})

            self.oSecWidgets = [
                urwid.Text(_("SSL Settings"), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("    Key File : "), edit_text=sslFiles.get('KeyFile', '')),
                urwid.Edit(_("   Cert File : "), edit_text=sslFiles.get('CertFile', '')),
                urwid.Edit(_(" CACert File : "), edit_text=sslFiles.get('CACertFile', '')),
                urwid.CheckBox(_("Using GmSSL"), state=(sslFiles.get('UsingGmSSL', False) == 'true')),
                urwid.CheckBox(_("Verify Peer"), state=(sslFiles.get('VerifyPeer', False) == 'true')),
                urwid.AttrWrap( urwid.Button(_("Generate Self-Signed Certificate"),
                    on_press=self.genSelfSignedCerts), 'button normal', 'button focus'),
                urwid.Divider('-'),
                urwid.Text(_("Authentication Settings"), align='center'),
                urwid.Divider('-'),
            ]

            if self.HasAuth():
                authInfo = oSecurity.get('AuthInfo', dict(AuthMech='SimpAuth'))
                simpleAuth = urwid.RadioButton([], _("SimpAuth"),
                    state=(authInfo.get('AuthMech', '') == 'SimpAuth'), on_state_change=self.on_choice_changed )
                krb5 = urwid.RadioButton(simpleAuth.group, _("Kerberos"),
                    state=(authInfo.get('AuthMech', '') == 'krb5'), on_state_change=self.on_choice_changed )
                OAuth2 = urwid.RadioButton(simpleAuth.group, _("OAuth2"),
                    state=(authInfo.get('AuthMech', '') == 'OAuth2'), on_state_change=self.on_choice_changed )
                self.authMechs = [
                        urwid.AttrMap(simpleAuth, "radio normal", focus_map='radio select'),
                        urwid.AttrMap(OAuth2, "radio normal", focus_map='radio select'),
                    ]
                if IsFeatureEnabled('krb5'):
                    self.authMechs.append( urwid.AttrMap(krb5, "radio normal", focus_map='radio select') )
                radioButtons = urwid.GridFlow(
                    self.authMechs,
                    cell_width=20,  # Adjust the width of each button
                    h_sep=2,        # Horizontal space between buttons
                    v_sep=0,        # Vertical space between rows (not needed here)
                    align='center'  # Center-align the buttons
                )
                widgetPile = self.buildAuthWidgets( authInfo )

                self.oSecWidgets.extend([
                radioButtons,
                *widgetPile,])

            oMisc = oSecurity.get('misc', {})
            bWebSocket = False

            oConnParams = self.initCfg.get('Connections', [])
            for conn in oConnParams[:]:
                if conn.get( 'EnableWS', False) == 'true':
                    bWebSocket = True
                    break
            oMiscWidgets = [urwid.Divider('-'),
                urwid.Text(_("Misc Options"), align='center'),
                urwid.Edit(_("Max Connections : "), edit_text=str(oMisc.get('MaxConnections', '512'))),
                urwid.CheckBox(_("Task Scheduler"), state=( oMisc.get("TaskScheduler", True ) == "RR") ),
            ]

            if bWebSocket:
                oMiscWidgets.append(
                    urwid.AttrWrap( urwid.Button( _( "Configure WebServer" ), on_press=self.configWebServer ),  "button normal", "button focus", ) )

            self.oSecWidgets.extend( oMiscWidgets )
            if self.bServer:
                distName = GetDistName()
                if distName in ( "debian", "ubuntu" ):
                    destPathCap = _("DEB Package Path : " )
                elif distName in ( "fedora" ):
                    destPathCap = _("RPM Package Path : " )

                oInstWidgets = [urwid.Divider('-'),
                    urwid.Text(_("Installer Options"), align='center'),
                    urwid.Edit( destPathCap, edit_text="" if self.pkgPath == None else self.pkgPath ),
                    urwid.Edit( _("  Destination Directory : "), edit_text="./" if self.destPath == None else self.destPath ),
                    urwid.CheckBox(_("Configure WebSever on installation"),
                        state=(oMisc.get("ConfigWebServer", False) == "true")),
                ]
                if self.HasAuth() and authInfo.get( "AuthMech", "" ) == "krb5":
                    oInstWidgets.append( urwid.CheckBox(_("Configure Kerberos on installation "),
                        state=(oMisc.get("ConfigKrb5", False) == "true") ) )
                    oInstWidgets.append( urwid.CheckBox(_("Enable kinit proxy on installation (client)"),
                        state=(oMisc.get("KinitProxy", False) == "true") ) )

                oInstWidgets.append( urwid.AttrWrap(
                    urwid.Button( _( "Generate Rpc-Frmwrk Installer" ), on_press=self.genInstaller ),
                    "button normal", "button focus" ) )
                self.oSecWidgets.extend( oInstWidgets )

            self.oSecWidgets.extend([
                urwid.Divider('-'),
                urwid.AttrWrap( urwid.Button(_("Return to Previous Level"),
                    on_press=self.update_security_config), 'button normal', 'button focus')
            ])
        
        list_walker = urwid.SimpleFocusListWalker(self.oSecWidgets)
        list_box = urwid.ListBox(list_walker)
        self.main_widget.body = urwid.AttrWrap(list_box, 'body')
        self.main_widget.body.revert_func = lambda: self.revert_to_security_settings()  # Store the current state for reverting

    def showSecuritySettings(self, button):
        # Save the current menu state to the stack before showing security settings
        self.menu_stack.append(self.main_widget.body)
        self.buildSecuritySettings()

    def go_back(self, button=None):
        if not button and self.main_widget.body and hasattr(self.main_widget.body, 'revert_func'):
            self.main_widget.body.revert_func()
        if self.menu_stack:
            previous_menu = self.menu_stack.pop()
            self.main_widget.body = previous_menu
        else:
            # return to main menu if stack is empty, or exit if already at main menu
            self.exit_program( button )                                             

    def close_message(self, button):
        self.go_back( button )

    def confirm_save_and_exit(self, button):
        tempInit = tempname()
        with open(tempInit, 'w') as f:
            json.dump(self.initCfg, f, indent=4)
        def updateRpcfCfg():
            self.showOutputDlg(f"rpcfctl updcfg {tempInit};rm -f {tempInit}>/dev/null",
                callback = lambda button : self.exit_program(button) )
        self.ElevatePrivilege2( updateRpcfCfg )

    def confirm_discard_and_exit(self, button):
        raise urwid.ExitMainLoop()

    def confirm_cancel(self, button):
        #pdlg = PasswordDialog( self )
        #pdlg.callback = lambda password: self.show_message(f"{password}")
        #pdlg.runDlg()
        self.go_back(button)

    def show_confirmation(self, message):
        text = urwid.Text(message, align='center')
        yes_button = urwid.Button(_("Apply Changes"), align='center', on_press=self.confirm_save_and_exit)
        no_button = urwid.Button(_("Discard Changes"), align='center', on_press=self.confirm_discard_and_exit)
        cancel_button = urwid.Button(_("Cancel"), align='center', on_press=self.confirm_cancel)

        buttons = urwid.GridFlow(
            [
                urwid.AttrMap(yes_button, "button normal", focus_map='button focus'),
                urwid.AttrMap(no_button, "button normal", focus_map='button focus'),
                urwid.AttrMap(cancel_button, "button normal", focus_map='button focus')
            ],
            cell_width=20,  # Adjust the width of each button
            h_sep=2,        # Horizontal space between buttons
            v_sep=0,        # Vertical space between rows (not needed here)
            align='center'  # Center-align the buttons
        )
        pile = urwid.Pile([text, urwid.Divider(), buttons])

        self.menu_stack.append(self.main_widget.body)
        self.main_widget.body = urwid.AttrWrap(
            urwid.Filler(pile, valign='middle'), 'body'
        ) 
        
    def exit_program(self, button):
        if not button or button.get_label() != _("Exit"):
            raise urwid.ExitMainLoop()
        self.show_confirmation(_("Apply Changes and Exit?") )
        
    def run(self):
        
        global palette
        self.main_loop = urwid.MainLoop(
            self.main_widget,
            palette,
            unhandled_input=self.unhandled_input
        )
        
        self.main_loop.run()
        
    def unhandled_input(self, key):
        if key in ('q', 'Q'):
            raise urwid.ExitMainLoop()
        elif key == 'esc':
            # ESC key also returns to the previous menu
            self.go_back()

    def ElevatePrivilege2( self, callback ) -> int:
        userid = os.getuid()
        if userid == 0 and callback:
            callback()
            return 0
        try:
            ret = rpcf_system( "command -v sudo > /dev/null" )
            if ret == 0:
                sudo = "sudo"
            else:
                sudo = ""
            if sudo == "":
                return -errno.EACCES 

            ret = rpcf_system( "sudo -n echo updating... 2>/dev/null" )
            if ret == 0 :
                callback()
                return 0

            # prompt a password dialog for priviledged task userCb
            passDlg = PasswordDialog( self )
            def ElevatePrivilegeCb2( password ):
                if password is None:
                    return
                process = subprocess.Popen( [ 'bash', '-c', f"echo {password} | sudo -S echo updating..." ],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT )
                process.communicate()
                ret = process.returncode
                if ret == 0 and callback:
                    callback()
                else:
                    self.show_message("Error password" )
                return

            passDlg.callback = ElevatePrivilegeCb2
            passDlg.runDlg()
            # informational status
            return errno.EAGAIN

        except Exception as err:
            print( err )
        return -errno.EACCES

    def ElevatePrivilege( self, strCommand ) -> int:
        userid = os.getuid()
        if userid == 0:
            self.showOutputDlg( f"{strCommand}")
            return errno.EAGAIN
        try:
            ret = rpcf_system( "command -v sudo > /dev/null" )
            if ret == 0:
                sudo = "sudo"
            else:
                sudo = ""
            if sudo == "":
                return -errno.EACCES 

            ret = rpcf_system( "sudo -n echo updating... 2>/dev/null" )
            if ret == 0 :
                self.showOutputDlg( f"sudo {strCommand}")
                return errno.EAGAIN

            # prompt a password dialog for priviledged task userCb
            passDlg = PasswordDialog( self )
            def ElevatePrivilegeCb( password ):
                if password is None:
                    return
                ret = rpcf_system( f"echo {password} | sudo -S echo updating..." )
                if ret < 0:
                    return
                self.showOutputDlg( f"{strCommand}" )

            passDlg.callback = ElevatePrivilegeCb
            passDlg.runDlg()
            # informational status
            return errno.EAGAIN
        except Exception as err:
            print( err )
        return -errno.EACCES

    def getNodeWidgetIndex( self, oNode ):
        for widget in oNode:
            if isinstance( widget, urwid.Text ):
                label, style = widget.get_text()
                words = label.split()
                if words[0] != _('Node'):
                    continue
                return int(words[1]) - 1
        return None 

    def remove_node(self, button, idx):    
        # remove the connection configuration at the specified index
        oNodeParams = self.initCfg.get('Multihop', [])
        if 0 <= idx < len(oNodeParams):
            for idx2, oNode in enumerate(self.oNodes):
                if self.getNodeWidgetIndex( oNode ) == idx:
                    del self.oNodes[idx2]
                # refresh the network settings page to reflect the removed connection
            oNode = oNodeParams[idx] if idx < len(oNodeParams) else None
            if oNode:
                oNode["deleted"] = True  # Mark the connection as deleted in the config
            self.showMultihopSettings(button)

    def add_node(self, button):
        # add a new connection configuration (default values)
        oNodeParams = self.initCfg.get('Multihop', [])
        newIdx = self.iNodeIdx
        self.iNodeIdx+=1
        oNodeParams.append({
            'NodeName' : f"node{newIdx}",
            'IpAddress': '127.0.0.1',
            'PortNumber': f'{4132+newIdx}',
            'Enabled' : 'true',
            'Compression': 'false',
            'EnableSSL': 'false',
            'EnableWS': 'false',
            'DestURL': "",
            'AddrFormat': 'ipv4',
            'Protocol': 'native',
            'ConnRecover': 'true',
            'added': True  # Mark the connection as newly added in the config
        })
        # refresh the network settings page to show the new connection
        self.showMultihopSettings(button)

    def update_multihop_config(self, button):
        # walk through the widgets in oConns and update the initCfg with the new values
        try:
            oNodeParams = self.initCfg.get('Multihop', [])
            for oNode in self.oNodes:
                idx = self.getNodeWidgetIndex( oNode )
                if idx is None:
                    raise Exception(_("Error internal error"))
                if idx < 0 or idx >= len(oNodeParams):
                    continue
                node_params = oNodeParams[idx]
                if node_params.get("deleted", False):
                    continue
                for widget in oNode:
                    if isinstance(widget, urwid.Edit):
                        label = widget.caption.strip()
                        if label == _("IP Address:"):
                            ipAddr = widget.edit_text.strip()
                            if CheckIpAddr(ipAddr) is None and not is_valid_domain(ipAddr):
                                raise ValueError(_("Invalid IP address")+": {}".format(ipAddr))
                            node_params['IpAddress'] = widget.edit_text.strip()
                        elif label == _("Port Number:"):
                            node_params['PortNumber'] = widget.edit_text.strip()
                        elif label == _("Node Name:"):
                            node_params['NodeName'] = widget.edit_text.strip()
                        elif label == _("WebSocket URL:"):
                            node_params['DestURL'] = widget.edit_text.strip()
                    elif isinstance(widget, urwid.CheckBox):
                        label = widget.get_label().strip()
                        state = 'true' if widget.get_state() else 'false'
                        if label == _("Enable this Interface"):
                            node_params['Enabled'] = state
                        elif label == _("Enable Compression"):
                            node_params['Compression'] = state
                        elif label == _("Enable SSL"):
                            node_params['EnableSSL'] = state
                        elif label == _("Enable WebSockets"):
                            node_params['EnableWS'] = state
            for oNode in oNodeParams[:]:
                if oNode.get( 'deleted', False):
                    oNodeParams.remove(oNode)  # remove the newly added connection from config
            for oNode in oNodeParams[:]:
                if oNode.get( 'added', False):
                    oNode.pop('added')  # remove the added mark for display purposes
        except Exception as e:
            self.show_message(_("Failed to update network configuration")+": {}".format(str(e)))
            return
        self.oNodes.clear()  # clear current widgets
        self.go_back( button )  # return to previous menu after updating config

    def revertToMultihopSettings(self, multihop_widgets):
        # revert the network settings page to the previous state (before changes)
        oNodeParams = self.initCfg.get('Multihop', [])
        self.oNodes.clear()  # clear current widgets
        oNodes = self.oNodes
        for oNode in oNodeParams[:]:
            if oNode.get( 'added', False):
                oNodeParams.remove(oNode)  # remove the newly added connection from config
        for idx, oNode in enumerate(oNodeParams):
            if oNode.get('deleted', False):
               del oNode['deleted']  # remove the deleted mark for display purposes
            oNode = [
                urwid.Text(_("Node") + " {}".format(idx + 1), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("Node Name: "), edit_text=oNode.get('NodeName', '')),
                urwid.Edit(_("IP Address: "), edit_text=oNode.get('IpAddress', '')),
                urwid.Edit(_("Port Number: "), edit_text=oNode.get('PortNumber', '')),
                urwid.CheckBox(_("Enable this Node"), state=(oNode.get('Enabled', False) == 'true')),
                urwid.CheckBox(_("Enable Compression"), state=(oNode.get('Compression', False) == 'true')),
                urwid.CheckBox(_("Enable SSL"), state=(oNode.get('EnableSSL', False) == 'true')),
                urwid.CheckBox(_("Enable WebSockets"), state=(oNode.get('EnableWS', False) == 'true')),
                urwid.Edit(_("WebSocket URL: "), edit_text=oNode.get('DestURL', '')),
                urwid.AttrWrap( urwid.Button( _("Remove This Node"), on_press=self.remove_node, user_data=idx ), "button normal", "button focus" )
            ]
            oNodes.append(oNode)

    def showMultihopSettings(self, button):
        # Save the current menu state to the stack
        action = button.get_label() 
        if button and action == _("Multihop Settings") : 
            self.menu_stack.append(self.main_widget.body)
        # Create widgets for each set of network_items in oConns
        oNodeParams = self.initCfg.get('Multihop', [])
        oNodes = self.oNodes
        if action == _("Multihop Settings") and not oNodes:
            for idx, oNode in enumerate(oNodeParams):
                oNodeWidgets = [
                    urwid.Text((_("Node") + " {}".format(idx + 1)), align='center'),
                    urwid.Divider('-'),
                    urwid.Edit(_("Node Name: "), edit_text=oNode.get('NodeName', '')),
                    urwid.Edit(_("IP Address: "), edit_text=oNode.get('IpAddress', '')),
                    urwid.Edit(_("Port Number: "), edit_text=oNode.get('PortNumber', '')),
                    urwid.CheckBox(_("Enable this Node"), state=(oNode.get('Enabled', False) == 'true')),
                    urwid.CheckBox(_("Enable Compression"), state=(oNode.get('Compression', False) == 'true')),
                    urwid.CheckBox(_("Enable SSL"), state=(oNode.get('EnableSSL', False) == 'true')),
                    urwid.CheckBox(_("Enable WebSockets"), state=(oNode.get('EnableWS', False) == 'true')),
                    urwid.Edit(_("WebSocket URL: "), edit_text=oNode.get('DestURL', '/')),
                    urwid.AttrWrap( urwid.Button( _("Remove This Node"), on_press=self.remove_node, user_data=idx ),  "button normal", "button focus", )
                ]
                oNodes.append(oNodeWidgets)
        elif action == _("Add New Node"):
            # Create a widget for the new connection
            oNode = oNodeParams[-1]
            idx = len(oNodes)  # index of the new node
            oNodeWidgets = [
                urwid.Text(_("Node") + " {}".format(idx + 1), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("Node Name: "), edit_text=oNode.get('NodeName', '')),
                urwid.Edit(_("IP Address: "), edit_text=oNode.get('IpAddress', '')),
                urwid.Edit(_("Port Number: "), edit_text=oNode.get('PortNumber', '')),
                urwid.CheckBox(_("Enable this Node"), state=(oNode.get('Enabled', False) == 'true')),
                urwid.CheckBox(_("Enable Compression"), state=(oNode.get('Compression', False) == 'true')),
                urwid.CheckBox(_("Enable SSL"), state=(oNode.get('EnableSSL', False) == 'true')),
                urwid.CheckBox(_("Enable WebSockets"), state=(oNode.get('EnableWS', False) == 'true')),
                urwid.Edit(_("WebSocket URL: "), edit_text=oNode.get('DestURL', '/')),
                urwid.AttrWrap( urwid.Button( _("Remove This Node"), on_press=self.remove_node, user_data=idx ),  "button normal", "button focus", )
            ]
            oNodes.append(oNodeWidgets)
        elif action == _("Remove This Connection"):
            pass

        # Create a list walker for the current set of network items
        widgetList = []
        for oNode in oNodes:
            widgetList.extend( oNode )
        
        widgetList.append(urwid.Divider('-'))  # Add a divider between sets

        addConnButton = urwid.AttrWrap( urwid.Button(_("Add New Node"), on_press=self.add_node),  "button normal", "button focus", )
        returnButton = urwid.AttrWrap( urwid.Button(_("Return to Previous Level"), on_press=self.update_multihop_config) ,  "button normal", "button focus", ) # 修改为通用返回方法
        widgetList.extend( [ addConnButton, returnButton ] )
        #list_walker = urwid.SimpleFocusListWalker([addConnButton, returnButton])
        list_walker = urwid.SimpleFocusListWalker( widgetList )
        list_box = urwid.ListBox(list_walker)

        multihop_widgets = []
        multihop_widgets.append(urwid.AttrWrap(list_box, 'body'))
        # Create the Pile with the correct focus_item
        combined_pile = urwid.Pile(multihop_widgets)
        combined_pile.revert_func = lambda: self.revertToMultihopSettings(multihop_widgets)  # Store the current state for reverting

        # Set the combined pile as the body of the main widget
        self.main_widget.body = combined_pile

def usage():
    print( "Usage: python3 rpcfgtui.py [-hc]" )
    print( "\t-c: to config a client host, otherwise a server host" )
    print( "\t-b: using 'Bolland' colorscheme." )

def main():
    bServer = True
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hbc" )
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(-errno.EINVAL)

    for o, a in opts:
        if o == "-h" :
            usage()
            sys.exit( 0 )
        elif o == "-c":
            bServer = False
        elif o == "-b":
            global palette
            palette = bolland_palette
        else:
            assert False, "unhandled option"
    app = MenuDialog()
    app.bServer = bServer
    app.run()

if __name__ == "__main__":
    main()
    print("\033[0m", end='', flush=True)
