/*
 * =====================================================================================
 *
 *       Filename:  appmancli.cpp
 *
 *    Description:  a builtin CAppManager_CliImpl for self monitoring usage
 *
 *        Version:  1.0
 *        Created:  06/10/2025 05:38:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include <limits.h>
#include <signal.h>
#include "../client/cpp/appmancli/AppManagercli.h"

extern void SignalHandler( int signum );

extern std::vector< InterfPtr > g_vecIfs;
extern stdstr g_strCmd;
static std::vector< InterfPtr > s_vecIfs;

struct CAMonSvrCallbacks : public CAsyncStdAMCallbacks
{
    gint32 GetPointValuesToUpdate(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override
    {
        gint32 ret = 0;
        UNREFERENCED( pIf );
        do{
            CFastRpcServerBase* pSvr = s_vecIfs[ 0 ];
            CFastRpcServerBase* pSvr1 = s_vecIfs[ 1 ];
            CFastRpcServerBase* pSvr2 = s_vecIfs[ 2 ];

            KeyValue okv;
            okv.strKey = O_CONNECTIONS;
            okv.oValue = ( guint32 )
                pSvr->GetStmSkelCount() +
                pSvr1->GetStmSkelCount() +
                pSvr2->GetStmSkelCount();
            veckv.push_back( okv );

            okv.strKey = O_VMSIZE_KB;
            okv.oValue = GetVmSize();
            veckv.push_back( okv );

            okv.strKey = O_OBJ_COUNT;
            okv.oValue = CObjBase::GetActCount();
            veckv.push_back( okv );

            okv.strKey = O_PENDINGS;
            guint32 dwVal = 0;
            Variant oVar;
            for( auto elem : s_vecIfs )
            {
                CFastRpcServerBase* pIf = elem;
                ret = pIf->GetProperty(
                    propPendingTasks, oVar );
                if( SUCCEEDED( ret ) )
                    dwVal += ( guint32& )oVar;
            }
            okv.oValue = dwVal;
            veckv.push_back( okv );

            okv.strKey = O_RXBYTE;
            guint64 qwVal = 0;
            for( auto elem : s_vecIfs )
            {
                CFastRpcServerBase* pIf = elem;
                ret = pIf->GetProperty(
                    propRxBytes, oVar );
                if( SUCCEEDED( ret ) )
                    qwVal += ( guint64& )oVar;
            }
            okv.oValue = qwVal;
            veckv.push_back( okv );

            okv.strKey = O_TXBYTE;
            qwVal = 0;
            for( auto elem : s_vecIfs )
            {
                CFastRpcServerBase* pIf = elem;
                ret = pIf->GetProperty(
                    propTxBytes, oVar );
                if( SUCCEEDED( ret ) )
                    qwVal += ( guint64& )oVar;
            }
            okv.oValue = qwVal;
            veckv.push_back( okv );

            ret = pSvr->GetProperty(
                propUptime, okv.oValue );
            if( SUCCEEDED( ret ) )
                veckv.push_back( okv );

            okv.strKey = O_FAIL_COUNT;
            dwVal = 0;
            for( auto elem : s_vecIfs )
            {
                CFastRpcServerBase* pIf = elem;
                ret = pIf->GetProperty(
                    propFailureCount, oVar );
                if( SUCCEEDED( ret ) )
                    dwVal += ( guint32& )oVar;
            }
            okv.oValue = dwVal;
            veckv.push_back( okv );

            okv.strKey = O_REQ_COUNT;
            dwVal = 0;
            for( auto elem : s_vecIfs )
            {
                CFastRpcServerBase* pIf = elem;
                ret = pIf->GetProperty(
                    propMsgCount, oVar );
                if( SUCCEEDED( ret ) )
                    dwVal += ( guint32& )oVar;
            }
            okv.oValue = dwVal;
            veckv.push_back( okv );

            okv.strKey = O_RESP_COUNT;
            dwVal = 0;
            for( auto elem : s_vecIfs )
            {
                CFastRpcServerBase* pIf = elem;
                ret = pIf->GetProperty(
                    propMsgRespCount, oVar );
                if( SUCCEEDED( ret ) )
                    dwVal += ( guint32& )oVar;
            }
            okv.oValue = dwVal;
            veckv.push_back( okv );

            okv.strKey = O_CPU_LOAD;
            okv.oValue = GetCpuUsage();
            veckv.push_back( okv );

            okv.strKey = O_OPEN_FILES;
            guint32 dwCount = 0;
            ret = GetOpenFileCount(
                getpid(), dwCount );
            if( SUCCEEDED( ret ) )
            {
                okv.oValue = dwCount;
                veckv.push_back( okv );
            }

            okv.strKey = O_PID;
            okv.oValue = ( guint32 )getpid();
            veckv.push_back( okv );

            okv.strKey = S_MAX_QPS;
            ret = pSvr->GetProperty(
                propQps, okv.oValue );
            if( SUCCEEDED( ret ) )
                veckv.push_back( okv );

            okv.strKey = S_MAX_STM_PER_SESS;
            ret = pSvr->GetProperty(
                propStmPerSess, okv.oValue );
            if( SUCCEEDED( ret ) )
                veckv.push_back( okv );

            okv.strKey = O_WORKING_DIR;
            char szPath[PATH_MAX];
            if( getcwd( szPath, sizeof(szPath)) != nullptr)
            {        
                BufPtr pBuf( true );
                ret = pBuf->Append( szPath,
                    strlen( szPath ) );
                if( SUCCEEDED( ret ) )
                {
                    okv.oValue = pBuf;
                    veckv.push_back( okv );
                }
            }
            ret = 0;
        }while( 0 );
        return ret;
    }

    gint32 GetPointValuesToInit(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override
    {
        gint32 ret = 0;
        KeyValue okv;
        okv.strKey = O_WORKING_DIR;
        char szPath[PATH_MAX];
        if( getcwd( szPath, sizeof(szPath)) != nullptr)
        {        
            BufPtr pBuf( true );
            ret = pBuf->Append( szPath,
                strlen( szPath ) );
            if( SUCCEEDED( ret ) )
            {
                okv.oValue = pBuf;
                veckv.push_back( okv );
            }
        }
        do{
            okv.strKey = S_CMDLINE;
            BufPtr pBuf( true );
            ret = pBuf->Append( g_strCmd.c_str(),
                g_strCmd.size() );
            if( ERROR( ret ) )
                break;
            okv.oValue = pBuf;
            veckv.push_back( okv );

            okv.strKey = O_PID;
            okv.oValue = ( guint32 )getpid();
            veckv.push_back( okv );

            okv.strKey = O_WORKING_DIR;
            char szPath[PATH_MAX];
            if( getcwd( szPath, sizeof(szPath)) != nullptr)
            {        
                BufPtr pBuf( true );
                ret = pBuf->Append( szPath,
                    strlen( szPath ) );
                if( SUCCEEDED( ret ) )
                {
                    okv.oValue = pBuf;
                    veckv.push_back( okv );
                }
            }

            okv.strKey = O_UPTIME;
            CRpcServices* pSvc = pIf;
            timespec tv = pSvc->GetStartTime();
            okv.oValue = ( guint32 )tv.tv_sec;
            veckv.push_back( okv );

        }while( 0 );
        return 0;
    }
};

gint32 StartLocalAppMancli()
{
    s_vecIfs = g_vecIfs;
    CRpcServices* pIf = s_vecIfs[ 0 ];
    CIoManager* pMgr = pIf->GetIoMgr();
    gint32 ret = pMgr->TryLoadClassFactory(
        "invalidpath/libappmancli.so" );
    if( ERROR( ret ) )
        return ret;
    signal( SIGUSR1, SignalHandler );
    PACBS pacbsIn( new CAMonSvrCallbacks );
    InterfPtr pAppMan;
    return StartStdAppManCli( s_vecIfs[0],
        "appmonsvr1", pAppMan, pacbsIn );
}

gint32 StopLocalAppMancli()
{
    gint32 ret = StopStdAppManCli();
    s_vecIfs.clear();
    return ret;
}
