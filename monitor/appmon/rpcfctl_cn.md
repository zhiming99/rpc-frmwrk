# rpcfctl - rpc-frmwrk 应用控制工具

## 简介

`rpcfctl` 是 rpc-frmwrk 框架下用于管理应用、用户、组、认证和安全设置的命令行工具。它支持启动、停止、重启、查看状态、列出应用，以及添加、删除、初始化应用，修改应用设置，进行用户管理。同时还支持 SSL 密钥/证书管理、认证、Web 和 Kerberos 集成配置，以及日志管理等高级功能。

---

## 用法

```bash
rpcfctl <命令> [参数]
```

---

## 主要命令

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

---

## 注册表的初始化

- **initappreg**  
  初始化（清空）应用注册表。**警告：此操作会清空应用注册表。**

- **initusereg**  
  初始化（清空）用户注册表。**警告：此操作会清空用户注册表。**

- **initsvr**  
  初始化服务器端的应用和用户注册表。等价于执行initusereg和initappreg.

- **initcli**  
  初始化客户端运行环境，用于需要安全认证的客户端。

## 应用的注册和配置管理

- **addapp <应用名>**  
  添加新应用到注册表。

- **rmapp <应用名>**  
  从注册表移除应用。

- **showapp <应用名>**  
  显示指定应用的详细信息。

- **cloneapp <待克隆应用名> <新的应用名>**
  添加新的应用，并把已有的应用的内容拷贝到新的应用中。

- **addpoint <应用名> <点名> <点类型> <值类型>**  
  向应用添加新点。点类型为 input、output 或 setpoint，值类型为 b(byte)、w(word)、i(int)、q(int64)、d(double)、f(float)、s(string, 小于94字符)、blob。

- **rmpoint <应用名> <点名>**  
  从应用中移除点。

- **setpv <应用名> <点名> <点类型> <值>**  
  设置指定应用点的值。

- **getpv <应用名> <点名>**  
  获取指定应用点的值。

- **setattr [-f] <应用名> <点名> <属性> <属性类型> <值>**  
  设置指定应用点的属性。属性类型同上，`-f` 选项用于 blob 类型，值为文件路径，文件内容作为属性值（文件小于8MB）。

- **getattr <应用名> <点名> <属性>**  
  获取指定应用点的属性值。

- **addlink <输出应用> <输出点> <输入应用> <输入点>**  
  在注册表中添加一对输出点和输入点之间的链接。

- **rmlink <应用1> <点1> <应用2> <点2>**  
  移除一对输出点和输入点之间的链接，顺序不限。

---

## 用户和用户组的管理
- **adduser <用户名>**  
  添加用户到用户注册表。

- **rmuser <用户名>**  
  从用户注册表移除用户。

- **showuser <用户名>**  
  显示用户详细信息。

- **listuser**  
  列出所有用户。

- **password <用户名>**  
  修改`SimpAuth`用户的密码。

- **addgroup <组名>**  
  添加组到组注册表。

- **rmgroup <组名>**  
  从组注册表移除组。

- **showgroup <组名>**  
  显示组详细信息。

- **listgroup**  
  列出所有组。

- **joingroup <组名> <用户名>**  
  将用户加入组。

- **leavegroup <组名> <用户名>**  
  将用户从组中移除。

---

## 日志管理

- **backuplog <应用名> <点名>**  
  备份指定设置点的日志文件。备份文件在用户的`$HOME`下，名字如<应用名>-<点名>-MM-DD.tar.gz）。

- **rotatelog <应用名> <点名>**  
  对指定设置点的日志文件进行轮转。

- **listlog <应用名> <点名>**  
  列出指定设置点的所有日志文件。

- **enablelog <应用名> <点名>**  
  启用指定设置点的日志记录。

- **disablelog <应用名> <点名>**  
  禁用指定的设置点的日志记录。

---

## 认证与安全

- **importkey <openssl|gmssl> <私钥文件> <证书文件> [<CA证书>]**  
  导入 rpc-frmwrk 的 SSL 密钥和证书。

- **genkey <openssl|gmssl> <客户端密钥数> <服务端密钥数> [<DNS 名称>]**  
  生成自签名公私钥和证书对。

