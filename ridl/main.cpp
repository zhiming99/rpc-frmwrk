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
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
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
#include <set>
#include <iostream>
#include <sstream>

extern CDeclMap g_mapDecls;
extern ObjPtr g_pRootNode;
extern bool g_bSemanErr;
extern std::string g_strAppName;
extern std::string g_strJsLibPath;
extern bool g_bAuth;
extern std::string g_strWebPath;
// service filter
std::set< stdstr > g_setServices;
extern std::vector<stdstr> g_vecMonApps;
extern bool g_bMonitoring;

// interface async mode to overrid
using SYNC_ELEM=std::pair< stdstr, guint32 >;
std::map< stdstr, SYNC_ELEM > g_mapIfSync;

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
        "and output the RPC skeleton files.\n" );

    printf( "Options -h:\tprint this help.\n");

    printf( "\t-I:\tSpecify the path to"
        " search for the included `ridl files'.\n"
        "\t\tAnd this option can repeat many"
        "times.\n" );

    printf( "\t-O:\tSpecify the path for\n"
        "\t\tthe output files. 'output' is the \n"
        "\t\tdefault path if not specified.\n" );

    printf( "\t-o:\tSpecify the file name as\n"
        "\t\tthe base of the target image. That is,\n"
        "\t\tthe <name>cli for client and <name>svr\n"
        "\t\tfor server. If not specified, the\n"
        "\t\t'appname' from the ridl will be used.\n" );

#ifdef PYTHON
    printf( "\t-p:\tGenerate the Python skeleton files.\n" );
#endif

#ifdef JAVA
    printf( "\t-j:\tGenerate the Java skeleton files\n" );
    printf( "\t-P:\tSpecify the Java package name prefix.\n" );
    printf( "\t\tThis option is for Java only.\n" );
#endif

#ifdef FUSE3
    printf( "\t-f:\tGenerate cpp skeleton files for rpcfs\n" );
    printf( "\t--async_proxy:\tGenerate the asynchronous proxy for rpcfs.\n" );
#endif

#ifdef JAVASCRIPT
    printf( "\t-J:\tGenerate the JavaScript client skeleton.\n" );
    printf( "\t--odesc_url=<url>: the <url> to specify path to get the object description \n" );
    printf( "\t\tfile for JS client. This is a mandatory option when '-J' is given\n" );
    printf( "\t--lib_path=<path>: the <path> to specify the alternative path to the JS support library\n" );
    printf( "\t--auth: Enable OAuth2 authentication for Javascript client\n" );
#endif

    printf( "\t-s:\tOutput the skeleton with fastrpc support.\n" );
    printf( "\t-m <app name>:\tGenerate code for monitoring support. <app name> is the app's directory in the app registry. '-s' option must be present.\n" );
    printf( "\t-b:\tOutput the skeleton with built-in router.\n" );
    printf( "\t-l:\tOutput a shared library instead of executables.\n" );
    printf( "\t--server:\tGenerate skeleton code for server only.\n" );
    printf( "\t--client:\tGenerate skeleton code for client only.\n" );
    printf( "\t--services <service list>:\tGenerate skeleton code for the specified services.The services are seperated with ','.\n" );
    printf( "\t--sync_mode <interface name>[.<method name>]=<async|async_p|async_s|sync>:\tOverride sync-tag of the methods "
    "from the specified interface defined in the ridl file with <interface name>.\n" );
    printf( "\t\tThis option is for CPP project only.\n" );
    printf( "\t-L<lang>:Output Readme in language <lang>.\n" );
    printf( "\t\t<lang> Can be 'cn' or 'en' for now.\n" );
    printf( "\t-v:\tPrint the version information.\n" );
}

static std::string g_strOutPath = "output";
static std::vector< std::string > g_vecPaths;
std::string g_strTarget;
bool g_bNewSerial = true;
stdstr g_strLang = "cpp";
stdstr g_strLocale ="en";
stdstr g_strCmdLine;
bool g_bBuiltinRt = false;

// the prefix for java package name
stdstr g_strPrefix = "org.rpcf.";
bool g_bMklib = false;
bool g_bRpcOverStm = false;
bool g_bAsyncProxy = false;

#include "seribase.h"
#include "gencpp.h"
#include "genpy.h"
#include "genjava.h"
#include "getopt.h"
#include "genjs.h"
guint32 g_dwFlags = GEN_BOTH;

