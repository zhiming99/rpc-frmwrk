/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmtest.h"
#include "StreamTestcli.h"

gint32 CStreamTest_CliImpl::OnReadStreamComplete(
    HANDLE hChannel, gint32 iRet,
    BufPtr& pBuf, IConfigDb* pCtx )
{
    this->SetError( iRet );
    if( ERROR( iRet ) )
    {
        OutputMsg( iRet,
            "ReadStreamAsync failed with error " );
        this->NotifyComplete();
        return iRet;
    }
    char szBuf[ 128 ];
    BUF2STR( pBuf, szBuf );
    OutputMsg( iRet,
        "Server says( async ): %s",
        szBuf );
    this->NotifyComplete();
    return iRet;
}

gint32 CStreamTest_CliImpl::OnWriteStreamComplete(
    HANDLE hChannel, gint32 iRet,
    BufPtr& pBuf, IConfigDb* pCtx )
{
    this->SetError( iRet );
    if( ERROR( iRet ) )
    {
        OutputMsg( iRet,
            "WriteStreamAsync failed with error " );
    }
    this->NotifyComplete();
    return iRet;

}
