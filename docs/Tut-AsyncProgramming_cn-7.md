# C++开发教程
## 第七节 异步编程进阶-使用任务和任务组
**本节要点**：   
* 任务组和任务简介。
* 使用任务组和任务组合代替ReadAndReply和WriteAndReceive。

### 项目需求
* 使用任务组和任务改写上一节的ReadAndReply和WriteAndReceive。

### 生成stmtest的C++项目框架
我们仍然使用[stmtest.ridl](../examples/stmtest.ridl)作为本节课的用例。

#### 任务组和任务简介
请参考[概念](../Concept_cn.md#任务tasklet和任务组taskgroup)关于任务组和任务的介绍。
这里我们介绍一下用于实战的任务组的两个类定义。
| 序号 | 类名 | 串行/并行| 描述 | 
| -------- | --------- | --------- |------------------|
|1|CIfTaskGroup|串行|顺序的执行一组任务，任务之间的逻辑关系，可以有'与'，'或'，'无'。无就是顺序执行，不管对错，直到执行完组内所有任务，'与'是所有组内任务必须成功执行，否则在第一个失败处，终止执行，余下的任务将被取消。'或'的含义相反，终止执行，余下任务将被取消。可以在任务组执行到一半，添加新任务。|
|2|CIfParallelTaskGrp|并行|并发的执行一组任务。直到所有的任务被执行完。可以在任务组执行到一半，添加新任务。|   

同时我们还将用到两个生成任务对象的宏

| 序号 | 宏定义| 描述 | 
| -------- | --------- |------------------|
|1|NEW_FUNC_CALL_TASKEX(pNewTask, pIoMgr, func, ...)|建立一个名为pNewTask的任务，执行函数func, 参数为后面的参数列表|
|2|NEW_COMPLETE_FUNCCALL(pos, pNewTask, pIoMgr, func, ...)|建立一个名为pNewTask的任务，该任务将作为回调函数使用，并在回调的过程中执行func函数，参数为后面的参数列表。同时把func的第pos个参数替换成pNewTask|   

以上这两个宏接受函数指针func作为参数。

#### 总体流程
1. 客户端和服务器端的流程和[前面一节](./Tut-Stream_cn-6.md#总体流程)的实现基本相同。

2. 服务器端略有改动。就是把ReadStreamAsync/WriteStreamAsync的功能用任务组和任务来完成，为此我们添加两个新函数，
   RunReadWriteTasks和BuildAsyncTask。并用RunReadWriteTask替换OnStreamReady中的ReadAndReply。

#### 代码解析
  * RunReadWriteTasks函数就是把BuildAsyncTask返回的异步任务，用TaskGroup连接起来，形成一个串行的序列。
    ```
        TaskGrpPtr pGrp;
        CParamList oGrpCfg;
        oGrpCfg[ propIfPtr ] = ObjPtr( this );
        ret = pGrp.NewObj(
            clsid( CIfTaskGroup ),
            oGrpCfg.GetCfg() );                                 // 建立一个任务组
        if( ERROR( ret ) )
            break;
        pGrp->SetRelation( logicAND );                          // 设置任务组的成员之间是逻辑与的关系，也就是说所有任务必须成功，
                                                                // 如果遇到第一个失败任务时，停止执行，并取消剩余的任务。
        TaskletPtr pTask;
        ret = BuildAsyncTask( hChannel, true, pTask );          // 建立一个异步任务
        if( ERROR( ret ) )
            break;
        pGrp->AppendTask( pTask );                              // 加入到任务组
        ret = BuildAsyncTask( hChannel, false, pTask );
        if( ERROR( ret ) )
            break;
        pGrp->AppendTask( pTask );                              // 添加第二个任务到任务组

        ...
        ret = pSvc->RunManagedTask2( pGrpTask );                // 执行该任务组，RunManagedTask2的作用是控制该任务组的生命周期必须再这个流通道的生命周期之内。

    ```
  * BuildAsyncTask函数是建立异步任务，如果要求是读操作，就生成读操作的异步任务，否则是写操作的异步任务。如下面的代码片段。
     * 该异步任务由两两个Task对象组成，
     * 一个是执行发起读写操作的rwFunc函数的部分，就是发出读写流的命令的Task。
     * 另一个是执行asyncComp函数的部分，即查看结果和返回状态码给任务组，以便任务组根据状态码判断是否继续。
    ```
    using CompType = gint32 ( * )( IEventSink*,
    CStreamTest_SvrImpl*, HANDLE, bool );
    CompType asyncComp =                                        // 定义一个异步完成函数指针
        ([]( IEventSink* pCb,
            CStreamTest_SvrImpl* pIf,
            HANDLE hChannel, bool bRead )->gint32
        {
            do{
                ...
                if( bRead )
                {                                               // 读操作完成
                    ...
                }
                else
                {                                               // 写操作完成
                    {
                        CStdRMutex oIfLock( pIf->GetLock() );
                        TransferContext* ptc =
                            GET_CHANCTX( hChannel, pIf );
                        if( ERROR( ret ) )
                        {
                            OutputMsg( ret,
                                "Cannot find stream 2@0x%llx",
                                hChannel );
                            break;
                        }
                        ptc->IncCounter();
                    }
                    ret = pIf->RunReadWriteTasks( hChannel );   // 本任务组到此结束，运行下一组读写任务
                }
            }while( 0 );

            Variant oVar;
            iRet = pCb->GetProperty( 0x12345, oVar );
            if( SUCCEEDED( iRet ) )
            {
                CIfRetryTask* pTask =                           // 这个task就是执行rwFunc的那个任务，它还在任务组里等待
                    ( ObjPtr& )oVar;

                pTask->OnEvent( eventTaskComp,                  // 通知它，已经完成本次操作，任务组可以执行下一个任务了
                    0, 0, nullptr );
                pCb->RemoveProperty( 0x12345 );
            }
            return ret;
        }
    )
    gint32 ( *rwFunc )(                                         // 定义一个读写函数指针
        IEventSink*,
        CStreamTest_SvrImpl*, HANDLE, bool,
        intptr_t ) =
    ([](IEventSink* pCb,                                        // 指向执行该函数的任务的指针
        CStreamTest_SvrImpl* pIf,                               // 服务器对象指针
        HANDLE hChannel,                                        // 读写操作的目标流通道
        bool bRead,                                             // 执行读操作还是写操作
        intptr_t compFunc )->gint32                             // compFunc将是上面定义的asyncComp的值
        {
            do{
                BufPtr pBuf;
                auto asyncComp = ( CompType )compFunc;
                TaskletPtr pCompTask;
                ret = NEW_COMPLETE_FUNCALL(                     // 建立一个完成通知任务
                    0, pCompTask, pIf->GetIoMgr(),
                    asyncComp, nullptr, pIf, hChannel,          // 这个任务会在系统读写流完成时，调用asyncComp函数。
                    bRead );
                if( ERROR( ret ) )
                    break;

                Variant oVar = ObjPtr( pCb );
                pCompTask->SetProperty( 0x12345, oVar );        // 这个新建立的任务记住当前的任务，为的是可以通知它，任务结果。
                if( bRead )
                {
                    ret = pIf->ReadStreamAsync( 
                        hChannel, pBuf, pCompTask );            // 注意这里传入的是新建立的回调函数任务。
                }
                else
                {
        ...
                }
                if( ret == STATUS_PENDING )
                    break;                                      // 还没完成，继续等待

                pCompTask->RemoveProperty( 0x12345 );
                CParamList oResp;                               // 这里是准备返回给asyncComp的参数
                oResp.SetIntProp(
                    propReturnValue, ret );

                if( SUCCEEDED( ret ) )
                    oResp.Push( pBuf );

                oVar = ObjPtr( oResp.GetCfg() );

                pCompTask->SetProperty(
                    propRespPtr, oVar );

                ret = asyncComp( pCompTask,
                    pIf, hChannel, bRead );                     // 直接调用完成函数
                ( *pCompTask )( eventCancelTask );              // pCompTask没有用上，通知它销毁自己

            }while( 0 );
            return ret;
        }
    )

    TaskletPtr pRWTask;
    ret = NEW_FUNCCALL_TASKEX2( 0, pRWTask,                     // 建立读写操作的`发起任务`。
        this->GetIoMgr(), rwFunc, nullptr,
        this, hChannel, bRead,
        ( intptr_t )asyncComp );
    if( ERROR( ret ) )
        break;

    pTask = pRWTask;                                            // 把这个`发起任务`返回给调用者
    ```

#### 问题
试分析本节的方法的优缺点，比较本节方法和上一节的方法的性能。

#### 总结
本节给出了和客户端交替发消息的另一种实现方法。该方法需要对任务组和任务有较好的理解。虽然在速度上，比前一种方法慢一些，
但是此方法的可扩展性强，容错性好，熟练掌握后，可以比较快速的搭建更加复杂且可靠的异步执行解决方案。关于任务组
间的同步，重试和生命周期的控制等技术我们会在后面的课程中继续讨论。

[上一讲](./Tut-Stream_cn-5.md)   

