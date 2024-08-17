[English](./README.md)
#### 名称
rpcrouter - `rpc-frmwrk`的守护进程

#### 命令格式
rpcrouter [ 选项 ]

#### 描述
rpcrouter是`rpc-frmwrk`的重要组件。
当`rpc-frmwrk`运行在`微服务模式`时，必须使用rpcrouter完成rpc请求/响应/事件的传输. 运行在服务器端，接收请求的rpcrouter，又叫`bridge`, 命令行必须使用`-r 2`。运行在客户端，发送请求的rpcrouter, 又叫`reqfwdr`, 命令行必须使用`-r 1`. 一台机器即接收请求，也发送请求的, 就启动两个示例，一个`bridge`, 一个`reqfwdr`。当客户端或服务器端的运行在`紧凑模式`或者通信仅限于本地通信，rpcrouter不需要运行。

#### 选项
* -r <索引号>
  *  索引号为1, rpcrouter用于发送请求，接受响应和事件， 又叫做reqfwdr, 就是`request forwarder`的意思。
  *  索引号为2: rpcrouter用于接收请求，发送响应和事件， 又叫做bridge, 就是桥接的意思。

* -a 
    启用身份认证，具体使用Kerberos还是OAuth2，通过[rpcfg.py](../../tools/README_cn#安全页security)进行配置。

* -c
    `reqfwdr`的选项，为每一个客户端实例建立一个连接到服务器. 一般用于测试。

* -m <路径名>   
    rpcrouter将在`路径名`处安装一个文件系统(FUSE)，并将一些运行信息和计数器信息输出到该目录下的文件中。这些文件的格式为JSON。在`user`的目录下面，目前有四个文件InConnections, OutConnections, Sessions, LocalReqProxies。这些文件的内容时实时变动的。

* -d rpcrouter将以守护进程的模式运行
* -g rpcrouter将日志信息发送到日志服务器。

#### 例子
*   在服务器端, `rpcrouter -dr 2` 无身份验证模式的守护进程, `rpcrouter -adr 2`则是有身份验证的守护进程。
*   在客户端, `rpcrouter -dr 1` 无身份验证模式的守护进程, `rpcrouter -adr 1` 则是有身份验证的守护进程。
*   导出统计信息, `rpcrouter -adr 2 -m ./foodir`将在`./foodir`下导出实时运行信息。

#### 相关的配置文件
*   [router.json](./router.json), [rtauth.json](./rtauth.json), [authprxy.json](../security/authprxy.json), [driver.json](../../ipc/driver.json)
