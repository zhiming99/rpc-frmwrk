# rpc-frmwrk开发教程
## 第二节 发送和处理服务器事件
**本节要点**：   
* 用[ridlc](../ridl/README_cn.md#启动ridlc)生成EventTest项目。
* EventTest服务器程序和客户端程序的结构
* 对客户端和服务器端进行修改，完成所需要的功能。
* 编译，运行和检查结果。

### 项目需求
1. 服务器端定时的发送事件，客户端需要处理这些事件，并打印。
2. 观察广播事件的效果。

### 生成EventTest的C++项目框架
本节课我们使用[evtest.ridl](../examples/evtest.ridl)来生成该[项目框架](../examples/cpp/evtest/)。

#### 首先，先看看该ridl里的内容和上节课的有什么差异：
  * 在接口IEventTest上定义了一个叫`OnHelloWorld`的方法，它的输入参数`strMsg`是一个字符串，strMsg将作为事件内容，从服务器发向客户端，`returns`关键字后面没有输出参数。
  * 方法的头部多了一个标记`[event]`。该标记指示，这个方法是一个事件方法。事件方法的另一个特征是没有输出参数, 因为事件的接收方不能返回处理结果给发送方。
  * `service`关键字定义了一个叫EventTest的服务，看上去和HelloWorldSvc没有什么差异。

#### 用ridlc生成项目框架
  * 在evtest.ridl顶端有生成各个语言的命令行。我们使用标有C++的那行命令。   
    `mkdir evtest; ridlc -Lcn -O ./evtest evtest.ridl`

#### 程序结构和流程简介
  * 客户端的程序入口定义在`maincli.cpp`， 服务端的程序入口定义在`mainsvr.cpp`中。
  * 客户端的流程是这样的：
      1. 初始化上下文，在这里创建和启动CIoManager对象，这是`rpc-frmwrk`中最重要的对象。对它的访问贯穿整个程序。
      2. 创建CEventTest_CliImpl对象。这个对象对应ridl文件里的EventTest服务。
      3. 启动CEventTest_CliImpl，连接远端的服务器。如果不成功就退出。
      4. 启动成功后，调用`maincli`, 从这里开始控制就转为用户逻辑了。这里将添加我们的代码。
      5. CEventTest_CliImpl的类图:   
         ```
         CEventTest_CliImpl              // 客户端的主要对象
            CEventTest_SvrSkel
                CStatCounterProxy        // rpc-frmwrk的支持类，封装各种客户端的计数器
                IEventTest_PImpl         // 自动生成的服务器端的接口IEventTest的客户端框架，
                                         // 参数的序列化反序列化，和接收服务器发来的事件。
         ```

  * 服务端的流程如下：
      1. 初始化上下文，在这里创建和启动CIoManager对象。
      2. 创建CEventTest_SvrImpl对象。这个对象对应ridl文件里的EventTest服务。
      3. 启动CEventTest_SvrImpl，进入循环等待客户端的请求。
      4. CEventTest_SvrImpl的类图:   
         ```
         CEventTest_SvrImpl
            CEventTest_SvrSkel
                CStatCounterServer      // rpc-frmwrk的支持类，封装各种服务器端的计数器
                IEventTest_SImpl        // 自动生成的服务器端的接口IEventTest的服务端框架，
                                        // 主要做参数的序列化反序列化，以及广播事件。
         ```

#### 添加代码：
  现在的代码依然是空的架子，不执行任何实际的任务。我们需要做些改动。
  * 在客户端，我们在`maincli`中添加一个循环sleep的语句，等待服务器发来的消息。   
    ```
    gint32 maincli(
        CEventTest_CliImpl* pIf,
        int argc, char** argv )
    {
        gint32 i = 30;
        while( i > 0 )
        {
            sleep(3);                   // 这里让客户端睡眠等待30秒， 等待服务器事件。
                                        // 不能直接推出，否则调用者就执行Stop，停止RPC了。
            i -= 3;
        }
        return ret;
    }
    ```
  * 在客户端，我们还需要在CEventTest_CliImpl添加OnHelloWorld的事件处理，这个函数目前还是个空架子。   
    ```
    gint32 CEventTest_CliImpl::OnHelloWorld(
      const std::string& strMsg /*[ In ]*/)  // In代表strText是输入参数
    {
        OutputMsg( 0, "received event %s",   // 打印接收到的消息，OutputMsg是个系统支持的函数，和printf类似
            strMsg.c_str() );
        return 0;
    }
    ```
    事件处理函数比较简单，不需要给发送方返回参数，或者状态。

  * 在服务器端我们需要添加定期发送事件的语句。在哪里呢？在睡觉的`mainsvr`函数中比较合适。   
    ```
    gint32 mainsvr( 
        CEventTest_SvrImpl* pIf,
        int argc, char** argv )
    {
        while( pIf->IsConnected() )
        {
            sleep( 3 );
            pIf->OnHelloWorld( "Hello, World!" );   // 服务端的OnHelloWorld由ridlc生成了，我们只需要在这里调用就可以了。
                                                    // 按照需求，我们每三秒钟广播一个OnHelloWorld的事件。
        }
        return STATUS_SUCCESS;
    }
    ```
  * 这样我们就完成了客户端和服务端的修改。
  * 其实这些代码是作为起步用的，编程就是多动手，不必为条条框框束缚，大胆的改就是了。

#### 编译
  * 在命令行下，我们可以看到在`evtest`目录下有`Makefile`。只要输入`make -j4`命令即可。如果想要debug版，就输入`make debug -j4`。
  * 编译完后会提示`Configuration updated successfully`。这表示已经和系统的设置同步了, 可以运行了。也可以手动的运行 `python3 synccfg.py`来同步。
  * 在`evtest/release`目录下，我们应该能找到`evtestsvr`和`evtestcli`两个程序。

#### 运行
  * 在我们的evtest例子，我们依然在同一机器上启动两个`rpcrouter`的实例。一个是`rpcrouter -dr 1`作为客户端的转发器，一个是`rpcrouter -dr 2`, 作为服务器端的桥接器。
  * 在服务器端运行`release/evtestsvr`。
  * 在客户端运行`release/evtestcli`。
  * 当客户端出现`received event Hello, World!(0)`时，表示项目的第一个目标完成。
  * 我们用`for((i=0;i<2;i++));do debug/evtestcli & done`启动两个`evtestcli`， 观察一下是两个客户端是否都打印了接收到的事件。这样我们的第二个目标就完成了。
 
[下一讲](./Tut-AsyncRequest_cn-3.md)   
[上一讲](./Tut-HelloWorld_cn-1.md)   