# rpcfctl - rpc-frmwrk 控制工具

## 简介

`rpcfctl` 是 rpc-frmwrk 框架下用于管理应用、用户、组、认证和安全设置的命令行工具。它支持启动、停止、重启、查看状态、列出应用，以及添加、删除、初始化应用和用户注册表。同时还支持 SSL 密钥/证书管理、认证、Web 和 Kerberos 集成配置等功能。

---

## 用法

```bash
rpcfctl <子命令> [参数]
```

---

## 常用命令

### 应用管理命令

- **start <应用名>**  
  启动指定应用。可以通过list命令获得可以启动的应用

- **stop <应用名>**  
  优雅地停止指定应用（SIGINT）。

- **kill <应用名>**  
  强制停止指定应用（SIGKILL）。

- **restart <应用名>**  
  重启指定应用。

- **status <应用名>**  
  显示指定应用的运行状态。

- **list**  
  列出所有已注册应用及其状态。

- **startall**  
  启动所有受管应用（核心服务优先）。

- **stopall**  
  停止所有受管应用（核心服务最后停止）。

- **restartall**  
  重启所有受管应用。

- **addapp <应用名>**  
  添加新应用到注册表。

- **rmapp <应用名>**  
  从注册表移除应用。

- **showapp <应用名>**  
  显示指定应用的详细信息。

- **initapps**  
  初始化（清空）应用注册表。**警告：此操作会清空应用注册表。**

- **inituser**  
  初始化（清空）用户注册表。**警告：此操作会清空用户注册表。**

- **initsvr**  
  初始化服务器端的应用和用户注册表。等价于执行inituser和initapps.

- **initcli**  
  初始化客户端运行环境，用于需要安全认证的客户端。

### 密钥管理命令
- **importkey <openssl|gmssl> <私钥文件> <证书文件> [<CA证书>]**  
  导入 rpc-frmwrk 的 SSL 密钥和证书。

- **genkey <openssl|gmssl> <客户端密钥数> <服务端密钥数> [<DNS 名称>]**  
  生成自签名公私钥和证书对。

- **printcert <openssl|gmssl> <证书文件>**  
  打印证书详细信息。

### 用户管理命令

- **adduser <用户名>**  
  添加用户到用户注册表。

- **rmuser <用户名>**  
  从用户注册表移除用户。

- **showuser <用户名>**  
  显示用户详细信息。

- **listuser**  
  列出所有用户。

- **addgroup <组名>**  
  添加用户组到注册表。

- **rmgroup <组名>**  
  从注册表移除用户组。

- **showgroup <组名>**  
  显示用户组详细信息。

- **listgroup**  
  列出所有用户组。

- **joingroup <组名> <用户名>**  
  将用户加入指定的用户组。

- **leavegroup <组名> <用户名>**  
  将用户从用户组中移除。

- **authmech**  
  显示当前使用的认证机制。

- **login [参数]**  
  使用 driver.json 中指定的认证机制进行用户认证。  
  - **SimpAuth(Password)：** login 会将凭证保存在安全位置，允许非 JS 客户端从命令行连接 rpc-frmwrk 服务器。JS 客户端通过浏览器登录。  
  - **krb5：** login 会通过 kinit 进行预认证登录，从 KDC 获取票据。  
  - **OAuth2：** login 会发起授权码流程，从 rpc-frmwrk 获取安全令牌（非 access token），非 JS 客户端可用该令牌连接 rpc-frmwrk。

### 配置管理命令

- **config**  
  启动rpc-frmwrk的配置工具rpcfg.py。

- **geninitcfg [<输出文件>]**  
  生成`rpc-frmwrk`的配置导出文件。该文件用于在别的系统上，重建rpc-frmwrk的配置文件集合。此命令主要用于调试时，发现配置错误的用途，并不单一使用。一般用`rpcfctl backup`和`rpcfctl restore`来完成配置的导出和恢复。

- **cfgweb**  
  使用当前 rpc-frmwrk 配置更新 Web 服务器（nginx 或 apache）配置。

- **cfgkrb5**  
  使用当前 rpc-frmwrk 配置更新 Kerberos 配置。

- **backup**  
  备份服务器端设置，输出为 tar 包（rpcf-backup-YYMMHH.tar.gz）。

- **restore <备份文件>**  
  从 tar 包恢复服务器端设置。

- **-h, --help**  
  显示帮助信息并退出。

---

## 示例

```bash
rpcfctl config
rpcfctl list
rpcfctl start myapp
rpcfctl adduser alice
rpcfctl importkey openssl /path/to/key.pem /path/to/cert.pem /path/to/cacert.pem
rpcfctl geninitcfg > /tmp/initcfg.json
rpcfctl login alice
```

---

## 相关文件

- showapp.sh, getpv.sh, listapps.sh, addapp.sh, rmapp.sh, initappreg.sh, inituser.sh, updattr.py, updatekeys.py, rpcfaddu.sh, rpcfrmu.sh, rpcfshow.sh, rpcfaddg.sh, rpcfrmg.sh, rpcfmodu.sh, updwscfg.sh, updk5cfg.sh, sinit, oinit.py, rpcfg.py  
  这些辅助脚本通常与 rpcfctl 位于同一目录或 `rpcf` 子目录下。

## 名词解释
`应用`在本文中指支持rpc-frmwrk的monitor功能的服务器程序。它可以由`ridlc`生成，或者手写实现。一个支持monitor的程序还必须在注册表中注册，才能使`appmonsvr`(monitor的服务器程序)和该应用程序建立起通信和管理。注册的过程如下：
* 由`ridlc`生成一个程序框架，并通过选项`-m <appname>`来指定注册表中使用的名称。
* 在终端运行命令`rpcfctl addapp <appname>`,就完成了99%的注册。
* 最后一步，手动运行rpc服务器端的程序。这一过程将会向注册表添加启动信息。当下次启动该服务器时，就可以使用`rpcfctl startall` 或者`rpcfctl start <appname>`在启动该服务器程序了。

---

## 参见

- appmonsvr (1)
- rpcrouter (1)
- regfsmnt (1)

---

## 作者

Ming Zhi <woodhead99@gmail.com>

---

## 许可证

本程序为自由软件，可根据 GNU 通用公共许可证第 3 版（GPLv3）进行再发布和修改。详情参见 http://www.gnu.org/licenses/gpl