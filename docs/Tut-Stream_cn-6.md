# C++开发教程
## 第六节 流的概念和编程
**本节要点**：   
* 流的概念和特点
* 使用流通道收发数据
### 项目需求
客户端和服务器端使用流收发消息，并打印。

### 生成stmtest的C++项目框架
我们使用[stmtest.ridl](../examples/stmtest.ridl)来生成该[项目框架](../examples/cpp/stmtest/)。
#### 首先，先看看该ridl里的内容：
  * 唯一需要注意的是在`service`名字后面有一个标stream的tag。它的含义是这个`service`需要使用流。
  * 如果该`service`的接口方法里有HSTREAM类型的参数，那么这个`service`也会自动的开启流的功能，不用显式的标出。
  * 在本例里，接口方法和流的使用并无关系。

#### 用ridlc生成项目框架
  * 在stmtest.ridl顶端有生成各个语言的命令行。我们使用标有C++的那行命令。   
    `mkdir stmtest; ridlc -Lcn -O ./stmtest stmtest.ridl`

#### 流的概念和特点
  * 流通道是全双工的数据通道。
  * 单个流通道可以传输2^64个字节。
  * 流通道保证数据的收发是严格顺序的，先进先出的。
  * 流是有流量控制的，用以匹配发送端发送速度和接收端的处理速度。
  * 流通道可用于文件传输，或者需要更灵活的传输控制的场景，如非固定格式的数据流媒体流等。
  * 流通道由客户端发起并建立，可以由任意一方随时关闭。

##### 流的接口
  * 当一个`service`启用流功能后，我们可以看到客户端和服务器端的提供系统支持的基类列表中有了
    *CStreamProxyAsync*，和*CStreamServerAsync*两个类，这两个类封装了流的所有功能。
    ```
    DECLARE_AGGREGATED_PROXY(
        CStreamTest_CliSkel,
        CStatCountersProxy,
        CStreamProxyAsync,
        IIStreamTest_PImpl );

    DECLARE_AGGREGATED_SERVER(
        CStreamTest_SvrSkel,
        CStatCountersServer,
        CStreamServerAsync,
        IIStreamTest_SImpl );
    ```
  * 如上面所说，流通道的建立由客户端发起。
  * 所有读写操作都有时间限制，默认为120秒。
  * 读操作是指从缓冲队列里取出第一个数据包，所以读操作的长度由读到的数据包的长度决定，而不是调用者指定。
  * 所有写操作有缓冲数据包个数限制，发送缓冲区的等待数据包个数不能超过32个。如果超过则返回ERROR_QUEUE_FULL。
    此时则要等待回调函数`OnWriteResumed`通知，或者某一个在途的异步写操作返回。相比较而言，保持并行发出的请求个数小于32，操作简单一些。
  * 流的读写有八个方法，及两个回调函数，这些函数都可以通过`service`的类对象访问到。本例里就是`CStreamTest_CliImpl`和`CStreamTest_SvrImpl`这两个对象：
    | 序号 | 方法 | 同步/异步| 描述 | 
    | -------- | --------- | --------- |------------------|
    |1|ReadStream(HANDLE hChan, BufPtr& pBuf) | 同步 |从指定的流读入字节数组，长度视缓冲区中的数据包大小而定，当读到数据后返回。|
    |2|WriteStream(HANDLE hChan, BufPtr& pBuf) |同步|向指定的流写入字节数组，长度由输入参数pBuf确定。当全部数据发出后，返回。|
    |3|ReadStreamNoWait(HANDLE hChan, BufPtr& pBuf)| 异步 |从指定的流读入字节数组，长度视缓冲区中的数据大小而定，若无数据，返回-EAGAIN|
    |4|WriteStreamNoWait(HANDLE hChan, BufPtr& pBuf)| 异步 |向指定的流写入字节数组，长度由输入参数pBuf确定。数据进入缓冲队列即返回。不等待数据包发出|
    |5|ReadStreamAsync(HANDLE hChan, BufPtr& pBuf, IConfigDb* pCtx)| 异步 |从指定的流读入字节数组，长度视缓冲区中的数据大小而定，若无数据，返回STATUS_PENDING。直到OnReadStreamComplete被调用。|
    |6|WriteStreamAsync(HANDLE hChan, BufPtr& pBuf, IConfigDb* pCtx)| 异步 |向指定的流写入字节数组，长度由输入参数pBuf确定。返回STATUS_PENDING。直到OnWriteStreamComplete被调用。|
    |7|ReadStreamAsync(HANDLE hChan, BufPtr& pBuf, IEventSink* pCb)| 异步 |从指定的流读入字节数组，长度视缓冲区中的数据大小而定，若无数据，返回STATUS_PENDING。直到参数指定的回调函数pCb被调用。|
    |8|WriteStreamAsync(HANDLE hChan, BufPtr& pBuf, IEventSink* pCb)| 异步 |向指定的流写入字节数组，长度由输入参数pBuf确定。返回STATUS_PENDING。直到参数指定的回调函数pCb被调用。|
    |9|OnReadStreamComplete(HANDLE hChan, gint32 r, BufPtr& pBuf, IEventSink* pCtx)| N/A |方法5的回调函数。r为返回值，只有当r==0时，pBuf有效。pCtx来自方法5的同名参数。|
    |10|OnWriteStreamComplete(HANDLE hChan, gint32 r, BufPtr& pBuf, IEventSink* pCtx)| N/A |方法6的回调函数。r为返回值，pCtx来自方法6的同名参数。|
  * 流的建立和结束还有几个流事件的虚函数，可以重写(override)以初始化或回收分配的资源。
    | 序号 | 方法 | 服务器/客户端 | 描述 | 
    | --- | --------- | --------- |------------------|
    |1| OnStreamReady(HANDLE hChan) |都有| 流hChan准备好，可以传输数据了。|
    |2| OnStmClosing(HANDLE hChan) |都有|流hChan正在关闭中，可以释放占用的资源了。|
    |3| AcceptNewStream(IEventSink* pCb, IConfigDb* pDataDesc) |仅限服务器|服务器是否接收新的建立流通道的请求。pDataDesc里有会话的信息和流相关的参数等。返回STATUS_SUCCESS表示允许建立新的流通道，`返回错误`表示拒绝该建立流通道的请求。|

