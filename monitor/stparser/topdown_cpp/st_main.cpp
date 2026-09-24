#include <iostream>
#include <fstream>
#include <string>
#include "antlr4-runtime.h"
#include "stlexer.h"
#include "stparser.h"

#include "parse_context.h"
#include "st_parse_listener.h"
#include "st_parser_ext.h"
#include <getopt.h>
#include "defines.h"

void Usage()
{
    printf( "Usage:" );
    printf( "rpcfstc [options] <ST file> \n" );
    printf( "\t compile the `ST file'"
        "and output the RPC skeleton files.\n" );
}

bool g_bTrace = false;

int main(int argc, char* argv[])
{
    int ret = 0;
    std::string strFile;
    if( argc < 1 )
    {
        Usage();
        return 1;
    }

    int option_index = 0;
    struct option long_options[] = {
        {0, 0,  0,  0 }
    };

    bool bQuit = false;
    bool bError = false;
    do{
        while( true ) 
        {
            int opt = getopt_long( argc, argv,
                "ht",
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
            case 'h' :
                {
                    Usage();
                    bQuit = true;
                    break;
                }
            default:
                bQuit = true;
                bError = true;
                break;
            }
            if( bQuit )
            {
                if( bError )
                    return 1;
                return 0;
            }
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

        strFile = argv[ optind ];

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

    }while( 0 );
    if( ERROR( ret ) )
        return ret;

    std::ifstream stream(strFile);
    if (!stream.is_open()) {
        std::cerr << "Cannot open file: "
            << strFile << std::endl;
        return 1;
    }

    // Set up the parse context with a symbol table
    CStParseContext parseContext;

    antlr4::ANTLRInputStream input(stream);
    stlexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    // Use the extended parser with predicates
    CStParser parser(&tokens);
    parser.SetParseContext(&parseContext);

    // Create and attach the listener
    CStParseListener listener(&parseContext);
    listener.SetTokenStream(&tokens);
    parser.addParseListener(&listener);

    std::cout << "Parsing " 
        << strFile << "..." << std::endl;

    // Enable parser trace
    if( g_bTrace )
        parser.setTrace(true);

    stparser::Start_pointContext* tree =
        parser.start_point();

    (void)tree; // suppress unused variable warning

    if (parser.getNumberOfSyntaxErrors() > 0) {
        std::cerr << "Parse failed with "
            << parser.getNumberOfSyntaxErrors()
            << " errors" << std::endl;
        return 1;
    }

    std::cout << "Parse successful!" << std::endl;

    // After parsing, check if there are any deferred type references
    // that need semantic resolution
    std::cout << "\n=== Parse Summary ===" << std::endl;
    std::cout << "Pending category after parse: " 
              << static_cast<int>(parseContext.m_iPendingCategory)
              << std::endl;
    return 0;
}
