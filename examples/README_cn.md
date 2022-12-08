[English](https://github.com/zhiming99/rpc-frmwrk/blob/master/examples/README.md)
#### 简介
* 本目录包含一些RPC程序开发中的典型用例.

* 这些例子程序的实现按所使用的语言进行了分类, 分别是Java, Python和C++.

* 本目录下的代码文件仅仅是做过修改，添加了业务逻辑的部分代码，并不是全部代码. 完整的代码可通过执行ridl文件首部注释中的命令行生成。
    * `hellowld.ridl`: HelloWorld程序，新手起步的简单程序. 
    * `iftest.ridl`: 展示如何定义和在客户和服务器之间传输复杂数据结构. 
    * `asynctst.ridl`: 展示客户端如何发起异步请求以及服务器端如何异步的处理一个请求。
    * `evtest.ridl`: 展示服务端如何推送一个事件，以及客户端如何处理一个服务器事件。
    * `actcancel.ridl`: 展示客户端如何主动的撤销一个已经发出的异步请求。
    * `katest.ridl`: 展示如何通过keep-alive心跳来保持一个长请求不被超时撤销.
    * `stmtest.ridl`: 展示如何建立一个`流通道`和通过`流通道`进行对话.
    * `sftest.ridl`: 展示如何通过`流通道`进行文件的上传和下载.

#### 生成例子程序'HelloWorld'
   * 首先确保`rpc-frmwrk`已经成功安装和启动.
   * 下面,我们以C++的`hellowld`为例.
      * 从当前目录切换到子目录`cpp`下
      * 在终端上运行命令`ridlc -O ./hellowld ../hellowld.ridl`. 
      * 然后再从当前目录切换到子目录`hellowld`。
      * 你会发现除了[maincli.cpp](https://github.com/zhiming99/rpc-frmwrk/blob/master/examples/cpp/hellowld/maincli.cpp)和[HelloWorldSvcsvr.cpp](https://github.com/zhiming99/rpc-frmwrk/blob/master/examples/cpp/hellowld/HelloWorldSvcsvr.cpp)外，还有有maincli.cpp.new和HelloWorldSvcsvr.cpp.new. 前者是已经修改过加入业务代码的文件，后者是`ridlc`新生成的。我们可以忽略新生成的两个文件。   
      * 当前目录下的`README.md`对所生成的各个文件有简要的介绍.
      * 在命令行下，执行`make`命令，就可以同时生成服务器端和客户端的可执行文件了. 在`release`目录下, `HelloWorldsvr`是服务器端程序, HelloWorldcli是客户端程序.
      * `hellowld`目录下的文件列表如下图。   
   ![图](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/hellowld-tree.png)
      * 在终端里，分别启动HelloWorldSvcsvr和HelloWorldSvccli后，客户端打印结果如下图。   
   ![输出](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/hellowld.png)
