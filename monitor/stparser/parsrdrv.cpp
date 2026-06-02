/*
 * =====================================================================================
 *
 *       Filename:  parsrdrv.cpp
 *
 *    Description:  the functions for ST parser 
 *
 *        Version:  1.0
 *        Created:  05/29/2026 10:55:36 AM
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

#define IsCondPragma( _iPragma_ ) \
    ( _iPragma_ == TOK_IF || \
        _iPragma_ == TOK_ELSIF || \
        _iPragma_ == TOK_ELSE || \
        _iPragma_ == TOK_END_IF )

#define READTOK( _val, _loc, _scanner, _ctx ) ({ do{ \
        if( _ctx->UseQueuedToken() && _ctx->GetTokenCount() ) \
        { \
            CTokenInfo oToken = _ctx->GetCurTok(); \
            *_val = oToken.pVal; *_loc = oToken.lloc; \
            ret = oToken.token;\
            _ctx->DequeToken();\
            break; \
        } \
        else if( _ctx->UseQueuedToken() ) \
            _ctx->SetUseQueuedToken( false ); \
        if( !_ctx->IsEOF() ) { \
            ret = ReadToken( _val, _loc, _scanner, _ctx ); \
            if( ret == YYEOF ) _ctx->SetEOF( true ); \
        }else{ ret = YYEOF; } \
    }while( 0 ); ret;})

gint32 ReadToken( YYSTYPE* cur_lval,
    YYLTYPE2* cur_loc,
    yyscan_t yyscanner,
    CSTParserContext* pCtx )
{
    gint32 ret = YYEOF;
    do{
        ret = yylex( cur_lval,
            cur_loc, yyscanner, pCtx );
        if( ret == YYEMPTY )
            continue;
        break;
    }while( 1 );
    return ret;
}

gint32 StartCaseSelectorCheck(
    CSTParserContext* pCtx,
    yypstate* main_ps )
{

    gint32 ret = 0;
    yypstate* case_ps = yypstate_new();

    pCtx->m_iLastCaseChk = -1;

    YYSTYPE current_lval( new YYSPAIR() );
    YYLTYPE2 current_lloc;
    gint32 current_tok;

    YYSTYPE next_lval( new YYSPAIR() );
    YYLTYPE2 next_lloc;
    gint32 next_tok;
    gint32 iState = -1;

    yypush_parse( case_ps,
        TOK_VSTART_CASESEL, &current_lval,
        &current_lloc, pCtx );

    bool bEnd = false;
    auto states = CASESEL_CHECK_STATES;
    for( int i = 0; i < pCtx->GetTokenCount(); i++ )
    {
        CTokenInfo oti = pCtx->GetCurTok( i );
        current_tok = oti.token;
        current_lloc = oti.lloc;
        current_lval = oti.pVal;
        ret = yypush_parse( case_ps,
            oti.token, &oti.pVal, &oti.lloc, pCtx );
        if( ret != YYPUSH_MORE )
            break;
        iState = GetParserState( case_ps );
        for( int i = 0; i < states.size(); i++ )
        {
            if( iState == states[ i ] )
            {
                bEnd = true;
                break;
            }
        }
    }

    do{
        if( ret != YYPUSH_MORE )
            break;
        if( bEnd )
        {
            current_tok = YYEOF;
            ret = yypush_parse( case_ps,
                current_tok, &current_lval,
                &current_lloc, pCtx );
            break;
        }

        while( ret == YYPUSH_MORE )
        {
            if( bEnd )
            {
                current_tok = YYEOF;
            }
            else
            {
                current_tok = READTOK(
                    &current_lval, &current_lloc,
                    pCtx->GetScanner(), pCtx );
                if( current_tok == TOK_SEMICOLON )
                {
                    pCtx->PushToken(
                        { current_lloc, current_tok, current_lval } );
                    do{
                        current_tok = READTOK(
                            &current_lval, &current_lloc,
                            pCtx->GetScanner(), pCtx );
                    }while( current_tok == TOK_SEMICOLON );
                    if( current_tok != YYEOF )
                    {
                        pCtx->PushToken(
                            { current_lloc, current_tok, current_lval } );
                    }
                    current_tok = YYEOF;
                }
                else if( current_tok == TOK_END_CASE ||
                    current_tok == TOK_ELSE )
                {
                    CTokenInfo oti;
                    oti.token = TOK_VCASE_SEP;
                    oti.lloc = current_lloc;
                    oti.lloc.last_column = oti.lloc.first_column;
                    pCtx->PushToken( oti );
                    pCtx->PushToken(
                        { current_lloc, current_tok, current_lval } );
                    current_tok = YYEOF;
                }
            }
            ret = yypush_parse( case_ps,
                current_tok, &current_lval,
                &current_lloc, pCtx );
    
            if( ret == YYPUSH_ABORT )
            {
                auto oPrev = pCtx->GetCurTokBack();
                YYSTYPE pVal( new YYSPAIR() );
                oPrev.lloc.text = "error";
                oPrev.lloc.last_line = oPrev.lloc.first_line;
                oPrev.lloc.last_column = oPrev.lloc.first_column;
                pVal->second = oPrev.pVal->second;
                pVal->second = oPrev.lloc;
                pCtx->PushToken(
                    { oPrev.lloc, YYerror, pVal } );
                pCtx->PushToken(
                    { current_lloc, current_tok, current_lval } );
                ret = YYPUSH_ACCEPT;
            }
            if( ret != YYPUSH_MORE )
                break;

            pCtx->PushToken(
                { current_lloc, current_tok, current_lval } );
            iState = GetParserState( case_ps );
            for( int i = 0; i < states.size(); i++ )
            {
                if( iState == states[ i ] )
                {
                    bEnd = true;
                    break;
                }
            }
        }

    }while( 0 );

    yypstate_delete( case_ps );
    if( ret == YYPUSH_ACCEPT )
        ret = YYPUSH_MORE;
    return ret;
}

gint32 AdjustTokens(
    CSTParserContext* pCtx,
    yypstate *current_ps,
    int& current_tok,
    YYSTYPE& current_lval,
    YYLTYPE2& current_lloc,
    int& next_tok, 
    YYSTYPE& next_lval,
    YYLTYPE2& next_lloc,
    int& prev_tok,
    YYSTYPE& prev_lval,
    YYLTYPE2& prev_lloc )
{

    gint32 ret = YYPUSH_MORE;
    do{
        if( current_tok == TOK_SEMICOLON &&
            next_tok == TOK_SEMICOLON )
        {
            // discard repeating simicolons
            do{
                next_tok = READTOK( &next_lval,
                    &next_lloc, pCtx->GetScanner(), pCtx );

            }while( next_tok == TOK_SEMICOLON );
        }

        gint32 iState = GetParserState( current_ps );
        switch( iState )
        {
        case CONFLICT_STATE:
            {
                if( pCtx->UseQueuedToken() )
                    break;

                if( current_tok == TOK_ELSE ||
                    current_tok == TOK_END_CASE )
                {
                    // lookahead one more token to
                    // resolve the shift/reduce
                    // conflict
                    YYLTYPE2 new_lloc = current_lloc;
                    new_lloc.last_line = new_lloc.first_line;
                    new_lloc.last_column = new_lloc.first_column;
                    new_lloc.text.clear();

                    YYSTYPE newVal( new YYSPAIR() );
                    CTokenInfo otok = {
                        new_lloc, TOK_VCASE_SEP, newVal };

                    pCtx->ClearTokens();
                    pCtx->PushToken(
                        { next_lloc, next_tok, next_lval } );

                    pCtx->SetUseQueuedToken( true );

                    next_lloc = current_lloc;
                    next_lval = current_lval;
                    next_tok = current_tok;

                    current_lloc = new_lloc;
                    current_lval = newVal;
                    current_tok = TOK_VCASE_SEP;
                }
                else if( current_tok == TOK_SEMICOLON &&
                    ( next_tok == TOK_ELSE ||
                    next_tok == TOK_END_CASE ) )
                {
                    current_tok = TOK_VCASE_SEP;
                }
                else if( current_tok == TOK_SEMICOLON )
                {
                    gint32 arrPrefix[] =
                    { TOK_IF, TOK_FOR, TOK_WHILE, TOK_REPEAT, TOK_CASE };
                    gint32 iCount =
                        sizeof( arrPrefix )/sizeof( gint32);
                    gint32 i = 0;
                    for( ; i < iCount; i++ )
                    {
                        if( next_tok == arrPrefix[ i ] )
                            break;
                    }
                    if( i < iCount )
                        break;

                    CTokenInfo oSemi =
                    { current_lloc, current_tok, current_lval };

                    // assignment or function call
                    current_tok = next_tok; 
                    current_lloc = next_lloc;
                    current_lval = next_lval;

                    next_tok = READTOK( &next_lval, &next_lloc,
                        pCtx->GetScanner(), pCtx );

                    pCtx->ClearTokens();
                    pCtx->PushToken(
                        { current_lloc, current_tok, current_lval } );

                    pCtx->PushToken(
                        { next_lloc, next_tok, next_lval } );

                    ret = StartCaseSelectorCheck(
                        pCtx, current_ps );
                    if( ret != YYPUSH_MORE )
                        break;

                    next_lloc = current_lloc;
                    next_lval = current_lval;
                    next_tok = current_tok;

                    current_tok = oSemi.token;
                    current_lloc = oSemi.lloc;
                    current_lval = oSemi.pVal;

                    pCtx->DequeToken();

                    if( pCtx->m_iLastCaseChk == 0 )
                    {
                        // case selector
                        current_tok = TOK_VCASE_SEP;
                    }
                    pCtx->m_iLastCaseChk = -1;
                    pCtx->SetUseQueuedToken( true );
                }
                else if( current_tok != TOK_SEMICOLON )
                {
                    gint32 arrPrefix[] =
                    { TOK_IF, TOK_FOR, TOK_WHILE, TOK_REPEAT, TOK_CASE };
                    gint32 iCount =
                        sizeof( arrPrefix )/sizeof( gint32);
                    gint32 i = 0;
                    for( ; i < iCount; i++ )
                    {
                        if( current_tok == arrPrefix[ i ] )
                            break;
                    }
                    if( i < iCount )
                        break;

                    // read in more tokens to decide if
                    // this is a case selector or something else.
                    pCtx->ClearTokens();
                    pCtx->PushToken(
                        { current_lloc, current_tok, current_lval } );

                    pCtx->PushToken(
                        { next_lloc, next_tok, next_lval } );

                    ret = StartCaseSelectorCheck(
                        pCtx, current_ps );
                    if( ret != YYPUSH_MORE )
                        break;
                    if( pCtx->m_iLastCaseChk == 0 )
                    {
                        next_lloc = current_lloc;
                        next_lval = current_lval;
                        next_tok = current_tok;

                        current_tok = TOK_VCASE_SEP;
                        pCtx->DequeToken();
                    }
                    else
                    {
                        pCtx->DequeToken();
                        pCtx->DequeToken();
                    }
                    pCtx->m_iLastCaseChk = -1;
                    pCtx->SetUseQueuedToken( true );
                }
                break;
            }
        case SUBEXPR_STATE:
            {
                if( current_tok == TOK_MINUS )
                {
                    // before bison can see this
                    // token, replace TOK_MINUS
                    // with TOK_SUB, so that bison
                    // can shift without error.
                    current_tok = TOK_VSUB;
                }
                break;
            }
        case METHOD_RET_STATE: 
            {
                // insert an virtual token 
                bool bInsert = true;
                if( current_tok == TOK_DOT &&
                    next_tok == TOK_ID )
                {
                    // assuming instance_path not span lines.
                    if( current_lloc.first_line ==
                        prev_lloc.first_line )
                        bInsert = false;
                }
                if( !bInsert )
                    break;
                YYLTYPE2 temploc = current_lloc;
                temploc.last_line = temploc.first_line;
                temploc.last_column = temploc.first_column;
                ret = yypush_parse( current_ps,
                    TOK_VSEMICOLON,
                    nullptr, &temploc, pCtx );
                break;
            }
        case USING_SEP_STATE:
            {
                if( current_tok == TOK_SEMICOLON )
                    current_tok = TOK_VSEMICOLON;
                break;
            }
        case LVALUE_BIT_STATES:
            {
                if( current_tok == TOK_DOT &&
                    next_tok == TOK_NUMBER )
                    current_tok = TOK_VPUNC;
                break;
            }
        default:
            break;
        }

    }while( 0 );
    return ret;
}

gint32 StartParse(
    CSTParserContext* pCtx,
    const stdstr& strFile )
{
    gint32 ret = 0;
    gint32 current_tok;
    gint32 status = YYPUSH_MORE;
    gint32 prev_tok;

    YYSTYPE current_lval( new YYSPAIR() );
    YYLTYPE2 current_lloc;

    yyscan_t yyscanner;
    yylex_init( &yyscanner );

    yypstate *main_ps = yypstate_new();
    pCtx->SetScanner( yyscanner );
    pCtx->SetParser( main_ps );

    yypstate *pragma_ps = nullptr;
    status = yypush_parse( main_ps,
        TOK_VSTART_MAIN, &current_lval, &current_lloc, pCtx );

    stdstr strFullPath;
    auto pFile = pCtx->TryOpenFile(
        strFile, strFullPath );
    if ( !pFile )
    {
        fprintf( stderr,
            "Error, cannot open file '%s'",
            strFile.c_str() );
        return -EINVAL;
    }

    pCtx->m_strCurFile = strFullPath;
    yyset_in( pFile, yyscanner );

    // Initialization: Get the first token
    current_tok = READTOK( &current_lval,
        &current_lloc, yyscanner, pCtx );

    yypstate* current_ps = main_ps;

    YYSTYPE next_lval( new YYSPAIR() );
    YYLTYPE2 next_lloc;

    YYSTYPE prev_lval( new YYSPAIR() );
    YYLTYPE2 prev_lloc;

    gint32 iState = 0;
    while( status == YYPUSH_MORE )
    {
        // 1. Peek: Get the next token from the lexer

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
            status = AdjustTokens( pCtx, current_ps,
                current_tok, current_lval, current_lloc,
                next_tok, next_lval, next_lloc,
                prev_tok, prev_lval, prev_lloc );
            if( status != YYPUSH_MORE )
                break;

            if( current_tok == TOK_LBRACE &&
                next_tok == TOK_IF )
            {
                if( pCtx->GetCondStackSize() > 256 )
                {
                    status = YYPUSH_ABORT;
                    ParserPrint( pCtx->GetCurFileName().c_str(),
                        current_lloc.first_line,
                        "error, too many "
                        "nested conditional pragmas.", true );
                    break;
                }
                current_tok = TOK_VSTART_PRAGMA;
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
                    ParserPrint( pCtx->GetCurFileName().c_str(),
                        current_lloc.first_line,
                        "error, "
                        "unexpected elsif or endif." );
                    break;
                }
                current_tok = TOK_VSTART_PRAGMA;
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
                current_tok = TOK_VSTART_PRAGMA;
                current_ps = pragma_ps = yypstate_new();
                pCtx->SetLastPragma( next_tok );
            }
            break;
        }

        if( status != YYPUSH_MORE )
            break;

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
                yypstate_delete( pragma_ps );
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
                    current_tok = TOK_VSTART_PRAGMA;
                    current_ps = pragma_ps = yypstate_new();
                    pCtx->SetLastPragma( next_tok );
                    bUpdate = false;
                }
                else if( next_tok == TOK_END_IF )
                {
                    cps.m_iCondDepth--;
                    YYLTYPE2 temploc = next_lloc;
                    next_tok = READTOK(
                        &next_lval, &next_lloc,
                        yyscanner, pCtx );
                    if( next_tok != TOK_RBRACE )
                    {
                        ParserPrint(
                            pCtx->GetCurFileName().c_str(),
                            temploc.first_line,
                            "error, expecting "
                            "'}' after 'end_if'", true );
                        status = YYPUSH_ABORT;
                        break;
                    }
                    next_tok = READTOK(
                        &next_lval, &next_lloc,
                        yyscanner, pCtx );
                }
            }
        }

        if( status == YYPUSH_ABORT ||
            status == YYPUSH_NOMEM )
            break;

        // 4. Advance: The 'next' becomes the 'current'
        if( bUpdate )
        {
            prev_tok = current_tok;
            prev_lval = current_lval;
            prev_lloc = current_lloc;

            current_tok = next_tok;
            current_lval = next_lval;
            current_lloc = next_lloc;
        }
    }

    if( status == YYPUSH_ACCEPT &&
        pCtx->GetErrorCount() == 0 )
    {
        ParserPrint( basename(
            pCtx->GetCurFileName().c_str() ),
            current_lloc.first_line, 
            "Parsing successfully" );
    }
    yylex_destroy( yyscanner );
    pCtx->SetScanner( nullptr );
    if( pFile )
        fclose( pFile );
    return ret;
}

