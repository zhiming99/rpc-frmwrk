import json
import re

import urwid
import gettext
import os
import errno
import getopt
import sys
from urwid.widget.constants import BOX_SYMBOLS
from updcfg import CheckIpAddr, IsFeatureEnabled 
from updwscfg import rpcf_system

# Initialize gettext
gettext.bindtextdomain('rpcfgtui', '/usr/share/locale')
gettext.textdomain('rpcfgtui')
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

palette = [
    ('body', 'yellow', 'dark blue', 'standout'),  # Yellow text on blue background
    ('header', 'white', 'dark blue', 'bold'),    # White text on blue background
    ('button normal', 'light gray', 'dark blue', 'standout'),  # Light gray text on blue background
    ('button select', 'yellow', 'dark red'),    # Yellow text on red background
    ('button focus', 'yellow', 'dark red', 'standout'),    # Yellow text on red background
    ('divider', 'light gray', 'light gray'),      # Divider color
    ('radio normal', 'light gray', 'dark blue'),  # Light gray text on blue background
    ('radio select', 'light gray', 'dark green'),      # Yellow text on blue background
]

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
        lineBox = urwid.Padding( lineBox, align='center', width=("relative", 50))
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

    def setup_ui(self):
        # 创建主菜单选项
        menu_items = [
            urwid.Divider(' '),
            urwid.Button(_("Network Settings"), on_press=self.showNetworkSettings),
            urwid.Divider(' '),
            urwid.Button(_("Security Settings"), on_press=self.showSecuritySettings),
            urwid.Divider(' '),
            urwid.Button(_("Configuration List"), on_press=self.config_list),
            urwid.Divider(' '),
            urwid.Button(_("Exit"), on_press=self.exit_program)
        ]
        
        # 创建列表框
        list_walker = urwid.SimpleFocusListWalker(menu_items)
        list_box = urwid.ListBox(list_walker)
        line_box = createDoubleLineBox(list_box,
            _("Configure Rpc-Framework") )
        
        # 创建标题和边框
        #header = urwid.Text(_("Configure Rpc-Framework"), align='center')
        #header = urwid.AttrWrap(header, 'header')
        
        # 创建主界面
        self.main_widget = urwid.Frame(
            urwid.AttrWrap(line_box, 'body'),
            #header=header,
            footer=urwid.Text(_("Use arrow keys to navigate, Enter to confirm"), align='center')
        )
    def remove_connection(self, button, idx):    
        # remove the connection configuration at the specified index
        if 0 <= idx < len(self.oConns):
            del self.oConns[idx]
            # refresh the network settings page to reflect the removed connection
            oConnParams = self.initCfg.get('Connections', [])
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

    def update_network_config(self, button):
        # walk through the widgets in oConns and update the initCfg with the new values
        try:
            oConnParams = self.initCfg.get('Connections', [])
            for conn in oConnParams[:]:
                if conn.get( 'deleted', False):
                    oConnParams.remove(conn)  # remove the newly added connection from config
            for conn in oConnParams[:]:
                if conn.get( 'added', False):
                    conn.pop('added')  # remove the added mark for display purposes
            for idx, conn_widgets in enumerate(self.oConns):
                if idx < len(oConnParams):
                    conn_params = oConnParams[idx]
                    for widget in conn_widgets:
                        if isinstance(widget, urwid.Edit):
                            label = widget.caption.strip()
                            if label == _("IP Address:"):
                                ipAddr = widget.edit_text.strip()
                                if CheckIpAddr(ipAddr) is None and not is_valid_domain(ipAddr):
                                    raise ValueError(_("Invalid IP address: {}").format(ipAddr))
                                conn_params['IpAddress'] = widget.edit_text
                            elif label == _("Port Number:"):
                                conn_params['PortNumber'] = widget.edit_text
                            elif label == _("Router Path:"):
                                conn_params['RouterPath'] = widget.edit_text
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
        except Exception as e:
            self.show_message(_("Failed to update network configuration: {}").format(str(e)))
            return
        self.oConns.clear()  # clear current widgets
        self.go_back( None )  # return to previous menu after updating config

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
                urwid.Text(_("Network Configuraselftion {}").format(idx + 1), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("IP Address: "), edit_text=conn.get('IpAddress', '')),
                urwid.CheckBox(_("Server bind to this Interface"), state=(conn.get('BindTo', False) == 'true')),
                urwid.Edit(_("Port Number: "), edit_text=conn.get('PortNumber', '')),
                urwid.Edit(_("Router Path: "), edit_text=conn.get('RouterPath', '/')),
                urwid.CheckBox(_("Enable Compression"), state=(conn.get('Compression', False) == 'true')),
                urwid.CheckBox(_("Enable SSL"), state=(conn.get('EnableSSL', False) == 'true')),
                urwid.CheckBox(_("Enable WebSockets"), state=(conn.get('EnableWS', False) == 'true')),
                urwid.CheckBox(_("Enable Authentication"), state=(conn.get('HasAuth', False) == 'true')),
                urwid.CheckBox(_("Enable Flow Control"), state=(conn.get('EnableBPS', False) == 'true')),
                urwid.Button( _("Remove This Connection"), on_press=self.remove_connection, user_data=idx ),
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
                    urwid.Text(_("Network Configuration {}").format(idx + 1), align='center'),
                    urwid.Divider('-'),
                    urwid.Edit(_("IP Address: "), edit_text=conn.get('IpAddress', '')),
                    urwid.CheckBox(_("Server bind to this Interface"), state=(conn.get('BindTo', False) == 'true')),
                    urwid.Edit(_("Port Number: "), edit_text=conn.get('PortNumber', '')),
                    urwid.Edit(_("Router Path: "), edit_text=conn.get('RouterPath', '/')),
                    urwid.CheckBox(_("Enable Compression"), state=(conn.get('Compression', False) == 'true')),
                    urwid.CheckBox(_("Enable SSL"), state=(conn.get('EnableSSL', False) == 'true')),
                    urwid.CheckBox(_("Enable WebSockets"), state=(conn.get('EnableWS', False) == 'true')),
                    urwid.CheckBox(_("Enable Authentication"), state=(conn.get('HasAuth', False) == 'true')),
                    urwid.CheckBox(_("Enable Flow Control"), state=(conn.get('EnableBPS', False) == 'true')),
                    urwid.Button( _("Remove This Connection"), on_press=self.remove_connection, user_data=idx ),
                ]
                oConns.append(conn_widgets)
        elif action == _("Add New Connection"):
            # Create a widget for the new connection
            conn = oConnParams[-1]
            idx = len(oConns)  # index of the new connection
            conn_widgets = [
                urwid.Text(_("Network Configuration {}").format(idx + 1), align='center'),
                urwid.Divider('-'),
                urwid.Edit(_("IP Address: "), edit_text=conn.get('IpAddress', '')),
                urwid.CheckBox(_("Server bind to this Interface"), state=(conn.get('BindTo', False) == 'true')),
                urwid.Edit(_("Port Number: "), edit_text=conn.get('PortNumber', '')),
                urwid.Edit(_("Router Path: "), edit_text=conn.get('RouterPath', '/')),
                urwid.CheckBox(_("Enable Compression"), state=(conn.get('Compression', False) == 'true')),
                urwid.CheckBox(_("Enable SSL"), state=(conn.get('EnableSSL', False) == 'true')),
                urwid.CheckBox(_("Enable WebSockets"), state=(conn.get('EnableWS', False) == 'true')),
                urwid.CheckBox(_("Enable Authentication"), state=(conn.get('HasAuth', False) == 'true')),
                urwid.CheckBox(_("Enable Flow Control"), state=(conn.get('EnableBPS', False) == 'true')),
                urwid.Button( _("Remove This Connection"), on_press=self.remove_connection, user_data=idx ),
            ]
            oConns.append(conn_widgets)
        elif action == _("Remove This Connection"):
            pass

        for conn in oConns:
            # Create a list walker for the current set of network items
            list_walker = urwid.SimpleFocusListWalker(conn)
            list_box = urwid.ListBox(list_walker)
            network_widgets.append(urwid.AttrWrap(list_box, 'body'))
            network_widgets.append(urwid.Filler(urwid.Divider('-')))  # Add a divider between sets

        #network_widgets.append(urwid.Button(_("Add New Connection"), on_press=self.add_connection))
        addConnButton = urwid.Button(_("Add New Connection"), on_press=self.add_connection)
        returnButton = urwid.Button(_("Return to Previous Level"), on_press=self.update_network_config)  # 修改为通用返回方法
        list_walker = urwid.SimpleFocusListWalker([addConnButton, returnButton])
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
            self.show_message(_("successfully export configuration to {}").format(file_path))
        except Exception as e:
            self.show_message(_("Failed to export configuration: {}").format(str(e)))

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
            cell_width=max(len( export_button1.get_label() ), len( back_button1.get_label() )),  # Adjust the width of each button
            h_sep=2,        # Horizontal space between buttons
            v_sep=0,        # Vertical space between rows (not needed here)
            align='left'  # Left-align the buttons
        )
        list_walker = urwid.SimpleFocusListWalker([info_text, urwid.Filler(urwid.Divider()), buttons])
        list_box = urwid.ListBox(list_walker)
        pile = urwid.Pile([urwid.AttrWrap(list_box, 'body')])
        self.main_widget.body = pile

    def ask_for_key_numbers( self ):
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
        lineBox = urwid.Padding( lineBox, align='center', width=("relative", 50))
        dlg = urwid.AttrMap( lineBox, 'body')
 
        self.key_dialog = urwid.Padding(
            urwid.AttrMap( urwid.Filler(dlg, valign='middle', ), 'body' ),
            align='center',
            )
        self.menu_stack.append( self.main_widget.body )
        self.main_widget.body = self.key_dialog

    def generate_self_signed_cert(self, button):
        self.ask_for_key_numbers()

        
    def on_choice_changed(self, button, new_state):
        if new_state:  # Only update the security settings when a radio button is selected
            label = button.get_label()  # Get the label of the selected radio button
            oSec = self.initCfg.get('Security', {})
            authMech = None
            if label == _("SimpAuth"):
                oSec['AuthInfo'] = dict(AuthMech='SimpAuth')
                authMech = 'SimpAuth'
            elif label == _("Kerberos"):
                oSec['AuthInfo'] = dict(AuthMech='krb5')
                authMech = 'krb5'
            elif label == _("OAuth2"):
                oSec['AuthInfo'] = dict(AuthMech='OAuth2')
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
            widgetPile = []
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
                widgetPile.append(urwid.CheckBox(_("Sign Messages"), state=(authInfo.get('SignMessages', False) == 'true')))

                kinitProxy = urwid.Button(_("Enable kinit proxy"), align='center', on_press=self.enableKinitProxy )
                oKinitButton = urwid.Padding(
                    urwid.AttrMap(kinitProxy, 'button normal', focus_map='button focus'),
                    align='left',
                    width='pack',
                )
                initKdc = urwid.Button(_("Initialize KDC"), align='center', on_press=self.initKdc )
                oInitKdcButton = urwid.Padding(
                    urwid.AttrMap(initKdc, 'button normal', focus_map='button focus'),
                    align='left',
                    width='pack',
                )

                updateAuthSettings = urwid.Button(_("Update Auth Settings"), align='center', on_press=self.updateAuthSettngsKrb5 )
                oUpdAuthSettings = urwid.Padding(
                    urwid.AttrMap(updateAuthSettings, 'button normal', focus_map='button focus'),
                    align='left',
                    width='pack',
                )
                buttons = urwid.GridFlow(
                    [oKinitButton, oInitKdcButton, oUpdAuthSettings],
                    cell_width=max(len( kinitProxy.get_label() ),
                        len( initKdc.get_label() ),
                        len( updateAuthSettings.get_label() )),  # Adjust the width of each button
                    h_sep=2,        # Horizontal space between buttons
                    v_sep=0,        # Vertical space between rows (not needed here)
                    align='left'  # Left-align the buttons
                )
                widgetPile.append(buttons)

            elif authMech is None:
                pass

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

    def update_security_config(self, button):
        if button.get_label() != _("Return to Previous Level"):
            self.revert_to_security_settings()
            self.go_back( button )  # return to previous menu after updating config
            return
        secItems = self.oSecWidgets
        try:
            oSecurity = self.initCfg.get('Security', {})
            sslFiles = oSecurity.get('SSLCred', {})
            authInfo = oSecurity.get('AuthInfo', {})
            for widget in secItems:
                if isinstance(widget, urwid.Edit):
                    label = widget.caption.strip()
                    if label == _("Key File :"):
                        sslFiles['KeyFile'] = widget.edit_text.strip()
                    elif label == _("Cert File :"):
                        sslFiles['CertFile'] = widget.edit_text.strip()
                    elif label == _("CACert File :"):
                        sslFiles['CACertFile'] = widget.edit_text.strip()
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
                elif isinstance(widget, urwid.CheckBox):
                    label = widget.get_label().strip()
                    state = 'true' if widget.get_state() else 'false'
                    if label == _("Using GmSSL"):
                        sslFiles['UsingGmSSL'] = state
                    elif label == _("Verify Peer"):
                        sslFiles['VerifyPeer'] = state
                    elif label == _("Enable SSL") and "OA2ChkIp" in authInfo:
                        authInfo['OA2SSL'] = state
                    elif label == _("Sign Messages") and "ServiceName" in authInfo:
                        authInfo['SignMessages'] = state
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
        except Exception as err:
            self.show_message(_("Failed to update security configuration: {}").format(str(err)))
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
        pass

    def initKdc(self, button):    
        pass

    def updateAuthSettngsKrb5(self, button):    
        pass

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
                    on_press=self.generate_self_signed_cert), 'button normal', 'button focus'),
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
                authMech = authInfo.get('AuthMech', '')
                widgetPile = []
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
                    widgetPile.append(urwid.CheckBox(_("Sign Messages"), state=(authInfo.get('SignMessages', False) == 'true')))

                    kinitProxy = urwid.Button(_("Enable kinit proxy"), align='center', on_press=self.enableKinitProxy )
                    oKinitButton = urwid.Padding(
                        urwid.AttrMap(kinitProxy, 'button normal', focus_map='button focus'),
                        align='left',
                        width='pack',
                    )
                    initKdc = urwid.Button(_("Initialize KDC"), align='center', on_press=self.initKdc )
                    oInitKdcButton = urwid.Padding(
                        urwid.AttrMap(initKdc, 'button normal', focus_map='button focus'),
                        align='left',
                        width='pack',
                    )

                    updateAuthSettings = urwid.Button(_("Update Auth Settings"), align='center', on_press=self.updateAuthSettngsKrb5 )
                    oUpdAuthSettings = urwid.Padding(
                        urwid.AttrMap(updateAuthSettings, 'button normal', focus_map='button focus'),
                        align='left',
                        width='pack',
                    )
                    buttons = urwid.GridFlow(
                        [oKinitButton, oInitKdcButton, oUpdAuthSettings],
                        cell_width=max(len( kinitProxy.get_label() ),
                            len( initKdc.get_label() ),
                            len( updateAuthSettings.get_label() )),  # Adjust the width of each button
                        h_sep=2,        # Horizontal space between buttons
                        v_sep=0,        # Vertical space between rows (not needed here)
                        align='left'  # Left-align the buttons
                    )
                    widgetPile.append(buttons)

                self.oSecWidgets.extend([
                radioButtons,
                *widgetPile,])

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
        if self.main_widget.body and hasattr(self.main_widget.body, 'revert_func'):
            self.main_widget.body.revert_func()
        if self.menu_stack:                                                                                                                                                                                      
            previous_menu = self.menu_stack.pop()                                                                                                                                                                
            self.main_widget.body = previous_menu                                                                                                                                                                
        else:                                                                                                                                                                                                    
            # return to main menu if stack is empty, or exit if already at main menu
            self.exit_program( button )                                             

    def close_message(self, button):
        # 关闭消息框，返回之前的状态
        self.go_back( button )

    def confirm_save_and_exit(self, button):
        raise urwid.ExitMainLoop()

    def confirm_discard_and_exit(self, button):
        raise urwid.ExitMainLoop()

    def confirm_cancel(self, button):
        #pdlg = PasswordDialog( self )
        #pdlg.callback = lambda password: self.show_message(f"{password}")
        #pdlg.runDlg()
        self.go_back(button)

    def show_confirmation(self, message):
        # 显示确认对话框
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

        # 保存当前状态并显示确认框
        self.menu_stack.append(self.main_widget.body)
        self.main_widget.body = urwid.AttrWrap(
            urwid.Filler(pile, valign='middle'), 'body'
        ) 
        
    def exit_program(self, button):
        # 退出程序
        if not button or button.get_label() != _("Exit"):
            raise urwid.ExitMainLoop()
        self.show_confirmation(_("Apply Changes and Exit?") )
        
    def run(self):
        # 定义颜色方案
        
        global palette
        # 创建主循环
        self.main_loop = urwid.MainLoop(
            self.main_widget,
            palette,
            unhandled_input=self.unhandled_input
        )
        
        self.main_loop.run()
        
    def unhandled_input(self, key):
        # 处理未绑定的按键输入
        if key in ('q', 'Q'):
            raise urwid.ExitMainLoop()
        elif key == 'esc':
            # ESC key also returns to the previous menu
            self.go_back()

    def ElevatePrivilege( self, callback ) -> int:
        ret = rpcf_system( "command -v sudo > /dev/null" )
        if ret == 0:
            sudo = "sudo"
        else:
            sudo = ""
        if sudo == "":
            return 0
        ret = rpcf_system( "sudo -n echo updating... 2>/dev/null" )
        if ret == 0 :
            return ret
        passDlg = PasswordDialog( self )
        passDlg.runDlg()

        # Run a blocking loop until the password is entered
        ret = rpcf_system( "echo '" + passwd + f"'| sudo -S echo updating..." )
        passwd = None
        return ret

def usage():
    print( "Usage: python3 rpcfgtui.py [-hc]" )
    print( "\t-c: to config a client host." )
    print( "\t\tOtherwise it is for a server host" )

def main():
    bServer = True
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hc" )
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
        else:
            assert False, "unhandled option"
    app = MenuDialog()
    app.bServer = bServer
    app.run()

if __name__ == "__main__":
    main()
    print("\033[0m", end='', flush=True)
