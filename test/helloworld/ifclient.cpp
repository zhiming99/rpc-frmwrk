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
 * the macro FORWARD_CALL, and return the error
 * code.
 *
 * while the Non-EX macro requires to call in two
 * steps, SyncCall to send the request and
 * FillArgs to assign the response to the output
 * parameters, which requires you have a better
 * understanding of the system.
 * 
 * 2. EX macro strictly requires the proxy
 * function to have both input and output
 * parameters in the formal parameter list, that
 * is, input parameters come first, and then the
 * output parameters follow.  both are in the same
 * order as the one on the server side.
 *
 * While the Non-EX macro's has no requirement on
 * the formal parameter list of the proxy method.
 *
 * 3. the macro's argument lists are different.
 * The EX macro requires the method and the number
 * of input parameters in the arg list. While the
 * Non-EX macro requires the types of all the
 * input parameters in the arg list of the macro.
 * 
 * for example of Non-EX, please refer to other
 * tests, such as prtest or katest under the
 * `test' directory.
 * @} */
gint32 CEchoClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD_EX( 1,
        CEchoClient::EchoPtr,
        METHOD_EchoPtr );

    ADD_USER_PROXY_METHOD_EX( 2,
        CEchoClient::EchoCfg,
        METHOD_EchoCfg );

    ADD_USER_PROXY_METHOD_EX( 1,
        CEchoClient::EchoUnknown,
        METHOD_EchoUnknown );

    ADD_USER_PROXY_METHOD_EX( 1,
        CEchoClient::Echo,
        METHOD_Echo );

    ADD_USER_PROXY_METHOD_EX( 0,
        CEchoClient::Ping,
        METHOD_Ping );

    END_PROXY_MAP;

    return 0;
}


gint32 CEchoClient::Echo(
    const std::string& strText,
    std::string& strReply )
{
    // the first parameter is the number of
    // input parameters
    //
    // METHOD_Echo is the method name to call strText
    // is the input parameter, and strReply is the output
    // parameter.
    // input comes first, and the output follows
    return FORWARD_CALL( 1, METHOD_Echo,
        strText, strReply );
}

gint32 CEchoClient::EchoPtr(
    const char* szText,
    const char*& szReply )
{
    return FORWARD_CALL( 1, METHOD_EchoPtr,
        szText, szReply );
}

gint32 CEchoClient::EchoCfg(
    gint32 iCount,
    const CfgPtr& pCfg,
    gint32& iCountReply,
    CfgPtr& pCfgReply )
{
    return FORWARD_CALL( 2, METHOD_EchoCfg,
        iCount, pCfg, iCountReply, pCfgReply );
}

gint32 CEchoClient::EchoUnknown(
    const BufPtr& pText,
    BufPtr& pReply )
{
    return FORWARD_CALL( 1, METHOD_EchoUnknown,
        pText, pReply );
}

gint32 CEchoClient::Ping()
{
    return FORWARD_CALL( 0,  METHOD_Ping );
}

