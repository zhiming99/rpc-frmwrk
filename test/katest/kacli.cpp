/*
 * =====================================================================================
 *
 *       Filename:  kacli.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2018 01:15:23 PM
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

using namespace rpcfrmwrk;
#include "kasvr.h"

using namespace std;

CKeepAliveClient::CKeepAliveClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CKeepAliveClient ) );
    Sem_Init( &m_semWait, 0, 0 );
}

gint32 CKeepAliveClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_LongWait,
        const std::string& );

    END_PROXY_MAP;

    return 0;
}

gint32 CKeepAliveClient::LongWait(
    const std::string& strText,
    std::string& strReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_LongWait, strText );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, strReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

