# rpc-frmwrk开发教程
## 第十一节 如何配置和启动rpc-frmwrk
rpc-frmwrk是一个高度可定制化的平台，有繁多的配置选项可供定制化。

### 基本配置
* 服务器IP地址(缺省127.0.0.1)
* 服务端口号(缺省4132)
* 压缩传输(压缩可以减小传输数据量50%)

### 确定RPC连接的需求
* **最简需求**，就是设置一下IP地址和端口号即可。可以通过`rpcfctl cfg`或者`rpcfctl tui`进行配置。
* **初级安全需求**，要求传输有SSL, 这个就需要先运行`rpcfctl initsvr`或者`rpcfctl initcli`，生成传输密钥，再通过`rpcfg.py`或者`rpcfctl tui`在网络连接页面勾选`启用SSL`. 如果需要使用非自签名的密钥，可以使用`rpcfctl importkeys`进行设置。
* **较高级安全需求**，要求有密码认证功能的，除了前两步的设置，还必须在配置工具的`安全页面`勾选`SimpAuth`, 并在网络连接页面勾选`Enable Authentication`选项。
* **SSO(单点登录)登陆认证** 可以选择kerberos，或者OAuth2，并填写相关的参数。Kerberos务必要先安装好kerberos服务器。如果是Active Driectory, 需要和管理联系商讨。如果是和rpc-frmwrk安装在一起的, 可以使用`rpcfctl tui`在安全页的`Initialize KDC`进行自动配置。
* **穿透防火墙和Web代理**，可以勾选网络页面的`Enable WebSocket`和填写`WebSocket URL`。此时就需要和管理员联系了。如果有权限自行安装Web服务器了，也可以使用`rpc-frmwrk`的自动配置功能`rpcfctl cfgweb`，配置同一台机器上的`nginx`或`apache`.
* **实时监控`rpc-frmwrk`系统运行**，可以使用`SimpAuth`或`OAuth2`+`WebSocket`+`OpenSSL`+`nginx`的组合，通过网页访问`rpc-frmwrk`的监控器。用户的业务逻辑也将以同样的连接进行传输。
* **使用`GmSSL`(国产SSL)**，可以在`安全页面`选中`GmSSL`。这时你需要同步修改关联的密钥和证书路径。自签名的密钥和证书，可以在`~/.rpcf/gmssl/`下面找到。

### 启动服务器
配置好后，可以按如下步骤启动服务器
* 有webserver的，确保webserver 已启动。有Kerberos服务器的确保服务器已启动。
* 有自装的Kerberos服务器的，确保kdc
* 不需要监控的，运行`rpcrouter -dr 2`, 需要认证该功能的，运行`rpcrouter -adr 2`
* 需要监控的，运行`rpcfctl restartall`
* 启动业务服务器

### 启动客户端
配置好后，可以按如下步骤连接服务器
* 运行`rpcrouter -dr 1`, 需要认证该功能的，运行`rpcrouter -adr 1`
* 有认证功能的，需要运行`rpcfctl login`进行预认证。
* 运行业务客户端程序。
* 如果只是连接监控器，则跳过前面的三步，在浏览器里打开链接https://your.server.com/rpcf/appmon.html，没有域名的，ip地址也可以。

### 故障排查
* 如果启动客户端后，发现连接不上可以按下面的列表排查：
  * 检查连接参数是否一致，服务器和客户端应该使用匹配的参数，比如一边启用认证，另一边没有，一边启用SSL，另一边没有，或者一边启用WebSocket,另一边没有。
  * 检查守护进程是否启动，即rpcrouter是否启动，并且参数是否匹配。有认证的会有`-a`选项，没认证的没有`-a`。客户端参数必须有`-r 1`, 服务器端必须有`-r 2`.
  * 客户端在有认证的情况时，需要先运行`rpcfctl login`，成功后才，客户端才能连接服务器。
  * 修改了配置参数后，需要重启守护进程。
  * 高级故障排查方法：
    * 使用wireshark确定出错方，服务器还是客户端。
    * 使用dbus-monitor可以查看客户端或者服务器端和守护进程的通信
    
### 其他配置选项
* 启用流量控制，限制每个对话的带宽。在网络页的配置中。具体的流量数值，目前需要通过监视器设置。
* 设置最大连接数。在安全页的`misc options`中。

## 附录
# rpc-frmwrk需求功能矩阵

| 需求种类 | 启用SSL | 密码认证(SimpAuth) | 开启认证功能 | 启用Kerberos | 启用OAuth2 | 启用WebSocket | 启用OpenSSL | 启用GmSSL | Nginx/Apache 代理 |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 最简需求（IP+端口） | | | | | | | | | |
| 初级安全（SSL 传输加密） | ✅ | | | | | | ✅ |✅| |
| 较高级安全（密码认证） | ✅ | ✅ | ✅ | | | | ✅ |✅| |
| SSO 登录认证（Kerberos） | ✅ | | ✅ | ✅ | | | ✅ |✅| |
| SSO 登录认证（OAuth2） | ✅ | | ✅ | | ✅ | | ✅ | | |
| 穿透防火墙/Web 代理 | ✅ | | | | | ✅ | ✅ | | ✅ |
| 实时监控（Web 访问） | ✅ | ✅ | ✅ | | ✅ | ✅ | ✅ | | ✅ |
| 国密 GmSSL | | | | | | | | ✅ | |

---

## 配置说明

### 通用前置步骤
- **最简需求**：设置 IP 地址和端口号即可，通过 `rpcfg.py` 或 `rpcfctl tui` 配置。

### 各级需求配置要点

| 需求等级 | 关键命令 / 操作 | 说明 |
|---|---|---|
| 最简 | `rpcfg.py` / `rpcfctl tui` | 仅配置 IP + 端口 |
| 初级（SSL） | `rpcfctl initsvr` / `rpcfctl initcli` → 生成密钥 → 勾选「启用 SSL」 | 自签名密钥默认路径 `~/.rpcf/`；非自签名用 `rpcfctl importkeys` |
| 较高级（密码认证） | 上述 + 安全页勾选 `SimpAuth` + 网络页勾选 `Enable Authentication` | 双层保护：加密 + 认证 |
| Kerberos SSO | 上述 + 安装 Kerberos 服务器 + 勾选 Kerberos 参数 | Active Directory 需联系管理员；同机部署可用 `rpcfctl tui` → 安全页 → `Initialize KDC` 自动配置 |
| OAuth2 SSO | 上述 + 填写 OAuth2 相关参数 | 需提前在身份提供商处注册应用 |
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
      └─ 国密 → GmSSL(+Nginx, RPC-only)
           ❌ GmSSL在WebSocket/浏览器/OAuth2/监控网页下不可用
```

> **核心原则**：每一级都在上一级基础上叠加功能，不存在「跳级单独启用」的情况（除 GmSSL 可视为 SSL 的替换变体）。
## 参考信息
* [rpcfctl cfg和rpcfctl tui的详细介绍](../tools/README_cn.md)
* [rpcfctl的详细介绍](../monitor/appmon/rpcfctl_cn.md)
* [安全认证的详细介绍](../rpc/security/README_cn.md)