/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  main function for ridl compiler
 *
 *        Version:  1.0
 *        Created:  03/12/2021 06:35:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rpc.h"

using namespace rpcf;
#include "astnode.h" 
#include "ridlc.h"

extern CDeclMap g_mapDecls;
extern ObjPtr g_pRootNode;
extern bool g_bSemanErr;
extern std::string g_strAppName;

// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRY( CBuffer );
    INIT_MAP_ENTRY( CAttrExp );
    INIT_MAP_ENTRY( CAttrExps );
    INIT_MAP_ENTRY( CPrimeType );
    INIT_MAP_ENTRY( CArrayType );
    INIT_MAP_ENTRY( CMapType );
    INIT_MAP_ENTRY( CStructRef );
    INIT_MAP_ENTRY( CFieldDecl );
    INIT_MAP_ENTRY( CFieldList );
    INIT_MAP_ENTRY( CStructDecl );
    INIT_MAP_ENTRY( CFormalArg );
    INIT_MAP_ENTRY( CArgList );
    INIT_MAP_ENTRY( CMethodDecl );
    INIT_MAP_ENTRY( CMethodDecls );
    INIT_MAP_ENTRY( CInterfaceDecl );
    INIT_MAP_ENTRY( CInterfRef );
    INIT_MAP_ENTRY( CInterfRefs );
    INIT_MAP_ENTRY( CServiceDecl );
    INIT_MAP_ENTRY( CStatements );
    INIT_MAP_ENTRY( CAliasList );
    INIT_MAP_ENTRY( CTypedefDecl );
    INIT_MAP_ENTRY( CAppName );
    INIT_MAP_ENTRY( CConstDecl );

    END_FACTORY_MAPS;
};

void Usage()
{
    printf( "Usage:" );
    printf( "ridlc [options] <ridl file> \n" );

    printf( "\t compile the `ridl file'"
        "and output the RPC skelton files.\n" );

    printf( "Options -h:\tTo print this help\n");

    printf( "\t-I:\tTo specify the path to"
        " search for the included files.\n"
        "\t\tAnd this option can repeat many"
        "times\n" );

    printf( "\t-O:\tTo specify the path for\n"
        "\t\tthe output files. 'output' is the \n"
        "\t\tdefault path if not specified\n" );

    printf( "\t-o:\tTo specify the file name as\n"
        "\t\tthe base of the target image. That is,\n"
        "\t\tthe <name>cli for client and <name>svr\n"
        "\t\tfor server. If not specified, the\n"
        "\t\t'appname' from the ridl will be used\n" );

    printf( "\t-p:\tTo generate the Python skelton files\n" );

    printf( "\t-l:\tTo output a shared library\n" );
    printf( "\t\tinstead of executables. This\n" );
    printf( "\t\toption is for CPP project only\n" );
}

static std::string g_strOutPath = "output";
static std::vector< std::string > g_vecPaths;
std::string g_strTarget;
bool g_bNewSerial = true;
static stdstr g_strLang = "cpp";
bool g_bMklib = false;

#include "seribase.h"
#include "gencpp.h"
#include "genpy.h"

