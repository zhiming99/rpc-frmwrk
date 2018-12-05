/*
 * =====================================================================================
 *
 *       Filename:  stmtest.h
 *
 *    Description:  declaration of test classes
 *
 *        Version:  1.0
 *        Created:  12/04/2018 10:37:00 AM
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
// this is for driver.json, and iomanager
#define MODULE_NAME "stmsvrtst"
#endif

#ifdef CLIENT
#define MODULE_NAME "stmclitst"
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
#endif
    CPPUNIT_TEST_SUITE_END();

    public:
    void setUp();
    void tearDown();
    void testSvrStartStop();
    void testCliStartStop();
};