#### 总体流程
1. 我们准备让客户端做如下的任务：
    * 建立流通道
    * 先用同步的接口`WriteStream`向服务器发一条带有序号的消息，然后用`ReadStream`接收服务器的应答。
    * 再用异步的接口`WriteStreamAsync`向服务器发一条序号+`0.5`消息，然后用`ReadStreamAsync`接收服务器的消息。由于这两个函数
      是异步的，我们用在第三节课`异步请求和处理`的做法，在CStreamTest_CliImpl里添加一个信号量和两个方法'WaitForComplete'和'NotifyComplete',
      用来从OnWriteStreamComplete或者OnReadStreamComplete通知maincli，当前操作结束，可以做下一步了。
    * 然后重复前面两步100次。

2. 服务器端的任务：
    * 流通道建立时，我们给和流关联的上下文里加一个TransferContext的对象，包含一个计数器，和一个传输错误码。
    * 流通道关闭时，我们释放这个TransferContexta对象。
    * 服务器端收发数据包将使用异步的读写操作，即ReadStreamAsync和WriteStreamAsync。
    * 综合上述的要求，具体的修改有:
        * 重写(override)OnStreamReady，建立和关联TransferContext。
        * 重写OnStmClosing，释放TransferContext。
        * 重写OnReadStreamComplete，用方法WriteAndReceive打印接收的消息，并发回一个响应消息。
        * 重写OnWriteStreamComplete，用函数ReadAndReply向客户端发一个消息，然后接收客户端发出来的消息。
        * 这样就形成了一个客户端和服务器端的一个应答循环。

