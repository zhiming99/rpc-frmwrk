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
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <rpc.h>

struct YYLTYPE1
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

#define YYLTYPE YYLTYPE1

struct YYLTYPE2 :
    public YYLTYPE
{
    stdstr text;
    int fidx = -1; // file name index into the m_vecFiles.
    void initialize()
    {
        first_line = 1;
        first_column = 1;
        last_line = 1;
        last_column = 1;
    }

    YYLTYPE2( const YYLTYPE2& rhs )
    {
        first_line = rhs.first_line;
        first_column = rhs.first_column;
        last_line = rhs.last_line;
        last_column = rhs.last_column;
        text = rhs.text;
    }
    YYLTYPE2()
    {
        initialize();
        fidx = -1;
    }
};

typedef std::pair< rpcf::Variant, YYLTYPE2 > YYSPAIR;
#define YYSTYPE std::shared_ptr< YYSPAIR >

#define YY_DECL int yylex(YYSTYPE * yylval_param, \
    YYLTYPE * yylloc_param, yyscan_t yyscanner, \
    void* pctx )

extern int yylex(YYSTYPE * yylval_param,
    YYLTYPE * yylloc_param, void* yyscanner,
    void* plexctx );

#define pLCtx ((rpcf::CSTParserContext*)pctx)

}

%{

#include "parsrctx.h"
#include "stparser.h"
#include "stbase.h"

using namespace rpcf;

#define YY_USER_ACTION \
    do {  \
        if( yylloc == nullptr ) break; \
        yylloc->first_line = yylloc->last_line = yyget_lineno(yyscanner); \
        yylloc->first_column = yyget_column(yyscanner); \
        yylloc->last_column = yyget_column(yyscanner) + yyleng - 1; \
        auto yylloc2 = static_cast< YYLTYPE2* >( yylloc ); \
        yylloc2->text = yyget_text( yyscanner );\
        yylloc2->fidx = pLCtx->GetFileIdx( pLCtx->m_strCurFile ); \
        yyset_column(yyget_column(yyscanner) + yyleng, yyscanner); \
        if( yylval != nullptr ) yylval->get()->second = *yylloc2; \
    }while( 0 );

int comment_depth = 0;
void lexerr(const char *s) {
    fprintf(stderr, "Error: %s\n", s);

}

%}

%option     nounistd
%option     8bit bison-bridge
%option     bison-locations
%option     warn
%option     reentrant
%option     yylineno
%option     stack 
%option     noyywrap


 /* Definitions */
ST_TYPE     (?i:BOOL|BYTE|WORD|DWORD|LWORD|SINT|INT|DINT|LINT|USINT|UINT|UDINT|ULINT)
REAL_TYPE   (?i:REAL|LREAL)
DIGIT       [0-9]
HEX_DIGIT   [0-9a-fA-F]
UNSIGNED_INT ({DIGIT}+(_?{DIGIT}+)*)
SIGNED_INT  ([-+]?{UNSIGNED_INT})

 /* Short and Long prefixes */
D_PREFIX    (?i:date|d)#
TOD_PREFIX  (?i:time_of_day|tod)#
DT_PREFIX   (?i:date_and_time|dt)#

ADDR_AREA   (?i:[IQM])
ADDR_SIZE   (?i:[XBWDL])
LETTER      [a-zA-Z]
IDENTIFIER  ({LETTER}|("_"({LETTER}|{DIGIT})))("_"?({LETTER}|{DIGIT}))*

 /* point type: I=input, O=output, S=setpoint */
ATTR_DATA_TYPE   (?i:[IOS][BWDQSOF])
/* Define Exclusive States */
%x ST_COMM C_COMM SINGLE_COMM


%%

 // (?i:TIME|T|TO)#-?([0-9]+(\.[0-9]+)?(_?[0-9]+)*(d|h|m|s|ms|us|ns))+ {
(?i:T|TIME|TO)#-?([0-9]+(\.[0-9]+)?d)?([0-9]+(\.[0-9]+)?h)?([0-9]+(\.[0-9]+)?m)?([0-9]+(\.[0-9]+)?s)?([0-9]+(\.[0-9]+)?ms)? {
    /* Copy the matched text to pass to Bison */
    yylval->get()->first = yytext; 
    return TOK_TIME;
}
(?i:LT|LTIME|LTO)#-?([0-9]+(\.[0-9]+)?d)?([0-9]+(\.[0-9]+)?h)?([0-9]+(\.[0-9]+)?m)?([0-9]+(\.[0-9]+)?s)?([0-9]+(\.[0-9]+)?ms)?([0-9]+(\.[0-9]+)?us)?([0-9]+(\.[0-9]+)?ns)? {
    /* Copy the matched text to pass to Bison */
    yylval->get()->first = yytext; 
    return TOK_LTIME;
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
(?i:ELSIF)                  return TOK_ELSIF;
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
(?i:PERSISTENT)             return TOK_PERSISTENT;
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
(?i:NAMESPACE)              return TOK_NAMESPACE;
(?i:END_NAMESPACE)          return TOK_END_NAMESPACE;
(?i:USING)                  return TOK_USING;
(?i:VAR_STAT)               return TOK_VAR_STAT;


 /* Matches D#2024-12-25 */
{D_PREFIX}[0-9]{4}-[0-9]{2}-[0-9]{2} {
    yylval->get()->first = ( guint64 )ParseStTimeToUnix( yytext );
    return TOK_DATE;
}

 /* Matches TOD#14:30:05.123 */
{TOD_PREFIX}[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)? {
    yylval->get()->first = ( guint64 )ParseStTimeToUnix( yytext );
    return TOK_TIME_OF_DAY;
}

 /* Matches DT#2024-12-25-14:30:05 */
{DT_PREFIX}[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)? {
    yylval->get()->first = ( guint64 )ParseStTimeToUnix( yytext );
    return TOK_DATE_TIME;
}

 /* Standard STRING literal */
