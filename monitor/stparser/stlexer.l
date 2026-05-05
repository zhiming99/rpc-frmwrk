/*
 * =====================================================================================
 *
 *       Filename:  st_lexer.l
 *
 *    Description:  The lexical scanner definition for Structured Text Language
 *
 *        Version:  1.0
 *        Created:  04/10/2026 12:00:00 PM
 *       Revision:  none
 *       Compiler:  Flex
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
%top{
#include <stdio.h>
#include "y.tab.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>

std::string ProcessSTString(const char* input);
typedef Variant YYSTYPE;

%}

 /* Definitions */
ST_TYPE     (?i:BOOL|BYTE|WORD|DWORD|LWORD|SINT|INT|DINT|LINT|USINT|UINT|UDINT|ULINT|REAL|LREAL)
DIGIT       [0-9]
HEX_DIGIT   [0-9a-fA-F]

 /* Short and Long prefixes */
D_PREFIX    (?i:date|d)#
TOD_PREFIX  (?i:time_of_day|tod)#
DT_PREFIX   (?i:date_and_time|dt)#

ADDR_AREA   (?i:[IQM])
ADDR_SIZE   (?i:[XBWDL])
IDENTIFIER  [a-zA-Z_][a-zA-Z0-9_]*

DATA_TYPE   (?i:[BWDQSOF])
int comment_depth = 0;
/* Define Exclusive States */
%x ST_COMM C_COMM SINGLE_COMM


%%

 // (?i:TIME|T|TO)#-?([0-9]+(\.[0-9]+)?(_?[0-9]+)*(d|h|m|s|ms|us|ns))+ {
(?i:T|TIME|TO)#-?([0-9]+(\.[0-9]+)?d)?([0-9]+(\.[0-9]+)?h)?([0-9]+(\.[0-9]+)?m)?([0-9]+(\.[0-9]+)?s)?([0-9]+(\.[0-9]+)?ms)? {
    /* Copy the matched text to pass to Bison */
    yylval.strval = strdup(yytext); 
    return TOK_TIME;
}

