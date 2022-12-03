#### 生成例子程序'HelloWorld'
   * 首先确保`rpc-frmwrk`已经正确安装和启动.
   * 我们以C++的`hellowld`为例.
      * 从当前目录切换到子目录`cpp`下
      * 在终端上运行命令`ridlc -O ./hellowld ../hellowld.ridl`. 
      * 然后再从当前目录切换到子目录`hellowld`。
      * 你会发现除了[maincli.cpp](https://github.com/zhiming99/rpc-frmwrk/blob/master/examples/cpp/hellowld/maincli.cpp)和[HelloWorldSvcsvr.cpp](https://github.com/zhiming99/rpc-frmwrk/blob/master/examples/cpp/hellowld/HelloWorldSvcsvr.cpp)外，还有有maincli.cpp.new和HelloWorldSvcsvr.cpp.new. 前者是已经修改过加入业务代码的文件，后者是`ridlc`新生成的。我们可以忽略新生成的两个文件。   
      * 当前目录下的`README.md`对所生成的各个文件有简要的介绍.
      * 在命令行下，执行`make`命令，就可以同时生成服务器端和客户端的可执行文件了. 在`release`目录下, `HelloWorldsvr`是服务器端程序, HelloWorldcli是客户端程序.
      * `hellowld`目录下的文件列表如下图。   
   ![图](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/hellowld-tree.png)
      * HelloWorldSvccli成功运行时输出如下图。   
   ![输出](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/hellowld.png)
