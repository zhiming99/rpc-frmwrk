%top{
#include <string>
#include <map>
#include "rpc.h"
#include "token.h"

struct FILECTX
{
    FILE*       m_fp = nullptr;
    BufPtr      m_pVal;
    std::string m_strPath;
    std::string m_strLastVal;

    FILECTX();
    FILECTX( const std::string& strPath );
    ~FILECTX();
};

std::string& curstr();
std::string& curpath();
BufPtr& curval();
EnumToken IsKeyword( char* szKeyword );
gint32 IsAllZero( char* szText );

#define PrintAndQuit( ret, szMsg ) \
{ \
    DebugPrintEx( logErr, ret, \
        "%s(%d): %s", curpath().c_str(), \
        yylineno, szMsg ); \
    yyterminate(); \
}

%}

OctDig [0-7] 
HexDig [0-9a-fA-F]

%option     8bit bison-bridge
%option     bison-locations
%option     warn nodefault
%option     yylineno
%option     outfile="lexer.cpp"
%option     header-file="lexer.h"
%option     stack

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
        return yytext[ 0 ];
    }

[[:blank:]]* /*eat*/

^include[[:blank:]]+\" {
        /*adapted from the info page*/
        yy_push_state(incl);
        yy_push_state( readstr );
        curstr().clear();
    }

<incl>[^"\n]*\n { /* got the include file name */
        std::string& strFile = curstr();
        if( strFile.empty() )
        {
            gint32 ret = -EINVAL;
            PrintAndQuit( ret, "Expect \"" );
        }

        yyin = fopen( yytext, "r");
        if ( !yyin )
        {
            PrintAndQuit(
                -errno, strerror( ret ) );
        }

        FILECTX& curFc = g_vecBufs.back();
        FILECTX fc;
        fc.m_strPath = strFile;
        fc.m_fp = yyin;

        g_vecBufs.push_back( fc );
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
        if( yy_top_state() != incl )
        {
            *curval() = curstr();
            return tok_strval;
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
        printAndQuit( -EINVAL,
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
            *curval() = iVal;
            return tok_intval;
        }

        guint32 iVal = strtoul(
            yytext, nullptr, 8);
        *curval() = iVal;
        return tok_intval;
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
            *curval() = iVal;
            return tok_uint64;
        }

        guint32 iVal = strtoul(
            yytext + 1, nullptr, 8);
        *curval() = iVal;
        return tok_intval;
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
                yytext + 2, nullptr, 8);
            *curval() = iVal;
            return tok_uint64;
        }

        guint32 iVal = strtoul(
            yytext + 2, nullptr, 8);
        *curval() = iVal;
        return tok_intval;
    }

[+-]?[1-9][[:digit:]]*.[[:digit:]]*[Ee][+-]?[1-9][[:digit:]]* {
        double dblVal = strtod( yytext, nullptr );
        if( dblVal == HUGE_VAL ||
            dblVal == -HUGE_VAL )
        {
            PrintAndQuit( -ERANGE,
                "error float value overflow" );
        }
        *curval() = dblVal;
        return tok_dblval;
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
        *curval() = dblVal;
        return tok_dblval;
    }

[+-]?[1-9][[:digit:]]* {
        decltype( yytext ) yptr = yytext;
        bool bNegative = false;
        if( *yptr == '+' )
            ++yptr;
        else if( *yptr == '-' )
            bNegative = true;

        guint32 iLen = strlen( yptr );
        double digits = 64 *.3010;
        if( iLen > digits )
        {
            PrintAndQuit( -ERANGE,
                "error hex value out of range" );
        }
        else if( ( iLen << 1 ) > digits )
        {
            guint64 iVal = strtoull(
                yptr, nullptr, 8);
            *curval() = iVal;
            return tok_intval;
        }

        guint32 iVal = strtoul(
            yptr, nullptr, 8);
        *curval() = iVal;
        return tok_intval;
    }

[[:alpha:]][[:alnum:]_]* {
        EnumToken iKey = IsKeyword( yytext );
        if( iKey != tok_invalid )
            return iKey;

        if( strcmp( yytext, "true" ) == 0 )
        {
            *curval() = true;
            return tok_boolval;
        }
        else if( strcmp( yytext, "false" ) == 0 )
        {
            *curval() = false;
            return tok_boolval;
        }

        *curval() = yytext;
        return tok_ident;
    }

"'.'" {
        guint8 c = yytext[ 1 ];
        *curval() = c;
        return tok_byval;
    }

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
        *curval() = c;
        return tok_byval;
    }

0   {
        *curval() = ( guint32 )0;
        return tok_intval;
    }

"/*"         yy_push_state(c_comment);
<c_comment>[^*\n]*        /* eat anything that's not a '*' */
<c_comment>"*"+[^*/\n]* /* eat up '*'s not * followed by * '/'s */
<c_comment>\n    
<c_comment>"*"+"/" yy_pop_state();

.*  {

        std::string strMsg = "error unknown token '";
        strMsg += yytext;
        strMsg += "'";
        PrintAndQuit( -EINVAL,
            strMsg.c_str() );
    }

<<EOF>> {
        g_vecBufs.pop_back();
        yypop_buffer_state();
        if ( !YY_CURRENT_BUFFER)
        {
            yyterminate();
        }
    }

%%

std::vector< FILECTX > g_vecBufs;

FILECTX::FILECTX()
{ m_pVal.NewObj(); }

FILECTX::FILECTX( const std::string& strPath )
{
    m_fp = fopen( strPath.c_str(), "r");
    if( m_fp != nullptr )
    {
        m_strPath = strPath;
    }
}

FILECTX::~FILECTX()
{
    if( m_fp != nullptr )
        fclose( m_fp );
    m_pVal.Clear()
}
std::string& curstr()
{ return g_vecBufs.back().m_strLastVal; }

std::string& curpath()
{ return g_vecBufs.back().m_strPath; }

BufPtr& curval()
{ return g_vecBufs.back().m_pVal; }

EnumToken IsKeyword( char* szKeyword )
{
    std::string strKey = szKeyword;

    std::map< std::string, EnumToken >::iterator
        itr = g_mapKeywords.find( strKey );
    if( itr == g_mapKeywords.end() )
        return tok_invalid;
    return itr->second;
}

gint32 IsAllZero( char* szText )
{
    std::string strFloat = szText;
    size_t posPt =
        strFloat.find_first_of( '.' );
    if( posPt = std::string::npos )
        return -ENOENT;

    size_t posExp =
        strFloat.find_first_of( "eE");
    if( posExp = std::string::npos )
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

