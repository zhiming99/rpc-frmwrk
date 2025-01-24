
# rpc-frmwrk开发教程
## 第三节 异步请求和处理
**本节要点**：   
* 异步请求和异步处理的特点及流程
* AsyncTest服务器程序和客户端程序的结构
* 对客户端和服务器端进行修改，完成所需要的功能。
* 编译，运行和检查结果。

### 项目需求
1. 实现客户端以异步的方式发送请求和处理返回结果。
2. 实现服务器端以异步的方式处理请求。
3. 运行并检查结果。

### 生成AsyncTest的C++项目框架
本节课我们使用[asynctst.ridl](../examples/asynctst.ridl)来生成该[项目框架](../examples/cpp/asynctst)。
阅读理解本文时，结合asynctst目录下的代码，效果会更好
异步编程复杂很多。但是为什么要用异步编程呢，因为它很快!

#### 接口的定义的解释和说明
  首先介绍一条ridlc代码的生成规则:   
  * 客户端所有异步的接口方法都已实现，所有异步接口方法的回调方法待用户自己实现。这两类方法均可以通过顶端的CAsyncTest_CliImpl类访问到。类名字格式是'C'+'服务名'+'_CliImpl'
  * 服务器端和客户端相反。所有异步接口方法有待用户自己实现。所有异步接口方法的完成函数都已实现。这两类方法均可以通过顶端的CAsyncTest_SvrImpl类访问到。类名字是'C'+'服务名'+'_SvrImpl'

  在接口IAsyncTest上定义了三个方法，让我们来详细分析一下最具代表性的第一个方法。
  ##### [timeout=97, async]LongWait( string i0 ) returns ( string i0r )   
  * 标签一指定时间限制(timeout)为97秒，超过了97秒就返回失败
  * 标签二指定本方法，在客户端异步的发送请求，然后通过回调函数来处理服务器返回的消息。在服务端也是异步处理请求，然后返回结果。
  * 我们分别来看ridlc在客户端和服务器端为异步LongWait生成的方法。
    1. 客户端   
          * 方法LongWait有一个输入参数i0, 是字符串类型, 和一个输出类型i0r, 也是字符串类型, 和一个名为context的参数，含义见下方的注释。
          * 当使用ridlc生成框架代码时，客户端会生成以下两个方法:
              ```
              //RPC Async Req Sender
              gint32 LongWait(                                  // 发送LongWait的方法，已实现，直接调用就可以了
                  IConfigDb* context,                           // context用于存放用户相关的参数
                  const std::string& i0,  /*[ In ]*/            // 输入参数i0
                  std::string& i0r /*[ Out ]*/);                // 输出参数i0r
                                                                // 返回值需要关注
                                                                // 返回 0， 表示该方法完成了，i0r里面是服务器的消息
                                                                // 返回 STATUS_PENDING，表示系统已经接收了请求，然后通过下面的
                                                                // 'LongWaitCallBack'回调函数处理服务器的返回消息
                                                                // 返回值小于0, 表示出错了，进行出错处理。

              // IAsyncTest
              virtual gint32 LongWaitCallback(                  // 异步方法LongWait的回调函数, 命名格式是'方法名+Callback'
                                                                // 定义在AsyncTestcli.cpp中，等待用户添加实现代码。
                  IConfigDb* context,                           // 该参数来自上面的LongWait
                  gint32 iRet,                                  // iRet是系统给的返回值，0：成功，返回消息在i0r里面。小于0，出错，没有返回消息。
                  const std::string& i0r /*[ In ]*/ );          // i0r是服务器端的返回消息，只有在上面的iRet为0时才有意义。
                                                                // 注意: 当返回值时错误码时，错误的来源既可能来自服务器，也可能来自本地的代码。
              ```
    2. 服务器端
          * 方法LongWait有一个输入参数i0, 是字符串类型, 一个输出类型i0r, 也是字符串类型，和一个pReqCtx_参数，含义见注释。
          * 当使用ridlc生成框架代码时， 服务器端会生成以下三个方法:
            ```
            gint32 LongWait(                                    // LongWait请求的主处理方法， 定义在AsyncTestsvr.cpp中， 等待用户实现代码
                IConfigDb* pReqCtx_,                            // LongWait请求的系统上下文，里面有一些Callback和Session信息，你也可以往里面加东西，但不要删掉
                                                                // 或者覆盖原有的东西。
                const std::string& i0 /*[ In ]*/,               // 输入参数 i0, 来自客户端的参数
                std::string& i0r /*[ Out ]*/ );                 // 输出参数 i0r用来放返回给客户端的参数。
                                                                // 返回值：返回值是0时，代表告诉系统，处理成功，i0r中有返回参数
                                                                //        返回值<0时，代表该请求出错，通知系统返回错误码给客户端。
                                                                //        返回值是STATUS_PENDING时，代表该请求仍在处理中，后续会调用LongWaitComplete通知完成
                                                                // 注释：有人会费解'后续调用'是什么操作，很简单，比如你把用户请求传递给一个任务或者专门的线程去处理，
                                                                // 当处理完成后，该任务或者线程调用LongWaitComplete通知结果。当然这个阶段，你需要把pReqCtx_找个
                                                                // 地方存起来，以便在调用LongWaitComplete的时候，可以提供该参数。

            gint32 LongWaitComplete(                            // LongWaitComplete是系统生成代码，相当于服务器端异步处理的收尾函数。命名格式 '方法名'+Complete.
                                                                // 它通知系统，由用户代码调用， 通知系统向客户端返回处理结果。
                IConfigDb* pReqCtx_,                            // pReqCtx_, 来自LongWait方法的输入参数。
                gint32 iRet,                                    // 返回值： 返回值是0时，代表该请求已完成，i0r中有返回参数
                                                                //         返回值<0时，代表该请求出错，通知系统返回错误码给客户端。
                                                                //         不能返回STATUS_PENDING
                const std::string& i0r );                       // 返回参数 i0r, 只有当iRet为0时有效。其他时候忽略。

            gint32 OnLongWaitCanceled(                          // OnLongWaitCanceled，当请求被系统取消时，会被调用到的方法。该方法定义在AsyncTestsvr.h中，
                                                                // 你可以自行决定是否替换
                                                                // 现有的打印语句，换上更有意义的代码。命名格式 'On'+'方法名'+'Cacneled'。当系统取消长时间没有调用
                                                                // LongWaitComplete的请求时，客户端主动取消该请求时，或者发生严重错误，如连接中断时，系统会在清理未
                                                                // 完成请求的过程中，
                                                                // 调用此方法。此时系统已经向客户端发送了处理结果，所以不能在这里发LongWaitComplete了。此方法只是提供
                                                                // 一个方便用户回收占用的资源的时机。
                IConfigDb* pReqCtx,                             // 输入参数 pReqCtx, 来自LongWait的输入参数pReqCtx_
                gint32 iRet,                                    // 返回值：该返回值为一个错误码，通知错误的原因。
                const std::string& i0 /*[ In ]*/ )              // 输入参数 i0，来自LongWait的输入参数i0。

            ```

  ##### [async_p]LongWaitNoParam() returns ()
  * 方法LongWaitNoParam没有输入和输出参数。
  * 标签async_p说明该方法在客户端是异步的，在服务器端是同步的。
  * 参考上面对LongWait的客户端的方法的分析，ridlc为它生成了两个客户端方法。
    ```
    gint32 LongWaitNoParam(                                     // 已实现
        IConfigDb* context );

    gint32 LongWaitNoParamCallback(                             // 待实现
        IConfigDb* context, gint32 iRet );
    ```
  * 在服务器端，ridlc生成了一个方法:
    ```
    gint32 LongWaitNoParam();                                   // 待实现，注意比较同步的服务器处理函数和异步的服务器处理函数的参数上有什么不同。
    ```

 ##### [async_s]LongWait2( string i1 ) returns ( string i1r )
  * 方法LongWait2和上面的LongWait有相同的输入和输出参数。
  * 标签async_s说明该方法在客户端是同步的，在服务器端是异步的。
  * 参考上面对LongWait的服务器端的方法的分析，ridlc为它生成了一个客户端方法。
    ```
    gint32 LongWait2( const std::string& i1,                    // 已实现, 注意比较客户端同步的接口方法和异步的接口方法在参数上有什么不同。
      std::string& i1r );  
    ```
  * 参考上面对LongWait的服务器端的方法的分析，ridlc为LongWait2生成了三个服务器端方法。
    ```
    gint32 LongWait2(                                           // 待实现
        IConfigDb* pReqCtx_,
        const std::string& i1 /*[ In ]*/,
        std::string& i1r /*[ Out ]*/ );

    gint32 LongWait2Complete(                                   // 已实现
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& i1r );

    gint32 OnLongWait2Canceled(                                 // 已实现，可替换。
        IConfigDb* pReqCtx,
        gint32 iRet,
        const std::string& i1 /*[ In ]*/ );
    ```

