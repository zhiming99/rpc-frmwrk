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

    ADD_USER_PROXY_METHOD(
        METHOD_EchoMany, 
        gint32, gint16, gint64, float, double, const std::string& );

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

gint32 CPauseResumeClient::EchoMany(
    gint32 i1, gint16 i2, gint64 i3, float i4, double i5, const std::string& strText,
    gint32& o1, gint16& o2, gint64& o3, float& o4, double& o5, std::string& strReply )
{
    gint32 ret = ProxyCall< 6, // number of input
        gint32&, gint16&, gint64&, float&, double&, const std::string&, // [ in ]
        gint32&, gint16&, gint64&, float&, double&, std::string& >      // [ out ]
        ( METHOD_EchoMany, i1 , i2, i3, i4, i5, strText, o1, o2, o3, o4, o5, strReply );

    return ret;
}

gint32 CPauseResumeClient::LongWait(
    const std::string& strText,
    std::string& strReply )
{

    return ProxyCall<1, const std::string&, std::string&  >(
        METHOD_LongWait, strText, strReply  );
}

