/*
 * =====================================================================================
 *
 *       Filename:  ridlc.lex
 *
 *    Description:  The lexical scanner definition for RPC IDL
 *
 *        Version:  1.0
 *        Created:  02/10/2021 05:01:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

%top{
#include <string>
#include <map>
#include "rpc.h"
#include <math.h>
#include <memory>

using namespace rpcfrmwrk;

#include "ridlc.h"

extern std::map< std::string, yytokentype > g_mapKeywords;

struct YYLTYPE2 :
    public YYLTYPE
{
    void initialize(
        const char* szFileName ) 
    {
        first_line = 1;
        first_column = 1;
        last_line = 1;
        last_column = 1;
    }
};

/* This is executed before every action. */
#define YY_USER_ACTION \
do{\
    YYLTYPE* ploc = curloc();\
    ploc->first_line = \
        ploc->last_line;\
    ploc->first_column = \
        ploc->last_column;\
    if( yylineno == ploc->last_line ) \
        ploc->last_column += yyleng;\
    else{ \
        for( ploc->last_column = 1;\
            yytext[ yyleng - ploc->last_column ] != '\n';\
            ++ploc->last_column ) {} \
        ploc->last_line = yylineno; \
    } \
    memcpy( yylloc, ploc, sizeof( YYLTYPE ) );\
}while( 0 );


struct FILECTX
{
    FILE*       m_fp = nullptr;
    BufPtr      m_pVal;
    std::string m_strPath;
    std::string m_strLastVal;
    YYLTYPE2    m_oLocation;

    FILECTX();
    FILECTX( const std::string& strPath );
    FILECTX( const FILECTX& fc );
    ~FILECTX();
};

extern std::vector< std::unique_ptr< FILECTX > > g_vecBufs;
typedef BufPtr YYSTYPE;

std::string& curstr();
std::string& curpath();
YYLTYPE* curloc();
CBuffer& newval();
yytokentype IsKeyword( char* szKeyword );
gint32 IsAllZero( char* szText );

#define DebugPrintMsg( ret, szMsg ) \
    DebugPrintEx( logErr, ret, \
        "%s(%d): %s", curpath().c_str(), \
        yylineno, szMsg );

#define PrintMsg( ret, szMsg ) \
    printf( "%s(%d): error %s(%d)\n", curpath().c_str(), \
        yylineno, szMsg, ret );

#define PrintAndQuit( ret, szMsg ) \
{ \
    PrintMsg( ret, szMsg )\
    yyterminate(); \
}

#define curval ( g_vecBufs.back()->m_pVal )
FILE* TryOpenFile( const std::string& strFile );

}

OctDig [0-7] 
HexDig [0-9a-fA-F]

%option     8bit bison-bridge
%option     bison-locations
%option     warn
%option     yylineno
%option     outfile="lexer.cpp"
%option     header-file="lexer.h"
%option     stack 
%option     noyywrap nounput 
/* %option     debug */

%x incl readstr c_comment
%%

";"     |
"["     |
"]"     |
"{"     |
"}"     |
","     |
"="     |
"("     |
")"     |
"<"     |
">"    {
        
        newval() = ( guint8 )yytext[ 0 ];
        *yylval = curval;
        return yytext[ 0 ];
    }

[[:blank:]]* /*eat*/

"'\\.'" {
        guint8 c = yytext[ 2 ];
        if( c == 'a' )
            c = '\a';
        else if( c == 'b' )
            c = '\b';
        else if( c == 't' )
            c = '\t';
        else if( c == 'n' )
            c = '\n';
        else if( c == 'v' )
            c = '\v';
        else if( c == 'f' )
            c = '\f';
        else if( c == 'r' )
            c = '\r';
        newval() = c;
        *yylval = curval;
        return TOK_BYVAL;
    }

'([^'\\\n]|\\.)' {
        guint8 c = yytext[ 1 ];
        newval() = c;
        *yylval = curval;
        return TOK_BYVAL;
    }

^include[[:blank:]]+\" {
        /*adapted from the info page*/
        yy_push_state(incl);
        yy_push_state( readstr );
        curstr().clear();
        ( yy_top_state() );
    }

