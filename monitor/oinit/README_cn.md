### oinit 简介
**oinit** 是一个OAuth2登陆工具，用于从非 JS 客户端（例如 C++、Python 或 Java 客户端）使用 `OAuth2 授权码`方式登录到 rpc-frmwrk 服务器。
它可以记住下次登录的登录信息和凭据，方便下次使用。也可以从命令行清理过期的登录信息。

### 安装
`oinit`是一个Python脚本工具。它和`rpc-frmwrk`的其他组件安装不太相同，不能通过`make install`的方式安装。它的安装有两种方式
1. 通过deb包或者rpm包安装。在代码树的monitor/oinit下，运行`make deb`或者`make rpm`即可在`output`目录下获得安装包。然后用`dpkg`或者`rpm`命令行工具进行安装。
2. 通过wheel包安装。在代码树的monitor/oinit下，运行`make wheel`，即可在`output`下获得wheel安装包。然后使用`pip`安装。
3. `oinit`未能整合到`rpc-frmwrk`的一键安装的架构下面，原因是它的依赖比较独特，而且不同的平台对安装的要求也和`rpc-frmwrk`的`make install`相差较大。

### 从命令行运行`oinit`
#### 命令格式
* 如果是python脚本    
`python3 oinit.py [options] [<auth url>]`
* 如果是安装了`python3-oinit` deb包，您可以使用以下命令   
`python3 -m oinit [options] [<auth url>]`
#### 命令行选项
* **-d** <对象描述文件>。从给定的`对象描述文件`中查找登录信息。
* **-c 1** 使用示例容器[`django_oa2check_https`](../rpc/security/README_cn.md#oauth2)进行OAuth2登录。
* **-c 2** 使用示例容器[`springboot_oa2check_https`](../../rpc/security/README_cn.md#oauth2)进行OAuth2登录。
* **-e** 加密本次登陆获得的凭据。
* **-r** 从存储的登录信息列表中删除。
* **-f** 默认情况下，oinit 使用“chrome”进行登录。如果指定了 `-f`，oinit将使用firefox执行登录。
#### 命令行参数
命令行参数包括`auth url`。如果命令行只有这一个参数，没有上面的命令行选项，oinit将使用这一个参数进行OAuth2登录。
`auth url`是一个部署好的rpc-frmwrk的JS应用的登陆链接。这个网页将引导进入OAuth2的登录页面，登陆成功后，返回至该部署好的JS应用页面。可以参考上面提到的示例容器中部署的JS应用, JS应用的网页在`/var/www/html/rpcf`下。
另外，如果没有给出选项或参数，oinit将尝试使用注册表中存储的登录信息执行OAuth2登录。实际上，并不需要每次都使用本工具登陆，因为很多OAuth2的服务器的登陆凭据的有效期可能有几天。

### 依赖
`oinit` 依赖于 `selenium` 用于通过UI登录。`selenium`可以使用`firefox`或者`chrome`通过OAuth2提供程序的网页进行身份验证。因此，您需要确保您的客户端机器安装了 `firefox` 的 `chrome`。并且确保安装了用于与和浏览器通信的浏览器驱动程序。`firefox` 的驱动程序在 [这里](https://github.com/mozilla/geckodriver/releases)，`chrome` 的驱动程序在此 [站点](https://developer.chrome.com/docs/chromedriver/downloads)。如果`oinit`无法启动浏览器，很可能是由于驱动程序丢失或无法通过环境变量 `PATH` 找到驱动程序。