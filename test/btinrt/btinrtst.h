/*
 * =====================================================================================
 *
 *       Filename:  btinrtst.h
 *
 *    Description:  declaration of test classes
 *
 *        Version:  1.0
 *        Created:  07/15/2018 11:27:55 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
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

#ifdef SERVER
#define MODULE_NAME "btinrtsvr"
#endif

#ifdef CLIENT
#define MODULE_NAME "btinrtcli"
#endif

class CIfSmokeTest :
    public CppUnit::TestFixture
{ 
    ObjPtr m_pMgr;

    public: 

    CPPUNIT_TEST_SUITE( CIfSmokeTest );
#ifdef SERVER
    CPPUNIT_TEST( testSvrStartStop );
#endif
#ifdef CLIENT
    CPPUNIT_TEST( testCliStartStop );
    // CPPUNIT_TEST( testDBusLeak );
    // CPPUNIT_TEST( testObjAlloc );
#endif
    CPPUNIT_TEST_SUITE_END();

    public:
    void setUp();
    void tearDown();
    CfgPtr InitRouterCfg();
    gint32 StartRouter( InterfPtr& pRt );
#ifdef SERVER
    void testSvrStartStop();
#endif
#ifdef CLIENT
    void testCliStartStop();
    void testDBusLeak();
    void testObjAlloc();
#endif
};