#### 用ridlc生成项目框架
  * 在asynctst.ridl顶端有生成各个语言的命令行。我们使用标有C++的那行命令。   
    `mkdir asynctst; ridlc -Lcn -O ./asynctst asynctst.ridl`

#### 程序结构和流程简介
程序结构和流程和HelloWorld的大同小异，就不再赘述了。

#### 添加代码：
  通过以上的分析，我们大体上知道了应该在那些地方添加或者修改代码。对了，就是那些标有`待实现`的方法。除此之外，还有必改之处`maincli`函数。
  * 在客户端，我们需要在`maincli`中向服务器发出请求。由于LongWait方法和LongWaitNoParam方法是异步的，我们在CAsyncTest_CliImpl类里
    添加了一个信号量和两个方法'WaitForComplete'和'NotifyComplete', 用来同步`maincli`和LongWaitCallback或者LongWaitNoParamCallback。
    还添加了一个返回值变量用以记录本次请求的返回值。
    ```
    gint32 maincli(
        CAsyncTest_CliImpl* pIf,
        int argc, char** argv )
    {
      gint32 ret = 0;
      do{
          stdstr i0, i0r;
          i0 = "hello, LongWait";
          CParamList oCtx;
          ret = pIf->LongWait( oCtx.GetCfg(), i0, i0r );        // 发出异步请求，这里第一个参数对应的形式参数pReqCtx_，
                                                                // 没有额外的东西需要放到pReqCtx_中，就给了一个空的ParamList, 也可以给个nullptr。
          if( ERROR( ret ) )
          {
              OutputMsg( ret,
                  "LongWait failed with error " );
              break;
          }
          if( ret == STATUS_PENDING )                           // 异步程序关键的一步，判断是否STATUS_PENDING。
          {
              // waiting for server response
              pIf->WaitForComplete();                           // 等待来自LongWaitCallback的信号
              ret = pIf->GetError();                            // 该请求已在LongWaitCallback处理完，查看状态码
              if( ERROR( ret ) )
              {
                  OutputMsg( ret,
                      "LongWait failed with error" );
                  break;
              }
              OutputMsg( ret,
                  "LongWait completed successfully" );
          }
          // 下面的操作类似，参考maincli.cpp就可以了

      }while( 0 );
      return ret;
    }
    ```
  * 在客户端，我们还需要在CAsyncTest_CliImpl添加LongWaitCallback的实现代码
    ```
      gint32 CAsyncTest_CliImpl::LongWaitCallback(              // 定义在AsyncTestcli.cpp的Callback函数。
          IConfigDb* context,                                   // 来自LongWait第一个输入参数pReqCtx_。
          gint32 iRet,                                          // 返回值
          const std::string& i0r /*[ In ]*/ )                   // 返回参数i0r。
      {
          if( ERROR( iRet ) )
              OutputMsg( iRet,
                  "LongWait returned with error" );
          else
              OutputMsg( iRet,
            "LongWait returned with response %s", i0r.c_str() );
          SetError( iRet );                                     // 设置状态码。
          NotifyComplete();                                     // 通知maincli，LongWait请求已完成。
          return 0;                                             // Callback的返回值一般被忽略
      }
    ```   
    另外两个接口方法的修改方法与上面的方法相同。

  * 服务器端, ridlc为LongWait产生的三个方法中, 只有LongWait是待实现，也就是我们要修改的重点。
    ```
      gint32 LongWait(                                          // 待实现
          IConfigDb* pReqCtx_,
          const std::string& i0 /*[ In ]*/,
          std::string& i0r /*[ Out ]*/ );

      gint32 OnLongWaitCanceled(                                // 已实现，可替换
          IConfigDb* pReqCtx,
          gint32 iRet,
          const std::string& i0 /*[ In ]*/ );

      
      gint32 LongWaitComplete(                                  // 已实现
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& i0r );
    ```
    一下是LongWait的改造的任务列表：
    * 为了模拟一个异步处理，我们使用了一个定时器。 约定5秒之后完成LongWait请求。
    * 由于定时器也是异步的，也要自己的回调函数，我们在CAsyncTest_SvrImpl里增加了一个LongWaitCb的定时器回调函数。
    * 由于定时器分配了系统资源，所以我们要改写OnLongWaitCanceled, 万一请求被取消，可以释放定时器资源。
    * 这样我们有三个函数要改
      ```
        gint32 CAsyncTest_SvrImpl::LongWaitCb(                  // 定时器的回调函数
            IEventSink*, IConfigDb* pReqCtx_,
            const std::string& i0 )
        {
            LongWaitComplete( pReqCtx_, 0, i0 );                // 调用LongWaitComplete，通知系统，向客户端返回消息
            return 0;
        }

        gint32 CAsyncTest_SvrImpl::LongWait(                    // LongWait的服务器端处理函数
            IConfigDb* pReqCtx_,                                // 系统传入的上下文参数
            const std::string& i0 /*[ In ]*/,                   // 客户端发来的输入参数
            std::string& i0r /*[ Out ]*/ )                      // 服务器返回给客户的输出参数，如果本函数返回0的话，表示同步的处理完成，那么i0r中
                                                                // 的参数将被发给客户端
        {
            gint32 ret = 0;

            ADD_TIMER( this, pReqCtx_, 5,                       // 系统提供的一个定时器宏，将在5秒后触发，并用给定的参数调用LongWaitCb.
                &CAsyncTest_SvrImpl::LongWaitCb,                // 这个宏内部会把包裹Timer回调函数的任务对象存如pReqCtx_的propContext属性中,
                                                                // 同时还会在变量ret中设置返回值。
                i0 );

            if( ERROR( ret ) )
                return ret;

            return  STATUS_PENDING;                             // 返回STATUS_PENDING, 通知请求仍在处理中
        }

        gint32 CAsyncTest_SvrImpl::OnLongWaitCanceled(
            IConfigDb* pReqCtx,                                 // 来自LongWait的第一个参数 pReqCtx_
            gint32 iRet,                                        // 错误码，取消原因
            const std::string& i0 /*[ In ]*/ )                  // LongWait的输入参数i0
        {
            ...
            CParamList oReqCtx( pReqCtx );

            IEventSink* pTaskEvt = nullptr;
            gint32 ret = oReqCtx.GetPointer(
                propContext, pTaskEvt );                        // 取出执行Timer的任务对象

            if( ERROR( ret ) )
                return ret;

            // cancel the timer task
            TaskletPtr pTask = ObjPtr( pTaskEvt );
            ( *pTask )( eventCancelTask );                      // 执行取消Timer任务的操作，回收系统资源。
                                                                // 假如Timer不取消，Timer最终会执行并被释放资源, 但是Timer回调函数会调用
                                                                // LongWaitComplete，而此时该请求早已取消了。这是逻辑错误。
            return 0;
        }

      ```
  * 服务器端, ridlc为LongWait2同样产生了三个方法, 和LongWait的服务器端生成方法类似。不再重复。
    这里解释一下LongWait2的模拟异步处理的实现方法。
    ```
      gint32 CAsyncTest_SvrImpl::LongWait2(
          IConfigDb* pReqCtx_,
          const std::string& i1 /*[ In ]*/,
          std::string& i1r /*[ Out ]*/ )
      {
          CIoManager* pMgr = GetIoMgr();
          gint32 ret = DEFER_CALL(                              // DEFER_CALL宏生成一个任务对象(CTasklet)，并加入到执行队列中
              pMgr, this,                                       // DEFER_CALL的作用是在当前调用栈之外，用给定的参数调用给定的函数。
              &CAsyncTest_SvrImpl::LongWait2Cb,                 // 具体到这个任务对象, 就是在其他线程上或者退出LogWait2后，以pReqCtx_和i1为参数，
                                                                // 调用方法LongWait2Cb。
              pReqCtx_, i1 );                                   // 这也是异步的处理，但是和LongWait不一样的是，它不需要OnLongWait2Canceled回收资源。

          if( ERROR( ret ) )
              return ret;

          return STATUS_PENDING;                                // 返回STATUS_PENDING
      }

    ```
  * 剩下的LongWaitNoParam的改造过程，大家可以参考[asynctst](../examples/cpp/asynctst/)目录下的代码，分析和理解。

  * 这样我们就完成了客户端和服务端的修改。
#### 问题
  1. 异步函数对应的必须会产生回调函数，那么就要在类里面定义新的方法，有没有更加简便的方法呢？有的，也可以使用lambda函数。在后面的课程中，我们将会讲到这个方法。

#### 编译
  * 在命令行下，我们可以看到在`asynctst`目录下有`Makefile`。只要输入`make -j4`命令即可。如果想要debug版，就输入`make debug -j4`。
  * 编译完后会提示`Configuration updated successfully`。这表示已经和系统的设置同步了, 可以运行了。也可以手动的运行 `python3 synccfg.py`来同步。
  * 在`asynctst/release`目录下，我们应该能找到`asyntstsvr`和`asyntstcli`两个程序。

#### 运行
  * 在asynctst例子中，我们依然在同一机器上启动两个`rpcrouter`的实例。一个是`rpcrouter -dr 1`作为客户端的转发器，一个是`rpcrouter -dr 2`, 作为服务器端的桥接器。
  * 在服务器端运行`release/asynctstsvr`。
  * 在客户端运行`release/asynctstcli`。
  * 验证输出是不是`maincli`函数中的输出。


[下一讲](./Tut-CancelRequest_cn-4.md)   
[上一讲](./Tut-SendEvent_cn-2.md)   