<incl>[^"\n]*\n { /* got the include file name */
        std::string& strFile = curstr();
        if( strFile.empty() )
        {
            gint32 ret = -EINVAL;
            PrintAndQuit( ret, "Expect \"" );
        }

        yyin = TryOpenFile( strFile.c_str() );
        if ( !yyin )
        {
            PrintAndQuit(
                -errno, strerror( errno ) );
        }

        FILECTX* pfc = new FILECTX();
        pfc->m_strPath = strFile;
        pfc->m_fp = yyin;

        g_vecBufs.push_back(
            std::unique_ptr< FILECTX >( pfc ) );
        yypush_buffer_state(
            yy_create_buffer( yyin, YY_BUF_SIZE ) );
        yy_pop_state();
    }

\"  {
        curstr().clear();
        yy_push_state( readstr );
    }

<readstr>\"  { /* saw closing quote - all done */
        yy_pop_state();
        /* return string constant token type and
         * value to parser
         */
        if( YY_START != incl )
        {
            newval() = curstr();
            *yylval = curval;
            return TOK_STRVAL;
        }
        else
        {
            if( curstr().empty() )
            {
                gint32 ret = -EINVAL;
                PrintAndQuit( ret,
                    "error empty file name" );
            }
        }
     }

<readstr>\n        {
         /* error - unterminated string constant */
         PrintAndQuit( -EINVAL,
            "unterminated string constant" );
     }

<readstr>\\[0-7]{1,3} {
         /* octal escape sequence */
         gint32 result = 0x10000;
         (void) sscanf( yytext + 1, "%o", &result );
         if ( result > 0xff )
         {
            /* error, constant is out-of-bounds */
            PrintAndQuit( -EINVAL,
                "Invalid octal number" );
         }

         curstr().push_back( result );
     }

<readstr>\\[0-9]+ {
     /* generate error - bad escape sequence; something
      * like '\48' or '\0777777'
      */
        PrintAndQuit( -EINVAL,
            "bad escape sequence" );
     }

<readstr>\\n  curstr().push_back( '\n' );
<readstr>\\t  curstr().push_back( '\t' );
<readstr>\\r  curstr().push_back( '\r' );
<readstr>\\b  curstr().push_back( '\b' );
<readstr>\\f  curstr().push_back( '\f' );

<readstr>\\(.|\n)  curstr().push_back( yytext[1] );

<readstr>[^\\\n\"]+        {
         char *yptr = yytext;
         while( *yptr )
             curstr().push_back( *yptr++ );
     }

\/\/.*\n        /* c++ comment line */

0(OctDig+) {
        guint32 iLen = strlen( yytext + 1 );
        if( iLen * 3 > 64 )
        {
            PrintAndQuit( -ERANGE,
                "error octal value out of range"
                );
        }
        if( iLen * 3 > 32 )
        {
            guint64 iVal = strtoull(
                yytext, nullptr, 8);
            newval() = iVal;
            *yylval = curval;
            return TOK_INTVAL;
        }

        guint32 iVal = strtoul(
            yytext, nullptr, 8);
        newval() = iVal;
        *yylval = curval;
        return TOK_INTVAL;
    }

0(OctDig)+ {
        guint32 iLen = strlen( yytext + 1 );
        if( iLen * 3 > 64 )
        {
            PrintAndQuit( -ERANGE,
                "error octal value out of range"
                );
        }
        if( iLen * 3 > 32 )
        {
            guint64 iVal = strtoull(
                yytext + 1, nullptr, 8);
            newval() = iVal;
            *yylval = curval;
            return TOK_INTVAL;
        }

        guint32 iVal = strtoul(
            yytext + 1, nullptr, 8);
        newval() = iVal;
        *yylval = curval;
        return TOK_INTVAL;
    }

0[xX](HexDig)+ {
        guint32 iLen = strlen( yytext + 2 );
        if( iLen > 16 )
        {
            PrintAndQuit( -ERANGE,
                "error hex value out of range"
                );
        }
        if( iLen > 8 )
        {
            guint64 iVal = strtoull(
                yytext + 2, nullptr, 16 );
            newval() = iVal;
            *yylval = curval;
            return TOK_INTVAL;
        }

        guint32 iVal = strtoul(
            yytext + 2, nullptr, 16 );
        newval() = iVal;
        *yylval = curval;
        return TOK_INTVAL;
    }

