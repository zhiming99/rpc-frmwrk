[[English]](https://github.com/zhiming99/rpc-frmwrk/blob/master/README.md)
# rpc-frmwrk ![badge](https://img.shields.io/badge/RPC-C%2B%2B%2C%20Java%2C%20Python-green)


## 概述
* `rpc-frmwrk`是一个用C++实现的，支持多语言的，完全异步和并发的RPC系统，专注于跨网络，跨平台的互联互通，同时兼具高吞吐量和高可靠性。本系统占用较少的系统资源，可以在嵌入式系统中运行。本项目目前支持C++, Python和Java语言。

* `rpc-frmwrk`提供接口描述语言和框架代码的生成器，可以帮助用户快速搭建业务系统，平滑新系统的学习曲线。尤其当使用`简单模式`的框架时，不但可以帮有经验的开发人员以最快的速度搭建自定义的业务框架，学习时间亦十分短暂。

* `rpc-frmwrk`在架构上可以灵活定制，既可以生成分布式的微服务架构，也可以生成可热插拔的集中式服务进程。可分别应对强调业务隔离松耦合，或者强调性能吞吐量的不同应用场景。
  * 当运行在微服务架构下，服务端和客户端各有一个桥接的守护进程。用户开发的各个业务进程位于这两个守护进程后面，通过`Unix Socket`或者`DBus`与守护进程通信。因而具有更大的灵活性和安全性，同时减少了系统框架的复杂度和代码的耦合性。尤其当使用`简单模式`开发时，客户代码可以不用链接`rpc-frmwrk`的任何运行库。
  * 当运行在集中式服务架构下，服务端和客户端分别为两个独立的进程，此时`rpc-frmwrk`拥有更好的性能。服务端和客户端都可在`运行时`加载和启停用户开发的服务插件。
  * 在任何一种架构下，`rpc-frmwrk`提供配置工具和`运行时接口`完成对传输功能的配置，包括加密，认证，压缩，WebSocket，Multihop等功能，和运行时参数。
* `rpc-frmwrk`十分注重传输的安全性，其设计独特的I/O模块，可以支持国密(GmSSL)和OpenSSL两种安全传输协议，亦可通过`Kerberos`提供单点登陆(SSO)服务。`rpc-frmwrk`的配置工具提供简便的密钥分发和管理工具，可以有效减轻GmSSL和OpenSSL的部署和使用的负担。
* 更多概念方面的介绍请参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept_cn.md)。

## 功能列表   
1. `同步异步请求的发送和处理`   
2. `主动/被动取消已经发出的请求`   
3. `服务器主动推送事件`   
4. `长请求的保活功能`   
5. `对服务器的访问可以同时来自互联网, 不同进程，或者同一进程` 
6. `通信双方上线/下线的检测和通知`
7. `同一网络端口可以支持数目不限的多个服务端`
8. [`数目可配的全双工流控的流通道`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#streaming)
9. [`OpenSSL的支持`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)
10. [`Websocket支持`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md)
11. [`基于multihop技术的Object访问`](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support)
12. [`基于Kerberos 5的认证功能`](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security/README.md)
13. [`节点冗余和负载均衡`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#load-balance--node-redudancy)
14. [`自动生成C++, Python和Java的框架代码`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl/README.md)
15. [`守护进程的图形配置工具`](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools/README.md)
16. [`RPC文件系统rpcfs`](https://github.com/zhiming99/rpc-frmwrk/tree/master/fuse/README.md)
17.  [`支持国密TLS13(SM2+SM4+GCM)`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/gmsslport/README_cn.md)

## 编译`rpc-frmwrk`   
* 请参考[`如何编译rpc-frmwrk`](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-build-%60rpc-frmwrk%60)给出的详细的信息和步骤.   
* 如果你的目标系统是Ubuntu或者Fedora, 你应该也会对安装[deb包或rpm包](https://github.com/zhiming99/rpc-frmwrk/releases/tag/0.5.0)感兴趣.

## 安装和运行
1. 当你已经编译成功`rpc-frmwrk`，在源码的根目录下运行 `sudo make install`即可进行安装.
2. 在运行之前，你需要配置`rpc-frwmrk`的守护进程。有关配置工具可以参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools/README.md).
3. 运行时首先启动服务器端的守护进程，如`rpcrouter -dr 2`和客户端的守护进程，如`rpcrouter -dr 1`.  有关守护进程的详细信息请参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/router/README.md).
4. 对于新用户，可以运行系统自带的`HelloWorld`来验证配置. 先在服务器端启动`helloworld`的服务器程序`hwsvrsmk`, 然后在客户端启动`hwclismk`.这篇[wiki](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-get-Helloworld-run%3F)有更详细的介绍.

## 开发
`rpc-frmwrk`支持两种模式的`RPC`应用程序的开发.
1. `高级模式`： `rpc-frmwrk`提供接口描述语言[`ridl`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl/README.md)和编译程序`ridlc`生成可以支持`C++`, `Pyhton`, `Java`的框架代码. 例子代码在[这里](https://github.com/zhiming99/rpc-frmwrk/tree/master/examples#generating-the-example-program-of-hellowld).`高级模式`使用的是`rpc-frmwrk`的原生架构，用户通过运用`rpc-frmwrk`运行库提供的接口，和各种内建的工具类，执行同步，异步，超时，取消，以及`流`传输等功能，从而实现高性能的业务逻辑。
2. [`简单模式`](https://github.com/zhiming99/rpc-frmwrk/tree/master/fuse#the-introduction-to-fuse-integration-and-the-rpcfs-filesystem): `ridlc`编译器可以生成一对文件系统（`rpcfs`)，分别部署在服务器端和客户端. 这时，所有的`RPC`传输都会以对文件操作来完成，包括文件的读写，文件的建立删除，文件的轮询，以及目录的建立和删除，完全封装了繁琐的`回调`，`超时`和`资源回收`等技术细节，开发的内容转化为文件的操作，真正的语言中立. 特别值得一提的是，守护进程也内建了对该文件系统的接口的支持，用于提供实时的运行信息以供管理和监控.

## 第三方依赖  
1. `dbus-1.0 (dbus-devel)`
2. `libjson-cpp (jsoncpp-devel)` 
3. `lz4 (lz4-devel)`   
4. `cppunit-1 仅用于测试用例的代码中`   
5. `openssl-1.1用于支持SSL的连接.`
6. `MIT krb5 用于用户认证.`
7. `C++11 和 GCC 5.x+.`
8. `Python 3.5+.`
9. `Java OpenJDK 8+.`
10. `FUSE-3`
10. `GmSSL 3.0`

## TODO
1. 添加ridl的教程
2. 性能优化
3. 自适应流量控制