(?i:INCLUDE)                return TOK_INCLUDE;
(?i:STRUCT)                 return TOK_STRUCT;
(?i:END_STRUCT)             return TOK_END_STRUCT;
(?i:ARRAY)                  return TOK_ARRAY;
(?i:INT)                    return TOK_INT;
(?i:UINT)                   return TOK_UINT;
(?i:DINT)                   return TOK_DINT;
(?i:UDINT)                  return TOK_UDINT;
(?i:LINT)                   return TOK_LINT;
(?i:ULINT)                  return TOK_ULINT;
(?i:SINT)                   return TOK_SINT;
(?i:USINT)                  return TOK_USINT;
(?i:BOOL)                   return TOK_BOOL;
(?i:BYTE)                   return TOK_BYTE;
(?i:WORD)                   return TOK_WORD;
(?i:DWORD)                  return TOK_DWORD;
(?i:LDWORD)                 return TOK_LDWORD;
(?i:REAL)                   return TOK_REAL;
(?i:LREAL)                  return TOK_LREAL;
(?i:PROGRAM)                return TOK_PROGRAM;
(?i:VAR)                    return TOK_VAR;
(?i:VAR_GLOBAL)             return TOK_VAR_GLOBAL;
(?i:VAR_INPUT)              return TOK_VAR_INPUT;
(?i:VAR_OUTPUT)             return TOK_VAR_OUTPUT;
(?i:VAR_IN_OUT)             return TOK_VAR_IN_OUT;
(?i:VAR_TEMP)               return TOK_VAR_TEMP;
(?i:CONSTANT)               return TOK_CONSTANT;
(?i:END_VAR)                return TOK_END_VAR;
(?i:VAR_EXTERNAL)           return TOK_VAR_EXTERNAL;
(?i:VAR_CONFIG)             return TOK_VAR_CONFIG;
(?i:IF)                     return TOK_IF;
(?i:THEN)                   return TOK_THEN;
(?i:ELSE)                   return TOK_ELSE;
(?i:ELSIF)                  return TOK_ELSEIF;
(?i:END_IF)                 return TOK_END_IF;
(?i:CASE)                   return TOK_CASE;
(?i:OF)                     return TOK_OF;
(?i:END_CASE)               return TOK_END_CASE;
(?i:FOR)                    return TOK_FOR;
(?i:TO)                     return TOK_TO;
(?i:DO)                     return TOK_DO;
(?i:END_FOR)                return TOK_END_FOR;
(?i:WHILE)                  return TOK_WHILE;
(?i:END_WHILE)              return TOK_END_WHILE;
(?i:REPEAT)                 return TOK_REPEAT;
(?i:UNTIL)                  return TOK_UNTIL;
(?i:END_REPEAT)             return TOK_END_REPEAT;
(?i:MOD)                    return TOK_MOD;
(?i:AND)                    return TOK_AND;
(?i:OR)                     return TOK_OR;
(?i:XOR)                    return TOK_XOR;
(?i:NOT)                    return TOK_NOT;
(?i:FUNCTION_BLOCK)         return TOK_FUNCTION_BLOCK;
(?i:FUNCTION)               return TOK_FUNCTION;
(?i:END_FUNCTION_BLOCK)     return TOK_END_FUNCTION_BLOCK;
(?i:END_FUNCTION)           return TOK_END_FUNCTION;
(?i:END_PROGRAM)            return TOK_END_PROGRAM;
(?i:INTERFACE)              return TOK_INTERFACE;
(?i:END_INTERFACE)          return TOK_END_INTERFACE;
(?i:IMPLEMENTS)             return TOK_IMPLEMENTS;
(?i:EXTENDS)                return TOK_EXTENDS;
(?i:TYPE)                   return TOK_TYPE;
(?i:END_TYPE)               return TOK_END_TYPE;
(?i:TIME)                   return TOK_TIME_TYPE;
(?i:STRING)                 return TOK_STRING_TYPE;
(?i:WSTRING)                return TOK_WSTRING_TYPE;
(?i:AT)                     return TOK_AT;
(?i:BY)                     return TOK_BY;
(?i:RETAIN)                 return TOK_RETAIN;
(?i:PERSISTANT)             return TOK_PERSISTANT;
(?i:ABSTRACT)               return TOK_ABSTRACT;
(?i:FINAL)                  return TOK_FINAL;
(?i:SUPER)                  return TOK_SUPER;
(?i:THIS)                   return TOK_THIS;
(?i:PRIVATE)                return TOK_PRIVATE;
(?i:PUBLIC)                 return TOK_PUBLIC;
(?i:INTERNAL)               return TOK_INTERNAL;
(?i:PROTECTED)              return TOK_PROTECTED;
(?i:REFERENCE)              return TOK_REFERENCE;
(?i:REF_TO)                 return TOK_REF_TO;
(?i:ATTRIBUTE)              return TOK_ATTRIBUTE;
(?i:INFO)                   return TOK_INFO;
(?i:REGION)                 return TOK_REGION;
(?i:END_REGION)             return TOK_END_REGION;


 /* Matches D#2024-12-25 */
{D_PREFIX}[0-9]{4}-[0-9]{2}-[0-9]{2} {
    yylval.qwVal = ParseStTimeToUnix( yytext );
    return TOK_DATE;
}

 /* Matches TOD#14:30:05.123 */
{TOD_PREFIX}[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)? {
    yylval.qwVal = ParseStTimeToUnix( yytext );
    return TOK_TIME_OF_DAY;
}

 /* Matches DT#2024-12-25-14:30:05 */
{DT_PREFIX}[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)? {
    yylval.qwVal = ParseStTimeToUnix( yytext );
    return TOK_DATE_TIME
}

 /* Standard STRING literal */
'(''|[^'\n]|(\$[0-9a-fA-F]{2})|\$[LlPpRrTtQ$])*' {
    // Action: process string (strip quotes, resolve escapes)
    yylval.strVal = TranslateSTString( yytext );
    return TOK_STRING;
}

 /* WSTRING literal */
\"(\"\"|[^\"\n]|(\$[0-9a-fA-F]{4})|\$[LlPpRrTtQ$])*\" {
    yylval.strVal = TranslateSTString( yytext );
    return TOK_WSTRING;
}

 /* Error handling for unterminated strings */
'[^'\n]* { yyerror("Unterminated string literal"); }
\"[^\"\n]* { yyerror("Unterminated wide string literal"); }


 /* Match Typed Literals like INT#10, DWORD#16#FFFF, REAL#10.5 */
 /* Match Typed Hex/Binary like WORD#16#FF or BYTE#2#1010 */
{ST_TYPE}#([-+]?{DIGIT}+(_?{DIGIT}+)*(\.{DIGIT}+(_?{DIGIT}+)*)?) |
{ST_TYPE}#{DIGIT}+#({HEX_DIGIT}+(_?{HEX_DIGIT}+)*) {
    std::string s(yytext);
    size_t hashPos = s.find('#');
    std::string type = s.substr(0, hashPos);
    std::string valStr = s.substr(hashPos + 1);

    // Remove underscores from value string
    valStr.erase(std::remove(valStr.begin(), valStr.end(), '_'), valStr.end());

    if (type == "REAL" || type == "LREAL") {
        printf("f32.const %s\n", valStr.c_str());
    } else {
        // Handle hex if 16# is present
        int32_t finalVal;
        if (valStr.find("16#") == 0) {
            finalVal = std::stol(valStr.substr(3), nullptr, 16);
        } else {
            finalVal = std::stol(valStr);
        }
        printf("i32.const %d\n", finalVal);
    }
    return TOK_NUMBER;
}

 /* Matches Bit addresses: %IX0.0, %Q0.1, %M10.5 */
