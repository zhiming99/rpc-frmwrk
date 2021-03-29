/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  for the smoke test purpose
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
#include <string>
#include <iostream>
#include <unistd.h>
#include <cppunit/TestFixture.h>

#include <rpc.h>

using namespace rpcf;

#include "ifsvr.h"
#include "ifhelper.h"

using namespace std;

CEchoServer::CEchoServer( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CEchoServer ) );
}
/**
* @name InitUserFuncs
*    to init the service handler map for this interface
*    object. It is mandatory to call
*    super::InitUserFuncs(), otherwise some of the
*    functions, such as the admin interface for
*    pause/resume/cancel cannot work.
* @{ */
/** 
 * 
 * You can use two sets of macro's to to init the
 * function map, one include ADD_USER_SERVICE_HANDLER
 * and ADD_SERVICE_HANDLER, and the other includes
 * ADD_USER_SERVICE_HANDLER_EX and
 * ADD_SERVICE_HANDLER_EX. Usually it is recommended to
 * use the EX set of macro's
 * 
 * The differences between the two are as following:
 *
 * 1. EX macro does not require to call
 * FillAndSetResponse when the method is done. Just
 * return the proper error code.
 *
 * while the Non-EX macro requires to call
 * FillAndSetResponse before return, either error or
 * success. It is not necessary to call this method if
 * STATUS_PENDING is returned.
 * 
 * 2. EX macro requires to have both input and output
 * parameters in the formal parameter list of the
 * handler function, that is, input parameters come
 * first, and then the output parameters follow. both
 * are in the same order as the one on the client side.
 *
 * While the Non-EX macro's require only the input
 * parameters following the mandatory parameter,
 * pCallback.
 * 
 * @} */
gint32 CEchoServer::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_HANDLER_MAP;

    ADD_USER_SERVICE_HANDLER_EX( 1,
        CEchoServer::Echo,
        METHOD_Echo );

    // use this macro if you want to pack the response
    // in a different way from the standard one.
    ADD_USER_SERVICE_HANDLER(
        CEchoServer::EchoPtr,
        METHOD_EchoPtr );

    ADD_USER_SERVICE_HANDLER(
        CEchoServer::EchoUnknown,
        METHOD_EchoUnknown );

    ADD_USER_SERVICE_HANDLER_EX( 2,
        CEchoServer::EchoCfg,
        METHOD_EchoCfg );

    ADD_USER_SERVICE_HANDLER_EX( 0,
        CEchoServer::Ping,
        METHOD_Ping );

    ADD_USER_SERVICE_HANDLER_EX( 2,
        CEchoServer::Echo2,
        "Echo2" );

    END_HANDLER_MAP;
    return 0;
}

/**
* @name Echo: to send back the string in strText
*
* @{ */
/** 
 *   Parameters:
 *
 *   pCallback: the running context. a mandatory
 *   parameter. if you don't need it, leave it alone.
 *   it would be useful when the method wants to do
 *   some asynchronous things.
 *
 *   [ in ]strText: the string received from the client
 *   side.
 *
 *   [ out ]strReply: a string same as strText is sent
 *   back to the proxy
 *
 *   Return Values:
 *
 *   STATUS_SUCCESS or 0: the output parameters will be
 *   packaged and sent back to the proxy
 * 
 *   ERROR( ret ) : only the return code will be sent
 *   back to the proxy.
 *
 *   STATUS_PENDING: on return, nothing will be sent
 *   back to the proxy. you need to call the
 *   FillAndSetResponse and pCallback's OnEvent(
 *   eventTaskComp ) sometime later once the method
 *   have done. It implies you are doing the work
 *   asynchronously.
 *
 * @} */

gint32 CEchoServer::Echo(
    IEventSink* pCallback,
    const std::string& strText,
    std::string& strReply )
{
    // business logics goes here
    strReply = strText;

    // inform to send back the respons
    return STATUS_SUCCESS;
}

gint32 CEchoServer::EchoPtr(
    IEventSink* pCallback,
    const char* szText )
{
    gint32 ret = 0;

    // business logics goes here

    // Non-EX method, requires to explicitly make the
    // response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, szText );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CEchoServer::EchoUnknown(
    IEventSink* pCallback,
    const BufPtr& pBuf )
{

    gint32 ret = 0;

    // business logics goes here

    // Non-EX method, requires to explicitly make the
    // response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, pBuf );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CEchoServer::EchoCfg(
    IEventSink* pCallback,
    gint32 iCount,
    const CfgPtr& pCfg, gint32& iCountReply, 
    CfgPtr& pCfgReply )
{

    // business logics goes here
    iCountReply = iCount;
    pCfgReply = pCfg;

    return 0;
}

gint32 CEchoServer::Ping(
    IEventSink* pCallback )
{
    // the simplest thing is to do nothing.
    return 0;
}

gint32 CEchoServer::Echo2(
    IEventSink* pCallback,
    gint32 i1, double f2, double& i3 )
{
    i3 = i1 + f2;
    return 0;
}

// mandatory part, just copy/paste
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CEchoServer );
    INIT_MAP_ENTRYCFG( CEchoClient );

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
