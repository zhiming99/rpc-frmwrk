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

#include "stlexer.h"
#include "stscript.h"

std::shared_ptr< CStParserContext > g_pParserCtx;

gint32 start_parse( CSTParserContext* pCtx )
{
    gint32 current_tok;
    gint32 status = YYPUSH_MORE;

    YYSTYPE current_lval;
    YYLTYPE current_lloc;

    yypstate *main_ps = yypstate_new();
    yypstate *pragma_ps = nullptr;
    yypush_parse(main_ps, TOK_START_MAIN, NULL, NULL);

    // Initialization: Get the first token
    current_tok = yylex(
        &current_lval, &current_lloc );

    yypstate* current_ps = main_ps;
    while (status == YYPUSH_MORE)
    {
        // 1. Peek: Get the next token from the lexer
        YYSTYPE next_lval;
        YYLTYPE next_lloc;
        gint32 next_tok = yylex( &next_lval, &next_lloc );

        // 2. Logic: Now you have both current and next!

        // swallow tokens in false branch
        bool bCond = false;
        gint32 iPragma = pCtx->GetLastPragma();
        if( iPragma == TOK_IF ||
            iPragma == TOK_ELSIF ||
            iPragma == TOK_ELSE ||
            iPragma == TOK_END_IF )
            bCond = true;

        bool bSwallow = false;
        if( bCond && current_ps == main_ps &&
            !pCtx->IsCondStackEmpty() &&
            pCtx->GetTopState().m_iProgress != building )
            bSwallow = true;

        while( !bSwallow )
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
                    char szBuf[ 512 ];
                    sprintf( szBuf,
                        "%s(%d): Fatal error, unexpected elsif or endif.",
                        current_lloc->name, current_lloc->first_line );
                    yyerror( szBuf );
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
                next_tok == TOK_INCLUDE )
            {
                current_tok = TOK_START_PRAGMA;
                pCtx->SetLastPragma( next_tok );
            }
            break;
        }

        // 3. Push: Send the current token to the parser
        bool bParse = false;
        if( pCtx->IsCondStackEmpty() ||
            current_ps != main_ps ||
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
                if( iPragma == TOK_INCLUDE )
                {
                    pCtx->SetLastPragma( nt_invalid );
                    // TODO: switch to new buffer
                    break;
                }

                if( iPragma != TOK_IF &&
                    iPragma != TOK_ELSIF &&
                    iPragma != TOK_ELSE &&
                    iPragma != TOK_END_IF )
                    break;

                auto& cstat = pCtx->GetTopState();
                switch( cstat.m_iBlkType )
                {
                    case endifBlock:
                    {
                        status = YYPUSH_MORE;
                        if( cstat.m_iCondDepth )
                            cstat.m_iCondDepth--;
                        else
                            pCtx->PopState();
                        break;
                    }
                    case firstBlock:
                    {
                        if( cstat.m_iCondDepth )
                            break;
                        if( cstat.m_bExprValue )
                            cstat.m_iProgress = building;
                        break;
                    }
                    case elsifBlock:
                    {
                        if( cstat.m_iCondDepth )
                            break;
                        if( cstat.m_bExprValue &&
                            cstat.m_iProgress == scanning )
                            cstat.m_iProgress = building;
                        else if( cstat.m_iProgress == building )
                            cstat.m_iProgress = leaving;
                        break;
                    }
                    case elseBlock:
                    {
                        if( cstat.m_iCondDepth )
                            break;
                        if( cstat.m_bExprValue &&
                            cstat.m_iProgress == scanning )
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
            if( current_tok == TOK_LBRACE &&
                next_tok == TOK_IF )
            {
                current_tok = TOK_START_PRAGMA;
                current_ps = pragma_ps = yypstate_new();
                CondPragmaState cps = pCtx->GetTopState();
                cps.m_iCondDepth++;
                pCtx->SetLastPragma( next_tok );
            }
            else if( current_tok == TOK_LBRACE &&
                ( next_tok == TOK_ELSIF ||
                  next_tok == TOK_ELSE  ||
                  next_tok == TOK_END_IF ) )
            {
                auto& cstat = pCtx->GetTopState();
                current_tok = TOK_START_PRAGMA;
                current_ps = pragma_ps = yypstate_new();
                pCtx->SetLastPragma( next_tok );
            }
        }

        // 4. Advance: The 'next' becomes the 'current'
        current_tok = next_tok;
        current_lval = next_lval;
        current_lloc = next_lloc;
    }
    return status;
}

int main( int argc, char** argv[] )
{
    g_pParserCtx.reset( new CSTParserContext );
}

