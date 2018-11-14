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

    ADD_USER_PROXY_METHOD_EX( 1,
        CPauseResumeClient::LongWait,
        METHOD_LongWait );

    ADD_USER_PROXY_METHOD_EX( 1,
        CPauseResumeClient::EchoUnknown,
        METHOD_EchoUnknown );

    ADD_USER_PROXY_METHOD_EX( 1,
        CPauseResumeClient::Echo,
        METHOD_Echo );

    ADD_USER_PROXY_METHOD_EX( 6,
        CPauseResumeClient::EchoMany,
        METHOD_EchoMany );

    END_PROXY_MAP;

    return 0;
}

gint32 CPauseResumeClient::Echo(
    const std::string& strText, // [ in ]
    std::string& strReply ) // [ out ]
{
    return FORWARD_CALL( 1, METHOD_Echo,
        strText, strReply  );
}

gint32 CPauseResumeClient::EchoUnknown(
    const BufPtr& pText,
    BufPtr& pReply )
{
    return FORWARD_CALL( 1, METHOD_EchoUnknown,
        pText, pReply  );
}

gint32 CPauseResumeClient::EchoMany(
    gint32 i1, gint16 i2, gint64 i3, float i4, double i5, const std::string& strText,
    gint32& o1, gint16& o2, gint64& o3, float& o4, double& o5, std::string& strReply )
{
    gint32 ret = FORWARD_CALL( 6,  METHOD_EchoMany,
        i1, i2, i3, i4, i5, strText, o1, o2, o3, o4, o5, strReply );
    return ret;
}

gint32 CPauseResumeClient::LongWait(
    const std::string& strText,
    std::string& strReply )
{

    return FORWARD_CALL( 1, METHOD_LongWait,
        strText, strReply );
}

