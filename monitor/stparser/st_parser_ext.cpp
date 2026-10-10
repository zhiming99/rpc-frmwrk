/*
 * =====================================================================================
 *
 *       Filename:  st_parser_ext.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/19/2026 01:16:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "st_parser_ext.h"

typedef enum : int {
    enumSignedInt = 0,
    enumUnsignedInt,
    enumFloat,
    enumMultibit,
    enumBool,
    enumString,
    enumDate,
    enumTime,

}EnumTypeIdx;

static std::vector< std::vector< int > > g_vecElemTypes = {
    {
        stparser::TOK_INT_TYPE,
        stparser::TOK_DINT,
        stparser::TOK_LINT,
        stparser::TOK_SINT,
    },
    
    {
        stparser::TOK_UINT,
        stparser::TOK_UDINT,
        stparser::TOK_ULINT,
        stparser::TOK_USINT,
    },
    {
        stparser::TOK_REAL_TYPE,
        stparser::TOK_LREAL_TYPE,
    },
    {
        stparser::TOK_BYTE,
        stparser::TOK_WORD,
        stparser::TOK_DWORD,
        stparser::TOK_LWORD,
    },
    {
        stparser::TOK_BOOL,
    },
    {
        stparser::TOK_STRING_TYPE,
        stparser::TOK_WSTRING_TYPE,
        stparser::TOK_USTRING_TYPE,
        stparser::TOK_CHAR_TYPE,
        stparser::TOK_WCHAR_TYPE,
        stparser::TOK_UCHAR_TYPE
    },
    {
        stparser::TOK_TIME_TYPE,
        stparser::TOK_LTIME_TYPE,
        stparser::TOK_DATE_TYPE,
        stparser::TOK_LDATE_TYPE,
    },
};

bool CStParser::IsElementaryType(int tokenType)
{
    for (const auto& vec : g_vecElemTypes) {
        for (int t : vec) {
            if (t == tokenType) {
                return true;
            }
        }
    }
    return false;
}

bool CStParser::IsStringType(int tokenType)
{
    return tokenType == stparser::TOK_STRING_TYPE ||
           tokenType == stparser::TOK_WSTRING_TYPE ||
           tokenType == stparser::TOK_USTRING_TYPE ||
           tokenType == stparser::TOK_CHAR_TYPE ||
           tokenType == stparser::TOK_WCHAR_TYPE ||
           tokenType == stparser::TOK_UCHAR_TYPE;
}

bool CStParser::IsElemTypeName()
{
    // Use the stored token stream
    antlr4::TokenStream* pTokens= _input;
    if (!pTokens) return false;

    while( true )
    {
        antlr4::Token* tok = pTokens->LT(1);
        if( !tok || tok->getType() == antlr4::Token::EOF)
            break;
    
        if( tok->getChannel() !=
            antlr4::Token::DEFAULT_CHANNEL )
            continue;

        if( IsElementaryType( tok->getType() ) )
            return true;
        break;
    }
    return false;
}

void CStParser::PredictVarDeclInitSimpleSet()
{
    CStParseContext* pCtx = m_pContext;
    if (!pCtx) return;

    // Use the stored token stream
    antlr4::TokenStream* pTokens= _input;
    if (!pTokens) return;

    auto* pStream =
        dynamic_cast<antlr4::CommonTokenStream*>(pTokens);

    if (pStream != nullptr)
        pStream->fill();

    do{
        int i = 1;
        antlr4::Token* tok = pTokens->LT(i++);
        if( !tok || tok->getType() == antlr4::Token::EOF)
            break;
        
        if ( tok->getChannel() !=
            antlr4::Token::DEFAULT_CHANNEL )
            continue;

        if( IsElementaryType( tok->getType() ) )
        {
            // possibly a simple type or a subrange type
            antlr4::Token* nextTok = pTokens->LT(i++);
            if( !nextTok || nextTok->getType() == antlr4::Token::EOF)
                break;
            if( nextTok->getChannel() != antlr4::Token::DEFAULT_CHANNEL )
                continue;

            if( nextTok->getType() == stparser::TOK_LPAREN )
            {
                // subrange_spec
                pCtx->m_iPendingSpec =
                    EnumSimpleSpecType::SubrangeSpec;
            }
            else
            {
                // simple_spec
                pCtx->m_iPendingSpec =
                    EnumSimpleSpecType::SimpleSpec;
            }
        }
        else if( tok->getType() == stparser::TOK_REF_TO )
        {
            pCtx->m_iPendingSpec =
                EnumSimpleSpecType::RefSpec;
        }
        else if( IsStringType( tok->getType() ) )
        {
            pCtx->m_iPendingSpec =
                EnumSimpleSpecType::StringSpec;
        }
        else if( tok->getType() == stparser::TOK_ID ||
                 tok->getType() == stparser::TOK_DOT ||
                 tok->getType() == stparser::TOK_UNDERSCORE )
        {
            // derived type 
            pCtx->m_iPendingSpec =
                EnumSimpleSpecType::Unknown;
            pCtx->m_strPendingTypeName = tok->getText();
        }
        else
        {
            pCtx->m_iPendingSpec =
                EnumSimpleSpecType::Invalid;
        }
        break;
    }while(1);
    return;
}


void CStParser::PredictVarDeclInit()
{
    CStParseContext* pCtx = m_pContext;
    if (!pCtx) return;

    // Use the stored token stream
    antlr4::TokenStream* tokens = _input;
    if (!tokens) return;

    auto* pStream =
        dynamic_cast<antlr4::CommonTokenStream*>(tokens);

    if (pStream != nullptr)
        pStream->fill();

    // We need to find the type name. The pattern is:
    // variable_list TOK_COLON <type_name> ...
    // Look ahead from the start position to find COLON

    // Look ahead to find COLON (limit search to reasonable distance)
    
    int i = 1;
    do{
        antlr4::Token* tok = _input->LT(i++);
        if( !tok || tok->getType() == antlr4::Token::EOF)
            break;
        
        if( tok->getChannel() != antlr4::Token::DEFAULT_CHANNEL )
            continue;

        if( tok->getType() != stparser::TOK_COLON )
            continue;

        // Found COLON, next token should be the type name
        std::string strTypeName; 
        antlr4::Token* nextTok = tokens->LT(i++);
        // Skip whitespace/hidden tokens
        while(nextTok && nextTok->getType() != antlr4::Token::EOF)
        {
            if (nextTok->getChannel() == antlr4::Token::DEFAULT_CHANNEL && 
                nextTok->getType() != antlr4::Token::EPSILON) {
                strTypeName = nextTok->getText();
                break;
            }
            nextTok = tokens->LT(i++);
        }

        // Resolve the type
        if( !nextTok || nextTok->getType() == antlr4::Token::EOF)
            break;

        if( IsElementaryType(nextTok->getType()) )
        {
            pCtx->m_iPendingCategory =
                EnumTypeCategory::Simple;
        }
        else if( nextTok->getType() == stparser::TOK_ARRAY )
        {
            pCtx->m_iPendingCategory =
                EnumTypeCategory::Array;
        }
        else if( nextTok->getType() == stparser::TOK_STRUCT )
        {
            pCtx->m_iPendingCategory =
                EnumTypeCategory::Struct;
        }
        else if( nextTok->getType() ==
            stparser::TOK_FUNCTION_BLOCK )
        {
            pCtx->m_iPendingCategory =
                EnumTypeCategory::FunctionBlock;
        }
        else if( nextTok->getType() == stparser::TOK_NULL )
        {
            pCtx->m_iPendingCategory =
                EnumTypeCategory::Interface;
        }
        else if( nextTok->getType() == stparser::TOK_ID ||
            nextTok->getType() == stparser::TOK_DOT ||
            nextTok->getType() == stparser::TOK_UNDERSCORE )
        {
            // derived type, need to resolve from symbol table
            pCtx->m_iPendingCategory =
                EnumTypeCategory::Ambiguous;
            pCtx->m_strPendingTypeName = strTypeName;
        }
        else
        {
            pCtx->m_iPendingCategory =
                EnumTypeCategory::Invalid;
        }

        pCtx->m_strPendingTypeName = strTypeName;

        std::cout << "[PredictVarDeclInitSet] Resolved type '"
            << strTypeName 
            << "' to category "
            << static_cast<int>(pCtx->m_iPendingCategory) 
            << std::endl;
        break;
    }while(1);
}
