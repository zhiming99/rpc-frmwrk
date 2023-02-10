/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -O . ../../asynctst.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "asynctst.h"
#include "AsyncTestcli.h"

// IAsyncTest Proxy
/* Async Req Complete Handler*/
gint32 CAsyncTest_CliImpl::LongWaitNoParamCallback(
    IConfigDb* context, gint32 iRet )
{
    OutputMsg( iRet,
        "LongWaitNoParam returned with status" );
    SetError( iRet );
    NotifyComplete();
    return 0;
}


