[English](./README.md)
- [rpc-frmwrk配置工具](#rpc-frmwrk配置工具)
  - [连接页](#连接页)
  - [安全页](#安全页)


# rpc-frmwrk配置工具
[`rpcfg.py`](./rpcfg.py)是一个配置和部署`rpc-frmwrk`的图形化工具。它能够生成`rpc-frmwrk`系统级的配置文件，如`driver.json, router.json, rtauth.json和authprxy.json`。这些文件对于`rpcrouter`或者`紧凑模式`的用户应用程序很重要。同时它也具备简单的密钥管理，Kerberos服务器的自动配置，以及nginx或者httpd的自动配置能力。并且能够生成安装rpc-frmwrk，部署密钥，配置web服务器和KDC主机的安装包，方便部署和维护。

`rpcfg.py`的UI界面是一组在对话框下的标签页，我们将分别介绍`连接页`，`安全页`，`级联页`和`负载均衡页`。
## 连接页
  连接页主要用于设置`rpc-frmwrk`服务器的ip地址，端口号，以及传输协议或者选项等。
  * 绑定地址(Binding IP): 对于服务器来说是监听的IP地址。对于客户端来说，则是需要连接的服务器地址。
  * 端口号(Listening Port): RPC服务的端口号。缺省是4132。
  * 安全连接(SSL): RPC连接将使用SSL协议进行加密，具体使用OpenSSL还是GmSSL，密钥的存放等在安全页配置。也就是说所有的连接，如果启用SSL，则会用同一套SSL设置。
  * WebSocket： RPC连接将使用WebSocket协议传输，`WebSocket URL`编辑框里指定进行websocket通信的网络连接。注意这里使用的是https, 而不是wss。处于安全的考虑，启用websocket将强制开启SSL选项。
  * 压缩(Compression)：对传输数据进行压缩。
  * 身份认证(Auth)：对连接用户进行身份认证。具体使用Kerberos或者OAuth2在安全页进行配置。和SSL类似。所有的连接，如果启用了身份认证，那么会用同样的认证设置。
  * 添加网络接口(Add Interface): 如果还需要绑定不同的IP地址，可以使用这个按钮添加。
  * 删除网络接口(Remove Interface): 按此按钮将删除按钮上方的设置。
  * 连接页的布局:   
        ![rpcfg tab1](../pics/rpcfg.png)
## 安全页
  安全页的`安全连接文件`和`身份认证信息`两部分只有在连接页的某个网络接口启用了安全连接或者身份认证功能时，才生效。  
  * 安全连接文件   
      SSL文件是用于安全连接协议的一组密钥文件。这些密钥文件可以是从证书机构获得的密钥和证书，也可以是自行制作的在内部使用的密钥和证书。这两种证书对于`rpc-frmwrk`来说没有区别，只是第一种证书可能预先安装在了操作系统中。   
        ![rpcfg tab2](../pics/rpcfg2-1.png)
      * 密钥文件(Key File): 密钥文件路径名。
      * 证书文件(Cert File): 密钥的证书路径名。
      * CA证书(CA cert file)： CA的证书路径名。用于需要`互相验证(Peer Verify)`的场景。如果目标系统的/etc/ssl/certs里已经有颁发机构的证书，可以忽略此项。
      * 密码文件(secret file): 密钥有密码保护时，该文件存有密钥文件的密码. 
      * 启用国密安全连接(Using GmSSL): 连接将使用GmSSL的国产的安全加密套件(SM2+SM4). OpenSSL一般使用的是RSA+AES。
      * 互相验证(Verify Peer): 在收到对方证书后，先验证证书是否合法，如不合法终止握手流程。
      * 生成自签名密钥(Gen Self-signed Keys): 生成自己签名的密钥和证书。不启用GmSSL时，用openssl命令生成密钥和证书，启用GmSSL时，用gmssl命令生成密钥和证书。
        * 客户端密钥个数(Number of client keys): 要生成/部署的客户端密钥个数。
        * 服务器端密钥个数(Number of Server keys): 要生成/部署的服务器端密钥个数。   
            ![自签名密钥对话框](../pics/gen-self-signed-key.png)
        * 自动生成密钥后，`密钥文件`，`证书文件`，和`CA证书`将自动替换成新生成服务器端的密钥。
  * 身份认证信息(Auth Information)   
        ![rpcfg2-2.png](../pics/rpcfg2-2.png)
    * 认证机制(Auth Mech): 该下拉菜单目前有Kerberos和OAuth2两项。   
        ![select-auth](../pics/select-auth-mech.png)
    * Kerberos的认证信息:
        Kerberos的详细介绍请参看这篇[文章](../rpc/security/README_cn.md)
      * 服务名(Service Name), 服务器端的服务名称，比如`rpcrouter@host1`
      * 域名(Realm), 比如`rpcf.org`
      * 签名/加密，指定数据包的加密方式。签名是在包尾追加一个数字签名防止伪造，加密费时，更安全。
      * KDC地址(KDC address), Kerberos服务器的ip地址。
      * 用户名，客户端的用户名，用于客户端的设置。
      * 启用/停用KProxy，KProxy可中继kinit的登陆请求，到客户端访问不到，服务器端可以访问的Kerberos服务器. 按此按钮将在本体安装的rpc-frmwrk中启用。如果已经启用，那么停用此功能。
      * 更新本地的认证设置(Update Auth Settings)：按此按钮更西本地安装的rpc-frmwrk的身份认证设这。
      * 初始化本地的Kerberos服务器(Initialize KDC):如果本地机器上安装了Kerberos服务器，可以一按此按钮进行初始化。
      * 不对本地`rpc-frmwrk`的设置做更新：勾选此选项，在按底部的OK按钮时，不对本地的身份认证设置作改动。
    * OAuth2的信息：
      OAuth2的认证发生在`rpc-frmwrk`之外，所以不需要用户信息。有关OAuth2的详细信息请参看这篇[文章](../rpc/security/README_cn.md#oauth2)
      * OAuth2检查器地址(Checker IP): OAuth2检查器是一个嵌入到`django`, `springboot`或者`cgi`中的`rpc-frmwrk`服务器。用于检查指定session的登陆状态。如果检查器和业务`rpc-frmwrk`在一个主机上，这个字段保持空白即可。如果不在同一主机上，填写检查器所在的IP地址。
      * 检查器的端口: 当检查器的IP地址有效时，这里填写检查器的端口号。
  * 杂项(Misc Options)   
        ![rpcfg2-3.png](../pics/rpcfg2-3.png)
    * 最大连接数(Max Connections):系统允许的最大连接数。
    * 客户端任务调度器(Task Scheduler): 勾选此选项，将在reqfwdr中启用任务调度器。任务调度器对于有多个客户端，公用同一个`reqfwdr`(rpcrouter -r 1)时，可以公平调度。该调度器在使用[fastrpc](../Concept_cn.md#fastrpc和builtinrt-app)的项目中并不起作用。
  * 安装包选项(Installer Options)：   
        ![rpcfg2-4.png](../pics/rpcfg2-4.png)
    * 安装包将配置nginx或者apache服务器(WS Installer): 安装包将根据本地安装的web服务器类型，配置目标机器的web服务器的SSL选项，websocket选项, 并安装服务器端或客户端密钥和证书。
    * 安装包将安装Kerberos的设置和用户信息(Krb5 Installer): 安装信息将分为服务方的认证信息和客户端的认证信息，分别进行配置，条件是Kerberos的服务器必须在本地机器上。
    * 安装包将在客户端启用KProxy功能。
    * `rpc-frmwrk`的`rpm`或`deb`包的路径(deb/rpm package): 如果制定路径，且`rpm`或`deb`有效，安装包将打包`rpm`或者`deb`文件，并在目标机器上首先安装`rpc-frmwrk`，然后进行设置。
    * `安装包`按钮(Installer)：
        * 将生成部署用的安装包两个，一个是服务器端，一个是客户端。
        * 安装包的名字如下图所示：   
        ![安装包名称](../pics/installer-name.png)
