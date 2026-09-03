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
 * @brief Extract string from token value
 */
inline std::string GetString( const YYSTYPE& oVal )
{
    if( oVal == nullptr )
        return "";
    return ( stdstr& )oVal->first;
}

/**
 * @brief Extract number from token value
 */
inline gint32 GetNumber( const YYSTYPE& oVal )
{
    if( oVal == nullptr )
        return 0;
    Variant oVar = oVal->first;
    // Use GetTypeId to check the type
    gint32 iType = oVar.GetTypeId();
    // Try to get numeric value
    // The actual implementation depends on the Variant type
    // For now, return 0 as placeholder
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

#define NUM(tok) GetNumber(tok)

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
