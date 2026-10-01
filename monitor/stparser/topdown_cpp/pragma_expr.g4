/*
 * =====================================================================================
 *
 *       Filename:  pragma_expr.g4
 *
 *    Description:  The lexer/parser for pragma of Structured Text Language (IEC 61131-3)
 *
 *        Version:  1.0
 *        Created:  09/28/2026 12:00:00 PM
 *       Revision:  none
 *       Compiler:  antlr4
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
grammar pragma_expr;

options {
    language = Cpp;
    caseinsensitive = true;
}

// --- Parser Rules ---

/**
 * Root rule for a conditional pragma expression.
 */
pragma_condition
    : expr EOF
    | include_directive EOF
    ;

include_directive
    : TOK_INCLUDE TOK_LSTRING
    ;
/**
 * Logical OR expression (lowest precedence).
 */
expr
    : expr TOK_OR and_expr
    | expr TOK_XOR and_expr
    | and_expr
    ;

/**
 * Logical AND expression.
 */
and_expr
    : and_expr TOK_AND primary_expr
    | primary_expr
    ;

/**
 * Primary terms, unary operators, built-in query functions, and grouped expressions.
 */
primary_expr
    : TOK_NOT primary_expr
    | TOK_DEFINED TOK_LPAREN identifier_type_pair TOK_RPAREN
    | TOK_HASATTRIBUTE TOK_LPAREN target=TOK_IDENTIFIER TOK_COMMA attr=TOK_IDENTIFIER TOK_RPAREN
    | TOK_HASVALUE TOK_LPAREN target=TOK_IDENTIFIER TOK_COMMA val=TOK_IDENTIFIER TOK_RPAREN
    | TOK_HASCONSTANT TOK_LPAREN target=TOK_IDENTIFIER TOK_RPAREN
    | TOK_IDENTIFIER
    | TOK_LPAREN expr TOK_RPAREN
    ;

/**
 * Structured argument for the defined() query (e.g., variable: x or type: MyFB).
 */
identifier_type_pair
    : (query_type TOK_COLON)? TOK_IDENTIFIER
    ;

query_type
    : TOK_VARIABLE
    | TOK_TYPE
    ;


// --- Lexer Rules (Clean Case-Insensitive Definitions) ---

TOK_OR           : 'OR' ;
TOK_XOR          : 'XOR' ;
TOK_AND          : 'AND' ;
TOK_NOT          : 'NOT' ;

// Built-in Evaluation Query Functions
TOK_DEFINED      : 'DEFINED' ;
TOK_HASATTRIBUTE : 'HASATTRIBUTE' ;
TOK_HASVALUE     : 'HASVALUE' ;
TOK_HASCONSTANT  : 'HASCONSTANT' ;

TOK_VARIABLE     : 'VARIABLE' ;
TOK_TYPE         : 'TYPE' ;

TOK_LPAREN       : '(' ;
TOK_RPAREN       : ')' ;
TOK_COLON        : ':' ;
TOK_COMMA        : ',' ;

// Standard IEC 61131-3 Identifier
TOK_IDENTIFIER
    : [A-Z_] [A-Z0-9_]*
    ;

// Skip all formatting spacing inside the pragma text chunk
WS
    : [ \t\r\n]+ -> skip
    ;

