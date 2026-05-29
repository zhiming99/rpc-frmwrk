/*
 * =====================================================================================
 *
 *       Filename:  stmain.cpp
 *
 *    Description:  main entry for ST compiler
 *
 *        Version:  1.0
 *        Created:  05/03/2026 09:04:52 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include <rpc.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "lvalvar.h"
#include "stclsids.h"
#include "parsrctx.h"
#include "stlexer.h"
#include "stparser.h"

using namespace rpcf;
std::shared_ptr< CSTParserContext > g_pParserCtx;

gint32 StartParse(
    CSTParserContext* pCtx,
    const stdstr& strFile );

static gint32 IsValidDir( const char* szDir )
{
    gint32 ret = 0;
    do{
        struct stat sb;
        if (lstat( szDir, &sb) == -1)
        {
            ret = -errno;
            break;
        }
        mode_t iFlags =
            ( sb.st_mode & S_IFMT );
        if( iFlags != S_IFDIR )
        {
            ret = -ENOTDIR;
            break;
        }
    }while( 0 );
    return ret;
}

// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRY( CLValueVariableInstPath );
    INIT_MAP_ENTRY( CLValueVariableDataMember  );
    INIT_MAP_ENTRY( CLValueVariableDefPtr  );
    INIT_MAP_ENTRY( CLValueVariableArrayAccess );

    END_FACTORY_MAPS;
};

void Usage()
{
    printf( "Usage:" );
    printf( "rpcfstc [options] <st file> \n" );

    printf( "\t compile the `ST file'"
        "and output the RPC skeleton files.\n" );

    printf( "Options -h:\tprint this help.\n");

    printf( "\t-I:\tSpecify the path to"
        " search for the included `ST files'.\n"
        "\t\tAnd this option can repeat multiple "
        "times.\n" );
}

int main( int argc, char* argv[] )
{
    gint32 ret = 0;
    bool bUninit = false;
    do{
        stdstr strFile;
        ret = CoInitialize( COINIT_NORPC );
        if( ERROR( ret ) )
            break;
        bUninit = true;

        g_pParserCtx.reset( new CSTParserContext );
        auto pCtx = g_pParserCtx.get();

        FactoryPtr pFactory = InitClassFactory();
        ret = CoAddClassFactory( pFactory );
        if( ERROR( ret ) )
            break;

        int opt = 0;
        bool bQuit = false;

        int option_index = 0;
        static struct option long_options[] = {
            {"version", no_argument, 0,  0 },
            {0, 0,  0,  0 }
        };

        stdstr strMsg;
        while( true ) 
        {

            opt = getopt_long( argc, argv,
                "hI:",
                long_options, &option_index );

            switch( opt )
            {
            case 0:
                {
                    if( option_index == 0 )
                    {
                        printf( "%s", Version() );
                        bQuit = true;
                    }
                    break;
                }
            case 'I' :
                {
                    ret = IsValidDir( optarg );
                    if( ret == -ENOTDIR )
                    {
                        strMsg = "Error '";

                        strMsg += optarg;
                        strMsg +=
                           "' is not a directory";
                        bQuit = true;
                        break;
                    }
                    else if( ERROR( ret ) )
                    {
                        printf( "%s : %s\n", optarg,
                            strerror( -ret ) );
                        bQuit = true;
                        break;
                    }

                    char szBuf[ 512 ];
                    int iSize = strnlen(
                        optarg, sizeof( szBuf ) + 1 );
                    if( iSize > sizeof( szBuf ) )
                    {
                        strMsg =
                           "Error path is too long";
                        ret = -ERANGE;
                        bQuit = true;
                        break;
                    }
                    stdstr strFullPath;
                    if( optarg[ 0 ] == '/' )
                    {
                        strFullPath = optarg;
                    }
                    else
                    {
                        char* szPath = getcwd(
                            szBuf, sizeof( szBuf ) );
                        if( szPath == nullptr )
                        {
                            strMsg =
                               "Error path is too long";
                            ret = -errno;
                            bQuit = true;
                            break;
                        }
                        strFullPath = szPath;
                        strFullPath += "/";
                        strFullPath += optarg;
                        if( strFullPath.size() >
                            sizeof( szBuf ) )
                        {
                            strMsg =
                               "Error path is too long";
                            ret = -ERANGE;
                            bQuit = true;
                            break;
                        }
                    }
                    pCtx->m_vecInclPaths.push_back(
                        strFullPath );
                    break;
                }
            case -1:
            default:
                break;
            }

            if( argv[ optind ] == nullptr )
            {
                printf( "Missing file to compile\n" );
                Usage();
                ret = -ENOENT;
                break;
            }

            strFile = argv[ optind ];
            if( strFile.size() > REG_MAX_PATH )
            {
                printf( "File name too long\n" );
                ret = -ENAMETOOLONG;
                break;
            }
            break;
        }
        if( ERROR( ret ) )
            break;

        ret = StartParse( pCtx, strFile );

    }while( 0 );
    if( bUninit )
        CoUninitialize();

    return ret;
}

