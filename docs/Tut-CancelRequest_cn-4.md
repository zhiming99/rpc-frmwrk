# C++开发教程
## 第四节 取消已发出的请求
**本节要点**：   
* 如何取消一个已经发出的请求
* 对客户端进行修改，完成所需要的功能。
* 编译，运行和检查结果。

### 项目需求
1. 发送一个长异步请求，等待数秒后取消该请求。
2. 观察服务器返回的消息和错误码。

### 生成ActiveCancel的C++项目框架
本节课我们使用[actcancel.ridl](../examples/actcancel.ridl)来生成该项目框架。

#### ridl文件解析
  * 在接口IActiveCancel上定义了一个叫`LongWait`的方法，该方法和上一节`异步请求`的方法基本一样。
  * LongWait方法的标签为async，意即客户端和服务器端都是异步的。
    设置成async是有原因的，即rpc-frmwrk可以取消的请求必须是在服务器和客户端都是异步的。

#### 用ridlc生成项目框架
  * 在actcancel.ridl顶端有生成各个语言的命令行。我们使用标有C++的那行命令。   
    `mkdir actcancel; ridlc -Lcn -O ./actcancel actcancel.ridl`

#### 程序结构和流程简介
  * 由于程序结构和HelloWorld基本相同，请参考[HelloWorld](./Tut-HelloWorld_cn-1.md#程序结构和流程简介)一节的讲解

#### 添加代码：
  * 对LongWait函数的修改，可以参考[上一节](./Tut-AsyncRequest_cn-3.md#添加代码)里对同名函数的修改。
  * 这里我们主要讲解一下对`maincli`函数的修改。因为取消请求主要是一个客户端的操作。
    ```
    gint32 maincli(
        CActiveCancel_CliImpl* pIf,
        int argc, char** argv )
    {
        gint32 ret = 0;
        do{
            stdstr i0 = "hello, actctest!";
            stdstr i0r;
            CParamList oCtx;
            ret = pIf->LongWait( oCtx.GetCfg(), i0, i0r );          // 发出一个异步请求
            if( ERROR( ret ) )                                      // 检查返回值，出错就退出。
                break;

            if( SUCCEEDED( ret ) )                                  // 成功的话就打印信息
            {                                                       // 你可能会疑惑，跨网络的请求，异步的请求不可能立即返回成功吧？
                OutputMsg( ret,                                     // 现实是，什么稀奇古怪的事情都会发生。不要有任何侥幸心理。
                    "LongWait succeeded with response %s",
                    i0r.c_str() );
                break;
            }
            if( ret != STATUS_PENDING )
            {
                OutputMsg( ret, "LongWait failed with error " );
                break;
            }
            OutputMsg( 0, "LongWait is pending, wait 3 seconds" );
                                                                    
            guint64 qwTaskId = 0;
            oCtx.GetQwordProp( propTaskId, qwTaskId );              // 返回值是STATUS_PENDING, 这个时候，context里会返回一个任务id
                                                                    // 这个id是标识LongWait请求的一个64位整数，在客户端和服务器端唯一。
            sleep( 3 );

            OutputMsg( 0, "Time up, canceling LongWait Request..." );
            ret = pIf->CancelRequest( qwTaskId );                   // CancelRequest是接口`IInterfaceServer`上的一个方法，在'actcanceldesc.json'
                                                                    // 中可以看到这个系统接口，它目前有三个方法，一个是CancelRequest，另两个是
                                                                    // KeepAliveRequest和OnKeepAlive。CancelRequest的作用就是向服务器发一个
                                                                    // '取消请求的请求'，参数就是上面提到的任务Id。
                                                                    // CancelRequest是一个同步函数，它还有一个异步的版本CancelReqAsync。
            if( ERROR( ret ) )                                      // CancelRequest不一定能保证成功，比如，在服务器收到'取消请求'前，原来的请求正好被
                                                                    // 服务器成功处理，那么服务器就找不到这个'原请求'， 也就没办法取消了。
            {
                // have to wait for complete
                OutputMsg( ret,                                     // CancelRequest失败，继续等待原请求从LongWaitCallback发完成信号，此时不能退出，
                    "CancelRequest failed with error %d, wait...",  // 原请求不管是成功还是失败还是取消，都会返回一个说法的， 需要等待回复。
                    ret );
                pIf->WaitForComplete();                             
                ret = pIf->GetError();
                OutputMsg( ret,
                    "LongWait returned with error " );
                break;
            }
            else                                                    
            {
                OutputMsg( ret, "CancelRequest succeeded" );        // CancelRequest成功，仍然等待原请求从LongWaitCallback发信号过来。
                pIf->WaitForComplete();
            }

        }while( 0 );

        return ret;
    }
    ```
#### 问题
1. 被取消的请求有返回值吗？有的，就像上面提到的一样，被取消的请求，不会原地消失，要给客户端一个说法，这个说法就是返回值，不论这个返回值来自服务器，还是本地。
2. 既然`被取消请求`和CancelRequest都要返回结果，那么哪个结果先返回呢？两种情况都有可能。在服务器端，这两个请求的返回消息几乎同时发出，两者没有采取同步的措施。所以不能保证两者的返回顺序固定。不过代码中，`maincli`调用的CancelRequest是同步的，所以在`maincli`是CancelRequest先返回，然后再等待LongWaitCallback的信号。

#### 编译
  * 在命令行下，我们可以看到在`actcancel`目录下有`Makefile`。只要输入`make -j4`命令即可。如果想要debug版，就输入`make debug -j4`。
  * 编译完后会提示`Configuration updated successfully`。这表示已经和系统的设置同步了, 可以运行了。也可以手动的运行 `python3 synccfg.py`来同步。
  * 在`actcancel/release`目录下，我们应该能找到`actcancelsvr`和`actcancelcli`两个程序。

#### 运行
  * 在同一机器上启动两个`rpcrouter`的实例。一个是`rpcrouter -dr 1`作为客户端的转发器，一个是`rpcrouter -dr 2`, 作为服务器端的桥接器。
  * 在服务器端运行`release/actcancelsvr`。
  * 在客户端运行`release/actcancelcli`。
  * 检查客户端的输出是否显示`CancelRequest succeeded(0)`。
 

[下一讲](./Tut-Serialization_cn-5.md)   
[上一讲](./Tut-AsyncRequest_cn-3.md)   
