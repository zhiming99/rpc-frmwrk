/*
 * =====================================================================================
 *
 *       Filename:  stsymtab.h
 *
 *    Description:  Symbol table for the Structured Text semantic phase
 *
 *        Version:  1.0
 *        Created:  09/05/2026
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

#include <map>
#include <deque>
#include "rpc.h"
#include "stast.h"
#include "astnodes.h"

/**
 * @brief What kind of entity a symbol represents
 */
namespace rpcf
{

enum enumSymKind {
    symUnknown = 0,
    symVariable,      // variable, incl. FB instances and vars with AT addresses
    symEnumValue,     // enumerated value constant
    symDataType,      // named data type from TYPE...END_TYPE
    symProgram,
    symFunctionBlock,
    symFunction,
    symMethod,
    symInterface,
    symNamespace,
    symClass,
    symProperty,
    symAccess,
    symResource,
    symTask,
};

/**
 * @brief What kind of lexical scope a CStScope represents
 */
enum enumScopeKind {
    scopeGlobal = 0,  // the root scope, holds globals and top-level decls
    scopeNamespace,   // NAMESPACE...END_NAMESPACE
    scopePou,         // PROGRAM/FUNCTION/FUNCTION_BLOCK/METHOD members
    scopeStruct,      // STRUCT member fields
    scopeInterface,   // INTERFACE methods
};

struct CStScope;

/**
 * @brief One declared entity, e.g. a variable, a data type or a POU
 *
 * The symbol does not duplicate the declaration info: m_pDeclNode
 * points at the AST node (CStVarDeclNode, CStTypeDecl, CStPouDeclNode,
 * CStNamespaceDecl, CStEnumValueNode, ...), which carries the type,
 * the initial value and the source location. The fields below are the
 * lookup keys and the cheap-to-read summary the semantic phase needs
 * on its hot paths.
 */
class CStSymbol : public rpcf::CObjBase
{
    public:
    std::string m_strName;
    enumSymKind m_eKind;

    // for symVariable: copied from CStVarDeclNode::m_eCategory so
    // callers can filter input/output/inout/external without casting
    // to the decl node
    CStVarDeclNode::enumVarCategory m_eCategory;

    // declared data type name, e.g. "INT", or "Counter" for an FB
    // instance declared as 'counter : Counter'. Cached at declare
    // time so the by-type lookups don't have to walk the type node.
    // Empty when not applicable, e.g. namespaces.
    std::string m_strTypeName;

    // the AST declaration node that introduced this symbol
    ObjPtr m_pDeclNode;

    // the scope that holds this symbol
    CStScope* m_pScope;

    // for symbols that open a scope of their own (namespace, POU,
    // struct type, interface): the scope holding their members, so
    // member access like 'fbInst.count' or 'Pump.Start' can resolve
    // through the symbol. Null otherwise.
    CStScope* m_pDefScope;

    bool m_bResolved = false;

    typedef CObjBase super;
    CStSymbol() : super(),
        m_eKind( symUnknown ),
        m_eCategory( CStVarDeclNode::vcLocal ),
        m_pScope( nullptr ),
        m_pDefScope( nullptr )
    { SetClassId( clsid( CStSymbol ) ); }

    std::string GetSymbolInfo() const
    {
        const char* szKind = "unknown";
        switch( m_eKind )
        {
        case symVariable:
            szKind = "variable";
            break;
        case symEnumValue:
            szKind = "enum-value";
            break;
        case symDataType:
            szKind = "data-type";
            break;
        case symProgram:
            szKind = "program";
            break;
        case symFunctionBlock:
            szKind = "function-block";
            break;
        case symFunction:
            szKind = "function";
            break;
        case symMethod:
            szKind = "method";
            break;
        case symInterface:
            szKind = "interface";
            break;
        case symNamespace:
            szKind = "namespace";
            break;
        case symClass:
            szKind = "class";
            break;
        case symProperty:
            szKind = "property";
            break;
        case symAccess:
            szKind = "access";
            break;
        case symResource:
            szKind = "resource";
            break;
        case symTask:
            szKind = "task";
            break;
        default:
            break;
        }
        std::string strInfo =
            m_strName + " : " + szKind;
        if( !m_strTypeName.empty() )
            strInfo += " : " + m_strTypeName;
        return strInfo;
    }