#### 添加代码
通过以上的分析，我们可以给出客户端和服务器端需要作出的修改。本节代码量稍大，不过也都是些很基本的操作。
##### 客户端需要改写的函数
  1. maincli
     ```
        gint32 maincli(
            CStreamTest_CliImpl* pIf,
            int argc, char** argv )
        {
            gint32 ret = 0;
            do{
                stdstr i0r;
                ret = pIf->Echo( "Hello, stmtest!", i0r );
                if( ERROR( ret ) )
                    break;
                OutputMsg( ret,
                    "Echo completed with response %s",
                    i0r.c_str() );
                HANDLE hChannel = INVALID_HANDLE;
                ret = pIf->StartStream(                         // 客户端需要调用`StartStream`。发起建立流通道。
                    hChannel, nullptr, nullptr );               // 最后一个参数是回调函数，如果不空，StartStream将
                                                                // 返回STATUS_PENDING，并在回调函数中通知流通道是否
                                                                // 建立成功。如果没有回调函数，就同步的等待结果。
                if( ERROR( ret ) )
                {
                    OutputMsg( ret, "Error open stream " );
                    break;
                }

                for( gint32 i = 0; i < 100; i++ )
                {
                    stdstr strMsg = "a message to server ";
                    strMsg += std::to_string( i );
                    BufPtr pBuf( true );
                    *pBuf = strMsg;
                    ret = pIf->WriteStream( hChannel, pBuf );   // 同步的发送strMsg
                    if( ERROR( ret ) )
                        break;
                    pBuf.Clear();
                    ret = pIf->ReadStream( hChannel, pBuf );    // 同步的接收数据
                    if( ERROR( ret ) )
                        break;
                    stdstr strBuf = BUF2STR( pBuf );
                    OutputMsg( ret,
                        "Server says (sync): %s",
                        strBuf.c_str() );
                    double dbval = i + .1;
                    strMsg = "a message to server " +
                        std::to_string( dbval );                // 给strMsg加点变化，区分消息同步的还是异步的。
                    pBuf.NewObj();
                    *pBuf = strMsg;
                    ret = pIf->WriteStreamAsync(
                        hChannel, pBuf,
                        ( IConfigDb* )nullptr );                // 异步的发送strMsg，回调函数为OnWriteStreamComplete
                    if( ERROR( ret ) )
                        break;

                    if( ret == STATUS_PENDING )
                    {
                        pIf->WaitForComplete();                 // 等待来自OnWriteStreamComplete的通知
                        ret = pIf->GetError();
                        if( ERROR( ret ) )
                            break;
                    }
                    pBuf.Clear();
                    ret = pIf->ReadStreamAsync(
                        hChannel, pBuf, 
                        ( IConfigDb* )nullptr );                // 异步的接收strMsg，回调函数为OnReadStreamComplete
                    if( ERROR( ret ) )
                        break;

                    if( ret == STATUS_PENDING )
                    {
                        pIf->WaitForComplete();                 // 等待来自OnReadStreamComplete的通知
                        ret = pIf->GetError();
                        if( ERROR( ret ) )
                            break;
                        // the response is handled by
                        // callback
                        continue;
                    }
                    // immediate return
                    strBuf = BUF2STR( pBuf );
                    OutputMsg( ret,
                        "Server says(async): %s",               // 打印服务器的响应
                        strBuf.c_str() );
                }
                ret = pIf->CloseStream( hChannel );             // 主动关闭流通道
            }while( 0 );

            return ret;
        }
     ```
  2. OnReadStreamComplete和OnWriteStreamComplete   
     ``` 
        gint32 CStreamTest_CliImpl::OnReadStreamComplete(
            HANDLE hChannel, gint32 iRet,
            BufPtr& pBuf, IConfigDb* pCtx )
        {
            this->SetError( iRet );                             // 设置错误码
            if( ERROR( iRet ) )
            {
                OutputMsg( iRet,
                    "ReadStreamAsync failed with error " );
                this->NotifyComplete();
                return iRet;
            }
            stdstr strBuf = BUF2STR( pBuf );
            OutputMsg( iRet, "Server says( async ): %s",
                strBuf.c_str() );
            this->NotifyComplete();                             // 通知maincli
            return iRet;
        }

        gint32 CStreamTest_CliImpl::OnWriteStreamComplete(
            HANDLE hChannel, gint32 iRet,
            BufPtr& pBuf, IConfigDb* pCtx )
        {
            this->SetError( iRet );                             // 设置错误码
            if( ERROR( iRet ) )
            {
                OutputMsg( iRet,
                    "WriteStreamAsync failed with error " );
            }
            this->NotifyComplete();                             // 通知maincli
            return iRet;

        }
     ```
  4. 添加NotifyComplete和WaitForcomplete。
