# rpc-frmwrk ![badge](https://img.shields.io/badge/RPC-C%2B%2B%2C%20Java%2C%20Python-green)

这是一个嵌入式的RPC实现，关注于跨网络，跨协议，跨平台的互联互通。本项目欢迎有兴趣的人士加入!   
This is an asynchronous and event-driven RPC implementation for embeded system with small system footprint. It is targeting at the IOT platforms with high-throughput and high availability over hybrid networks. Welcome to join!  

#### Concept
[`Here`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md) is an introduction to the concept of `rpc-frwmrk`.

#### Features   
1. `Synchronous/asynchronous request handling`   
2. `Active/passive request canceling.`   
3. `Server-push events`   
4. `Keep-alive for time-consuming request.`   
5. `Simultaneous object access over network and IPC.` 
6. `Peer online/offline awareness.`
7. `Publishing multiple local/remote object services via single network port.`
8. [`Full-duplex streaming channels`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#streaming)
9. [`Secure Socket Layer (SSL) support`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)
10. [`Websocket support`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md)
11. [`Object access via Multihop routing`](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support)
12. [`Authentication support with Kerberos 5`](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security/README.md)
13. [`Node Redudancy/Load Balance`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#load-balance--node-redudancy)
14. [`A skelton generator for CPP, Python and Java`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl/README.md)
15. [`A GUI config tool for rpcrouter`](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools/README.md)
16. [`rpcfs - filesystem interface for rpc-frmwrk`](https://github.com/zhiming99/rpc-frmwrk/tree/master/fuse/README.md)

#### Building `rpc-frmwrk`   
* Please refer to this article [`How to build rpc-frmwrk`](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-build-%60rpc-frmwrk%60) for detail description.   
* If you are using Ubuntu or Fedora, you may want to install the [`deb package` or `rpm package`](https://github.com/zhiming99/rpc-frmwrk/releases/tag/0.5.0) to skip the painstaking building process.

#### Installation
1. Run `sudo make install` from the root directory of `rpc-frmwrk` source tree.
2. Configure the runtime parameters for `rpc-frwmrk` as described on [this page](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools/README.md).
3. Start the daemon process `rpcrouter -dr 2` on server side, and on start daemon process `rpcrouter -dr 1` on client side. And now we are ready to run the `helloworld` program. For more information about `rpcrouter`, please follow this [link](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/router/README.md).
4. Smoketest with `HelloWorld`. Start the `hwsvrsmk`, the `helloworld` server on server side. And start the `hwclismk` on the client side.
5. This [wiki](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-get-Helloworld-run%3F) has some detail information.

#### Development
`rpc-frmwrk` supports two approaches for distributed application development.
1. The classic RPC. `rpc-frmwrk` has an `interface description language`, [`ridl`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl/README.md) to help you to generate the skelton code rapidly. Examples can be found [here](https://github.com/zhiming99/rpc-frmwrk/tree/master/examples#generating-the-example-program-of-hellowld).
2. Programming with [`rpcfs`](https://github.com/zhiming99/rpc-frmwrk/tree/master/fuse#the-introduction-to-fuse-integration-and-the-rpcfs-filesystem). The `ridl` compiler can generate a pair of filesystems for server and client respectively with the `ridl` file. And all the `rpc` traffic goes through file read/write and other file operations. And moreover `rpcfs` hosted by the `rpcrouter` provides information for runtime monitoring and management.

#### Runtime Dependency  
This project depends on the following 3rd-party packags at runtime:  
1. `dbus-1.0 (dbus-devel)`
2. `libjson-cpp (jsoncpp-devel)` 
3. `lz4 (lz4-devel)`   
4. `cppunit-1 (for the test cases, cppunit and cppunit-devel)`   
5. `openssl-1.1 for SSL communication.`
6. `MIT krb5 for authentication and access control.`
7. `c++11 is required, and make sure the GCC is 5.x or higher.`
8. `python 3.5+ is required for Python support.`
9. `Java OpenJDK 8 or higher for Java support.`
10. `FUSE-3 for rpcfs support`

#### Todo
1. Skelton code generator for `rpcfs` programming
2. Performance Tuning
3. Adaptive flow control
4. A tree-like hierarchical persistant registry.