'(''|[^'\n]|(\$[0-9a-fA-F]{2})|\$[LlPpRrTtQ$])*' {
    // Action: process string (strip quotes, resolve escapes)
    yylval->get()->first = TranslateSTString( yytext );
    return TOK_STRING;
}

 /* WSTRING literal */
\"(\"\"|[^\"\n]|(\$[0-9a-fA-F]{4})|\$[LlPpRrTtQ$])*\" {
    lexerr( "wide character string are not supported, yet" );
}

 /* Error handling for unterminated strings */
'[^'\n]* { lexerr("Unterminated string literal"); }
\"[^\"\n]* { lexerr("Unterminated wide string literal"); }


 /* Typed REAL/LREAL literal per EBNF:
    Real_Type_Name: REAL | LREAL
    Real_Type_Name '#' Signed_Int ( '.' Unsigned_Int ( 'E' Signed_Int )? )?
    e.g. REAL#10, LREAL#-1.5, REAL#1.5E-3 */
{REAL_TYPE}#{SIGNED_INT}("."{UNSIGNED_INT}("E"{SIGNED_INT})?)? {
    std::string s(yytext);
    size_t hashPos = s.find('#');
    std::string type = s.substr(0, hashPos);
    std::string valStr = s.substr(hashPos + 1);
    /* strip the underscores allowed between digits */
    valStr.erase(std::remove(
        valStr.begin(), valStr.end(), '_'), valStr.end());
    try
    {
        double dVal = std::stod( valStr );
        /* LREAL# is double precision, REAL# is single */
        if( tolower( type[ 0 ] ) == 'l' )
            yylval->get()->first = dVal;
        else
            yylval->get()->first = ( float )dVal;
    }
    catch( ... )
    {
        lexerr( "invalid real literal" );
        return YYerror;
    }
    return TOK_NUMBER;
}

 /* Typed integer literals: INT#10, INT#-5, DWORD#16#FF, WORD#2#1010 */
{ST_TYPE}#{SIGNED_INT} |
{ST_TYPE}#{DIGIT}+#({HEX_DIGIT}+(_?{HEX_DIGIT}+)*) {
    std::string s(yytext);
    size_t hashPos = s.find('#');
    std::string valStr = s.substr(hashPos + 1);
    /* strip the underscores allowed between digits */
    valStr.erase(std::remove(
        valStr.begin(), valStr.end(), '_'), valStr.end());
    gint32 iVal = 0;
    try
    {
        size_t basePos = valStr.find('#');
        if( basePos != std::string::npos )
        {
            /* based literal, the base must be 2, 8, 10 or 16 */
            gint32 iBase = std::stoi( valStr.substr( 0, basePos ) );
            std::string strDigits = valStr.substr( basePos + 1 );
            if( iBase != 2 && iBase != 8 && iBase != 10 &&
                iBase != 16 )
            {
                lexerr( "unsupported numeric base" );
                return YYerror;
            }
            size_t pos = 0;
            iVal = ( gint32 )std::stol( strDigits, &pos, iBase );
            if( pos != strDigits.size() )
            {
                lexerr( "invalid digits for the numeric base" );
                return YYerror;
            }
        }
        else
        {
            iVal = std::stoi( valStr );
        }
    }
    catch( ... )
    {
        lexerr( "invalid integer literal" );
        return YYerror;
    }
    yylval->get()->first = ( guint32 )iVal;
    return TOK_NUMBER;
}

 /* Matches Peripheral Access (PI): %IW0:P or %PIW0 */
