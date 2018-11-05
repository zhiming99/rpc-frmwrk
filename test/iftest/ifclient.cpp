/*
 * =====================================================================================
 *
 *       Filename:  ifclient.cpp
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
#include "ifsvr.h"
#include "ifhelper.h"

using namespace std;

CEchoClient::CEchoClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CEchoClient ) );
}

/**
* @name InitUserFuncs
*    to init the service handler map for this interface
*    object. It is mandatory to call
*    super::InitUserFuncs(), otherwise some of the
*    interfaces, such as the admin interface with 
*    pause/resume/cancel cannot work.
* @{ */
/** 
 * 
 * You can use two sets of macro's to to init the
 * function map, one include ADD_USER_PROXY_METHOD
 * and ADD_PROXY_METHOD, and the other includes
 * ADD_USER_PROXY_METHOD_EX and
 * ADD_PROXY_METHOD_EX. Usually it is recommended to
 * use the EX set of macro's
 * 
 * The differences between the two are as following:
 *
 * 1. EX macro requires the proxy function to call
 * ProxyCall and return the error code.
 *
 * while the Non-EX macro requires to call
 * two steps, SyncCall and FillArgs, which
 * requires you have a better understanding of the
 * system
 * 
 * 2. EX macro requires to have both input and
 * output parameters in the formal parameter list,
 * that is, input parameters come first, and then
 * the output parameters follow. both are in the
 * same order as the one on the client side.
 *
 * While the Non-EX macro's require only the input
 * parameters besices the pCallback.
 *
 * 3. the macro's argument lists are different.
 * The EX macro requires the method and the number
 * of input parameters in the arg list. While the
 * Non-EX macro requires the types of all the
 * input parameters in the arg list of the macro.
 * 
 * @} */
gint32 CEchoClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_EchoPtr, const char* );

    ADD_USER_PROXY_METHOD(
        METHOD_EchoCfg, gint32, const CfgPtr& );

    ADD_USER_PROXY_METHOD(
        "Unknown", const BufPtr& );

    ADD_USER_PROXY_METHOD_EX( 1,
        CEchoClient::Echo,
        METHOD_Echo );

    ADD_USER_PROXY_METHOD_EX( 0,
        CEchoClient::Ping,
        METHOD_Ping );


    END_PROXY_MAP;

    return 0;
}


gint32 CEchoClient::EchoPtr(
    const char* szText,
    const char*& szReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_EchoPtr, szText );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, szReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CEchoClient::EchoCfg(
    gint32 iCount,
    const CfgPtr& pCfg,
    gint32& iCountReply,
    CfgPtr& pCfgReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_EchoCfg, iCount, pCfg );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, iCountReply, pCfgReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CEchoClient::EchoUnknown(
    const BufPtr& pText,
    BufPtr& pReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, "Unknown", pText );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, pReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CEchoClient::Echo(
    const std::string& strText,
    std::string& strReply )
{

    return ProxyCall< 1, const std::string&, std::string&  >
        ( METHOD_Echo, strText, strReply );
}

gint32 CEchoClient::Ping()
{
    return ProxyCall< 0 >( METHOD_Ping );
}
