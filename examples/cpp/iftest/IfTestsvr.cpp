/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -O . ../../iftest.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "iftest.h"
#include "IfTestsvr.h"

// IEchoThings Server
/* Sync Req Handler*/
gint32 CIfTest_SvrImpl::Echo(
    GlobalFeatureList& i0 /*[ In ]*/,
    GlobalFeatureList& i0r /*[ Out ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    i0r = i0;
    return STATUS_SUCCESS;
}


