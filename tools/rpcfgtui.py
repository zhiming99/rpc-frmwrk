import json

import urwid
import gettext
import os
import errno

# Initialize gettext
gettext.bindtextdomain('rpcfgtui', '/usr/share/locale')
gettext.textdomain('rpcfgtui')
_ = gettext.gettext

class MenuDialog:
    def __init__(self):
        self.main_loop = None
        self.menu_stack = []  # 用于保存菜单历史记录
        self.setup_ui()
        self.initCfg = json.loads(os.popen('rpcfctl geninitcfg').read())   
        self.oConns = []  # 用于保存网络配置的widgets

    def setup_ui(self):
        # 创建主菜单选项
        menu_items = [
            urwid.Text(_("System Configuration"), align='center'),
            urwid.Divider('-'),
            urwid.Button(_("Network Settings"), on_press=self.network_settings),
            urwid.Button(_("Configuration List"), on_press=self.system_info),
            urwid.Button(_("Device Management"), on_press=self.device_management),
            urwid.Button(_("Exit"), on_press=self.exit_program)
        ]
        
        # 创建列表框
        list_walker = urwid.SimpleFocusListWalker(menu_items)
        list_box = urwid.ListBox(list_walker)
        
        # 创建标题和边框
        header = urwid.Text(_("Configure Rpc-Framework"), align='center')
        header = urwid.AttrWrap(header, 'header')
        
        # 创建主界面
        self.main_widget = urwid.Frame(
            urwid.AttrWrap(list_box, 'body'),
            header=header,
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
            self.network_settings(button)

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
        self.network_settings(button)

    def update_network_config(self, button):
        # walk through the widgets in oConns and update the initCfg with the new values
        oConnParams = self.initCfg.get('Connections', [])
        for idx, conn_widgets in enumerate(self.oConns):
            if idx < len(oConnParams):
                conn_params = oConnParams[idx]
                for widget in conn_widgets:
                    if isinstance(widget, urwid.Edit):
                        label = widget.caption.strip()
                        if label == _("IP Address:"):
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
        self.go_back( None )  # return to previous menu after updating config

    def revert_to_network_settings(self, network_widgets):
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

    def network_settings(self, button):
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
        combined_pile.revert_func = lambda: self.revert_to_network_settings(network_widgets)  # Store the current state for reverting

        # Set the combined pile as the body of the main widget
        self.main_widget.body = combined_pile

    def show_message(self, message):
        # 显示消息对话框
        text = urwid.Text(message, align='center')
        ok_button = urwid.Button(_("OK"), on_press=self.close_message)
        pile = urwid.Pile([text, urwid.Divider(), ok_button])

        # 保存当前状态并显示消息
        self.menu_stack.append(self.main_widget.body)
        self.main_widget.body = urwid.AttrWrap(
            urwid.Filler(pile, valign='middle'), 'body'
        )
            
    def ask_file_path(self):
        # Create input field and buttons
        input_edit = urwid.Edit("Enter file path: ", edit_text="initcfg_exported.json")
        ok_button = urwid.Button("OK", on_press=self.confirm_file_path, user_data=input_edit)
        cancel_button = urwid.Button("Cancel", on_press=self.close_message)

        # Arrange buttons horizontally
        buttons = urwid.GridFlow(
            [
                urwid.AttrMap(ok_button, None, focus_map='reversed'),
                urwid.AttrMap(cancel_button, None, focus_map='reversed')
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
            self.show_message(_("successfully export configuration to {}").format(file_path))
        except Exception as e:
            self.show_message(_("Failed to export configuration: {}").format(str(e)))

    def export_config(self, button): 
        # export to a JSON file for external use, such as debugging or manual editing; save current menu state to stack before showing message
        self.ask_file_path()

    def system_info(self, button):
        # 保存当前菜单状态到堆栈
        self.menu_stack.append(self.main_widget.body)
        
        # 系统信息对话框
        initcfg = json.dumps(self.initCfg, indent=4)
        info_text = urwid.Text( initcfg , align='left')
        export_button = urwid.Button(_("Export Configuration"), on_press=self.export_config)
        back_button = urwid.Button(_("Return to Previous Level"), on_press=self.go_back)
        list_walker = urwid.SimpleFocusListWalker([info_text, urwid.Filler(urwid.Divider()), export_button, back_button])
        list_box = urwid.ListBox(list_walker)
        pile = urwid.Pile([urwid.AttrWrap(list_box, 'body')])
        self.main_widget.body = pile
        
        
    def device_management(self, button):
        # 保存当前菜单状态到堆栈
        self.menu_stack.append(self.main_widget.body)
        
        # 设备管理对话框
        device_items = [
            urwid.Text(_("Device Management"), align='center'),
            urwid.Divider('-'),
            urwid.Button(_("Return to Previous Level"), on_press=self.go_back)  # 修改为通用返回方法
        ]
        
        list_walker = urwid.SimpleFocusListWalker(device_items)
        list_box = urwid.ListBox(list_walker)
        
        self.main_widget.body = urwid.AttrWrap(list_box, 'body')

    def go_back(self, button=None):                                                                                                                                                                              
        # return to the previous menu state from the stack, if available; otherwise exit
        if self.main_widget.body and hasattr(self.main_widget.body, 'revert_func'):
            self.main_widget.body.revert_func()  # call revert_func to restore previous state if it exists
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
        self.go_back()

    def show_confirmation(self, message):
        # 显示确认对话框
        text = urwid.Text(message, align='center')
        yes_button = urwid.Button(_("Save Changes"), align='center', on_press=self.confirm_save_and_exit)
        no_button = urwid.Button(_("Discard Changes"), align='center', on_press=self.confirm_discard_and_exit)
        cancel_button = urwid.Button(_("Cancel"), align='center', on_press=self.confirm_cancel)
        width, height = os.get_terminal_size()

        spaces = str( ' ' * ((width - 27 ) // 2) )
        placeholder = urwid.Text(spaces, align='center')
        buttons = urwid.GridFlow(
            [
                urwid.AttrMap(yes_button, None, focus_map='reversed'),
                urwid.AttrMap(no_button, None, focus_map='reversed'),
                urwid.AttrMap(cancel_button, None, focus_map='reversed')
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
        self.show_confirmation(_("Save and Exit?") )
        
    def run(self):
        # 定义颜色方案
        palette = [
            ('body', 'yellow', 'dark blue', 'standout'),  # Yellow text on blue background
            ('header', 'white', 'dark blue', 'bold'),    # White text on blue background
            ('button normal', 'light gray', 'dark blue', 'standout'),  # Light gray text on blue background
            ('button select', 'yellow', 'dark red'),    # Yellow text on red background
            ('divider', 'light gray', 'dark blue'),      # Divider color
        ]
        
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
            # ESC键也可以返回上一级
            self.go_back()

def main():
    app = MenuDialog()
    app.run()

if __name__ == "__main__":
    main()
