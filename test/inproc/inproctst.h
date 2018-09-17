/*
 * =====================================================================================
 *
 *       Filename:  inproctst.h
 *
 *    Description:  this example is to demonstrate the technique to how the
 *    client and server communicate within a single process. Generally speaking,
 *    there is no difference from those communicate across the process boundary.
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
#include <thread>

#include <rpc.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#define MODULE_NAME "inproctst"

class CIfSmokeTest :
    public CppUnit::TestFixture
{ 
    ObjPtr m_pMgr;

    public: 

    CPPUNIT_TEST_SUITE( CIfSmokeTest );

    CPPUNIT_TEST( testCliStartStop );

    CPPUNIT_TEST_SUITE_END();

    public:
    void setUp();
    void tearDown();
    void testSvrStartStop(
        IEventSink* pSyncCallback );

    void testCliStartStop();
    gint32 startServer(
        std::thread*& pThread,
        IEventSink*);
};

