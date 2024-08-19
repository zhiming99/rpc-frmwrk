### 简介
`rpc-frmwrk`的JavaScript模块是一个运行在浏览器中的功能全面的客户端。用户可以通过`ridlc`生成的JS框架，添加适当的业务逻辑，即可访问已有的`rpc-frmwrk`服务器。JavaScript模块的优势是部署快速，且跨平台。

### 技术特点
`rpc-frmwrk`的JavaScript依赖于html5，通过`websocket`和`rpc-frmwrk`的服务器通信。JS的运行库一部分运行在浏览器的`web-worker`上，另一部分运行于`main-thread`上，确保后台RPC传输和前台页面展示不会互相影响。JavaScript模块和Python，Java的客户端一样，可以连接所有类型的`rpc-frmwrk`的服务器，不同之处则是JavaScript的客户端是完全独立的，不依赖C++运行库，而Python和Java的客户端跑在C++的运行库之上，以获得更好的性能和扩展性。由于B/C和C/S授权模式的不同，JavaScript客户端不支持Kerberos认证，不过，`rpc-frmwrk`服务器的OAuth2.0的支持，弥补了JS客户端的安全认证功能。

### 注意事项
所有的JS例子程序的部署的链接均被设置为假想的`http://example.com/rpcf`。所以用户在开发时，务必使用`ridlc`的`--odesc_path`选项，将其替换成真实的网络地址。

### 开发
相较于其他语言的开发，JS的流程相对复杂一些，因为涉及两种语言。我们以[hellowld.ridl](../examples/hellowld.ridl)为例. 假设目标网站的rpc-frmwrk服务的url为`https://example.com/rpcf`。
1. 在HelloWorld的JS例子程序的[目录](../examples/js/hellowld)下运行`ridlc -J --odesc_url=https://example.com/rpcf -O . ../../hellowld.ridl`，生成客户端网页和JS代码。如果需要OAuth2，加上选项`--auth`。
2. 在当前目录下运行`make`将用`webpack`打包生成的JS代码并存放在`./dist`目录下。
3. HelloWorld服务器代码我们可以在C++的例子程序[HelloWorld](../examples/cpp/hellowld)的目录下运行 `ridlc -O . ../../hellowld.ridl`生成完整的项目源文件。然后输入`make release`生成可执行文件`HelloWorldsvr`。

### 部署
下边简要介绍部署支持JS客户端的基本流程。rpc-frmwrk的服务器和web服务器既可以部署在同一台机器上，也可以在不同的机器上。

#### 要部署的组件
1. rpc-frmwrk的系统组件。
2. 用户的业务服务器。
3. 客户端的html和JavaScript。
4. 如果有OAuth2，则需要一个OAuth2检查器的插件，有关这个插件更多信息，请参看本文底部连接。

#### 服务器的部署
服务器的配置的设置
1. 运行命令`python3 /usr/bin/rpcf/rpcfg.py`，打开配置工具界面。
2. 配置WebSocket如下图，如果无OAuth2认证，去掉`auth`旁的勾选，并跳到第四步。
   * ![OAuth2-1](../pics/OAuth2-1_cn.png)
3. 选择认证方法
   * ![OAuth2-2](../pics/OAuth2-2_cn.png) 
4. 选择OK，将更新本地系统，选择Installer，将生成用于部署到目标机器的安装包。

#### 部署JS的skeleton代码
1. 如果目标主机就是开发机器可忽略2,3步。
2. 在目标主机上运行安装包，安装`rpc-frmwrk`系统，并配置系统。
3. 把HelloWorld的服务器程序`HelloWorldsvr`拷贝到目标主机上。
4. 启动`rpcrouter -dr 2`或`rpcrouter -adr 2`(启用OAuth2), 和`HelloWorldsvr`.
5. 将上边提到的`./dist`目录,`HelloWorld.html`和`HelloWorlddesc.json`拷贝到WebServer预设的目录下。该目录应该已经在`nginx`或者`apache`的配置中设置好，例如`/var/www/html/rpcf`.
6. 重启web服务器。
7. 在浏览器中, 打开网页`https://example.com/rpcf/HelloWorld.html`。输出结果在浏览器调试窗口的控制台中。
8. 有关OAuth2的更多信息请参考这篇[说明](../rpc/security/README.md#oauth2-support)


