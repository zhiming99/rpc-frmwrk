/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "evtest.h"
#include "EventTestcli.h"

// IEventTest Proxy
/* Event */
gint32 CEventTest_CliImpl::OnHelloWorld(
    const std::string& strMsg /*[ In ]*/ )
{
    // TODO: Processing the event here
    // return code ignored
    OutputMsg( 0,
        "received event %s",
        strMsg.c_str() );

    return 0;
}


