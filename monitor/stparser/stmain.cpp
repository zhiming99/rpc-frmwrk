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
#include "stlexer.h"
#include "stscript.h"
#include "getopt.h"
using namespace rpcf;

std::shared_ptr< CStParserContext > g_pParserCtx;

#define IsCondPragma( _iPragma_ ) \
    ( _iPragma_ == TOK_IF || \
        _iPragma_ == TOK_ELSIF || \
        _iPragma_ == TOK_ELSE || \
        _iPragma_ == TOK_END_IF )

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

    INIT_MAP_ENTRY( CStructDecl );
    INIT_MAP_ENTRY( CFuncDecl );
    INIT_MAP_ENTRY( CFuncBlockDecl );

    END_FACTORY_MAPS;
};

gint32 ReadToken( YYLTYPE* current_lval,
    YYSTYPE* current_loc,
    yyscan_t yyscanner,
    CSTParserContext* pCtx )
{
    gint32 ret = YYEOF;
    do{
        ret = yylex( current_lval,
            current_loc, yyscanner, pCtx );
        if( ret == YYEMPTY )
            continue;
        break;
    }while( 1 );
    return ret;
}

#define READTOK( _val, _loc, _scanner, _ctx ) ( do{ \
        ret = ReadToken( _val, _loc, _scanner, _ctx ); \
        if( ret == YYEOF ) \
            ret = TOK_EOF; \
    }while( 0 ), ret )