[+-]?[1-9][[:digit:]]*.[[:digit:]]*[Ee][+-]?[1-9][[:digit:]]* {
        double dblVal = strtod( yytext, nullptr );
        if( dblVal == HUGE_VAL ||
            dblVal == -HUGE_VAL )
        {
            PrintAndQuit( -ERANGE,
                "error float value overflow" );
        }
        newval() = dblVal;
        *yylval = curval;
        return TOK_DBLVAL;
    }
[+-]?[0]?.[[:digit:]]*[Ee][+-]?[1-9][[:digit:]]* {
        gint32 ret = IsAllZero( yytext );
        if( SUCCEEDED( ret ) )
        {
            PrintAndQuit( -EINVAL,
                "error base value is zero" );
        }
        double dblVal = strtod( yytext, nullptr );
        if( dblVal == HUGE_VAL ||
            dblVal == -HUGE_VAL )
        {
            PrintAndQuit( -ERANGE,
                "error float value overflow" );
        }
        newval() = dblVal;
        *yylval = curval;
        return TOK_DBLVAL;
    }

[+-]?[1-9][[:digit:]]* {
        decltype( yytext ) yptr = yytext;
        bool bNegative = false;
        if( *yptr == '+' )
            ++yptr;
        else if( *yptr == '-' )
            bNegative = true;

        guint32 iLen = strlen( yptr );
        if( bNegative )
            iLen -= 1;

        double digits = 64 *.3010;
        if( iLen > digits )
        {
            PrintAndQuit( -ERANGE,
                "error hex value out of range" );
        }
        else if( ( iLen << 1 ) > digits )
        {
            gint64 iVal = strtoll(
                yptr, nullptr, 10 );
            newval() = ( guint64 )iVal;
            *yylval = curval;
            return TOK_INTVAL;
        }

        gint32 iVal = strtol(
            yptr, nullptr, 10 );
        newval() = ( guint32 )iVal;
        *yylval = curval;
        return TOK_INTVAL;
    }

[[:alpha:]][[:alnum:]_]* {
        yytokentype iKey = IsKeyword( yytext );
        if( iKey != TOK_INVALID )
        {
            newval() = ( guint32 )iKey;
            *yylval = curval;
            return iKey;
        }

        if( strcmp( yytext, "true" ) == 0 )
        {
            newval() = true;
            *yylval = curval;
            return TOK_BOOLVAL;
        }
        else if( strcmp( yytext, "false" ) == 0 )
        {
            newval() = false;
            *yylval = curval;
            return TOK_BOOLVAL;
        }

        std::string strIdent = yytext;
        newval() = strIdent;
        *yylval = curval;
        return TOK_IDENT;
    }

0   {
        newval() = ( guint32 )0;
        *yylval = curval;
        return TOK_INTVAL;
    }

"/*"         yy_push_state(c_comment);
<c_comment>[^*\n]*        /* eat anything that's not a '*' */
<c_comment>"*"+[^*/\n]* /* eat up '*'s not * followed by * '/'s */
<c_comment>\n    
<c_comment>"*"+"/" yy_pop_state();

[\n]      /**/

<<EOF>> {
        g_vecBufs.pop_back();
        yypop_buffer_state();
        if ( !YY_CURRENT_BUFFER)
        {
            return YYEOF;
        }

        YYLTYPE* ploc = curloc();
        memcpy( yylloc,
            ploc, sizeof( YYLTYPE ) );
        yylineno = ploc->last_line;
    }

[.]+  {

        std::string strMsg = "error unknown token '";
        strMsg += yytext;
        strMsg += "'";
        PrintMsg( -EINVAL, strMsg.c_str() );
        return YYerror;
    }
%%

#include "ridlc.h"

std::vector< std::unique_ptr< FILECTX > > g_vecBufs;

