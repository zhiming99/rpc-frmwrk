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
#include "prsvr.h"

using namespace std;


CPauseResumeClient::CPauseResumeClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    // mandatory statement
    SetClassId( clsid( CPauseResumeClient ) );
}

gint32 CPauseResumeClient::InitUserFuncs()
{
    // mandatory statement
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_LongWait,
        const std::string& );

    ADD_USER_PROXY_METHOD(
        "Unknown", const BufPtr& );

    ADD_USER_PROXY_METHOD(
        METHOD_Echo, const stdstr& );

    END_PROXY_MAP;

    return 0;
}

gint32 CPauseResumeClient::Echo(
    const std::string& strText, // [ in ]
    std::string& strReply ) // [ out ]
{
    // 1: the number of input
    // the second template parameter is the type of input parameter
    // the third is the type of the output parameter
    //
    // METHOD_Echo is the method name to call
    // strText is the input parameter
    // strReply is the output parameter
    // input comes first, and the output follows
    //
    return ProxyCall<1, const std::string&, std::string&  >(
        METHOD_Echo, strText, strReply  );
}

gint32 CPauseResumeClient::EchoUnknown(
    const BufPtr& pText,
    BufPtr& pReply )
{
    return ProxyCall<1, const BufPtr&, BufPtr&  >(
        "Unknown", pText, pReply  );
}

gint32 CPauseResumeClient::LongWait(
    const std::string& strText,
    std::string& strReply )
{

    return ProxyCall<1, const std::string&, std::string&  >(
        METHOD_LongWait, strText, strReply  );
}

