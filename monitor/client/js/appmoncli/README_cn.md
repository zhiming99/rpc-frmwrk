[English](./README.md)
### 功能简介
   * 本监视器可以监控服务器上部署的`rpc-frmwrk`的所有应用程序，提供实时数据和设置运行参数。
   * 本监视器也可以监视以`multihop`方式连接到`rpc-frmwrk`的网关服务器的设备的运行。
   * 本监视器使用`用户名+密码`的方式登陆，后端可以接内建的`SimpAuth`认证，或者第三方的`OAuth2`认证。

### 部署和使用网页版Rpc-Frmwrk监视器
   * 首先确保目标机器安装了`nginx`。`apache`也可以使用，不过目前还没测试。
   * 成功安装和配置好rpc-frmwrk后，在`/usr/local/etc/rpcf/`或`/etc/rpcf`下的`appmonui.tar.gz`是打包好的文件。如何配置rpc-frmwrk可参考这篇[文章](../../../../tools/README_cn.md#rpc-frmwrk配置工具).
   * 通过命令`rpcfctl initsvr`建立监视器运行环境。该命令将建立`用户注册表`和`应用注册表`，和一个`admin`用户。
   * 把`appmonui.tar.gz`解压到`nginx`配置好的目录中。
   * 然后通过命令`rpcfctl startall`启动`rpc-frwmrk`的所有程序。
   * 最后打开浏览器，在地址栏输入`https://127.0.0.1/rpcf/appmon.html`即可。 对于自签名的证书，需无视浏览器发出的警告。
   * 提示输入用户名和密码时，可以输入`admin`和账号的密码。
   * 如果你希望业务服务器也可以被监控程序管理，可以通过一下步骤实现：
      * 可以在生成业务服务器框架代码的`ridlc`的命令行加上 `-m <应用名称>`，更新相关的代码。该选项只影响服务器端代码。
      * 使用命令`rpcfctl addapp <应用名称>`注册该程序。
      * 手工运行一次该服务器，以便其注册启动信息和其他一些实例信息。
      * 此时该应用已经可以通过监视器网页进行监控了。zhiming99/django-oa2cli-cgi/general:SimpAuth-https-2
      * 关于如何定制监视的内容，不久将会有新的文章进行叙述，请耐心等待。
   * `zhiming99/django-oa2cli-cgi:SimpAuth-https-2`是一个docker镜像，有最新的监视器的演示程序。使用`docker pull`可下载使用。
   * 下面是监视器程序的一些用户界面：
   * 登陆界面   
   ![login](../../../../pics/mon-login.png)
   * 主界面   
   ![main](../../../../pics/mon-main.png)
   * 应用详情页面   
   ![appdetail](../../../../pics/mon-rpcrouter.png)
   * 更改配置页面   
   ![setpv](../../../../pics/mon-setpv.png)
   * 查看设置点位日志页面   
   ![ptlog-chart](../../../../pics/mon-ptlog-chart.png)