    inline void SetResolved()
    { m_bResolved = true; }

    inline bool IsReserved() const
    { return m_bResolved; }
};

typedef CAutoPtr<clsid( CStSymbol ), CStSymbol> SymPtr;
typedef std::vector< SymPtr > SymVector;

/**
 * @brief A lexical scope holding named symbols
 *
 * Scopes form a tree via m_pParent/m_mapChildren and are owned by
 * CStSymbolTable (allocated in a deque, so pointers stay valid).
 * Symbols themselves are SymPtr-managed CObjBase objects.
 */
struct CStScope;
typedef CAutoPtr< clsid( CStScope ), CStScope > ScopePtr;

struct CStScope : public rpcf::CObjBase
{
    enumScopeKind m_eKind;
    std::string m_strName;
    CStScope* m_pParent;

    // the symbol representation of this scope;
    SymPtr m_pSymbol;
    // name -> symbols; a vector per name leaves room for overloaded
    // functions, which share the same name
    std::map< std::string, SymVector > m_mapSymbols;

    // child scopes by name (namespaces, POUs, structs), used by the
    // qualified lookups; symbols are checked for duplicates at
    // declare time, which keeps these names unique
    std::map< std::string, ScopePtr > m_mapChildren;

    CStScope() : m_eKind( scopeGlobal ),
        m_pParent( nullptr )
    {}

    /**
     * @brief Add a symbol to this scope
     *
     * @return false on a duplicate name. Exception: functions may
     *         share a name with functions (overloads).
     */
    bool AddSymbol( SymPtr pSymbol )
    {
        if( pSymbol.IsEmpty() )
            return false;
        CStSymbol* pSym = pSymbol;
        if( pSym == nullptr ||
            pSym->m_strName.empty() )
            return false;
        auto& vec = m_mapSymbols[ pSym->m_strName ];
        if( !vec.empty() )
        {
            bool bOverload =
                ( pSym->m_eKind == symFunction );
            for( guint32 i = 0;
                i < vec.size() && bOverload;
                ++i )
            {
                CStSymbol* pOld = vec[ i ];
                if( pOld == nullptr ||
                    pOld->m_eKind != symFunction )
                    bOverload = false;
            }
            if( !bOverload )
                return false;
        }
        pSym->m_pScope = this;
        vec.push_back( pSymbol );
        return true;
    }

    //! first symbol with this name, or a null SymPtr
    SymPtr LookupLocal( const std::string& strName ) const
    {
        auto itr = m_mapSymbols.find( strName );
        if( itr == m_mapSymbols.end() ||
            itr->second.empty() )
            return SymPtr();
        return itr->second.front();
    }

    //! all candidates with this name, incl. overloads
    void GetAllLocal(
        const std::string& strName, SymVector& vecOut ) const
    {
        auto itr = m_mapSymbols.find( strName );
        if( itr == m_mapSymbols.end() )
            return;
        vecOut.insert( vecOut.end(),
            itr->second.begin(), itr->second.end() );
        return;
    }

    void AddChild( ScopePtr pChild )
    {
        if( pChild.IsEmpty() )
            return;
        pChild->m_pParent = this;
        m_mapChildren[ pChild->m_strName ] = pChild;
        return;
    }

