/*
 * =====================================================================================
 *
 *       Filename:  stsymtab.cpp
 *
 *    Description:  implementations of CStSymbolTable, CStScope and CStSymbol classes
 *
 *        Version:  1.0
 *        Created:  10/16/2026 12:08:49 PM
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

#include "stsymtab.h"

namespace rpcf
{

gint32 CStScope::AddSymbol( SymPtr pSymbol )
{
    gint32 ret = 0;
    do{
        if( pSymbol.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }
        CStSymbol* pSym = pSymbol;
        if( pSym == nullptr ||
            pSym->m_strName.empty() )
        {
            ret = -ENOENT;
            break;
        }
        auto& vec = m_mapSymbols[ pSym->m_strName ];
        if( !vec.empty() )
        {
            for( auto elem : vec )
            {
                if( pSym == elem )
                {
                    ret = -EEXIST;
                    break;
                }
            }
        }
        if( ERROR( ret ) )
            break;
        pSym->m_pScope = this;
        vec.push_back( pSymbol );
    }while( 0 );
    return 0;
}

ScopePtr CStSymbolTable::NewScopeInternal(
    enumScopeKind eKind, const std::string& strName )
{
    ScopePtr pScope( true );
    m_dequeScopes.push_back( pScope );
    pScope->m_eKind = eKind;
    pScope->m_strName = strName;
    return pScope;
}
void CStSymbolTable::WalkByKind(
    CStScope* pScope,
    enumSymKind eKind,
    bool bRecurse,
    SymVector& vecOut )
{
    if( pScope == nullptr )
        return;
    std::map< std::string, SymVector >::iterator itr;
    for( itr = pScope->m_mapSymbols.begin();
        itr != pScope->m_mapSymbols.end(); ++itr )
    {
        SymVector& vec = itr->second;
        for( guint32 i = 0; i < vec.size(); ++i )
        {
            CStSymbol* pSym = vec[ i ];
            if( pSym != nullptr && pSym->m_eKind == eKind )
                vecOut.push_back( vec[ i ] );
        }
    }
    if( !bRecurse )
        return;
    std::map< std::string, ScopePtr >::iterator itrC;
    for( itrC = pScope->m_mapChildren.begin();
        itrC != pScope->m_mapChildren.end(); ++itrC )
    {
        WalkByKind( itrC->second, eKind, true, vecOut );
    }
    return;
}

void CStSymbolTable::WalkByDataType(
    CStScope* pScope,
    const std::string& strTypeName,
    bool bRecurse,
    SymVector& vecOut )
{
    if( pScope == nullptr )
        return;
    std::map< std::string, SymVector >::iterator itr;
    for( itr = pScope->m_mapSymbols.begin();
        itr != pScope->m_mapSymbols.end(); ++itr )
    {
        SymVector& vec = itr->second;
        for( guint32 i = 0; i < vec.size(); ++i )
        {
            CStSymbol* pSym = vec[ i ];
            if( pSym == nullptr )
                continue;
            if( pSym->m_strTypeName == strTypeName )
                vecOut.push_back( vec[ i ] );
        }
    }
    if( !bRecurse )
        return;
    std::map< std::string, ScopePtr >::iterator itrC;
    for( itrC = pScope->m_mapChildren.begin();
        itrC != pScope->m_mapChildren.end(); ++itrC )
    {
        WalkByDataType( itrC->second, strTypeName, true, vecOut );
    }
    return;
}

void CStSymbolTable::DumpScope( CStScope* pScope,
    gint32 iDepth, std::string& strOut )
{
    if( pScope == nullptr )
        return;
    const char* szKind = "unknown";
    switch( pScope->m_eKind )
    {
    case scopeGlobal:
        szKind = "global";
        break;
    case scopeNamespace:
        szKind = "namespace";
        break;
    case scopePou:
        szKind = "pou";
        break;
    case scopeStruct:
        szKind = "struct";
        break;
    case scopeInterface:
        szKind = "interface";
        break;
    default:
        break;
    }
    std::string strIndent( iDepth * 2, ' ' );
    strOut += strIndent + szKind;
    if( !pScope->m_strName.empty() )
        strOut += " " + pScope->m_strName;
    strOut += " {\n";
    std::map< std::string, SymVector >::iterator itr;
    for( itr = pScope->m_mapSymbols.begin();
        itr != pScope->m_mapSymbols.end(); ++itr )
    {
        SymVector& vec = itr->second;
        for( guint32 i = 0; i < vec.size(); ++i )
        {
            CStSymbol* pSym = vec[ i ];
            if( pSym == nullptr )
                continue;
            strOut += strIndent + "  " +
                pSym->GetSymbolInfo() + "\n";
        }
    }
    std::map< std::string, ScopePtr >::iterator itrC;
    for( itrC = pScope->m_mapChildren.begin();
        itrC != pScope->m_mapChildren.end(); ++itrC )
    {
        DumpScope( itrC->second, iDepth + 1, strOut );
    }
    strOut += strIndent + "}\n";
    return;
}

SymPtr CStSymbolTable::Declare(
    const std::string& strName,
    enumSymKind eKind,
    ObjPtr pDeclNode,
    const std::string& strTypeName,
    CStVarDeclNode::enumVarCategory eCategory )
{
    CStScope* pScope = GetCurrentScope();
    if( pScope == nullptr || strName.empty() )
        return SymPtr();
    SymPtr pSym( true );
    pSym->m_strName = strName;
    pSym->m_eKind = eKind;
    pSym->m_eCategory = eCategory;
    pSym->m_strTypeName = strTypeName;
    pSym->m_pDeclNode = pDeclNode;
    if( ERROR( pScope->AddSymbol( pSym ) ) )
        return SymPtr();
    return pSym;
}

SymPtr CStSymbolTable::DeclareScopeOwner(
    enumSymKind eKind,
    enumScopeKind eScopeKind,
    const std::string& strName,
    ObjPtr pDeclNode )
{
    SymPtr pSymbol = Declare(
        strName, eKind, pDeclNode );
    CStSymbol* pSym = pSymbol;
    if( pSym == nullptr )
        return pSymbol;
    pSym->m_pDefScope =
        PushScope( eScopeKind, strName );
    return pSymbol;
}

SymPtr CStSymbolTable::Lookup( const std::string& strName ) const
{
    for( auto i = ( gint32 )m_stackScopes.size() - 1;
        i >= 0; --i )
    {
        SymPtr pSym =
            m_stackScopes[ i ]->LookupLocal( strName );
        if( !pSym.IsEmpty() )
            return pSym;
    }
    return SymPtr();
}

void CStSymbolTable::LookupAll(
    const std::string& strName,
    SymVector& vecOut ) const
{
    for( gint32 i = ( gint32 )m_stackScopes.size() - 1;
        i >= 0; --i )
    {
        m_stackScopes[ i ]->GetAllLocal(
            strName, vecOut );
    }
    return;
}

void CStSymbolTable::LookupAllLocal(
    const std::string& strName,
    SymVector& vecOut ) const
{
    if( m_stackScopes.size() )
        m_stackScopes.back()->GetAllLocal(
            strName, vecOut );
    return;
}

SymPtr CStSymbolTable::LookupLocal(
    const std::string& strName,
    ScopePtr pScope ) const
{
    if( pScope.IsEmpty() )
        pScope = GetCurrentScope();
    if( pScope.IsEmpty() )
        return SymPtr();
    return pScope->LookupLocal( strName );
}

SymPtr CStSymbolTable::LookupQualified(
    const std::vector< std::string >& vecNames,
    bool bAbsolute )
{
    if( vecNames.empty() )
        return SymPtr();
    SymPtr pSym = bAbsolute ?
        m_pGlobal->LookupLocal( vecNames[ 0 ] ) :
        Lookup( vecNames[ 0 ] );
    for( guint32 i = 1; i < vecNames.size(); ++i )
    {
        CStSymbol* pCur = pSym;
        if( pCur == nullptr ||
            pCur->m_pDefScope == nullptr )
            return SymPtr();
        pSym = pCur->m_pDefScope->LookupLocal(
            vecNames[ i ] );
    }
    return pSym;
}

std::vector<std::string>
CStSymbolTable::BreakdownQualified(
    const std::string& strFullName)
{
    std::vector<std::string> vecComponents;
    if (strFullName.empty())
        return vecComponents;

    std::string_view svPath(strFullName);
    size_t posStart = 0;

    // 1. Detect and normalize root escape prefixes
    if (svPath.substr(0, 2) == "_.")
    {
        vecComponents.push_back("_.");
        posStart = 2;
    }
    else if (svPath[0] == '.')
    {
        vecComponents.push_back("_.");
        posStart = 1;
    }

    // 2. Loop through and extract individual tokens
    size_t posEnd = strFullName.find('.', posStart);
    while (true) {
        std::string strPart =
            ( posEnd == std::string::npos )
            ? strFullName.substr(posStart)
            : strFullName.substr(
                posStart, posEnd - posStart);

        if (!strPart.empty()) {
            vecComponents.push_back(strPart);
        }

        if (posEnd == std::string::npos) {
            break;
        }
        posStart = posEnd + 1;
        posEnd = strFullName.find('.', posStart);
    }
    return vecComponents;
}


SymPtr CStSymbolTable::LookupQualified(
    const std::string& strFullName,
    bool bAbsolute )
{
    auto vecComps = BreakdownQualified(
        strFullName );
    if( vecComps.empty() )
        return SymPtr();
    return LookupQualified(
        vecComps, bAbsolute );
}

void CStSymbolTable::FindByKind(
    enumSymKind eKind,
    SymVector& vecOut,
    CStScope* pScope )
{
    if( pScope == nullptr )
        pScope = GetCurrentScope();
    WalkByKind( pScope,
        eKind, false, vecOut );
    return;
}

void CStSymbolTable::FindByKindAll(
    enumSymKind eKind,
    SymVector& vecOut,
    CStScope* pScope )
{
    if( pScope == nullptr )
        pScope = m_pGlobal;
    WalkByKind( pScope,
        eKind, true, vecOut );
    return;
}

void CStSymbolTable::FindByDataType(
    const std::string& strTypeName,
    SymVector& vecOut,
    CStScope* pScope )
{
    if( pScope == nullptr )
        pScope = GetCurrentScope();
    WalkByDataType( pScope,
        strTypeName, false, vecOut );
    return;
}

void CStSymbolTable::FindByDataTypeAll(
    const std::string& strTypeName,
    SymVector& vecOut,
    CStScope* pScope )
{
    if( pScope == nullptr )
        pScope = m_pGlobal;
    WalkByDataType( pScope,
        strTypeName, true, vecOut );
    return;
}

std::string CStSymbolTable::Dump()
{
    std::string strOut;
    DumpScope( m_pGlobal, 0, strOut );
    return strOut;
}
}
