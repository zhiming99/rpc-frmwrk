/*
 * =====================================================================================
 *
 *       Filename:  sfsvr.cpp
 *
 *    Description:  Implementation of interface class CEchoServer, and global helper
 *                  function, InitClassFactory and DllLoadFactory
 *
 *        Version:  1.0
 *        Created:  06/14/2018 09:27:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cppunit/TestFixture.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace rpcfrmwrk;
#include "sfsvr.h"

using namespace std;

gint32 CEchoServer::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CEchoServer );

    ADD_USER_SERVICE_HANDLER_EX( 1,
        CEchoServer::Echo,
        METHOD_Echo );

    END_IFHANDLER_MAP;

    return 0;
}

gint32 CEchoServer::Echo(
    IEventSink* pCallback,
    const std::string& strText, // [ in ]
    std::string& strReply )     // [ out ]
{
    gint32 ret = 0;

    // business logics goes here
    strReply = strText;

    return ret;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CSimpFileServer );
    INIT_MAP_ENTRYCFG( CSimpFileClient );
    INIT_MAP_ENTRY( CTransferContext );

    END_FACTORY_MAPS;
};

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 CMyFileServer::InitUserFuncs()
{
    super::InitUserFuncs();
    BEGIN_IFHANDLER_MAP( CMyFileServer );

    ADD_USER_SERVICE_HANDLER_EX( 1,
        CMyFileServer::GetFileInfo,
        "GetFileInfo" );

    END_IFHANDLER_MAP;
    return 0;
}

gint32 CMyFileServer::OnRecvData_Loop(
    HANDLE hChannel, gint32 iRet )
{
    gint32 ret = 0;
    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        if( hChannel == INVALID_HANDLE )
        {
            ret = -EINVAL;
            break;
        }

        ObjPtr pObj;
        ret = GetTransCtx( hChannel, pObj );
        if( ERROR( ret ) )
            break;

        CTransferContext* ptctx = pObj;
        if( ERROR( ret ) )
            break;

        if( !ptctx->m_bUpload )
            break;

        BufPtr pBuf;
        ret = ReadMsg( hChannel, pBuf, -1 );
        if( ret == -EAGAIN )
        {
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        ret = write( ptctx->m_iFd,
            pBuf->ptr(), pBuf->size() );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        ptctx->m_qwCurOffset +=
            ( guint32 )ret;

        guint32 dwSize = ptctx->m_qwCurOffset -
            ptctx->m_qwStart;

        if( dwSize == ptctx->m_dwReqSize )
        {
            // done
            ret = 0;
            close( ptctx->m_iFd );
            ptctx->m_iFd = -1;
            ScheduleToClose( hChannel );
        }
        else if( dwSize > ptctx->m_dwReqSize )
        {
            ret = -EFBIG;
            break;
        }

    }while( 1 );

    if( ERROR( ret ) )
        ScheduleToClose( hChannel );

    return ret;
}

gint32 CMyFileServer::OnWriteEnabled_Loop(
    HANDLE hChannel )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ObjPtr pObj;
        bool bFirst = false;
        ret = GetTransCtx( hChannel, pObj );
        if( ret == -ENOENT )
        {
            ret = OnStreamStarted( hChannel );
            if( ERROR( ret ) )
                break;

            ret = GetTransCtx( hChannel, pObj );
            if( ERROR( ret ) )
                break;

            bFirst = true;
        }

        if( ERROR( ret ) )
            break;

        CTransferContext* ptctx = pObj;
        if( ptctx == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        // for receiving end, do nothing here
        if( ptctx->m_bUpload ==
            this->IsServer() )
            break;

        if( unlikely( bFirst ) )
        {
            ret = ReadAndSend( hChannel, ptctx );
        }
        else
        {
            ret = ResumeBlockedSend(
                hChannel, ptctx );
        }

    }while( 0 );

    if( ERROR( ret ) )
        ScheduleToClose( hChannel );

    return ret;
}

gint32 CMyFileServer::OnSendDone_Loop(
    HANDLE hChannel, gint32 iRet )
{
    gint32 ret = 0;
    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        // get channel specific context
        ObjPtr pObj;
        ret = GetTransCtx( hChannel, pObj );
        if( ERROR( ret ) )
            break;

        CTransferContext* ptctx = pObj;
        if( ptctx->m_bUpload ==
            this->IsServer() )
            break;

        guint32 dwSent =
            ptctx->m_qwCurOffset -
            ptctx->m_qwStart;

        if( dwSent + STM_MAX_BYTES_PER_BUF >
            ptctx->m_dwReqSize )
        {
            // download is done
            ptctx->m_qwCurOffset = ptctx->m_qwStart
                + ptctx->m_dwReqSize;
            ret = ptctx->SetError( 0 );
            // don't quit immediately, waiting the
            // peer side to close the stream
            break;
        }

        ptctx->m_qwCurOffset +=
            STM_MAX_BYTES_PER_BUF;
        ret = ReadAndSend( hChannel, ptctx );

    }while( 0 );

    if( ERROR( ret ) )
        ScheduleToClose( hChannel );

    return ret;
}

gint32 CMyFileServer::OnCloseChannel_Loop(
    HANDLE hChannel )
{ return 0; }


gint32 CMyFileServer::GetFileInfo(
    IEventSink* pCallback,
    const std::string& strFileName, // [ in ]
    CfgPtr& pInfo )    // [ out ]
{
    gint32 ret = 0;
    CParamList oResp;
    do{
        std::string strPath = SF_ROOT_DIR;
        struct passwd* pwst =
            getpwuid( getuid() );

        if( pwst == nullptr )
        {
            ret = -errno;
            if( ret == 0 )
                ret = -ENOENT;
            break;
        }

        std::string strLogin = pwst->pw_name;

        strLogin += "/";
        strPath += strLogin + strFileName + "-1";
        ret = access( strPath.c_str(), 0 );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        struct stat sBuf;
        ret = stat( strPath.c_str(), &sBuf );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        CParamList oInfo;
        oInfo[ SF_REQSIZE_IDX ] =
            ( guint32 )sBuf.st_size;
        oInfo[ SF_FILEMODE_IDX ] =
            ( guint32 )sBuf.st_mode;
        pInfo = oInfo.GetCfg();

    }while( 0 );

    return ret;
}

template<>
gint32 rpcfrmwrk::GetIidOfType< const CMyFileServer >(
    std::vector< guint32 >& vecIids, const CMyFileServer* pType )
{
    vecIids.push_back( pType->CMyFileServer::super::GetIid() );
    vecIids.push_back( pType->CMyFileServer::GetIid() );
    return 0;
}

