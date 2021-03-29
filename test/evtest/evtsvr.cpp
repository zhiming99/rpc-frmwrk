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
#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cppunit/TestFixture.h>
#include "ifhelper.h"

using namespace rpcf;
#include "evtsvr.h"

using namespace std;

CEventServer::CEventServer( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CEventServer ) );
}

gint32 CEventServer::InitUserFuncs()
{
    gint32 ret = super::InitUserFuncs();

    if( ERROR( ret ) )
        return ret;

    return 0;
}

gint32 CEventServer::OnHelloWorld(
    const std::string& strEvent )
{
    BROADCAST_USER_EVENT( GetClsid(), strEvent );
    return 0;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CEventServer );
    INIT_MAP_ENTRYCFG( CEventClient );

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
