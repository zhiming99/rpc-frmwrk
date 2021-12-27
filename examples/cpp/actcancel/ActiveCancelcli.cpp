/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "actcancel.h"
#include "ActiveCancelcli.h"

// IActiveCancel Proxy
/* Async Req */
gint32 CActiveCancel_CliImpl::LongWaitCallback(
    IConfigDb* context, gint32 iRet,
    const std::string& i0r /*[ In ]*/ )
{
    OutputMsg( iRet,
        "LongWait returned with status" );
    SetError( iRet );
    NotifyComplete();
    return 0;
}

