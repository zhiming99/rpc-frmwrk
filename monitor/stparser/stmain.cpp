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

int start_parse(  )
{
    int current_tok;
    int status = YYPUSH_MORE;

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
        int next_tok = yylex( &next_lval, &next_lloc );

        // 2. Logic: Now you have both current and next!
        if( GetParserState( ps ) == CONFLICT_STATE &&
            current_ps == main_ps &&
            current_tok == TOK_SEMICOLON &&
            next_tok == TOK_ELSE )
            current_tok = TOK_CASE_SEP;
        else if( ( current_tok == TOK_LBRACE &&
            next_tok == TOK_IF ) ||
            current_ps == main_ps )
        {
            current_tok = TOK_START_CONDITIONAL;
            current_ps = pragma_ps = yypstate_new();
        }

        // 3. Push: Send the current token to the parser
        status = yypush_parse( current_ps, current_tok,
            &current_lval, &current_lloc );

        // 4. Advance: The 'next' becomes the 'current'
        current_tok = next_tok;
        current_lval = next_lval;
        current_lloc = next_lloc;
        if( status == 0 && current_ps == pragma_ps )
        {
            yypush_delete( pragma_ps );
            pragma_ps = nullptr;
        }
    }

}

int main( int argc, char** argv[] )
{

}

