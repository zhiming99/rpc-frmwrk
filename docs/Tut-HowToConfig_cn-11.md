# rpc-frmwrk开发教程
## 第十一节 如何配置和启动rpc-frmwrk
rpc-frmwrk是一个高度可定制化的平台，有较丰富的配置选项可供定制化。

### 基本配置
* 服务器IP地址(缺省127.0.0.1)
* 服务端口号(缺省4132)
* 压缩传输(压缩可以减小传输数据量50%)

### 确定RPC连接的需求
* **最简需求**，就是设置一下IP地址和端口号即可。可以通过`rpcfctl cfg(rpcfg.py)`或者`rpcfctl tui`命令进行配置。`rpcfctl cfg`是图形界面的工具，而`rpcfctl tui`是字符终端下的工具，以下通称`配置工具`。注意这里的配置都是用于`rpc-frmwrk`的各个自带程序的。
  * 用户的客户端，如果是`ridlc`生成的，则须在代码目录下运行`python3 synccfg.py`，或者手工修改该目录下的`*desc.json`文件。
  * 用户的服务器端不再需要额外的配置。
* **初级安全需求**，具有SSL安全连接。 此需求需要先运行`rpcfctl initsvr`或者`rpcfctl initcli`，生成传输密钥，再通过`配置工具`在网络连接页面勾选`启用SSL`. 如果需要使用机构认证的正式密钥，可以使用`rpcfctl importkeys`进行导入。
* **较高级安全需求**，即需要密码认证功能连接，除了前两步的设置，还必须在`配置工具`的`安全页面`勾选`SimpAuth`, 并在网络连接页面勾选`Enable Authentication`选项。
* **SSO(单点登录)登陆认证** 可以选择Kerberos，或者OAuth2，并填写相关的参数。Kerberos务必要先安装好kerberos服务器。如果是Active Driectory, 需要和管理联系商讨。如果是和rpc-frmwrk安装在一起的, 可以通过`配置工具`的安全页的`Initialize KDC`按钮进行自动配置。有关如何填写Kerberos或OAuth2的登陆参数可以参考[rpcfctl cfg(rpcfg.py)使用说明](../tools/README_cn.md#安全页security).
* **穿透防火墙和Web代理**，可以勾选网络页面的`Enable WebSocket`和填写`WebSocket URL`。此时就需要和管理员联系了。如果有权限自行安装Web服务器了，也可以使用`rpc-frmwrk`的自动配置功能`rpcfctl cfgweb`，配置同一台机器上的`nginx`或`apache`.
* **实时监控`rpc-frmwrk`系统运行**，可以使用`SimpAuth`或`OAuth2`+`WebSocket`+`OpenSSL`+`nginx`的组合，通过网页访问`rpc-frmwrk`的监控器。用户的业务逻辑也将以同样的连接进行传输。
* **使用`GmSSL`(国产SSL)**，可以在`安全页面`选中`GmSSL`。这时你需要同步修改关联的密钥和证书路径。自签名的密钥和证书，可以在`~/.rpcf/gmssl/`下面找到。

### 启动服务器
配置好后，可以按如下步骤启动服务器
* 有webserver的，确保webserver已启动。有Kerberos服务器的确保服务器已启动。
* 有自装的Kerberos服务器的，确保kdc已启动。
* 不需要监控的，非认证对话，运行`rpcrouter -dr 2`, 需要认证功能的，运行`rpcrouter -adr 2`
* 需要监控的，服务器端需运行`rpcfctl restartall`，而不再运行命令`rpcrouter -dr 2`
* 启动业务服务器

### 启动客户端
配置好后，可以按如下步骤连接服务器
* 运行`rpcrouter -dr 1`, 需要认证该功能的，运行`rpcrouter -adr 1`
* 有认证功能的，需要运行`rpcfctl login`进行预认证，注意此操作一般只需一次，便可在数天内不需重复登陆。
* 运行业务客户端程序。
* 如果只是连接监控器，则跳过前面的三步，在浏览器里打开链接https://your.server.com/rpcf/appmon.html，没有域名的，ip地址也可以。

### 故障排查
* 如果启动客户端后，发现连接故障，可以按下面的列表排查：
  * 检查连接参数是否一致，服务器和客户端应该使用匹配的参数，比如一边启用认证，另一边没有，一边启用SSL，另一边没有，或者一边启用WebSocket,另一边没有。
  * 检查守护进程是否启动，即rpcrouter是否启动，并且参数是否匹配。有认证的会有`-a`选项，没认证的没有`-a`。客户端参数必须有`-r 1`, 服务器端必须有`-r 2`.去除命令行中的`-d`选项，可以让`rpcrouter`打印到控制台，以便查看输出信息。rpcrouter有`-L<0-6>`选项，可以设置`-L6`以输出更多的运行信息，`-L0`则不打印运行信息。
  * 客户端在需要认证的情况时，要先运行`rpcfctl login`，成功后，客户端才能连接服务器。
  * 使用`rpc-frmwrk`自带的测试程序`hwsvrsmk`和`hwclismk`进行检测。
  * 如果修改了配置参数，需要重启守护进程。可以直接`kill -2 $(pidof rpcrouter)`。
  * 高级故障排查方法：
    * 使用wireshark确定出错方，服务器还是客户端。
    * 可使用dbus-monitor可以查看客户端或者服务器端和守护进程的通信. 如果dbus-monitor报错‘Unable to autolaunch a dbus-daemon without a \$DISPLAY for X11’，则需设置环境变量`export DBUS_SESSION_BUS_ADDRESS=$(cat $HOME/.rpcf/dbusaddr)`。
    
### 其他配置选项
* 启用流量控制，限制每个对话的带宽。在网络页的配置中。具体的流量数值，目前需要通过监视器设置。
* 设置最大连接数, 限制本服务器的最大连接数。在安全页的`misc options`中。

### 级联
* `rpcrouter`工作在 `-r2`时，是可以进行级联的，也就是适当配置后， 客户端可以通过一个根`rpcrouter`可以访问到其上游的节点的`rpcrouter`。这个叶子节点的`rpcrouter`也可以暴露它的上游节点，供客户端访问。
* 有关配置级联的详细说明，可参考[`rpcfctl cfg`](../tools/README_cn.md#级联页multihop)的相关叙述。
* 在网络页的`router path`也是为此目的设置的。

## 附录
# rpc-frmwrk需求功能矩阵

| 需求种类 | 启用SSL | 密码认证(SimpAuth) | 开启认证功能 | 启用Kerberos | 启用OAuth2 | 启用WebSocket | 启用OpenSSL | 启用GmSSL | Nginx/Apache 代理 |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 最简需求（IP+端口） | | | | | | | | | |
| 初级安全（SSL 传输加密） | ✅ | | | | | | ✅ |✅| |
| 较高级安全（密码认证） | ✅ | ✅ | ✅ | | | | ✅ |✅| |
| SSO 登录认证（Kerberos） | ✅ | | ✅ | ✅ | | | ✅ |✅| |
| SSO 登录认证（OAuth2） | ✅ | | ✅ | | ✅ | ✅| ✅ | | |
| 穿透防火墙/Web 代理 | ✅ | | | | | ✅ | ✅ | | ✅ |
| 实时监控（Web 访问） | ✅ | ✅ | ✅ | | ✅ | ✅ | ✅ | | ✅ |

---

## 配置说明

### 通用前置步骤
- **最简需求**：设置 IP 地址和端口号即可，通过 `配置工具`设置。

### 各级需求配置要点

| 需求等级 | 关键命令 / 操作 | 说明 |
|---|---|---|
| 最简 | `rpcfg.py` / `rpcfctl tui` | 仅配置 IP + 端口 |
| 初级（SSL） | `rpcfctl initsvr` / `rpcfctl initcli` → 生成密钥 → 勾选「启用 SSL」 | 自签名密钥默认路径 `~/.rpcf/`；非自签名用 `rpcfctl importkeys` |
| 较高级（密码认证） | 上述 + 安全页勾选 `SimpAuth` + 网络页勾选 `Enable Authentication` | 双层保护：加密 + 认证 |
| Kerberos SSO | 上述 + 安装 Kerberos 服务器 + 勾选 Kerberos 参数 | Active Directory 需联系管理员；同机部署可用 `配置工具` → 安全页 → `Initialize KDC` 自动配置 |
| OAuth2 SSO | 上述 + 填写 OAuth2 相关参数 | 需提前在身份提供商处注册应用, 在测试环境也可使用rpc-frmwrk的OAuth2样例服务器，参考`3`中有相关介绍 |
| 穿透防火墙/代理 | 上述 + 网络页勾选 `Enable WebSocket` + 填写 `WebSocket URL` | 需联系管理员或自行配置 Nginx/Apache |
| 实时监控 | SimpAuth/OAuth2 + WebSocket + OpenSSL + Nginx | 业务逻辑复用同一连接，通过网页访问监控器 |
| 国密 GmSSL | 安全页勾选 `GmSSL` + 修改密钥和证书路径 | 自签名密钥和证书位于 `~/.rpcf/gmssl/` |

---

## 依赖关系速查

```
最简需求
 └─ SSL → OpenSSL
      ├─ SimpAuth
      │    └─ WebSocket(+Nginx) → 实时监控
      ├─ OAuth2 → WebSocket → Nginx → 实时监控
      ├─ Kerberos → KDC/AD
      └─ 国密 → GmSSL(RPC-only)
           ❌ GmSSL在有Web服务器/浏览器/OAuth2/监控网页等场景下不可用
```

> **核心原则**：每一级都在上一级基础上叠加功能，不存在「跳级单独启用」的情况（除 GmSSL 可视为 SSL 的替换变体）。
## 参考
1. [配置工具的详细介绍](../tools/README_cn.md)
2. [rpcfctl的详细介绍](../monitor/appmon/rpcfctl_cn.md)
3. [安全认证的详细介绍](../rpc/security/README_cn.md)
4. [WebSocket支持的详细介绍](../rpc/wsport/Readme.md)
5. [rpc-frwmrk监控器的详细介绍](../monitor/client/js/appmoncli/README_cn.md)
6. [国密GmSSL的详细介绍](../rpc/gmsslport/README_cn.md)