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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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

using namespace rpcfrmwrk;

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

