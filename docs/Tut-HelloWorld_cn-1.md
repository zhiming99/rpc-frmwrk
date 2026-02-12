# rpc-frmwrk开发教程
## 第一节 HelloWorld
**本节要点**：   
* 用[ridlc](../ridl/README_cn.md#启动ridlc)生成HelloWorld项目。
* HelloWorld服务器程序和客户端程序的结构
* 对客户端和服务器端进行修改，完成所需要的功能。
* 编译，运行和检查结果。

### 项目需求
客户端发送字符串`Hello, World!`到服务器，接受服务器的响应，并打印。

### 仅参考命令的读者请跳到文章结尾

### 生成HelloWorld的C++项目框架
我们使用[hellowld.ridl](../examples/hellowld.ridl)来生成该[项目框架](../examples/cpp/hellowld/)。
#### 首先，先看看该ridl里的内容：
  * appname 应用名`HelloWorld`，ridl编译器会以该名称加后缀的方式命名一些生成文件，和一些变量。
  * 在接口IHelloWorld上定义了一个叫`Echo`的方法，它的输入参数`strText`是一个字符串，`returns`关键字后面跟的是输出参数`strResp`，也是一个字符串。
  * 这个方法由于没有标注是否是异步，所以默认为同步方法，即调用者将被阻塞，直到处理完成。这是最简单的调用方式。
  * `service`关键字定义了一个叫HelloWorldSvc的服务，这个服务实现了一个接口IHelloWorld。
#### 用ridlc生成项目框架
  * 在hellowld.ridl顶端有生成各个语言的命令行。我们使用标有C++的那行命令。   
    `mkdir hellowld; ridlc -Lcn -O ./hellowld hellowld.ridl`
  * 命令行有个小改动，加了`-Lcn`选项，生成中文的readme文件。阅读readme是一个好习惯。

#### 程序结构和流程简介
  * 客户端的程序入口定义在`maincli.cpp`， 服务端的程序入口定义在`mainsvr.cpp`中。
  * 客户端的流程是这样的：
      1. 初始化上下文，在这里创建和启动CIoManager对象，这是`rpc-frmwrk`中最重要的对象。对它的访问贯穿整个程序。
      2. 创建CHelloWorldSvc_CliImpl对象。这个对象对应ridl文件里的HelloWorldSvc服务。命名格式'C'+'服务名'+'_CliImpl'。
      3. 启动CHelloWorldSvc_CliImp，连接远端的服务器。如果不成功就退出。
      4. 启动成功后，调用`maincli`, 从这里开始控制就转为用户逻辑了。这里将添加我们的代码。
      5. CHelloWorldSvc_CliImpl的类图:   
         ```
         CHelloWorldSvc_CliImpl          // 客户端的主要对象
            CHelloWorldSvc_SvrSkel
                CStatCounterProxy        // rpc-frmwrk的支持类，封装各种客户端的计数器
                IHelloWorld_PImpl        // 自动生成的服务器端的接口IHelloWorld的客户端框架，
                                         // 用于发送请求，参数的序列化反序列化，以及接收响应。
         ```

  * 服务端的流程如下：
      1. 初始化上下文，在这里创建和启动CIoManager对象。
      2. 创建CHelloWorldSvc_SvrImpl对象。这个对象对应ridl文件里的HelloWorldSvc服务。命名格式'C'+'服务名'+'_SvrImpl'。
      3. 启动CHelloWorldSvc_SvrImpl，进入循环等待客户端的请求。
      4. 不象`maincli`。`mainsvr`函数不需要作改动。
      5. CHelloWorldSvc_SvrImpl的类图:   
         ```
         CHelloWorldSvc_SvrImpl          // 用户添加ridl接口方法的地方
            CHelloWorldSvc_SvrSkel
                CStatCounterServer       // rpc-frmwrk的支持类，封装各种服务器端的计数器
                IHelloWorld_SImpl        // 自动生成的服务器端的接口IHelloWorld的服务端框架，
                                         // 主要接收请求并分发，参数的序列化反序列化，以及返回处理结果。
         ```

#### 添加代码：
  现在的代码不执行任何实际的任务。离我们的项目需求还有距离。
  * 代码规范
    `rpc-frmwrk`的函数一般要有返回值，返回值为POSIX定义的错误码，或者在头文件[defines.h](../include/defines.h)定义的错误码。错误码小于0。
  * 在客户端，我们在`maincli`中添加发送字符串的功能。   
    ```
    gint32 maincli(
        CHelloWorldSvc_CliImpl* pIf,
        int argc, char** argv )
    {
      std::string strResp;                        // 服务器的响应将放入此变量
      gint32 ret = pIf->Echo(                     // 发送字符串 'Hello, World!'去服务器
          "Hello, World!", strResp );             // Echo是接口定义的方法，包含两个参数，一个输入参数，一个输出参数。
                                                  // 在HelloWorld.cpp中有它的定义。
      if( ERROR( ret ) )                          // 检查返回值
          OutputMsg( ret, "Error Echo request" ); // 失败，打印错误码
      else
          OutputMsg( ret,
              "Echo request succeeded with resp '%s'",
              strResp.c_str() );                  // 成功，打印返回的字符串。
      return ret;
    }
    ```
  * 在服务端，我们需要在CHelloWorldSvc_SvrImpl实现Echo的服务器版做修改。   
    ```
    gint32 CHelloWorldSvc_SvrImpl::Echo(
      const std::string& strText /*[ In ]*/,  // In代表strText是输入参数
      std::string& strResp /*[ Out ]*/ )      // Out代表strResp是输出参数
    {
        strResp = strText;                    // 为输出参数赋值
        return STATUS_SUCCESS;                // 返回成功，通知系统，处理完成，strResp将发送回客户端。
    }
    ```
 
  * 这样我们就完成了客户端和服务端的修改。
#### 看一眼HelloWorlddesc.json
  HelloWorlddesc.json是客户端和服务器端的配置文件。不需要修改。   
  ```
  {
      "ClassFactories": [],
      "ServerName": "HelloWorld",                   // 服务器名称，ridl文件中的`appname`，用于dbus的寻址。
      "Objects": [
          {
              "ObjectName": "HelloWorldSvc",        // 对象名称，ridl文件中的`service`名称。用于dbus的寻址，和对象设置的加载。
              "PortClass": "DBusLocalPdo",          // 服务器端连接方式
              "PortId": "0",
              "ProxyPortClass": "DBusProxyPdo",     // 客户端连接reqfwdr的方式, 
              "ProxyPortClass-1": "DBusLocalPdo",   // 如果用DBusLocalPdo，意味着服务器和客户端通过IPC连接。不走网络，适合做本地调试。
              "BusName": "DBusBusPort_0",
              "Compression": "true",                // 压缩数据包
              "EnableSSL": "true",                  // 启用安全连接
              "EnableWS": "false",                  // 不使用websocket
              "Interfaces": [
                  {
                      "BindToClsid": "true",
                      "DummyInterface": "true",     // 不使用该Interface
                      "InterfaceName": "CHelloWorldSvc_SvrImpl"
                  },
                  {
                      "InterfaceName": "IInterfaceServer" // 系统接口，用于发送KeepAlive和取消请求。
                  },
                  {
                      "InterfaceName": "IUnknown"         // 系统接口，用于枚举服务器的接口。
                  },
                  {
                      "InterfaceName": "IHelloWorld"      // 我们的IHelloWorld接口。
                  }
              ],
              "IpAddress": "172.17.0.1",            // 服务器地址172.17.0.1。
              "PortNumber": "4132",                 // 服务器端口地址4132。
              "KeepAliveTimeoutSec": "60",          // 发送keepalive的周期为60秒。
              "QueuedRequests": "false",
              "RequestTimeoutSec": "120",           // 请求最长时限120秒。
              "RouterPath": "/",                    // 不使用级联。
          }
      ],
      "Version": "1.0"
  }
  ```
#### driver.json
  * `driver.json`是IoManager的配置文件，这里的`driver.json`不同于系统的`driver.json`，它只用于启动`HellWorldsvr`和`HelloWorldcli`，这个文件里有关于`HelloWorldsvr`和`HelloWorldcli`启动的信息，这是系统`driver.json`中没有的，所以如果把HelloWord目录下的`driver.json`删掉，使用系统的`driver.json`，会无法启动。
  * 同样`rpcrouter`等系统程序，不要在`HelloWorld`这样的有`driver.json`的目录下运行，否则它会使用当前目录下的`driver.json`，而无法启动。
#### 编译
  * 目前`rpc-frmwrk`只支持Linux，所以所有的环境都是Linux系统。Windows用户请先用WSL或者虚拟机进行操作。
  * 在命令行下，我们可以看到在`hellowld`目录下有`Makefile`。只要输入`make -j4`命令即可。如果想要debug版，就输入`make debug -j4`。
  * 编译完后会提示`Configuration updated successfully`。这表示已经和系统的设置同步了, 可以运行了。也可以手动的运行 `python3 synccfg.py`来同步。
  * **需要注意的是，如果使用`rpcfg.py`更新完系统设置，也必须再运行一下`make sync`来更新本项目的设置。否则就会出现连接失败或者其他奇怪的问题。**
  * 在`hellowld/release`目录下，我们应该能找到`HelloWorldsvr`和`HelloWorldcli`两个程序。

#### 运行
  * 在我们的HelloWorld例子，在同一机器上启动两个`rpcrouter`的实例。一个是`rpcrouter -dr 1`作为客户端的转发器，一个是`rpcrouter -dr 2`, 作为服务器端的桥接器。
  * 在服务器端运行`release/HelloWorldsvr`。
  * 在客户端运行`release/HelloWorldcli`。
  * 当出现`Echo request succeeded with resp 'Hello, World!'(0)`时，表示项目需求已完成。
  * 我们之所以使用同一机器为的是简单。更理想的方式是在两台机器上作实验，或者在虚拟机，Docker容器里也可以。就留给大家自行练习吧。

### 总结本节用到的命令：
  * 所有操作均在`rpc-frmwrk`的源代码树下
  * `rpcrouter -dr 2;rpcrouter -dr 1`
  * `mkdir -p examples/cpp/hellowld;cd examples/cpp`   
  * `ridlc -Lcn -O ./hellowld  ../hellowld.ridl`
  * 修改和添加业务代码
  * `cd hellowld;make`
  * `release/HelloWorldsvr &`
  * `release/HelloWorldcli`

[下一讲](./Tut-SendEvent_cn-2.md)   
[上一讲](./Tut-Overview_cn-0.md)   
[目录](./Tut-Index.md)   
