/*
 * =====================================================================================
 *
 *       Filename:  routmain.h
 *
 *    Description:  the declarations for the main entry of the stand-alone rpc router
 *
 *        Version:  1.0
 *        Created:  12/21/2018 03:45:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#include <string>
#include <iostream>
#include <unistd.h>

#include <rpc.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#define MODULE_NAME MODNAME_RPCROUTER

class CIfRouterTest :
    public CppUnit::TestFixture
{ 
    ObjPtr m_pMgr;

    public: 

    CPPUNIT_TEST_SUITE( CIfRouterTest );
    CPPUNIT_TEST( testSvrStartStop );
    CPPUNIT_TEST_SUITE_END();

    public:
    void setUp();
    void tearDown();
    void testSvrStartStop();
    void testCliStartStop();
    CfgPtr InitRouterCfg( guint32 dwRole );
};

