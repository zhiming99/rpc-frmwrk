# rpc-frmwrk开发教程
## 第十一节 如何配置和启动rpc-frmwrk
rpc-frmwrk是一个高度可定制化的平台，有繁多的配置选项可供定制化。

### 基本配置
* 服务器IP地址(缺省127.0.0.1)
* 服务端口号(缺省4132)
* 压缩传输(压缩可以减小传输数据量50%)

### 确定需求
* 最简需求，就是设置一下IP地址和端口号即可。可以通过`rpcfg.py`或者`rpcfctl tui`进行配置。
* 初级安全需求，要求传输有SSL, 这个就需要先运行`rpcfctl initsvr`或者`rpcfctl initcli`，生成传输密钥，再通过`rpcfg.py`或者`rpcfctl tui`在网络连接页面勾选`启用SSL`. 如果需要使用非自签名的密钥，可以使用`rpcfctl importkeys`进行设置。
* 较高级安全需求，要求有密码认证功能的，除了前两步的设置，还必须在配置工具的`安全页面`勾选`SimpAuth`, 并在网络连接页面勾选`Enable Authentication`选项。
* 需要SSO登陆认证的可以选择kerberos，或者OAuth2，并填写相关的参数。Kerberos务必要先安装好kerberos服务器。如果是Active Driectory, 需要和管理联系商讨。如果是和rpc-frmwrk安装在一起的, 可以使用`rpcfctl tui`在安全页的`Initialize KDC`进行自动配置。
* 对于要穿过防火墙和Web服务器代理的环境，可以勾选网络页面的`Enable WebSocket`和填写`WebSocket URL`。此时就需要和管理员联系了。如果有权限自行安装Web服务器了，也可以使用`rpc-frmwrk`的自动配置功能，配置同一台机器上的`nginx`或`apache`.
* 如果需要实时监控`rpc-frmwrk`系统运行的，可以使用`SimpAuth`或`OAuth2`+`WebSocket`+`OpenSSL`+`nginx`的组合，通过网页访问`rpc-frmwrk`的监控器。用户的业务逻辑也将以同样的连接进行传输。
* 如果需要使用`GmSSL`(国产SSL)的，可以在`安全页面`选中`GmSSL`。这时你需要同步修改关联的密钥和证书路径。自签名的密钥和证书，可以在`~/.rpcf/gmssl/`下面找到。

### 启动服务器
配置好后，可以按如下步骤启动服务器
* 有webserver的，确保webserver 已启动。有Kerberos服务器的确保服务器已启动。
* 有自装的Kerberos服务器的，确保kdc
* 不需要监控的，运行`rpcrouter -dr 2`, 需要认证该功能的，运行`rpcrouter -adr 2`
* 需要监控的，运行`rpcfctl restartall`

### 启动客户端
配置好后，可以按如下步骤连接服务器
* 运行`rpcrouter -dr 2`, 需要认证该功能的，运行`rpcrouter -adr 2`
* 有认证功能的，需要运行`rpcfctl login`进行预认证。
* 运行你的客户端程序。
* 如果是连接监控器，则跳过前面的三步，打开浏览器登陆https://your.server.com/rpcf/appmon.html，没有域名的，ip地址也可以。

### 故障排查
* 如果启动客户端后，发现连接不上可以按下面的列表排查：
  * 检查连接参数是否一致，服务器和客户端应该使用匹配的参数，比如一边启用认证，另一边没有，一边启用SSL，另一边没有，或者一边启用WebSocket,另一边没有。
  * 检查守护进程是否启动，即rpcrouter是否启动，并且参数是否匹配。有认证的会有`-a`选项，没认证的没有`-a`。客户端参数必须有`-r 1`, 服务器端必须有`-r 2`.
  * 客户端在有认证的情况时，需要先运行`rpcfctl login`，成功后才，客户端才能连接服务器。
  * 修改了配置参数后，需要重启守护进程。
  * 高级故障排查方法：
    * 使用wireshark确定哪一方出错
    * 使用dbus-monitor查看客户端或者服务器端和守护进程的通信
    
