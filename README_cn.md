# rpc-frmwrk ![badge](https://img.shields.io/badge/RPC-C%2B%2B%2C%20Java%2C%20Python-green)


#### 概述
* `rpc-frmwrk`是一个用C++实现的，支持多语言的，完全异步和并发的RPC系统，专注于跨网络，跨平台的互联互通，同时兼具高吞吐量和高可靠性。和大厂注重业务管理，聚合各种的RPC引擎，以及优化资源分配的RPC平台有所不同。本系统是单纯的`RPC`系统，不需要大量的系统资源，可以在嵌入式系统中运行，亦是本系统的设计目标之一。

* `rpc-frmwrk`提供接口描述语言和框架代码的自动生成器，可以帮助用户快速搭建业务系统，节省学习新系统的曲线。尤其当使用`简单模式`的框架时，不但可以帮有经验的开发人员以最快的速度搭建自定义的业务框架，学习时间亦十分短暂。

* `rpc-frmwrk`在架构上不同于`gRPC`或者`thrift`这以端口寻址服务的系统，而更类似Windows的COM系统， 提供可寻址的Object服务。并且服务器和客户端各有一个桥接的守护进程。用户开发的业务进程位于这两个守护进程后面，通过`DBus`和`Unix Socket`与之通信。因而具有更大的灵活性和安全性，同时减少了框架的复杂度和代码的耦合性。尤其当使用`简单模式`开发时，客户代码可以不用链接`rpc-frmwrk`的任何运行库。守护进程完成传输的加密，认证，压缩，和可自由组合的传输协议。`rpc-frmwrk`提供配置工具完成对守护进程的配置。

#### 功能列表   
1. `同步异步请求的发送和处理`   
2. `主动/被动取消已经发出的请求`   
3. `服务器主动推送事件`   
4. `长请求的保活功能`   
5. `对服务器的访问可以同时来自互联网, 不同进程，或者同一进程` 
6. `通信双方上线/下线的检测和通知`
7. `同一网络端口可以支持数目不限的多个服务端`
8. [`数目可配的全双工流控的流通道`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#streaming)
9. [`SSL支持`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)
10. [`Websocket支持`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md)
11. [`基于multihop技术的Object访问`](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support)
12. [`基于Kerberos 5的认证功能`](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security/README.md)
13. [`节点冗余和负载均衡`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#load-balance--node-redudancy)
14. [`自动生成C++, Python和Java的框架代码`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl/README.md)
15. [`守护进程的图形配置工具`](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools/README.md)
16. [`RPC文件系统rpcfs`](https://github.com/zhiming99/rpc-frmwrk/tree/master/fuse/README.md)

#### 编译`rpc-frmwrk`   
* 请参考[`如何编译rpc-frmwrk`](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-build-%60rpc-frmwrk%60)给出的详细的信息和步骤.   
* 如果你的目标系统是Ubuntu或者Fedora, 你应该也会对安装[`deb包`或者`rpm 包`](https://github.com/zhiming99/rpc-frmwrk/releases/tag/0.4.0)感兴趣.

#### 安装和运行
1. 当你已经编译成功`rpc-frmwrk`，在源码的根目录下运行 `sudo make install`即可进行安装.
2. 当然在运行之前，你需要配置`rpc-frwmrk`的守护进程。有关配置工具可以参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools/README.md).
3. 运行时首先启动服务器端的守护进程，如`rpcrouter -dr 2`和客户端的守护进程，如`rpcrouter -dr 1`.  有关守护进程的详细信息请参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/router/README.md).
4. 对于新用户，可以运行系统自带的`HelloWorld`来验证配置. 先在服务器端启动`helloworld`的服务器程序`hwsvrsmk`, 然后在客户端启动`hwclismk`.这篇[wiki](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-get-Helloworld-run%3F)有更详细的介绍.

#### 开发
`rpc-frmwrk`支持两类`RPC`应用程序的开发.
1. `高级模式`： `rpc-frmwrk`提供接口描述语言[`ridl`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl/README.md)和编译程序`ridlc`生成可以支持`C++`, `Pyhton`, `Java`的框架代码. 例子代码在[这里](https://github.com/zhiming99/rpc-frmwrk/tree/master/examples#generating-the-example-program-of-hellowld).`高级模式`下，用户可以使用`rpc-frmwrk`运行库提供的接口，和各种内建的工具类，完成同步，异步，超时和取消，以及`流`传输的功能。
2. [`简单模式`](https://github.com/zhiming99/rpc-frmwrk/tree/master/fuse#the-introduction-to-fuse-integration-and-the-rpcfs-filesystem): `ridlc`编译器可以生成一对文件系统（`rpcfs`)，分别部署在服务器端和客户端. 这时，所有的`RPC`传输都会以对文件操作来完成，包括文件的读写，文件的建立删除，文件的轮询，以及目录的建立和删除，完全抛开了繁琐的`回调`，`超时`和`资源回收`等技术细节，开发的内容转化为文件的操作，听着就很简单吧. 特别值得一提的是，守护进程也内建了对该文件系统的接口的支持，用于提供实时的运行信息以供管理和监控.

#### 运行库依赖的第三方软件  
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

#### Todo
1. 简单模式下的框架代码生成
2. 性能优化
3. 自适应流量控制
