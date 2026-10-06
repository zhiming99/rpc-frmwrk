/*
 * =====================================================================================
 *
 *       Filename:  st_main.h
 *
 *    Description:  the main entry of rpcfstc
 *
 *        Version:  1.0
 *        Created:  09/16/2026
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <iostream>
#include <fstream>
#include <string>
#include "antlr4-runtime.h"
#include "stlexer.h"
#include "stparser.h"

#include "st_parse_listener.h"
#include "st_parser_ext.h"
#include <getopt.h>
#include "defines.h"
#include "stclsids.h"
#include "astnodes.h"
#include "stsymtab.h"
#include <sys/stat.h>
#include "stlcont.h"

using namespace rpcf;
#include "parse_context.h"
#include "pragma.h"

bool g_bTrace = false;

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

    // AST Expression Nodes
    INIT_MAP_ENTRY( CStLiteralExpr );
    INIT_MAP_ENTRY( CStIdentifierExpr );
    INIT_MAP_ENTRY( CStDirectAddressNode );
    INIT_MAP_ENTRY( CStBinaryExpr );
    INIT_MAP_ENTRY( CStUnaryExpr );
    INIT_MAP_ENTRY( CStCallExpr );
    INIT_MAP_ENTRY( CStArgListNode );
    INIT_MAP_ENTRY( CStArrayAccessExpr );
    INIT_MAP_ENTRY( CStMemberAccessExpr );
    INIT_MAP_ENTRY( CStDereferenceExpr );
    INIT_MAP_ENTRY( CStPointerMemberExpr );
    INIT_MAP_ENTRY( CStLValueNode );
    INIT_MAP_ENTRY( CStLValueExtNode );
    INIT_MAP_ENTRY( CStInstancePathNode );
    INIT_MAP_ENTRY( CStFullExpressionNode );
    INIT_MAP_ENTRY( CStSubrangeNode );
    INIT_MAP_ENTRY( CStSubrangeListNode );
    INIT_MAP_ENTRY( CStStmtListNode );
    INIT_MAP_ENTRY( CStIfBranchListNode );

    // AST Type Nodes
    INIT_MAP_ENTRY( CStBasicTypeNode );
    INIT_MAP_ENTRY( CStArrayTypeNode );
    INIT_MAP_ENTRY( CStStructTypeNode );
    INIT_MAP_ENTRY( CStEnumTypeNode );
    INIT_MAP_ENTRY( CStEnumValueNode );
    INIT_MAP_ENTRY( CStEnumValueListNode );
    INIT_MAP_ENTRY( CStDataTypeSpecNode );
    INIT_MAP_ENTRY( CStTypeSpecNode );
    INIT_MAP_ENTRY( CStTypeDefinitionBlockNode );
    INIT_MAP_ENTRY( CStPointerTypeNode );
    INIT_MAP_ENTRY( CStReferenceTypeNode );
    INIT_MAP_ENTRY( CStDerivedTypeNode );

    // AST Variable Declaration
    INIT_MAP_ENTRY( CStVarDeclNode );

    // AST Statement Nodes
    INIT_MAP_ENTRY( CStAssignStmt );
    INIT_MAP_ENTRY( CStCallStmt );
    INIT_MAP_ENTRY( CStIfStmt );
    INIT_MAP_ENTRY( CStForStmt );
    INIT_MAP_ENTRY( CStWhileStmt );
    INIT_MAP_ENTRY( CStRepeatStmt );
    INIT_MAP_ENTRY( CStCaseStmt );
    INIT_MAP_ENTRY( CStPragmaStmt );

    // AST POU Declaration Nodes
    INIT_MAP_ENTRY( CStProgramDecl );
    INIT_MAP_ENTRY( CStFunctionBlockDecl );
    INIT_MAP_ENTRY( CStFunctionBlockHeaderNode );
    INIT_MAP_ENTRY( CStFunctionDecl );
    INIT_MAP_ENTRY( CStMethodDecl );

    // AST Other Declaration Nodes
    INIT_MAP_ENTRY( CStNamespaceDecl );
    INIT_MAP_ENTRY( CStInterfaceDecl );
    INIT_MAP_ENTRY( CStTypeDecl );
    INIT_MAP_ENTRY( CStVarConfigDecl );
    INIT_MAP_ENTRY( CStTaskConfigNode );
    INIT_MAP_ENTRY( CStTaskConfigListNode );
    INIT_MAP_ENTRY( CStUsingDirective );
    INIT_MAP_ENTRY( CStAccessDeclNode );
    INIT_MAP_ENTRY( CStAccessDeclListNode );
    INIT_MAP_ENTRY( CStAccessDeclsNode );
    INIT_MAP_ENTRY( CStGlobalVarDeclListNode );
    INIT_MAP_ENTRY( CStConfigDeclNode );
    INIT_MAP_ENTRY( CStDeclsNode );
    INIT_MAP_ENTRY( CStSingleResourceDeclNode );
    INIT_MAP_ENTRY( CStResourceDeclNode );
    INIT_MAP_ENTRY( CStResourceDeclListNode );
    INIT_MAP_ENTRY( CStResourceSectionNode );
    INIT_MAP_ENTRY( CStInitialValueNode );
    INIT_MAP_ENTRY( CStArrayInitNode );
    INIT_MAP_ENTRY( CStArrayRepeatNode );
    INIT_MAP_ENTRY( CStStructInitNode );
    INIT_MAP_ENTRY( CStIdentifierListNode );
    INIT_MAP_ENTRY( CStVarDeclListNode );
    INIT_MAP_ENTRY( CStMethodDeclListNode );
    INIT_MAP_ENTRY( CStCaseBranchListNode );
    INIT_MAP_ENTRY( CStVarConfigListNode );
    INIT_MAP_ENTRY( CStProgConfigListNode );
    INIT_MAP_ENTRY( CStProgConfigNode );
    INIT_MAP_ENTRY( CStSymbolicVarNode );
    INIT_MAP_ENTRY( CStFbTaskNode );
    INIT_MAP_ENTRY( CStProgCnxnNode );
    INIT_MAP_ENTRY( CStProgConfElemNode );

    // AST Root Node
    INIT_MAP_ENTRY( CStRootNode );

    // Symbol table
    INIT_MAP_ENTRY( CStSymbol );

    END_FACTORY_MAPS;
};

void Usage( const char* pszAppName)
{
    printf( "Usage:" );
    printf( "%s [options] <ST files> \n", pszAppName );
    printf( "\t compile the `ST file'"
        "and output the RPC skeleton files.\n" );
    printf( "Options -h:\tPrint this help.\n");

    printf( "-D:\t-D MACRO[=VALUE], define a macro, "
        "for example '-DDEBUG -DABC=1', this option can be"
        "specified repeatedly\n");

    printf( "-I:\tSpecify a directory to search for st files"
        " this option can be specified repeatedly\n");

    printf( "\t-t:\tPrint trace messages.\n" );
}

int main(int argc, char* argv[])
{
    int ret = 0;
    std::string strFile;
    if( argc < 1 )
    {
        Usage( argv[0] );
        return 1;
    }

    int option_index = 0;
    struct option long_options[] = {
        {0, 0,  0,  0 }
    };

    bool bQuit = false;

    ret = CoInitialize( COINIT_NORPC );
    if( ret )
        return ret;

    CStParseContext oCtx;
    do{

        FactoryPtr pFactory = InitClassFactory();
        ret = CoAddClassFactory( pFactory );
        if( ERROR( ret ) )
            break;

        // Set up the parse context with a symbol table
        while( true ) 
        {
            int opt = getopt_long( argc, argv,
                "D:I:ht",
                long_options, &option_index );

            if( opt == -1 )
                break;

            switch( opt )
            {
            case 0:
                break;
            case 't':
                {
                    g_bTrace = true;
                    break;
                }
            case 'I':
                {
                    if( !IsValidDir( optarg ) )
                    {
                        bQuit = true;
                        ret = -EINVAL;
                        break;
                    }
                    (*oCtx.m_pvecIncludePaths)().push_back(
                        std::string( optarg ) );
                    break;
                }
            case 'D':
                {
                    oCtx.m_oMacros.ParseAndRegisterMacro(
                        std::string(optarg) );
                    break;
                }
            case 'h' :
                {
                    Usage( argv[0] );
                    bQuit = true;
                    break;
                }
            default:
                bQuit = true;
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
            Usage( argv[0] );
            ret = -ENOENT;
            break;
        }

        while( optind < argc )
        {
            strFile = argv[ optind++ ];

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
            ( *oCtx.m_pvecSrcFiles )().push_back(
                std::string( pszFile ) );
        }

    }while( 0 );
        
    do{
        if( bQuit || ERROR( ret ) )
            break;

        std::ifstream stream(strFile);
        if (!stream.is_open()) {
            std::cerr << "Cannot open file: "
                << strFile << std::endl;
            ret = -EINVAL;
            break;
        }

        antlr4::ANTLRInputStream input(stream);
        stlexer lexer(&input);

        auto pIncludeMaager = std::make_unique
            <CStIncludeManager>();

        auto pPipeline = std::make_unique
            <CStTokenPipeline>( pIncludeManager );

        auto pMainStream = std::make_unique
            <CStPragmaFilteringTokenStream>( pPipeline.get() );

        // Set up the root token source reference so
        // exception loggers are happy
        pPipeline->m_pRootTokenSource = &lexer;

        pPipeline->InjectIncludeStream(
            strFile, nullptr);

        CStParser parser(pMainStream.get());


        // Use the extended parser with predicates
        CStParser parser(&tokens);
        parser.SetParseContext(&oCtx);

        // Create and attach the listener
        CStParseListener listener(&oCtx);
        listener.SetTokenStream(&tokens);
        parser.addParseListener(&listener);

        // Create a shared pointer holding your
        // custom strategy instance
        std::shared_ptr< CPragmaRecoveryStrategy >
            pStrategy = std::make_shared
                < CPragmaRecoveryStrategy >(
                stlexer::PRAGMA,
                &oCtx );

        // Hot-swap the parser's error handler pipeline
        parser.setErrorHandler( pStrategy );

        std::cout << "Parsing " 
            << strFile << "..." << std::endl;

        // Enable parser trace
        if( g_bTrace )
            parser.setTrace(true);

        stparser::Start_pointContext* tree =
            parser.start_point();

        (void)tree; // suppress unused variable warning

        if( parser.getNumberOfSyntaxErrors() > 0 )
        {
            std::cerr << "Parse failed with "
                << parser.getNumberOfSyntaxErrors()
                << " errors" << std::endl;
            return ERROR_FAIL;
        }

        std::cout << "Parse successful!" << std::endl;

        // After parsing, check if there are any
        // deferred type references that need semantic
        // resolution
        std::cout << "\n=== Parse Summary ==="
            << std::endl;
        std::cout << "Pending category after parse: " 
            << (int)(oCtx.m_iPendingCategory)
            << std::endl;

    }while( 0 );
    CoUninitialize();
    return ret;
}