int main( int argc, char** argv )
{
    gint32 ret = 0;
    bool bUninit = false;
    do{
        ret = CoInitialize( COINIT_NORPC );
        if( ERROR( ret ) )
            break;
        bUninit = true;

        FactoryPtr pFactory = InitClassFactory();
        ret = CoAddClassFactory( pFactory );
        if( ERROR( ret ) )
            break;

        int opt = 0;
        bool bQuit = false;

        while( ( opt =
            getopt( argc, argv, "ahlI:O:o:p" ) ) != -1 )
        {
            switch( opt )
            {
            case 'h' :
                {
                    Usage();
                    bQuit = true;
                    break;
                }
            case 'O':
            case 'I' :
                {
                    struct stat sb;
                    if (lstat( optarg, &sb) == -1)
                    {
                        ret = -errno;
                        printf( "%s : %s\n", optarg,
                            strerror( errno ) );
                        bQuit = true;
                        break;
                    }
                    mode_t iFlags =
                        ( sb.st_mode & S_IFMT );
                    if( iFlags != S_IFDIR )
                    {
                        std::string strMsg =
                            "warning '";

                        strMsg += optarg;
                        strMsg +=
                           "' is not a directory";
                        printf( "%s\n",
                            strMsg.c_str() );
                        break;
                    }
                    if( opt == 'I' )
                    {
                        g_vecPaths.push_back(
                            std::string( optarg ) );
                    }
                    else
                    {
                        g_strOutPath = optarg;
                    }
                    break;
                }
            case 'o':
                {
                    g_strTarget = optarg;
                    break;
                }
            case 'p':
                {
                    g_strLang = "py";
                    break;
                }
            case 'l':
                {
                    g_bMklib = true;
                    break;
                }
            default:
                break;
            }

            if( bQuit )
                break;
        }

        if( bQuit )
            break;

        if( argv[ optind ] == nullptr )
        {
            printf( "Missing file to compile\n" );
            Usage();
            break;
        }

        std::string strFile = argv[ optind ];

        if( argv[ optind + 1 ] != nullptr )
        {
            printf( "too many arguments\n" );
            Usage();
            break;
        }

        try {
            ret = yyparse( strFile.c_str() );
        }
        catch (std::exception& e)
        {
            printf( "%s\n", e.what() );
        }

        if( g_pRootNode.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        if( g_bSemanErr )
        {
            ret = ERROR_FAIL;
            break;
        }

        std::string strAppName;
        CStatements* pRoot = g_pRootNode;
        if( pRoot != nullptr )
        {
            ret = pRoot->CheckAppName();
            if( ERROR( ret ) )
            {
                printf( "error app name not"
                     " declared \n" );
                break;
            }
            else
            {
                strAppName = pRoot->GetName();
                printf(
                    "info using app name '%s' \n",
                     strAppName.c_str() );
            }
            std::vector< ObjPtr > vecSvcs;
            ret = pRoot->GetSvcDecls( vecSvcs );
            if( ERROR( ret ) )
            {
                printf( "error no service"
                     " declared \n" );
                break;
            }
            if( vecSvcs.size() > 1 )
            {
                printf(
                "warning more than one services declared, only\n"
                "the first service will be used to generate\n"
                "the `main' function\n" );
            }
        }

        printf( "Successfully parsed %s\n",
            strFile.c_str() );

        if( g_strTarget.empty() )
            g_strTarget = g_strAppName;

        printf( "Generating files.. \n" );
        if( g_strLang == "cpp" )
        {
            ret = GenCppProj(
                g_strOutPath, strAppName, pRoot );
        }
        else if( g_strLang == "py" )
        {
            ret = GenPyProj(
                g_strOutPath, strAppName, pRoot );
        }
        else
        {
            Usage();
            ret = -ENOTSUP;
        }


    }while( 0 );

    g_mapDecls.Clear();
    g_pRootNode.Clear();

    if( bUninit )
        CoUninitialize();

    return ret;
}

gint32 CheckNameDup(
    ObjPtr& pInArgs,
    ObjPtr& pOutArgs,
    std::string& strDupName )
{
    CArgList* pIn = pInArgs;
    CArgList* pOut = pOutArgs;
    if( pIn == nullptr || pOut == nullptr )
        return 0;

    gint32 ret = ERROR_FALSE;
    do{
        guint32 dwSize = pIn->GetCount();
        std::set< std::string > setNames;
        for( guint32 i = 0; i < dwSize; i++ )
        {
            ObjPtr pObj = pIn->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            std::string strName = pfa->GetName();
            setNames.insert( strName );
        }
        dwSize = pOut->GetCount();
        for( guint32 i = 0; i < dwSize; i++ )
        {
            ObjPtr pObj = pOut->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            std::string strName = pfa->GetName();
            if( setNames.find( strName ) !=
                setNames.end() )
            {
                strDupName = strName;
                ret = STATUS_SUCCESS;
                break;
            }
        }

    }while( 0 );

    return ret;
}

FILE* TryOpenFile( const std::string& strFile )
{
    FILE* fp = fopen( strFile.c_str(), "r");
    if ( fp != nullptr )
        return fp;

    for( auto elem : g_vecPaths )
    {
        std::string strPath =
            elem + "/" + strFile;
        fp = fopen( strPath.c_str(), "r" );
        if( fp != nullptr )
            break;
    }

    return fp;
}
