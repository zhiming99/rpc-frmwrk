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
            urwid.Button(_("System Information"), on_press=self.system_info),
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
        # 从oConns中移除对应的连接配置
        if 0 <= idx < len(self.oConns):
            del self.oConns[idx]
            # 重新显示网络设置界面以更新列表
            oConnParams = self.initCfg.get('Connections', [])
            oConnParams.pop(idx)  # 同时从配置中移除对应的连接配置
            self.network_settings(button)

    def add_connection(self, button):
        # 添加一个新的连接配置（默认值）
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
            'EnableBPS': 'true'
        })
        # 重新显示网络设置界面以更新列表
        self.network_settings(button)

    def network_settings(self, button):
        # Save the current menu state to the stack
        action = button.get_label()  # 获取当前按钮标签以确定是进入网络设置还是返回上一级
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
            idx = len(oConns)  # 新连接的索引
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
        addConnButton = urwid.Button(_("Add New Connection"), align='center', on_press=self.add_connection)
        list_walker = urwid.SimpleFocusListWalker([addConnButton])
        list_box = urwid.ListBox(list_walker)
        network_widgets.append(urwid.AttrWrap(list_box, 'body'))
        # Create the Pile with the correct focus_item
        combined_pile = urwid.Pile(network_widgets)

        # Set the combined pile as the body of the main widget
        self.main_widget.body = combined_pile
        
    def system_info(self, button):
        # 保存当前菜单状态到堆栈
        self.menu_stack.append(self.main_widget.body)
        
        # 系统信息对话框
        info_text = urwid.Text([
            _("System Information\n"),
            _("--------\n"),
            _("Operating System: Embedded Linux\n"),
            _("Kernel Version: 5.4.0-embedded\n"),
            _("Memory Usage: 45%\n"),
            _("Storage Space: 2.1GB/8GB\n"),
            _("Uptime: 12 days 4 hours\n")
        ])
        
        back_button = urwid.Button(_("Return to Previous Level"), on_press=self.go_back)  # 修改为通用返回方法
        pile = urwid.Pile([info_text, urwid.Divider(), back_button])
        
        self.main_widget.body = urwid.AttrWrap(
            urwid.Filler(pile, valign='top'), 'body'
        )
        
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
        # 从堆栈中恢复上一级菜单                                                                                                                                                                                 
        if self.menu_stack:                                                                                                                                                                                      
            previous_menu = self.menu_stack.pop()                                                                                                                                                                
            self.main_widget.body = previous_menu                                                                                                                                                                
        else:                                                                                                                                                                                                    
            # 如果没有上级菜单，回到主菜单                                                                                                                                                                       
            self.exit_program( button )                                             

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
        
    def close_message(self, button):
        # 关闭消息框，返回之前的状态
        self.go_back( button )

    def confirm_save_and_exit(self, button):
        raise urwid.ExitMainLoop()

    def confirm_nosave_and_exit(self, button):
        raise urwid.ExitMainLoop()

    def confirm_cancel(self, button):
        self.go_back()

    def show_confirmation(self, message):
        # 显示确认对话框
        text = urwid.Text(message, align='center')
        yes_button = urwid.Button(_("Yes"), on_press=self.confirm_save_and_exit)
        no_button = urwid.Button(_("No"), on_press=self.confirm_nosave_and_exit)
        cancel_button = urwid.Button(_("Cancel"), on_press=self.confirm_cancel)
        width, height = os.get_terminal_size()

        spaces = str( ' ' * ((width - 27 ) // 2) )
        placeholder = urwid.Text(spaces, align='center')
        buttons = urwid.Columns(
            [
             ( "pack", placeholder ),
             ( "pack", yes_button),
             ( "pack", no_button),
             ( "pack", cancel_button)], dividechars=2)
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
            ('body', 'black', 'light gray', 'standout'),
            ('header', 'white', 'dark red', 'bold'),
            ('button normal', 'light gray', 'dark blue', 'standout'),
            ('button select', 'white', 'dark green'),
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
