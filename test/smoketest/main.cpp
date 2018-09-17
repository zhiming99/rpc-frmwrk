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
#include <cppunit/TestCase.h>

using namespace std;

class CSmokeTest :
    public CppUnit::TestCase
{ 
    public: 
      CSmokeTest( std::string name ) : CppUnit::TestCase( name )
      {}

    void runTest()
    {
        gint32 ret = 0;
        if( 1 )
        {
            ret = CoInitialize( 0 );
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );

            BufPtr pBuf( true );
            ObjPtr pMgr;
            CParamList oParams;

            ret = oParams.Push( string( "smoketest" ) );
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );

            ret = pMgr.NewObj(
                clsid( CIoManager ),
                oParams.GetCfg() );

            CPPUNIT_ASSERT( SUCCEEDED( ret ) );

            IService* pSvc = pMgr;
            if( pSvc != nullptr )
                ret = pSvc->Start();
            else
                ret = -EFAULT;

            CPPUNIT_ASSERT( SUCCEEDED( ret ) );

            ret = pSvc->Stop();
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );

            ret = CoUninitialize();
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        }

        CPPUNIT_ASSERT( CObjBase::GetActCount() == 0 );

        DebugPrint( ret, "object leaks: %ll",
            CObjBase::GetActCount() );
    }
};

int main( int argc, char** argv )
{
    gint32 ret = 0;
    CSmokeTest oTest( "Smoketest" );
    oTest.runTest();
    return ret;
}