"%"{ADDR_AREA}X?({DIGIT}+"."{DIGIT}+|\*) {
    //yylval.addr_val = parse_absolute_address(yytext);
    return TOK_ABS_ADDR_BIT;
}

 /* Matches Byte/Word/DWord: %IB4, %QW10, %MD20 */
"%"{ADDR_AREA}{ADDR_SIZE}({DIGIT}+|\*) {
    //yylval.addr_val = parse_absolute_address(yytext);
    return TOK_ABS_ADDR_BLOCK;
}

 /* Matches Peripheral Access (PI): %IW0:P or %PIW0 */
"%P"{ADDR_AREA}{ADDR_SIZE}?{DIGIT}+ |
"%"{ADDR_AREA}{ADDR_SIZE}?{DIGIT}+":P" {
    //yylval.addr_val = parse_absolute_address(yytext);
    return TOK_ABS_ADDR_PERIPHERAL;
}


 /* 2. Based Literals (e.g., 16#FF) */
 /* This wins if it sees digits followed by "#" */
{DIGIT}+#[0-9a-fA-F_]+ {
    return TOK_NUMBER;
}

 /* 3. Simple Decimal (e.g., 123) */
 /* This is the fallback for any sequence of digits without a "#" */
{DIGIT}[0-9_]* {
    return TOK_NUMBER;
}

"@"{DATA_TYPE}{IDENTIFIER}(\.{IDENTIFIER}){0,2}(:{IDENTIFIER})? { yylval = * strdup(yytext); return TOK_RPCF_ADDR; }

{IDENTIFIER} { yylval = strdup(yytext); return TOK_ID; }

    /* Entry into Native ST Comment (* ... *) */
"(*"            { comment_depth = 1; BEGIN(ST_COMM); }

    /* Entry into C-Style Comment */
"/*"            { comment_depth = 1; BEGIN(C_COMM); }

"//"            { BEGIN(SINGLE_COMM); }

    /* Native ST Comment Logic */
<ST_COMM>{
    "(*"        { comment_depth++; }
    "*)"        { 
                  comment_depth--; 
                  if (comment_depth == 0) BEGIN(INITIAL); 
                }
    "/*"        { /* Optional: handle C-style nested inside ST */ }
    "*/"        { /* Optional: handle C-style nested inside ST */ }
    .|\n        { /* Ignore all other characters */ }
}

    /* C-Style Comment Logic */
<C_COMM>{
    "/*"        { comment_depth++; }
    "*/"        { 
                  comment_depth--; 
                  if (comment_depth == 0) BEGIN(INITIAL); 
                }
    "(*"        { /* Optional: handle ST nested inside C-style */ }
    "*)"        { /* Optional: handle ST nested inside C-style */ }
    .|\n        { /* Ignore all other characters */ }
}

    /* --- Single Line Comment // ... --- */
<SINGLE_COMM>{
    \n          { BEGIN(INITIAL); }
    .           { /* Ignore until end of line */ }
}

    /* --- Error Handling for Unclosed Comments --- */
<ST_COMM,C_COMM,SINGLE_COMM><<EOF>> {
    fprintf(stderr, "Error: Unclosed comment before end of file\n");
    yyterminate();
}

"<>"                return TOK_NEQU;
">="                return TOK_NLE;
"=>"                return TOK_OUTPUT_ASSIGN;
"<="                return TOK_NGT;
":="                return TOK_ASSIGN;
"="                 return TOK_EQUAL;
";"                 return TOK_SEMICOLON;
":"                 return TOK_COLON;
","                 return TOK_COMMA;
"+"                 return TOK_PLUS;
"-"                 return TOK_MINUS;
"**"                return TOK_POWER;
"*"                 return TOK_MULTIPLY;
"/"                 return TOK_DIVIDE;
"["                 return TOK_LBRACKET;
"]"                 return TOK_RBRACKET;
"{"                 return TOK_LBRACE; 
"}"                 return TOK_RBRACE;
"("                 return TOK_LPAREN;
")"                 return TOK_RPAREN;
"<"                 return TOK_LE;
">"                 return TOK_GT;
".."                return TOK_RANGE;
"."                 return TOK_DOT;
"#"                 return TOK_PUNC;
"^"                 return TOK_CARET;

[ \t\n]+        ; /* Ignore whitespace */
.               { fprintf(stderr, "Unknown character: %s\n", yytext); }

%%

int yywrap() {
    return 1;
}