##### 服务器端要改写的函数
  1. OnStreamReady
     ```
     gint32 CStreamTest_SvrImpl::OnStreamReady(
        HANDLE hChannel )
     {
        gint32 ret = 0;
        do{
            OutputMsg( ret, "Haha" );
            stdstr strGreeting = "Hello, Proxy!";
            BufPtr pBuf( true );
            *pBuf = strGreeting;
            ret = this->WriteStreamNoWait(                      // 先给客户端发一个问候。不关心是否成功。
                hChannel, pBuf );
            if( ERROR( ret ) )
                break;
            TransferContext* ptc =
                new TransferContext();

            ret = SET_CHANCTX( hChannel, ptc );                 // 把和hChannel关联的TransferContext对象，放到hChannel专属的上下文对象中。
            ReadAndReply( hChannel );                           // 接收客户端消息，并发应答。

        }while( 0 );
        // error will cause the stream to close
        return ret;
     }
     ```
  2. OnStmClosing
     ```
     gint32 CStreamTest_SvrImpl::OnStmClosing(
        HANDLE hChannel )
     {
        gint32 ret = 0;
        do{
            TransferContext* ptc =
                GET_CHANCTX( hChannel );                        // 从hChannel关联的上下文对象中取出TransferContext对象
            if( ERROR( ret ) )
                break;

            OutputMsg( ptc->GetError(),
                "Stream 0x%x is closing with status ",
                hChannel );
            delete ptc;                                         // 释放TransferContext
            SET_CHANCTX( hChannel, nullptr );

        }while( 0 );
        return super::OnStmClosing( hChannel );                 // 必须调用基类的OnStmClosing。
     }
     ```
  3. OnReadStreamComplete和OnWriteStreamComplete
     ```
        gint32 CStreamTest_SvrImpl::OnReadStreamComplete(
            HANDLE hChannel, gint32 iRet,
            BufPtr& pBuf, IConfigDb* pCtx )
        {
            ...
            stdstr strBuf = BUF2STR( pBuf );
            OutputMsg( iRet, "Proxy says %s",
                strBuf.c_str() );
            this->WriteAndReceive( hChannel );                  // 异步的发送消息，异步的接收消息。在服务器端，异步调用是首选

            return 0;
        }

        gint32 CStreamTest_SvrImpl::OnWriteStreamComplete(
            HANDLE hChannel, gint32 iRet,
            BufPtr& pBuf, IConfigDb* pCtx )
        {
            gint32 ret = 0;
            do{
                ...
                TransferContext* ptc = GET_CHANCTX( hChannel ); // 取出关联在hChannel上的TrasnferContext对象。
                if( ERROR( ret  ) ) 
                    break;

                ptc->IncCounter();                              // 发送计数器加一
                this->ReadAndReply( hChannel );                 // 异步的接收消息，异步的发送消息。

            }while( 0 );

            return 0;
        }
     ```
  5. 添加ReadAndReply和WriteAndReceive
     ```
        gint32 CStreamTest_SvrImpl::ReadAndReply(
            HANDLE hChannel )
        {
            gint32 ret = 0;
            do{
                TransferContext* ptc =
                    GET_CHANCTX( hChannel );                    // 从hChannel关联的上下文对象中取出TransferContext对象
                if( ERROR( ret ) )
                    break;

                while( true )
                {
                    BufPtr pBuf;
                    ret = this->ReadStreamAsync(                // 发起异步的读一个数据包，这里即是一个消息
                        hChannel, pBuf,
                        ( IConfigDb* )nullptr );                // 回调函数为OnReadStreamComplete
                    if( ERROR( ret ) )
                        break;
                    if( ret == STATUS_PENDING )                 // 缓冲区里没有消息，本次调用结束，后续处理从OnReadStreamComplete继续
                    {
                        ret = 0;
                        break;
                    }

                    stdstr strBuf = BUF2STR( pBuf );
                    OutputMsg( ret, "Proxy says: %s",
                        strBuf.c_str() );

                    gint32 idx = ptc->GetCounter();
                    stdstr strMsg = "This is message ";
                    strMsg += std::to_string( idx );
                    pBuf.NewObj();
                    *pBuf = strMsg;
                    IConfigDb* pCtx = nullptr;
                    ret = this->WriteStreamAsync(               // 发起一个异步写操作
                        hChannel, pBuf, pCtx );
                    if( ERROR( ret ) )
                        break;

                    if( ret == STATUS_PENDING )                 // 发送未完成，本次调用结束，后续处理从OnWriteStreamComplete继续。
                    {
                        ret = 0;
                        break;
                    }
                    ptc->IncCounter();                          // 发送计数器加一，本次发送完成，循环继续。
                }
                if( ERROR( ret ) )
                {
                    ptc->SetError( ret );
                }
            }while( 0 );
            if( ERROR( ret ) )
                OutputMsg( ret,
                    "ReadAndReply failed with error " );
            return ret;
        }

        gint32 CStreamTest_SvrImpl::WriteAndReceive(
            HANDLE hChannel )
        {
            gint32 ret = 0;
            do{
                TransferContext* ptc =
                    GET_CHANCTX( hChannel );                    // 从hChannel关联的上下文对象中取出TransferContext对象
                if( ERROR( ret ) )
                    break;

                while( true )
                {
                    gint32 idx = ptc->GetCounter();
                    stdstr strMsg = "This is message ";
                    strMsg += std::to_string( idx );
                    BufPtr pBuf( true );
                    *pBuf = strMsg;
                    ret = this->WriteStreamAsync(               // 发起一个异步写操作
                        hChannel, pBuf, 
                        ( IConfigDb* )nullptr );
                    if( ERROR( ret ) )
                        break;

                    if( ret == STATUS_PENDING )                 // 发送未完成，本次调用结束，后续处理从OnWriteStreamComplete继续。
                    {
                        ret = 0;
                        break;
                    }
                    ptc->IncCounter();                          // 发送计数器加一，本次发送完成，继续接收客户端消息。
                    pBuf.Clear();
                    ret = this->ReadStreamAsync(
                        hChannel, pBuf,
                        ( IConfigDb* )nullptr );
                    if( ERROR( ret ) )
                        break;

                    if( ret == STATUS_PENDING )                 // 缓冲区里没有消息，本次调用结束，后续处理从OnReadStreamComplete继续
                    {
                        ret = 0;
                        break;
                    }
                    stdstr strBuf = BUF2STR( pBuf );
                    OutputMsg( ret, "Proxy says: %s",           // 打印收到的消息，循环继续。
                        strBuf.c_str() );
                }
                if( ERROR( ret ) )
                    ptc->SetError( ret );

            }while( 0 );

            if( ERROR( ret ) )
                OutputMsg( ret,
                    "ReadAndReply failed with error " );
            return ret;
        }
     ```
     由于异步的读写操作，这两个函数的写法不同于同步函数的写法，等待的过程由退出当前函数开始，进入回调函数结束。执行的过程，不象同步那么一目了然。
  6. 最后定义一个结构TransferContext。

#### 问题和思考
  可以看到WriteAndReceive和ReadAndReply的可读性不太好，那么有没有可以改进的办法呢？下一节，我们将给出另外一种异步编程的解决方案。

#### 编译
  * 在命令行下，我们可以看到在`stmtest`目录下有`Makefile`。只要输入`make -j4`命令即可。如果想要debug版，就输入`make debug -j4`。
  * 编译完后会提示`Configuration updated successfully`。这表示已经和系统的设置同步了, 可以运行了。也可以手动的运行 `python3 synccfg.py`来同步。
  * 在`stmtest/release`目录下，我们应该能找到`stmtestsvr`和`stmtestcli`两个程序。

#### 运行
  * 在stmtest例子中，我们在同一机器上启动两个`rpcrouter`的实例。一个是`rpcrouter -dr 1`作为客户端的转发器，一个是`rpcrouter -dr 2`, 作为服务器端的桥接器。
  * 在服务器端运行`release/stmtestsvr`。
  * 在客户端运行`release/stmtestcli`。
  * 检查客户端和服务器端是否交替的打印对方发出的消息。

[下一讲](./Tut-AsyncProgramming_cn-7.md)   
[上一讲](./Tut-Serialization_cn-5.md)   