std::map< std::string, yytokentype >
    g_mapKeywords = {
        { "string", TOK_STRING }, 
        { "uint64", TOK_UINT64 }, 
        { "int64", TOK_INT64 }, 
        { "uint32", TOK_UINT32 }, 
        { "int32", TOK_INT32 }, 
        { "uint16", TOK_UINT16 }, 
        { "int16", TOK_INT16 }, 
        { "float", TOK_FLOAT }, 
        { "double", TOK_DOUBLE }, 
        { "byte", TOK_BYTE },
        { "bool", TOK_BOOL },
        { "bytearray", TOK_BYTEARR },
        { "array", TOK_ARRAY },
        { "map", TOK_MAP },
        { "ObjPtr", TOK_OBJPTR },
        { "HSTREAM", TOK_HSTREAM },

        { "async", TOK_ASYNC },
        { "async_p", TOK_ASYNCP },
        { "async_s", TOK_ASYNCS },
        { "event", TOK_EVENT },
        { "returns", TOK_RETURNS },
        //{ "stream", TOK_STREAM },
        { "serial", TOK_SERIAL },
        { "timeout", TOK_TIMEOUT },
        { "rtpath", TOK_RTPATH },
        { "ssl", TOK_SSL },
        { "websock", TOK_WEBSOCK },
        { "compress", TOK_COMPRES },
        { "auth", TOK_AUTH },

        { "struct", TOK_STRUCT }, 
        { "interface", TOK_INTERFACE }, 
        { "service", TOK_SERVICE },
        { "typedef", TOK_TYPEDEF },
        { "appname", TOK_APPNAME },
    };

FILECTX::FILECTX()
{
    m_pVal.NewObj(); 
    m_oLocation.first_line =
    m_oLocation.first_column =  
    m_oLocation.last_line =
    m_oLocation.last_column = 1;
}

FILECTX::FILECTX( const std::string& strPath )
    : FILECTX()
{
    m_fp = fopen( strPath.c_str(), "r");
    if( m_fp != nullptr )
    {
        m_strPath = strPath;
    }
    else
    {
        std::string strMsg = "cannot open file '";
        strMsg += strPath + "'";
        throw std::invalid_argument( strMsg );
    }
}

FILECTX::~FILECTX()
{
    if( m_fp != nullptr )
    {
        fclose( m_fp );
        m_fp = nullptr;
    }
}

FILECTX::FILECTX( const FILECTX& rhs )
{
    m_fp = rhs.m_fp;
    m_pVal = rhs.m_pVal;
    m_strPath = rhs.m_strPath;
    m_strLastVal = rhs.m_strLastVal;
    memcpy( &m_oLocation,
        &rhs.m_oLocation,
        sizeof( m_oLocation ) );
}

std::string& curstr()
{ return g_vecBufs.back()->m_strLastVal; }

std::string& curpath()
{ return g_vecBufs.back()->m_strPath; }

YYLTYPE* curloc()
{ return &g_vecBufs.back()->m_oLocation; }

CBuffer& newval()
{ 
    g_vecBufs.back()->m_pVal.NewObj();
    return *g_vecBufs.back()->m_pVal;
}

yytokentype IsKeyword( char* szKeyword )
{
    std::string strKey = szKeyword;

    std::map< std::string, yytokentype >::iterator
        itr = g_mapKeywords.find( strKey );
    if( itr == g_mapKeywords.end() )
        return TOK_INVALID;
    return itr->second;
}

gint32 IsAllZero( char* szText )
{
    std::string strFloat = szText;
    size_t posPt =
        strFloat.find_first_of( '.' );
    if( posPt == std::string::npos )
        return -ENOENT;

    size_t posExp =
        strFloat.find_first_of( "eE");
    if( posExp == std::string::npos )
        return -ENOENT;

    if( posPt >= strFloat.size() ||
        posExp >= strFloat.size() )
        return -ENOENT;

    if( posPt + 1 == posExp )
        return -ENOENT;

    if( posPt + 1 < posExp )
    {
        for( size_t i = posPt + 1;
            i < posExp; ++i)
        {
            if( strFloat[ i ] != '0' )
                return 0;
        }
        return ERROR_FALSE;
    }
    return -ENOENT;
}