"%"[Pp]?{ADDR_AREA}{ADDR_SIZE}?{DIGIT}+("."{DIGIT}+|"*")?(":P"|":p")?  {
    ObjPtr pAddr = ParsePeriAddr( yytext, pLCtx );
    if( pAddr.IsEmpty() )
        return YYerror;
    yylval->get()->first = pAddr;
    return TOK_ABS_ADDR_PERIPHERAL;
}


 /* 2. Based Literals (e.g., 16#FF) */
 /* This wins if it sees digits followed by "#" */
{DIGIT}+#[0-9a-fA-F_]+ {
    std::string s(yytext);
    size_t hashPos = s.find('#');
    std::string strDigits = s.substr(hashPos + 1);
    /* strip the underscores allowed between digits */
    strDigits.erase(std::remove(
        strDigits.begin(), strDigits.end(), '_'), strDigits.end());
    try
    {
        gint32 iBase = std::stoi( s.substr( 0, hashPos ) );
        if( iBase != 2 && iBase != 8 && iBase != 10 && iBase != 16 )
        {
            lexerr( "unsupported numeric base" );
            return YYerror;
        }
        size_t pos = 0;
        gint32 iVal = ( gint32 )std::stol( strDigits, &pos, iBase );
        if( pos != strDigits.size() )
        {
            lexerr( "invalid digits for the numeric base" );
            return YYerror;
        }
        yylval->get()->first = ( guint32 )iVal;
    }
    catch( ... )
    {
        lexerr( "invalid integer literal" );
        return YYerror;
    }
    return TOK_NUMBER;
}

 /* 3. Real Literals (e.g., 1.5, 1.5E-3) per EBNF:
    Real_Literal: Signed_Int '.' Unsigned_Int ( 'E' Signed_Int )?
    Design note: the lexer intentionally does NOT take the leading
    sign of Signed_Int. If it did, flex longest-match would turn
    '3-1.5' into '3' '-1.5' and swallow the binary minus. A negative
    real literal (e.g. '-1.5') is therefore composed by the parser's
    unary minus (TOK_MINUS), same as integer literals. The exponent
    sign stays in the token since 'E' is unambiguous. */
{UNSIGNED_INT}"."{UNSIGNED_INT}("E"{SIGNED_INT})? {
    std::string s(yytext);
    /* strip the underscores allowed between digits */
    s.erase(std::remove(
        s.begin(), s.end(), '_'), s.end());
    try
    {
        /* an untyped real literal defaults to REAL (single) */
        yylval->get()->first = ( float )std::stod( s );
    }
    catch( ... )
    {
        lexerr( "invalid real literal" );
        return YYerror;
    }
    return TOK_NUMBER;
}

 /* 4. Simple Decimal (e.g., 123) */
 /* This is the fallback for any sequence of digits without a "#" */
[0-9][0-9_]* {
    std::string s(yytext);
    s.erase(std::remove(
        s.begin(), s.end(), '_'), s.end());
    gint32 iVal = 0;
    try
    {
        iVal = std::stoi( s );
    }
    catch( ... )
    {
        lexerr( "invalid integer literal" );
        return YYerror;
    }
    yylval->get()->first = ( guint32 )iVal;
    return TOK_NUMBER;
}

"@"{ATTR_DATA_TYPE}{IDENTIFIER}(\.{IDENTIFIER}){0,2}(:{IDENTIFIER})? {
     ObjPtr pAddr = ParseRpcfAddr( yytext, pLCtx );
     if( pAddr.IsEmpty() )
         return YYerror;
     yylval->get()->first = pAddr;
     return TOK_RPCF_ADDR;
 }

{IDENTIFIER} { yylval->get()->first = yytext; return TOK_ID; }

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
    .           {}
    "\n"        { yyset_column( 1, yyscanner ); }
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
    .           { /* Ignore all other characters */ }
    "\n"        { yyset_column( 1, yyscanner ); }
}

    /* --- Single Line Comment // ... --- */
<SINGLE_COMM>{
    "\n"          {
          BEGIN(INITIAL);
          yyset_column( 1, yyscanner );
    }
    .     { /* Ignore until end of line */ }
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
"+"                 return TOK_ADD;
"-"                 return TOK_MINUS;
"**"                return TOK_POWER;
"*"                 return TOK_MUL;
"/"                 return TOK_DIV;
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

[ \t]+        ; /* Ignore whitespace */
[\n]           { yyset_column( 1, yyscanner ); }
.              {
        fprintf(stderr, "Unknown character: %s\n", yytext);
        yyterminate();
        }

<<EOF>> {
    do{
        yypop_buffer_state( yyscanner );
        if ( !YY_CURRENT_BUFFER )
            break;

        if( pLCtx->m_vecFileStack.empty() )
            break;

        FILECTX2* pFileCtx =
            pLCtx->m_vecFileStack.back().get();
        YYLTYPE oloc = pFileCtx->m_oLocation;
        yyset_lineno( oloc.last_line, yyscanner );
        yyset_column( oloc.last_column, yyscanner );
        pLCtx->m_strCurFile = pFileCtx->m_strPath;
        pLCtx->m_vecFileStack.pop_back();
        // tell the caller to retry
        return YYEMPTY;
    }while( 0 );
    yyterminate();
}
%%