gint32 StartParse( CSTParserContext* pCtx )
{
    gint32 ret = 0;
    gint32 current_tok;
    gint32 status = YYPUSH_MORE;

    YYSTYPE current_lval;
    YYLTYPE current_lloc;

    yyscan_t yyscanner;
    yylex_init( &yyscanner );

    pCtx->yyscanner = yyscanner;
    yypstate *main_ps = yypstate_new();
    yypstate *pragma_ps = nullptr;
    status = yypush_parse(
        main_ps, TOK_START_MAIN, NULL, NULL);

    // Initialization: Get the first token
    current_tok = READTOK( &current_lval,
        &current_lloc, yyscanner, pCtx );

    yypstate* current_ps = main_ps;
    while( status == YYPUSH_MORE )
    {
        // 1. Peek: Get the next token from the lexer
        YYSTYPE next_lval;
        YYLTYPE next_lloc;
        gint32 next_tok = READTOK( &next_lval,
            &next_lloc, yyscanner, pCtx );
        bool bUpdate = true;

        // 2. Logic: Now you have both current and next!

        // swallow tokens in false branch
        bool bSwallow = false;
        if( current_ps == main_ps &&
            !pCtx->IsCondStackEmpty() &&
            pCtx->GetTopState().m_iProgress != building )
            bSwallow = true;

        while( current_ps == main_ps && !bSwallow )
        {
            if( GetParserState( ps ) == CONFLICT_STATE &&
                current_tok == TOK_SEMICOLON &&
                next_tok == TOK_ELSE )
            {
                current_tok = TOK_CASE_SEP;
                break;
            }

            if( current_tok == TOK_LBRACE &&
                next_tok == TOK_IF )
            {
                if( pCtx->GetCondStackSize() > 256 )
                {
                    status = YYPUSH_ABORT;
                    ParserPrint( 
                        current_lloc->name, 
                        current_lloc->first_line,
                        "Fatal error, too many "
                        "nested conditional pragmas." );
                    break;
                }
                current_tok = TOK_START_PRAGMA;
                current_ps = pragma_ps = yypstate_new();
                CondPragmaState cps;
                YYLTYPE2 loc( current_lloc );
                cps.m_vecBlockPos.push_back( loc );
                pCtx->PushState( cps );
                pCtx->SetLastPragma( next_tok );
            }
            else if( current_tok == TOK_LBRACE &&
                ( next_tok == TOK_ELSIF ||
                  next_tok == TOK_END_IF ) )
            {
                auto& cstat = pCtx->GetTopState();
                if( cstat.m_iProgress == invalidProgress )
                {
                    status = YYPUSH_ABORT;
                    ParserPrint( 
                        current_lloc->name, 
                        current_lloc->first_line,
                        "Fatal error, "
                        "unexpected elsif or endif." );
                    break;
                }
                current_tok = TOK_START_PRAGMA;
                current_ps = pragma_ps = yypstate_new();
                cstat.m_iBlkIdx++;
                if( next_tok == TOK_ELSIF )
                    cstat.m_iBlkType = elsifBlock;
                if( next_tok == TOK_ELSE )
                    cstat.m_iBlkType = elseBlock;
                else
                    cstat.m_iBlkType = endifBlock;
                YYLTYPE2 loc( current_lloc );
                cstat.m_vecBlockPos.push_back( loc );
                pCtx->SetLastPragma( next_tok );
            }
            else if( current_tok == TOK_LBRACE &&
                ( next_tok == TOK_INCLUDE ||
                next_tok == TOK_INFO ||
                next_tok == TOK_ATTRIBUTE ) )
            {
                current_tok = TOK_START_PRAGMA;
                current_ps = pragma_ps = yypstate_new();
                pCtx->SetLastPragma( next_tok );
            }
            break;
        }

        // 3. Push: Send the current token to the parser
        bool bParse = false;
        if( current_ps != main_ps ||
            pCtx->IsCondStackEmpty() ||
            pCtx->GetTopState().m_iProgress == building )
            bParse = true;

        if( bParse )
        {
            status = yypush_parse( current_ps, current_tok,
                &current_lval, &current_lloc, pCtx );

            while( current_ps != main_ps &&
                status == YYPUSH_ACCEPT )
            {
                yypush_delete( pragma_ps );
                pragma_ps = nullptr;
                current_ps = main_ps;

                gint32 iPragma = pCtx->GetLastPragma();
                if( iPragma == TOK_INCLUDE ||
                    iPragma == TOK_INFO ||
                    iPragma == TOK_ATTRIBUTE )
                {
                    pCtx->SetLastPragma( nt_invalid );
                    // TODO: switch to new buffer
                    break;
                }

                if( !IsCondPragma( iPragma ) )
                    break;

                auto& cstat = pCtx->GetTopState();
                switch( cstat.m_iBlkType )
                {
                    case endifBlock:
                    {
                        status = YYPUSH_MORE;
                        pCtx->PopState();
                        break;
                    }
                    case firstBlock:
                    {
                        if( cstat.m_bExprValue )
                            cstat.m_iProgress = building;
                        break;
                    }
                    case elsifBlock:
                    {
                        if( cstat.m_bExprValue &&
                            cstat.m_iProgress == scanning )
                            cstat.m_iProgress = building;
                        else if( cstat.m_iProgress == building )
                            cstat.m_iProgress = leaving;
                        break;
                    }
                    case elseBlock:
                    {
                        if( cstat.m_iProgress == scanning )
                            cstat.m_iProgress = building;
                        else if( cstat.m_iProgress == building )
                            cstat.m_iProgress = leaving;
                    }
                    default:
                    {
                        status = YYPUSH_ABORT;
                        break;
                    }
                }
                break;
            }

            if( status == YYPUSH_ABORT ||
                status == YYPUSH_NOMEM )
                break;
        }
        else 
        {
            // swallow the token on the false branch
            CondPragmaState cps = pCtx->GetTopState();
            if( current_tok == TOK_LBRACE &&
                next_tok == TOK_IF )
            {
                cps.m_iCondDepth++;
            }
            else if( current_tok == TOK_LBRACE &&
                ( next_tok == TOK_ELSIF ||
                  next_tok == TOK_ELSE  ||
                  next_tok == TOK_END_IF ) )
            {
                if( cps.m_iCondDepth == 0 )
                {
                    auto& cstat = pCtx->GetTopState();
                    current_tok = TOK_START_PRAGMA;
                    current_ps = pragma_ps = yypstate_new();
                    pCtx->SetLastPragma( next_tok );
                    bUpdate = false;
                }
                else if( next_tok == TOK_END_IF )
                {
                    cps.m_iCondDepth--;
                    gint32 iLastLine = next_lloc->last_line;
                    stdstr strName = next_lloc->name;
                    next_tok = READTOK(
                        &next_lval, &next_lloc,
                        yyscanner, pCtx );
                    if( next_tok != TOK_RBRACE )
                    {
                        ParserPrint( 
                            strName.c_str(), iLastLine,
                            "Fatal error, expecting "
                            "'}' after 'end_if'" );
                    }
                    next_tok = READTOK(
                        &next_lval, &next_lloc,
                        yyscanner, pCtx );
                }
            }
        }

        // 4. Advance: The 'next' becomes the 'current'
        if( bUpdate )
        {
            current_tok = next_tok;
            current_lval = next_lval;
            current_lloc = next_lloc;
        }
    }

    yylex_destroy( yyscanner );
    pCtx->yyscanner = nullptr;
    return ret;
}

void Usage()
{
    printf( "Usage:" );
    printf( "rpcfstc [options] <st file> \n" );

    printf( "\t compile the `ST file'"
        "and output the RPC skeleton files.\n" );

    printf( "Options -h:\tprint this help.\n");

    printf( "\t-I:\tSpecify the path to"
        " search for the included `ST files'.\n"
        "\t\tAnd this option can repeat many"
        "times.\n" );
}

int main( int argc, char** argv[] )
{
    gint32 ret = 0;
    bool bUninit = false;
    do{
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

        while( true ) 
        {

            opt = getopt_long( argc, argv,
                "hI:",
                long_options, &option_index );

            if( opt == -1 )
                break;

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
                    pCtx->m_vecInclPaths.push_back(
                        strFullPath );
                    break;
                }
            }
            break;
        }
        if( ERROR( ret ) )
            break;

        ret = StartParse( pCtx );

    }while( 0 );
    if( bUninit )
        CoUninitialize();

    return ret;
}