    ScopePtr GetChild( const std::string& strName ) const
    {
        auto itr = m_mapChildren.find( strName );
        if( itr == m_mapChildren.end() )
            return ScopePtr();
        return itr->second;
    }
    ScopePtr GetChild( const std::string& strName )
    {
        auto itr = m_mapChildren.find( strName );
        if( itr == m_mapChildren.end() )
            return ScopePtr();
        return itr->second;
    }
};

/**
 * @brief The symbol table facade used by the semantic phase
 *
 * Owns all scopes and keeps a stack of the open lexical scopes. The
 * global scope is always the bottom of the stack, so Lookup() walks
 * the full chain: current scope -> ... -> globals.
 *
 * Typical use while walking the AST:
 *
 *     CStSymbolTable oTab;
 *     // entering PROGRAM Pump
 *     oTab.PushScope( scopePou, "Pump" );
 *     oTab.Declare( "nLevel", symVariable, pDeclNode, "INT",
 *         CStVarDeclNode::vcInput );
 *     SymPtr pSym = oTab.Lookup( "nLevel" );   // found in this POU
 *     oTab.Lookup( "gBuzzer" );                // falls through to global
 *     oTab.PopScope();
 *     // ... and later, from anywhere:
 *     oTab.FindByDataType( "Counter", vec );   // all FB instances
 *
 * Note on namespaces: 'using' directives are applied by the semantic
 * phase, either by temporarily pushing the referenced namespace
 * scopes around a POU body, or by calling LookupQualified().
 */
class CStSymbolTable
{
public:
    typedef std::vector< ScopePtr > ScopeStack;

private:
    // all scopes live here; deque keeps addresses stable on growth
    std::deque< ScopePtr > m_dequeScopes;
    ScopePtr m_pGlobal;
    ScopeStack m_stackScopes;

    ScopePtr NewScopeInternal(
        enumScopeKind eKind, const std::string& strName )
    {
        ScopePtr pScope( true );
        m_dequeScopes.push_back( pScope );
        pScope->m_eKind = eKind;
        pScope->m_strName = strName;
        return pScope;
    }

    static void WalkByKind( CStScope* pScope,
        enumSymKind eKind, bool bRecurse, SymVector& vecOut )
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