- **printcert <openssl|gmssl> <证书文件>**  
  打印证书详细信息。

- **authmech**  
  打印当前使用的认证机制。

- **login [参数]**  
  使用 driver.json 中指定的认证机制进行用户认证。  
  - **SimpAuth(Password)：** login 会将凭证保存在安全位置，允许非 JS 客户端从命令行连接 rpc-frmwrk 服务器。JS 客户端通过浏览器登录。  
  - **Kerberos：** login 会通过 kinit 进行预认证登录，从 KDC 获取票据。  
  - **OAuth2：** login 会发起授权码流程，从 rpc-frmwrk 获取安全令牌（非 access token），非 JS 客户端可用该令牌连接 rpc-frmwrk。

---

## 配置与备份

- **geninitcfg [-c] [<输出文件>]**  
  用当前系统设置，生成用于部署的配置文件`initcfg.json`，此文件供检查和调试使用, 也可以用于调整本地的设置，或者作为下面的`geninstaller`命令的输入。`-c`选项要求生成用于客户端的`initcfg.json`文件。

- **geninstaller <initcfg的路径名> <生成的installer的存放目录> [<rpc-frmwrk安装包所在目录>]**
  用当前系统设置，生成部署到目标服务器和客户端的部署包。`initcfg`即为前一个命令所说的`initcfg.json`。当你`make deb`或者`make rpm`成功后，会在`rpc-frmwrk/bldpkg`目录下存放所有的`rpc-frmwrk`的deb包或者rpm包。这个目录就是rpc-frmwrk安装包所在的目录。如果`initcfg`是client专用的，那么只生成客户端部署包，其他情况下，将生成一个服务器包和一个客户端包。如果待部署系统的设置和当前的系统设置不同时，可以手工编辑`initcfg.json`，或使用[`rpcfg.py`](../../tools/README_cn.md#rpc-frmwrk配置工具)生成。手工编辑`initcfg.json`需注意不要更改里面的文件路径，`geninstaller`需要这些信息，并且安装包会在安装前修正。另外，如果SSL的密钥和证书不是`rpc-frmwrk`生成的自签名密钥和证书，`geninstaller`一次只生成一个部署包，由`initcfg.json`标注的是客户端还是服务器端决定。由于安装包里含有密钥等关键安全信息，需要妥善保存，防止发生安全问题。

- **updcfg <initcfg的路径名>**
  使用指定的`initcfg.json`文件更新本系统的rpc-frmwrk配置文件。本命令使用手动修改的`initcfg.json`更新rpc-frmwrk的系统设置。

- **cfgweb**  
  使用当前 rpc-frmwrk 配置更新 Web 服务器（nginx 或 apache）配置。

- **cfgkrb5**  
  使用当前 rpc-frmwrk 配置更新 Kerberos 配置。

- **backup**  
  备份服务器端设置，输出为 tar 包（rpcf-backup-YYMMHH.tar.gz）。

- **restore <备份文件>**  
  从 tar 包恢复服务器端设置。

---

## 其他
- **clearmount**  

- **-h, --help**  
  显示帮助信息并退出。

---

## 示例

```bash
rpcfctl start myapp
rpcfctl setpv abcapp mypoint i 0
rpcfctl getattr abcapp mypoint ptype
rpcfctl addlink abcapp1 algout abcapp2 algin
rpcfctl importkey openssl /path/to/key.pem /path/to/cert.pem /path/to/cacert.pem
rpcfctl geninitcfg > /tmp/initcfg.json
rpcfctl login alice
```

---

## 相关文件

- showapp.sh, getpv.sh, setpv.sh, listapps.sh, addapp.sh, rmapp.sh, initappreg.sh, inituser.sh, updattr.py, updatekeys.py, rpcfaddu.sh, rpcfrmu.sh, rpcfshow.sh, rpcfaddg.sh, rpcfrmg.sh, rpcfmodu.sh, updwscfg.sh, updk5cfg.sh, sinit, oinit.py, rpcfg.py logtool.py 
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

本程序为自由软件，可根据 GNU 通用公共许可证第 3 版（GPLv3）进行再发布和修改。详情参见 http://www.gnu.org/licenses/gpl-3.0.html

