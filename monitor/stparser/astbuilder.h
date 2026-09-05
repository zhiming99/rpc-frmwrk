/*
 * =====================================================================================
 *
 *       Filename:  astbuilder.h
 *
 *    Description:  AST building helpers for stparser.y
 *
 *        Version:  1.0
 *        Created:  09/01/2026
 *       Revision:  none
 *       Compiler:  gcc
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

#pragma once

#include "astfactory.h"
#include "stlexer.h"

namespace rpcf
{

/**
 * @brief Get location from token value
 */
inline YYLTYPE2 GetLocation( const YYSTYPE& oVal )
{
    if( oVal == nullptr )
        return YYLTYPE2();
    return oVal->second;
}

/**
 * @brief Get location from multiple tokens (from start to end)
 */
inline YYLTYPE2 GetLocationRange( const YYSTYPE& oStart, const YYSTYPE& oEnd )
{
    YYLTYPE2 oLoc;
    if( oStart != nullptr )
        oLoc = oStart->second;
    if( oEnd != nullptr )
    {
        oLoc.last_line = oEnd->second.last_line;
        oLoc.last_column = oEnd->second.last_column;
    }
    return oLoc;
}

/**
 * @brief Split a var_declarations accumulator by category
 *
 * The CStVarDeclNode::m_eCategory applied at block level decides
 * the target vector of each declaration. Categories without a
 * target vector (external/stat for now) are collected in vecOther
 * so nothing gets dropped silently.
 */
inline void SplitVarDeclList( ObjPtr pList,
    std::vector< ObjPtr >& vecInput,
    std::vector< ObjPtr >& vecOutput,
    std::vector< ObjPtr >& vecInOut,
    std::vector< ObjPtr >& vecLocal,
    std::vector< ObjPtr >& vecTemp,
    std::vector< ObjPtr >& vecOther )
{
    if( pList.IsEmpty() )
        return;
    CStVarDeclListNode* pNode = pList;
    if( pNode == nullptr )
        return;
    for( guint32 i = 0; i < pNode->m_vecVarDecls.size(); ++i )
    {
        ObjPtr pDecl = pNode->m_vecVarDecls[ i ];
        CStVarDeclNode* pVar = pDecl;
        if( pVar == nullptr )
            continue;
        switch( pVar->m_eCategory )
        {
        case CStVarDeclNode::vcInput:
            vecInput.push_back( pDecl );
            break;
        case CStVarDeclNode::vcOutput:
            vecOutput.push_back( pDecl );
            break;
        case CStVarDeclNode::vcInOut:
            vecInOut.push_back( pDecl );
            break;
        case CStVarDeclNode::vcLocal:
            vecLocal.push_back( pDecl );
            break;
        case CStVarDeclNode::vcTemp:
            vecTemp.push_back( pDecl );
            break;
        default:
            vecOther.push_back( pDecl );
            break;
        }
    }
    return;
}

/**
 * @brief Extract string from token value
 */
inline std::string GetString( const YYSTYPE& oVal )
{
    if( oVal == nullptr )
        return "";
    return ( stdstr& )oVal->first;
}

/**
 * @brief Extract an int_type number from token value
 *
 * Only integer-typed variants are accepted. A float or double value
 * is NOT silently truncated to int: contexts that require an
 * int_type number (e.g. the array length or the repetition count
 * 'n(value)') would otherwise hide the error. Instead the error is
 * reported via the parser context and 0 is returned.
 */
inline gint32 GetNumber( const YYSTYPE& oVal,
    CSTParserContext* pCtx = nullptr )
{
    if( oVal == nullptr )
        return 0;
    const Variant& oVar = oVal->first;
    switch( oVar.GetTypeId() )
    {
        case typeByte:
            return ( gint32 )( const guint8& )oVar;
        case typeUInt16:
            return ( gint32 )( const guint16& )oVar;
        case typeUInt32:
            return ( gint32 )( const guint32& )oVar;
        case typeUInt64:
            return ( gint32 )( const guint64& )oVar;
        default:
            break;
    }
    if( pCtx != nullptr )
    {
        const char* szType = "unknown";
        if( oVar.GetTypeId() == typeFloat )
            szType = "float";
        else if( oVar.GetTypeId() == typeDouble )
            szType = "double";
        fprintf( stderr,
            "Error: an int_type number is required, got %s at "
            "line %d, column %d\n", szType,
            oVal->second.first_line, oVal->second.first_column );
        pCtx->IncError();
    }
    return 0;
}

/**
 * @brief Extract boolean from token value
 */
inline bool GetBool( const YYSTYPE& oVal )
{
    if( oVal == nullptr )
        return false;
    // Convert variant to boolean
    Variant& oVar = oVal->first;
    // Use operator bool or similar
    return (bool)oVar;
}

/**
 * @brief Create a string from identifier token
 */
inline std::string GetIdentifier( const YYSTYPE& oVal )
{
    return GetString( oVal );
}

// Macros for parser use
#define GET_FACTORY(ctx) GetAstFactory(ctx)

#define LOC(tok) GetLocation(tok)

#define LOC_RANGE(start, end) GetLocationRange(start, end)

#define ID(tok) GetIdentifier(tok)

#define NUM(tok) GetNumber(tok, pCtx)

#define STR(tok) GetString(tok)

#define BOOL(tok) GetBool(tok)

// MAKE_VALUE - wrap value and location into YYSTYPE
inline YYSTYPE MakeValue( const Variant& oVal, const YYLTYPE2& oLoc )
{
    return YYSTYPE( new YYSPAIR( oVal, oLoc ) );
}

#define MAKE_VALUE(v, l) MakeValue(Variant(v), l)

// Check if variant is ObjPtr (using typeObj)
inline bool IsObjPtr( const Variant& oVar )
{
    return oVar.GetTypeId() == typeObj;
}

// Extract ObjPtr from Variant - use implicit conversion
inline ObjPtr VariantToObjPtr( const Variant& oVar )
{
    if( oVar.GetTypeId() != typeObj )
        return ObjPtr();
    return oVar;  // Uses implicit conversion operator
}

} // namespace rpcf
