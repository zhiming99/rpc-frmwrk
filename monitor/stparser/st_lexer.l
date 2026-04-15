%top{
#include <stdio.h>
#include "y.tab.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>

std::string process_st_string(const char* input);
typedef Variant YYSTYPE;

%}

 /* Definitions */
ST_TYPE     (BOOL|BYTE|WORD|DWORD|LWORD|SINT|INT|DINT|LINT|USINT|UINT|UDINT|ULINT|REAL|LREAL)
DIGIT       [0-9]
HEX_DIGIT   [0-9a-fA-F]

 /* Short and Long prefixes */
D_PREFIX    (date|d)#
TOD_PREFIX  (time_of_day|tod)#
DT_PREFIX   (date_and_time|dt)#

%%
(?i:STRUCT)         return TOK_STRUCT;
(?i:ARRAY)          return TOK_ARRAY;
(?i:INT)            return TOK_INT;
(?i:UINT)           return TOK_UINT;
(?i:DINT)           return TOK_DINT;
(?i:UDINT)          return TOK_UDINT;
(?i:SINT)           return TOK_SINT;
(?i:USINT)          return TOK_USINT;
(?i:BOOL)           return TOK_BOOL;
(?i:BYTE)           return TOK_BYTE;
(?i:WORD)           return TOK_WORD;
(?i:DWORD)          return TOK_DWORD;
(?i:REAL)           return TOK_REAL;
(?i:LREAL)          return TOK_LREAL;
(?i:PROGRAM)        return TOK_PROGRAM;
(?i:VAR)            return TOK_VAR;
(?i:END_VAR)        return TOK_END_VAR;
(?i:IF)             return TOK_IF;
(?i:THEN)           return TOK_THEN;
(?i:ELSE)           return TOK_ELSE;
(?i:ELSIF)          return TOK_ELSEIF;
(?i:END_IF)         return TOK_END_IF;
(?i:CASE)           return TOK_CASE;
(?i:OF)             return TOK_OF;
(?i:END_CASE)       return TOK_END_CASE;
(?i:FOR)            return TOK_FOR;
(?i:TO)             return TOK_TO;
(?i:DO)             return TOK_DO;
(?i:END_FOR)        return TOK_END_FOR;
(?i:WHILE)          return TOK_WHILE;
(?i:END_WHILE)      return TOK_END_WHILE;
(?i:REPEAT)         return TOK_REPEAT;
(?i:UNTIL)          return TOK_UNTIL;
(?i:END_REPEAT)     return TOK_END_REPEAT;
(?i:TON)            return TOK_TON;
(?i:MOD)            return TOK_MOD;
(?i:AND)            return TOK_AND;
(?i:OR)             return TOK_OR;
(?i:XOR)            return TOK_XOR;
(?i:NOT)            return TOK_NOT;

(TIME|T)#-?([0-9]+(\.[0-9]+)?(_?[0-9]+)*(d|h|m|s|ms|us|ns))+ {
    /* Copy the matched text to pass to Bison */
    yylval.strval = strdup(yytext); 
    return TOK_TIME;
}

 /* Matches D#2024-12-25 */
{D_PREFIX}[0-9]{4}-[0-9]{2}-[0-9]{2} {
    yylval.qwVal = parse_st_literal_to_unix( yytext );
    return TOK_DATE;
}

 /* Matches TOD#14:30:05.123 */
{TOD_PREFIX}[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)? {
    yylval.qwVal = parse_st_literal_to_unix( yytext );
    return TOK_TIME_OF_DAY;
}

 /* Matches DT#2024-12-25-14:30:05 */
{DT_PREFIX}[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)? {
    yylval.qwVal = parse_st_literal_to_unix( yytext );
    return TOK_DATE_TIME
}

 /* Standard STRING literal */
'(''|[^'\n]|(\$[0-9a-fA-F]{2})|\$[LlPpRrTtQ$])*' {
    // Action: process string (strip quotes, resolve escapes)
    return TOK_STRING;
}

 /* WSTRING literal */
\"(\"\"|[^\"\n]|(\$[0-9a-fA-F]{4})|\$[LlPpRrTtQ$])*\" {
    return TOK_WSTRING;
}

 /* Error handling for unterminated strings */
'[^'\n]* { yyerror("Unterminated string literal"); }
\"[^\"\n]* { yyerror("Unterminated wide string literal"); }


 /* Match Typed Literals like INT#10, DWORD#16#FFFF, REAL#10.5 */
{ST_TYPE}#([-+]?{DIGIT}+(_?{DIGIT}+)*(\.{DIGIT}+(_?{DIGIT}+)*)?) {
    yylval.strval = strdup(yytext); // Pass "INT#10" to Bison
    return TOK_TYPED_LITERAL;
}

 /* Match Typed Hex/Binary like WORD#16#FF or BYTE#2#1010 */
{ST_TYPE}#{DIGIT}+#({HEX_DIGIT}+(_?{HEX_DIGIT}+)*) {
    yylval.strval = strdup(yytext);
    return TOK_TYPED_LITERAL;
}


[0-9]+          { yylval = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval = strdup(yytext); return TOK_ID; }
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
"<>"                return TOK_NEQU;
"=>"                return TOK_NLE;
"<="                return TOK_NGT;

[ \t\n]+        ; /* Ignore whitespace */
.               { fprintf(stderr, "Unknown character: %s\n", yytext); }

%%

int yywrap() {
    return 1;
}