extern gint32 GenRpcFSkelton(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

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

int main( int argc, char** argv )
{
    gint32 ret = 0;
    bool bUninit = false;
    do{
        for( guint32 i = 0; i < argc; i++ )
        {
            g_strCmdLine += argv[ i ];
            g_strCmdLine.append( 1, ' ' );
        }

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

        int option_index = 0;
        static struct option long_options[] = {
            {"async_proxy", no_argument, 0,  0 },
            {"odesc_url", required_argument, 0,  0 },
            {"lib_path", required_argument, 0,  0 },
            {"auth", no_argument, 0,  0 },
            {"server", no_argument, 0,  0 },
            {"client", no_argument, 0,  0 },
            {"services", required_argument, 0,  0 },
            {"sync_mode", required_argument, 0,  0 },
            {0, 0,  0,  0 }
        };

        while( true ) 
        {

            opt = getopt_long( argc, argv,
                "abhvlm:I:O:o:pJjP:L:f::s",
                long_options, &option_index );

            if( opt == -1 )
                break;

            switch( opt )
            {
            case 0:
                {
                    if( option_index == 0 )
                        g_bAsyncProxy = true;
#ifdef JAVASCRIPT
                    else if( option_index == 1 )
                    {
                        g_strWebPath = optarg;
                    }
                    else if( option_index == 2 )
                    {
                        ret = IsValidDir( optarg );
                        if( ERROR( ret ) )
                        {
                            printf( "%s : %s\n", optarg,
                                strerror( -ret ) );
                            bQuit = true;
                            break;
                        }
                        g_strJsLibPath = optarg;
                    }
                    else if( option_index == 3 )
                    {
                        g_bAuth = true;
                    }
#else
                    else if( option_index == 1 ||
                        option_index == 2 ||
                        option_index == 3 )
                    {
                        printf( "%s : %s\n", optarg,
                            "Error option is invalid "
                            "since JS is disabled" );
                    }
                    bQuit = true;
                    ret = -EINVAL;
                    break;
#endif
                    else if( option_index == 4 )
                    {
                        g_dwFlags &= ~GEN_CLIENT;
                    }
                    else if( option_index == 5 )
                    {
                        g_dwFlags &= ~GEN_SERVER;
                    }
                    else if( option_index == 6 )
                    {
                        stdstr strSvcs = optarg;
                        if( strSvcs.size() > REG_MAX_PATH )
                        {
                            printf( "%s : %s\n", optarg,
                                "Error parameter is too long " );
                            ret = -ERANGE;
                            bQuit = true;
                            break;
                        }
                        std::regex s("^[a-zA-Z_][a-zA-Z0-9_]*$");
                        size_t start = 0;
                        size_t pos = strSvcs.find_first_of(
                            ',', start );
                        while( pos != stdstr::npos )
                        {
                            stdstr strSvc =
                                strSvcs.substr( start, pos );
                            if( !std::regex_match( strSvc, s) )
                            {
                                printf( "%s : %s\n", strSvc.c_str(),
                                    "Error invalid service name " );
                                ret = -EINVAL;
                                bQuit = true;
                                break;
                            }
                            g_setServices.insert( strSvc );
                            start = pos + 1;
                            if( start >= strSvcs.size() )
                                break;
                            pos = strSvcs.find_first_of(
                                ',', start );
                        }
                        if( start < strSvcs.size() )
                            g_setServices.insert(
                                strSvcs.substr( start ) );
                    }
                    else if( option_index == 7 )
                    {
                        stdstr strSyncIf = optarg;
                        if( strSyncIf.size() > REG_MAX_PATH )
                        {
                            printf( "%s : %s\n", optarg,
                                "Error parameter is too long " );
                            ret = -ERANGE;
                            bQuit = true;
                            break;
                        }
                        std::regex
                        e("(^[a-zA-Z_][a-zA-Z0-9_]*)(\\.[a-zA-Z_][a-zA-Z0-9_]*)?(=)"
                            "(async|async_p|async_s|sync)");
                        std::smatch m;
                        std::regex_match( strSyncIf, m, e);
                        auto it = m.begin() + 1;
                        if( it == m.end() )
                        {
                            printf( "%s : %s\n", optarg,
                                "Error invalid interface name" );
                            ret = -EINVAL;
                            bQuit = true;
                            break;
                        }
                        stdstr strIfName = *it;
                        it++;
                        if( it == m.end() )
                        {
                            printf( "%s : %s\n", optarg,
                                "Error missing sync mode" );
                            ret = -EINVAL;
                            bQuit = true;
                            break;
                        }
                        stdstr strVal = *it;
                        if( strVal.empty() )
                        {
                            it++;
                            strVal = *it;
                        }
                        stdstr strMethod = "";
                        if( strVal[0] == '.' )
                        {
                            strMethod += strVal.substr( 1 );
                            it++;
                            strVal = *it;
                        }
                        if( strVal[0] == '=' )
                        {
                            it++;
                        }
                        else
                        {
                            printf( "%s : %s\n", optarg,
                                "Error invalid sync override expression" );
                            ret = -EINVAL;
                            bQuit = true;
                            break;
                        }
                        stdstr strSync = *it;
                        if( strSync == "async" )
                            g_mapIfSync.insert( { strIfName,
                                { strMethod,TOK_ASYNC } });
                        else if( strSync == "async_p" )
                            g_mapIfSync.insert( { strIfName,
                                { strMethod, TOK_ASYNCP } } );
                        else if( strSync == "async_s" )
                            g_mapIfSync.insert( { strIfName,
                                { strMethod, TOK_ASYNCS } } );
                        else if( strSync == "sync" )
                            g_mapIfSync.insert( { strIfName,
                                { strMethod, TOK_SYNC } } );
                        else if( strSync == "sync_s" )
                            g_mapIfSync.insert( { strIfName,
                                { strMethod, TOK_ASYNCP } } );
                        else if( strSync == "sync_p" )
                            g_mapIfSync.insert( { strIfName,
                                { strMethod, TOK_ASYNCS } } );
                        else
                        {
                            printf( "%s : %s\n", optarg,
                                "Error invalid sync mode" );
                            ret = -EINVAL;
                            bQuit = true;
                            break;
                        }
                    }
                    break;
                }
            case 'O':
            case 'I' :
                {
                    ret = IsValidDir( optarg );
                    if( ret == -ENOTDIR )
                    {
                        std::string strMsg =
                            "Error '";

                        strMsg += optarg;
                        strMsg +=
                           "' is not a directory";
                        printf( "%s\n",
                            strMsg.c_str() );
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
                    stdstr strMsg;
                    if( iSize > sizeof( szBuf ) )
                    {
                        strMsg +=
                           "path is too long";
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
                            strMsg +=
                               "path is too long";
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
                            strMsg +=
                               "path is too long";
                            ret = -ERANGE;
                            bQuit = true;
                            break;
                        }
                    }
                    if( opt == 'I' )
                    {
                        g_vecPaths.push_back(
                            strFullPath );
                    }
                    else
                    {
                        g_strOutPath = strFullPath;
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
#ifdef PYTHON
                    g_strLang = "py";
                    break;
#else
                    fprintf( stderr,
                        "Error '-%c' is not supported by PYTHON disabled\n", opt );
                    ret = -ENOTSUP;
                    bQuit = true;
                    break;
#endif
                }
#ifdef JAVA
            case 'j':
                {
                    g_strLang = "java";
                    break;
                }
            case 'P':
                {
                    if( g_strLang == "java" )
                    {
                        g_strPrefix = optarg;
                        if( g_strPrefix.back() != '.' )
                            g_strPrefix.append( 1, '.' );
                    }
                    break;
                }
#else
            case 'j':
            case 'P':
                {
                    fprintf( stderr,
                        "Error '-%c' is not supported by JAVA disabled\n", opt );
                    ret = -ENOTSUP;
                    bQuit = true;
                    break;
                }
#endif
#ifdef JAVASCRIPT
            case 'J':
                {
                    g_strLang = "js";
                    break;
                }
#endif

            case 'l':
                {
                    g_bMklib = true;
                    if( g_bBuiltinRt )
                    {
                        fprintf( stderr,
                            "Error '-b' cannot be used with '-l' option\n" );
                        bQuit = true;
                    }
                    break;
                }
            case 'L':
                {
                    g_strLocale = optarg;
                    if( g_strLocale != "en" &&
                        g_strLocale != "cn" )
                    {
                        fprintf( stderr,
                            "Error -L specifies unsupported language\n" );
                        Usage();
                        bQuit = true;
                    }
                    break;
                }
            case 'f':
                {
#ifdef FUSE3
                    // make a shared lib for FUSE integration
                    if( optarg == nullptr )
                    {
                        g_dwFlags |= FUSE_BOTH;
                        break;
                    }
                    char ch = optarg[ 0 ];
                    if( ch == 's' )
                        g_dwFlags |= FUSE_SERVER;
                    else if( ch == 'p' )
                        g_dwFlags |= FUSE_PROXY;
                    else
                    {
                        fprintf( stderr,
                            "Error '-f' is followed with invalid argument\n" );
                        Usage();
                        bQuit = true;
                    }
#else
                    fprintf( stderr,
                        "Error '-f' is not supported with FUSE3 disabled\n" );
                    ret = -ENOTSUP;
                    bQuit = true;
#endif
                    break;
                }
            case 's':
                {
                    g_bRpcOverStm = true;
                    break;
                }
            case 'b':
                {
                    g_bBuiltinRt = true;
                    if( g_bMklib )
                    {
                        fprintf( stderr,
                            "Error '-l' cannot be used with '-b' option\n" );
                        bQuit = true;
                    }
                    break;
                }
            case 'v':
                {
                    printf( "%s", Version() );
                    bQuit = true;
                    break;
                }
            case 'm':
                {
                    g_bMonitoring = true;
                    std::istringstream iss(optarg);
                    std::string token;
                    while( std::getline(iss, token, ',') )
                        g_vecMonApps.push_back(token);
                    break;
                }
            case '?' :
            case ':' :
            case 'h' :
            default:
                {
                    Usage();
                    bQuit = true;
                    break;
                }
            }

            if( bQuit )
                break;
        }

        if( bQuit )
            break;

        if( g_bMonitoring && !g_bRpcOverStm )
        {
            printf( "'-s' option must be present to support monitoring\n" );
            Usage();
            ret = -1;
            break;
        }

        if( argv[ optind ] == nullptr )
        {
            printf( "Missing file to compile\n" );
            Usage();
            ret = -ENOENT;
            break;
        }

        if( argv[ optind + 1 ] != nullptr )
        {
            printf( "too many arguments\n" );
            Usage();
            ret = -EINVAL;
            break;
        }

        std::string strFile = argv[ optind ];
        if( strFile.size() > REG_MAX_PATH )
        {
            printf( "File name too long\n" );
            ret = -ENAMETOOLONG;
            break;
        }

        char* pszFile = realpath(
            strFile.c_str(), nullptr );
        if( pszFile == nullptr )
        {
            ret = -errno;
            break;
        }

        try {
            ret = yyparse( pszFile );
        }
        catch (std::exception& e)
        {
            printf( "%s\n", e.what() );
        }

        free( pszFile );
        pszFile = nullptr;

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
            // make sure the svc name does not conflict with
            // the appname.
            ret = pRoot->CheckSvcName();
            if( ERROR( ret ) )
                break;
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
        if( bFuse || g_strLang == "cpp" )
        {
            if( bFuse && g_strLang != "cpp" )
            {
                // generating rpcfs skeleton code
                ret = GenRpcFSkelton(
                    g_strOutPath, strAppName, pRoot );

                if( ERROR( ret ) )
                {
                    printf( "Error %s with rpcfs support is "
                        "not available yet\n",
                        g_strLang.c_str() );
                    break;
                }

                stdstr strOutPath = g_strOutPath + "/fs";
                ret = GenCppProj(
                    strOutPath, strAppName, pRoot );
            }
            else
            {
                ret = GenCppProj(
                    g_strOutPath, strAppName, pRoot );
            }
        }
        else if( g_strLang == "py" )
        {
            ret = GenPyProj(
                g_strOutPath, strAppName, pRoot );
        }
        else if( g_strLang == "java" )
        {
            ret = GenJavaProj(
                g_strOutPath, strAppName, pRoot );
        }
        else if( g_strLang == "js" )
        {
            ret = GenJsProj(
                g_strOutPath, strAppName, pRoot );
        }
        else
        {
            Usage();
            ret = -ENOTSUP;
        }

        if( ERROR( ret ) )
            break;

        // store the command to the file 'cmdline'
        stdstr strPath = g_strOutPath;
        strPath += "/cmdline";
        FILE* fp = fopen( strPath.c_str(), "w" );
        if( fp == nullptr )
            break;
        stdstr strShebang = "#!/bin/sh\n";
        fwrite( strShebang.c_str(),
            strShebang.size(), 1, fp );
        g_strCmdLine.append( 1, '\n' );
        fwrite( g_strCmdLine.c_str(),
            g_strCmdLine.size(), 1, fp );
        fclose( fp );

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
