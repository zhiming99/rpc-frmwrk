/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "HelloWorld.h"
#include "HelloWorldSvcsvr.h"

// IHelloWorld Server
/* Sync Req */
gint32 CHelloWorldSvc_SvrImpl::Echo(
    const std::string& strText /*[ In ]*/,
    std::string& strResp /*[ Out ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    strResp = strText;
    return STATUS_SUCCESS;
}