    static void WalkByDataType( CStScope* pScope,
        const std::string& strTypeName, bool bRecurse,
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

    static void DumpScope( CStScope* pScope,
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

public:
    CStSymbolTable() : m_pGlobal( nullptr )
    {
        m_pGlobal = NewScopeInternal( scopeGlobal, "" );
        // the global scope is the bottom of the stack and is never
        // popped, so plain name lookup always ends at the globals
        m_stackScopes.push_back( m_pGlobal );
    }

    ScopePtr GetGlobalScope()
    { return m_pGlobal; }

    const ScopePtr GetCurrentScope() const
    {
        if( m_stackScopes.empty() )
            return ScopePtr();
        return m_stackScopes.back();
    }
    ScopePtr GetCurrentScope()
    {
        if( m_stackScopes.empty() )
            return ScopePtr();
        return m_stackScopes.back();
    }

    //! open a scope; the caller must PopScope() at scope end
    ScopePtr PushScope(
        enumScopeKind eKind, const std::string& strName )
    {
        auto pParent = GetCurrentScope();
        auto pScope = NewScopeInternal( eKind, strName );
        pScope->m_pParent = pParent;
        if( !pParent.IsEmpty() )
            pParent->AddChild( pScope );
        m_stackScopes.push_back( pScope );
        return pScope;
    }

    void PushScope( ScopePtr pScope )
    {
        if( pScope.IsEmpty() )
            return;
        m_stackScopes.push_back( pScope );
        return;
    }

    //! close the innermost scope; the global scope is never popped
    void PopScope()
    {
        if( m_stackScopes.size() > 1 )
            m_stackScopes.pop_back();
        return;
    }

    /**
     * @brief Declare a symbol in the current scope
     *
     * @param pDeclNode the AST declaration node, kept as back link
     * @param strTypeName the declared data type name, see
     *                    CStSymbol::m_strTypeName
     * @return the new symbol, or a null SymPtr on a duplicate name
     *         (functions may overload functions)
     */
    SymPtr Declare(
        const std::string& strName,
        enumSymKind eKind,
        ObjPtr pDeclNode,
        const std::string& strTypeName = std::string(),
        CStVarDeclNode::enumVarCategory eCategory =
            CStVarDeclNode::vcLocal )
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
        if( !pScope->AddSymbol( pSym ) )
            return SymPtr();
        return pSym;
    }

    /**
     * @brief Declare a symbol that opens a scope of its own
     *
     * Use for namespaces, POUs, struct types and interfaces: the
     * symbol is declared in the current scope and the new scope is
     * pushed and linked to CStSymbol::m_pDefScope, so qualified and
     * member lookups can reach the members. PopScope() at scope end.
     */
    SymPtr DeclareScopeOwner(
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

    //! lookup by name, walking the lexical chain up to the globals
    SymPtr Lookup( const std::string& strName ) const
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

    //! all candidates along the lexical chain, incl. overloads
    void LookupAll(
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

    //! all candidates of current scope
    void LookupAllLocal(
        const std::string& strName,
        SymVector& vecOut ) const
    {
        if( m_stackScopes.size() )
            m_stackScopes.back()->GetAllLocal(
                strName, vecOut );
        return;
    }

    //! lookup in one scope only, no chain walk
    SymPtr LookupLocal(
        const std::string& strName,
        ScopePtr pScope = nullptr ) const
    {
        if( pScope.IsEmpty() )
            pScope = GetCurrentScope();
        if( pScope.IsEmpty() )
            return SymPtr();
        return pScope->LookupLocal( strName );
    }

    /**
     * @brief Lookup a qualified name, e.g. 'Pump.Start' or '::Msg'
     *
     * Each part after the first is looked up in the scope opened by
     * the previous symbol (namespace -> namespace, POU -> methods,
     * struct type -> fields).
     *
     * @param bAbsolute start at the global scope ('::a.b') instead
     *                  of the lexical chain
     */
    SymPtr LookupQualified(
        const std::vector< std::string >& vecNames,
        bool bAbsolute = false )
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

    std::vector<std::string> BreakdownQualified(
        const std::string& strFullName)
    {
        std::vector<std::string> vecComponents;
        size_t posStart = 0;
        size_t posEnd = strFullName.find('.');

        while (true) {
            std::string strPart =
                (posEnd == std::string::npos)
                ? strFullName.substr(posStart)
                : strFullName.substr(posStart,
                     posEnd - posStart);

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

    SymPtr LookupQualified(
        const std::string& strFullName,
        bool bAbsolute = false )
    {
        auto vecComps = BreakdownQualified(
            strFullName );
        if( vecComps.empty() )
            return SymPtr();
        return LookupQualified(
            vecComps, bAbsolute );
    }

    //! symbols of a kind in one scope (default: the current scope)
    void FindByKind(
        enumSymKind eKind,
        SymVector& vecOut,
        CStScope* pScope = nullptr )
    {
        if( pScope == nullptr )
            pScope = GetCurrentScope();
        WalkByKind( pScope,
            eKind, false, vecOut );
        return;
    }

    //! symbols of a kind in a scope subtree (default: the globals)
    void FindByKindAll(
        enumSymKind eKind,
        SymVector& vecOut,
        CStScope* pScope = nullptr )
    {
        if( pScope == nullptr )
            pScope = m_pGlobal;
        WalkByKind( pScope,
            eKind, true, vecOut );
        return;
    }

    //! symbols of a data type in one scope, e.g. all 'Counter'
    //! instances (default: the current scope)
    void FindByDataType(
        const std::string& strTypeName,
        SymVector& vecOut,
        CStScope* pScope = nullptr )
    {
        if( pScope == nullptr )
            pScope = GetCurrentScope();
        WalkByDataType( pScope,
            strTypeName, false, vecOut );
        return;
    }

    //! symbols of a data type in a scope subtree (default: globals)
    void FindByDataTypeAll(
        const std::string& strTypeName,
        SymVector& vecOut,
        CStScope* pScope = nullptr )
    {
        if( pScope == nullptr )
            pScope = m_pGlobal;
        WalkByDataType( pScope,
            strTypeName, true, vecOut );
        return;
    }

    //! indented scope tree listing, for debugging the semantic phase
    std::string Dump()
    {
        std::string strOut;
        DumpScope( m_pGlobal, 0, strOut );
        return strOut;
    }
};
}

