/*
 * =====================================================================================
 *
 *       Filename:  astnodes.cpp
 *
 *    Description:  Implementations of AST node methods
 *
 *        Version:  1.0
 *        Created:  09/03/2026
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

#include "defines.h"
using namespace rpcf;
#include "stlexer.h"
#include "stclsids.h"
#include "astnodes.h"
#include <sstream>
#include <string_view>
#include "stsymtab.h"
#include "st_parser_ext.h"

namespace stc
{

bool IsDerivedType( gint32 iType );
bool IsStringType( gint32 iType );

// Helper function to join vector of strings
static std::string JoinStrings(
    const std::vector< std::string >& vec,
    const std::string& strSep )
{
    std::ostringstream oss;
    for( size_t i = 0; i < vec.size(); i++ )
    {
        if( i > 0 ) oss << strSep;
        oss << vec[i];
    }
    return oss.str();
}


stdstr CStSymbolDeclNode::m_strEmpty;
const stdstr& CStSymbolDeclNode::GetName() const
{
    if( m_pSymbol )
        return m_pSymbol->m_strName;
    return m_strEmpty;
}
// ========================================================================
// CStRootNode Implementation
// ========================================================================

ObjPtr CStRootNode::GetGlobalScope() const
{
    for( const auto& pDecl : m_vecDeclarations )
    {
        CStNamespaceDecl* pNs = pDecl;
        if( pNs != nullptr && pNs->IsGlobalScope() )
            return pDecl;
    }
    return nullptr;
}

std::string CStRootNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "RootNode{ declarations: " << m_vecDeclarations.size() << " }";
    return oss.str();
}

// ========================================================================
// CStInitialValueNode Implementation
// ========================================================================

std::string CStInitialValueNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "InitialValue{ type: ";
    switch( m_eInitType )
    {
        case initExpression: oss << "expression"; break;
        case initArray: oss << "array"; break;
        case initStruct: oss << "struct"; break;
    }
    oss << " }";
    return oss.str();
}

// ========================================================================
// CStDataTypeDeclNode Implementation
// ========================================================================

std::string CStDataTypeDeclNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "DataTypeDecl{ ";
    if( !m_vecTypeDecls.empty() )
        oss << "type_decls set";
    else
        oss << "no type_decls";
    oss << " }";
    return oss.str();
}

gint32 CStDataTypeDeclNode::ResolveUsingNs(
    void* pctx )
{
    auto pCtx = static_cast< CStParseContext* >(pctx );
    ScopePtr pScope =
        pCtx->m_oSymTab.GetCurrentScope();
    if( !pScope.IsEmpty() )
        pScope->PushUsingArr( m_vecUsings );
    gint32 ret = stc::ResolveUsingNs(
        pctx, ObjPtr( pScope ) );
    if( !pScope.IsEmpty() )
        pScope->PopUsingArr();
    return ret;
}

// ========================================================================
// CStTypeDeclNode Implementation
// ========================================================================

CStTypeDeclNode& CStTypeDeclNode::operator=(
    const CStTypeDeclNode& rhs )
{
    m_pSymbol = rhs.m_pSymbol;

    m_oTypeRef = rhs.m_oTypeRef; 
    m_pTypeInit = rhs.m_pTypeInit;
    m_iTypeId = rhs.m_iTypeId;
    m_iDirectType = rhs.m_iDirectType;
    m_iRefRank = rhs.m_iRefRank;
    m_pTypeData = rhs.m_pTypeData;

    // the resolving state is deliberately not copied: this
    // node replaces an ambiguous node which only resolved the
    // type reference, and the concrete node still has to
    // resolve its own initializer, bounds, members, ...
    m_iState = tsUnresolved;

    // set the symbol's declare node to this one
    m_pDirectType = rhs.m_pDirectType;

    // node for type resolution
    m_pDtd = rhs.m_pDtd; 

    // overwriting sym's declnode as the last to
    // avoid releasing rhs unexpectedly.
    m_pSymbol->m_pDeclNode = ObjPtr( this );
    return *this;
}

std::string CStTypeDeclNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "TypeRef{ ";
    if( m_oTypeRef.first.size() )
        oss << "type is derived from '"
            << m_oTypeRef.first << "'";
    else
        oss << "no type";
    oss << " }";
    return oss.str();
}

gint32 CStTypeDeclNode::BeginResolve()
{
    if( m_iState == tsInvalid )
        return -EINVAL;

    if( m_iState == tsInProgress )
    {
        // reentered while still resolving: a cyclic type
        // declaration such as 'a : ARRAY [1..2] OF a'
        m_iState = tsInvalid;
        return -EINVAL;
    }

    if( m_iState == tsDone )
        return 1;

    m_iState = tsInProgress;
    return 0;
}

void CStTypeDeclNode::EndResolve( gint32 ret )
{
    if( ret == 0 )
        m_iState = tsDone;
    else if( ret == -ENOENT )
        // a retryable failure must not look like a cycle
        m_iState = tsUnresolved;
    else
        m_iState = tsInvalid;
}

gint32 CStTypeDeclNode::ResolveSymbol(
    void* pctx, enumSymKind iSymType ) 
{
    gint32 ret = BeginResolve();
    if( ERROR( ret ) )
        return ret;
    if( ret == 1 )
        return 0;

    ret = ResolveOwnSymbol( pctx, iSymType );
    EndResolve( ret );
    return ret;
}

gint32 CStTypeDeclNode::ResolveOwnSymbol(
    void* pctx, enumSymKind iSymType ) 
{
    if( IsSymbolResolved() )
        return 0;

    if( pctx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        stdstr strType = this->m_oTypeRef.first;
        auto pCtx = reinterpret_cast
            < CStParseContext* >( pctx );
        auto& ost = pCtx->m_oSymTab;
        std::vector< stdstr > vecComps =
            ost.BreakdownQualified( strType );

        auto pScope = ost.GetCurrentScope();
        SymPtr pSym;
        gint32 iType = -1;

        ObjPtr pDirectType;
        ret = ResolveTypeToDirectType(
            pCtx, vecComps, iSymType,
            pScope, pSym, iType, pDirectType );
        if( SUCCEEDED( ret ) &&
            !pDirectType.IsEmpty() )
            m_pDirectType = pDirectType;

        if( !pSym.IsEmpty() )
            this->m_oTypeRef.second = pSym;

        if( iType >= 0 )
            this->m_iDirectType = iType;

    }while( 0 );
    return ret;
}

void CStTypeDeclNode::ReplaceAmbiguousNode(
    CStTypeDeclNode* pAmbiguous )
{
    *this = *pAmbiguous;
    return;
}

bool CStTypeDeclNode::IsSymbolResolved() const
{
    if( m_iState == tsInProgress || m_iState == tsInvalid )
        return false;
    return ( m_iDirectType != -1 );
}

bool CStSimpleTypeDeclNode::IsSymbolResolved() const
{
    if( this->m_iDirectType == -1  )
        return false;
    if( m_pTypeInit.IsEmpty() &&
        m_pTypeData.IsEmpty() )
        return true;

    CStExprBase* pExpr = m_pTypeInit;
    bool bResolved = false;
    if( pExpr )
    {
        bResolved = pExpr->IsSymbolResolved();
        if( !bResolved )
            return false;
    }

    if( m_pTypeData.IsEmpty() )
        return true;

    pExpr = m_pTypeData;
    if( !pExpr )
        return true;
    return pExpr->IsSymbolResolved();
}

gint32 CStSimpleTypeDeclNode::ResolveSymbol(
    void* pctx, enumSymKind iSymType ) 
{
    // guard the whole body: the base part resolves the type
    // reference, the part below resolves the initializer, and
    // both belong to this node's resolution
    gint32 ret = BeginResolve();
    if( ERROR( ret ) )
        return ret;
    if( ret == 1 )
        return 0;

    ret = ResolveOwnSymbol( pctx, iSymType );
    if( SUCCEEDED( ret ) )
    do{
        if( !m_pTypeInit.IsEmpty() )
        {
            CStExprBase* pExpr = m_pTypeInit;
            if( pExpr )
                ret = pExpr->ResolveSymbol(
                    pctx, iSymType );
            if( ERROR( ret ) )
                break;
        }
        if( !m_pTypeData.IsEmpty() )
        {
            CStExprBase* pExpr = m_pTypeData;
            if( pExpr )
                ret = pExpr->ResolveSymbol(
                    pctx, iSymType );
            if( ERROR( ret ) )
                break;
        }
    }while( 0 );

    EndResolve( ret );
    return ret;
}

namespace type_decl
{

gint32 BuildStSimpleTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeInit;
    ObjPtr pTypeData;
    stdstr strType;
    gint32 iType = -1;

    gint32 ret = initInfo.GetIntProp(
        1, ( guint32& )iType );
    initInfo.GetStrProp( 0, strType );
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    initInfo.GetObjPtr( propStTypeInit, pTypeInit );
    if( ERROR( ret ) )
        return -EINVAL;

    // a string type carries its length, so it is a string
    // type node of its own, as string_type_decl would build
    if( IsStringType( iType ) )
        return BuildStStringTypeNode( pTypeInfo, pTypeNode );

    if( iType == stparser::RuleDerived_type_access &&
        pTypeInit.IsEmpty() )
    {
        ret = pTypeNode.NewObj(
            clsid( CStAmbiguousTypeDeclNode ) );
    }
    else
    {
        ret = pTypeNode.NewObj(
            clsid( CStSimpleTypeDeclNode ) );
    }
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStTypeDeclNode* ptd = pTypeNode;
    ptd->m_pTypeInit = pTypeInit;
    ptd->m_oTypeRef = { strType, nullptr };
    ptd->m_iTypeId = iType;

    if( !pTypeData.IsEmpty() )
        ptd->m_pTypeData = pTypeData;

    if( IsDirectType( iType ) )
        ptd->m_iDirectType = iType;

    return 0;
}

gint32 BuildStSubrangeTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeInit;
    ObjPtr pTypeData;
    stdstr strBaseType;
    gint32 iBaseType = -1;

    gint32 ret = initInfo.GetIntProp(
        1, ( guint32& )iBaseType );
    initInfo.GetStrProp( 0, strBaseType );
    initInfo.GetObjPtr( propStTypeInit, pTypeInit );
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    if( ERROR( ret ) )
        return -EINVAL;

    ret = pTypeNode.NewObj(
        clsid( CStSubrangeTypeDeclNode ) );
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStTypeDeclNode* ptd = pTypeNode;
    ptd->m_pTypeInit = pTypeInit;
    ptd->m_oTypeRef = { strBaseType, nullptr };
    ptd->m_iTypeId = iBaseType;

    if( !pTypeData.IsEmpty() )
        ptd->m_pTypeData = pTypeData;

    // an elementary base type needs no further lookup; a named
    // base type is resolved by ResolveSymbol()
    if( IsDirectType( iBaseType ) )
        ptd->m_iDirectType = stparser::RuleSubrange_type_decl;

    return 0;
}

gint32 BuildStEnumTypeNode( const ObjPtr& pTypeInfo,
    const char* szName, ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeInit;
    ObjPtr pTypeData;
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    initInfo.GetObjPtr( propStTypeInit, pTypeInit );

    gint32 ret = pTypeNode.NewObj(
        clsid( CStEnumTypeDeclNode ) );
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStTypeDeclNode* ptd = pTypeNode;
    CStEnumValueRef* pRef = pTypeInit;

    if( !pTypeInit.IsEmpty() )
        ptd->m_pTypeInit = pTypeInit;

    if( pRef )
        pRef->m_pDeclNode = pTypeNode;

    ptd->m_oTypeRef = {
        szName != nullptr ? szName : "", nullptr };
    ptd->m_iTypeId = stparser::RuleEnum_type_decl;

    if( IsDirectType( ptd->m_iTypeId ) )
        ptd->m_iDirectType = ptd->m_iTypeId;

    return 0;
}

gint32 BuildStNamedvalTypeNode( const ObjPtr& pTypeInfo,
    const char* szName, ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeInit;
    ObjPtr pTypeData;
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    initInfo.GetObjPtr( propStTypeInit, pTypeInit );

    gint32 ret = pTypeNode.NewObj(
        clsid( CStNamedvalTypeDeclNode ) );
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStTypeDeclNode* ptd = pTypeNode;
    ptd->m_pTypeInit = pTypeInit;
    ptd->m_oTypeRef = {
        szName != nullptr ? szName : "", nullptr };
    ptd->m_iTypeId = stparser::RuleNamedval_type_decl;

    if( IsDirectType( ptd->m_iTypeId ) )
        ptd->m_iDirectType = ptd->m_iTypeId;

    return 0;
}

gint32 BuildStArrayTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;

    do{
        CParamList initInfo( ( IConfigDb* )pTypeInfo );

        ObjPtr pTypeInit;
        ObjPtr pTypeData;
        stdstr strType;
        gint32 iType = -1;

        ret = initInfo.GetIntProp(
            1, ( guint32& )iType );
        initInfo.GetStrProp( 0, strType );
        initInfo.GetObjPtr( propStTypeInit, pTypeInit );
        initInfo.GetObjPtr( propStTypeData, pTypeData );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }

        // prop 1 is TOK_ARRAY for an inline 'ARRAY [..] OF T',
        // and RuleDerived_type_access for a bare name of an
        // existing array type, which resolves to that type
        // later
        if( IsDerivedType( iType ) )
        {
            ret = pTypeNode.NewObj(
                clsid( CStAmbiguousTypeDeclNode ) );
            if( ERROR( ret ) )
                break;
            CStTypeDeclNode* ptd1 = pTypeNode;
            ptd1->m_pTypeInit = pTypeInit;
            ptd1->m_oTypeRef = { strType, nullptr };
            ptd1->m_iTypeId = iType;
            break;
        }

        // 'ARRAY [..] OF T': always an array node, whatever
        // the element type is
        ret = pTypeNode.NewObj(
            clsid( CStArrayTypeDeclNode ) );
        if( ERROR( ret ) )
            break;
        if( pTypeNode.IsEmpty() )
        {
            ret = -ENOMEM;
            break;
        }

        CStArrayTypeDeclNode* ptd = pTypeNode;

        // explicit array declaration
        ObjPtr pElemObj;
        ret = initInfo.GetObjPtr(
            propStElemTypeInfo, pElemObj ); 
        if( ERROR( ret ) )
        {
            // TODO: emit unexpected error 
            ret = -EFAULT;
            break;
        }

        CParamList oElemCfg( ( IConfigDb* )pElemObj );
        oElemCfg.GetStrProp( 0, strType ); 
        oElemCfg.GetIntProp( 1, ( guint32& )iType );

        if( !pTypeData.IsEmpty() )
            ptd->m_pTypeData = pTypeData;

        if( !pTypeInit.IsEmpty() )
            ptd->m_pTypeInit = pTypeInit;

        // string type is always the element type
        ptd->m_oTypeRef = { strType, nullptr };

        ObjPtr pElemTypeData;
        oElemCfg.GetObjPtr(
            propStTypeData, pElemTypeData );

        if( IsDirectType( iType ) )
        {
            ptd->m_iTypeId = iType;
            ptd->m_iDirectType = iType;
            if( IsStringType( iType ) &&
                !pElemTypeData.IsEmpty() )
            {
                // a string element carries its length in a
                // type node of its own, so build that node
                // and make it the array's element type
                ObjPtr pElemNode;
                ret = BuildStStringTypeNode(
                    pElemTypeData, pElemNode );
                if( ERROR( ret ) )
                    break;
                ptd->m_pDirectType = pElemNode;
            }
        }
        else 
        {
            // element is of a derived type
            ptd->m_iTypeId = stparser::TOK_ARRAY;
            // leaving m_iDirectType -1 for the
            // resolving process
            UNREFERENCED( ptd->m_iDirectType );
        }

    }while( 0 );
    return ret;
}

gint32 BuildStStringTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeData;
    stdstr strType;
    gint32 iType = -1;

    gint32 ret = initInfo.GetIntProp(
        1, ( guint32& )iType );
    initInfo.GetStrProp( 0, strType );
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    if( ERROR( ret ) )
        return -EINVAL;

    if( !IsElementaryType( iType ) )
        return -EINVAL;

    ret = pTypeNode.NewObj(
        clsid( CStStringTypeDeclNode ) );
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStTypeDeclNode* ptd = pTypeNode;
    CParamList oTypeInit;
    ret = oTypeInit.CopyProp( propStTypeInit, pTypeInfo );
    if( SUCCEEDED( ret ) )
        ptd->m_pTypeInit = oTypeInit.GetCfg();

    ptd->m_oTypeRef = { strType, nullptr };
    ptd->m_iTypeId = iType;

    if( !pTypeData.IsEmpty() )
        ptd->m_pTypeData = pTypeData;

    ptd->m_iDirectType = iType;

    return 0;
}

gint32 BuildStRefTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeInit;
    ObjPtr pTypeData;
    stdstr strType;
    gint32 iType = -1;

    gint32 ret = initInfo.GetIntProp(
        1, ( guint32& )iType );
    initInfo.GetStrProp( 0, strType );
    initInfo.GetObjPtr( propStTypeInit, pTypeInit );
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    if( ERROR( ret ) )
        return -EINVAL;

    if( iType == stparser::RuleDerived_type_access &&
        pTypeInit.IsEmpty() )
    {
        ret = pTypeNode.NewObj(
            clsid( CStAmbiguousTypeDeclNode ) );
    }
    else
    {
        ret = pTypeNode.NewObj(
            clsid( CStRefTypeDeclNode ) );
    }
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStTypeDeclNode* ptd = pTypeNode;
    if( !pTypeInit.IsEmpty() )
        ptd->m_pTypeInit = pTypeInit;

    ptd->m_oTypeRef = { strType, nullptr };
    ptd->m_iTypeId = stparser::TOK_REF_TO;

    if( !pTypeData.IsEmpty() )
        ptd->m_pTypeData = pTypeData;

    // the referenced type is the element this node refers
    // to, see BuildStArrayTypeNode
    if( IsDirectType( iType ) )
        ptd->m_iDirectType = iType;

    // a referenced string keeps its length in a type node of
    // its own
    if( IsStringType( iType ) && !pTypeData.IsEmpty() )
    {
        ObjPtr pElemNode;
        ret = BuildStStringTypeNode( pTypeData, pElemNode );
        if( ERROR( ret ) )
            return ret;
        ptd->m_pDirectType = pElemNode;
    }

    if( !pTypeData.IsEmpty() )
    {
        CParamList oTypeData( pTypeData );
        gint32 iRank = 0;
        ret = oTypeData.GetIntProp(
            propStRefRank, ( guint32& )iRank );
        if( SUCCEEDED( ret ) )
            ptd->m_iRefRank = iRank;
    }

    return 0;
}

gint32 BuildStStructTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList initInfo( ( IConfigDb* )pTypeInfo );

    ObjPtr pTypeInit;
    ObjPtr pTypeData;
    stdstr strType;
    gint32 iType = -1;

    gint32 ret = initInfo.GetIntProp(
        1, ( guint32& )iType );
    initInfo.GetStrProp( 0, strType );
    initInfo.GetObjPtr( propStTypeInit, pTypeInit );
    initInfo.GetObjPtr( propStTypeData, pTypeData );
    if( ERROR( ret ) )
        return -EINVAL;

    if( IsDerivedType( iType ) )
    {
        // the name of an existing struct type: the node
        // resolves to that type later
        ret = pTypeNode.NewObj(
            clsid( CStAmbiguousTypeDeclNode ) );
        if( ERROR( ret ) )
            return ret;
        CStTypeDeclNode* ptd = pTypeNode;
        ptd->m_pTypeInit = pTypeInit;
        ptd->m_oTypeRef = { strType, nullptr };
        ptd->m_iTypeId = iType;
        return 0;
    }

    ret = pTypeNode.NewObj(
        clsid( CStStructTypeDeclNode ) );
    if( ERROR( ret ) )
        return ret;
    if( pTypeNode.IsEmpty() )
        return -ENOMEM;

    CStStructTypeDeclNode* pst = pTypeNode;
    pst->m_pTypeInit = pTypeInit;
    pst->m_oTypeRef = { strType, nullptr };
    pst->m_iTypeId = stparser::TOK_STRUCT;
    pst->m_iDirectType = stparser::TOK_STRUCT;

    if( pTypeData.IsEmpty() )
        return 0;   // an empty struct

    ObjVecPtr pvec( pTypeData );
    if( pvec.IsEmpty() )
        return 0;
    auto& vec = ( *pvec )();

    // reserve up front: a CStructMember holds a Variant whose
    // copy constructor rejects the default value, so the
    // vector must not reallocate its elements
    pst->m_vecMembers.reserve( vec.size() );

    for( auto& pElem : vec )
    {
        if( pElem.IsEmpty() )
            continue;
        CParamList oElem( ( IConfigDb* )pElem );
        stdstr strName;
        ObjPtr pMemberInfo;
        oElem.GetStrProp( 0, strName );
        oElem.GetObjPtr( propStTypeData, pMemberInfo );
        if( strName.empty() || pMemberInfo.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        ObjPtr pMemberType;
        ret = BuildStTypeNode( pMemberInfo, pMemberType );
        if( ERROR( ret ) )
            break;
        CStTypeDeclNode* ptd = pMemberType;
        if( ptd == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        pst->m_vecMembers.emplace_back();
        auto& member = pst->m_vecMembers.back();
        member.m_strName = strName;
        member.m_pType = pMemberType;
    }

    return ret;
}

gint32 BuildStTypeNode( const ObjPtr& pTypeInfo,
    ObjPtr& pTypeNode )
{
    pTypeNode.Clear();
    if( pTypeInfo.IsEmpty() )
        return -EINVAL;

    CParamList oInfo( ( IConfigDb* )pTypeInfo );
    gint32 iType = -1;
    gint32 ret = oInfo.GetIntProp(
        1, ( guint32& )iType );
    if( ERROR( ret ) )
        return -EINVAL;

    // a reference is marked by its rank in the type data, and
    // must be checked before the referenced type's own kind
    ObjPtr pTypeData;
    oInfo.GetObjPtr( propStTypeData, pTypeData );
    if( !pTypeData.IsEmpty() )
    {
        CParamList oData( ( IConfigDb* )pTypeData );
        gint32 iRank = 0;
        if( SUCCEEDED( oData.GetIntProp(
            propStRefRank, ( guint32& )iRank ) ) )
            return BuildStRefTypeNode(
                pTypeInfo, pTypeNode );
    }

    // an inline array is marked by TOK_ARRAY in prop 1; a
    // bare array type name goes through simple_type_access
    // instead, see BuildStArrayTypeNode
    if( iType == stparser::TOK_ARRAY )
        return BuildStArrayTypeNode( pTypeInfo, pTypeNode );

    if( iType == stparser::TOK_STRUCT )
        return BuildStStructTypeNode( pTypeInfo, pTypeNode );

    if( IsStringType( iType ) )
        return BuildStStringTypeNode( pTypeInfo, pTypeNode );

    // a subrange spec carries its CStSubrangeNode in the type
    // data; an elementary name alone has no other marker
    if( !pTypeData.IsEmpty() &&
        pTypeData->GetClsid() == clsid( CStSubrangeNode ) )
        return BuildStSubrangeTypeNode( pTypeInfo, pTypeNode );

    return BuildStSimpleTypeNode( pTypeInfo, pTypeNode );
}

}  // namespace type_decl

std::string CStTypeDefinitionBlockNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "TypeDefinitionBlock{ "
        << m_vecTypeDecls.size()
        << " declarations }";
    return oss.str();
}

// ========================================================================
// CStLValueNode Implementation
// ========================================================================

std::string CStLValueNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "LValue";
    if( !m_pExpression.IsEmpty() )
    {
        CStAstNodeBase* pBase = dynamic_cast< CStAstNodeBase* >(
            ( CObjBase* )m_pExpression );
        if( pBase )
            oss << "{" << pBase->GetNodeInfo() << "}";
    }
    return oss.str();
}

// ========================================================================
// CStLValueExtNode Implementation
// ========================================================================

std::string CStLValueExtNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "LValueExt";
    if( !m_pExpression.IsEmpty() )
    {
        CStAstNodeBase* pBase = dynamic_cast< CStAstNodeBase* >(
            ( CObjBase* )m_pExpression );
        if( pBase )
            oss << "{" << pBase->GetNodeInfo() << "}";
    }
    return oss.str();
}

// ========================================================================
// CStInstancePathNode Implementation
// ========================================================================

std::string CStInstancePathNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "InstancePath{ " << GetDottedName() << ", ";
    if( !m_pExpression.IsEmpty() )
        oss << "expr set";
    else
        oss << "no expr";
    oss << " }";
    return oss.str();
}

// ========================================================================
// CStFullExpressionNode Implementation
// ========================================================================

std::string CStExpressionNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "FullExpr{ ";
    if( !m_pSubExpr.IsEmpty() )
        oss << "expr set";
    else
        oss << "no expr";
    oss << " }";
    return oss.str();
}

std::string CStExpressionNode::GetSignature() const
{
    CStExprBase* pInner = m_pSubExpr;
    if( pInner != nullptr )
        return pInner->GetSignature();
    return std::string();
}

bool CStExpressionNode::IsSymbolResolved() const
{
    if( m_pSubExpr.IsEmpty() )
        return false;
    ObjPtr pCurExpr = m_pSubExpr;
    if( true )
    {
        CStUnaryExpr* pue;
        CStBinaryExpr* pbe;
        CStPrimaryExprNode* ppe;
        pue = pCurExpr;
        if( pue )
            return pue->IsSymbolResolved();

        pbe = pCurExpr;
        if( pbe )
            return pbe->IsSymbolResolved();

        ppe = pCurExpr;
        if( ppe )
            return ppe->IsSymbolResolved();
    }
    return false;
}

bool CStUnaryExpr::IsSymbolResolved() const
{
    if( m_pOperand.IsEmpty() )
        return false;
    ObjPtr pCurExpr = m_pOperand;
    if( true )
    {
        CStUnaryExpr* pue;
        CStBinaryExpr* pbe;
        CStPrimaryExprNode* ppe;
        pue = pCurExpr;
        if( pue )
            return pue->IsSymbolResolved();

        pbe = pCurExpr;
        if( pbe )
            return pbe->IsSymbolResolved();

        ppe = pCurExpr;
        if( ppe )
            return ppe->IsSymbolResolved();
    }
    return false;
}

bool CStBinaryExpr::IsSymbolResolved() const
{
    if( m_pLeft.IsEmpty() || m_pRight.IsEmpty() )
        return false;
    ObjPtr pCurExpr = m_pLeft;
    do{
        bool ret;
        CStUnaryExpr* pue;
        CStBinaryExpr* pbe;
        CStPrimaryExprNode* ppe;
        pue = pCurExpr;
        if( pue )
        {
            ret = pue->IsSymbolResolved();
            if( !ret )
                return ret;
            if( m_pRight->GetObjId() ==
                pCurExpr->GetObjId() )
                break;
            pCurExpr = m_pRight;
            continue;
        }
        pbe = pCurExpr;
        if( pbe )
        {
            ret = pbe->IsSymbolResolved();
            if( !ret )
                return ret;
            if( m_pRight->GetObjId() ==
                pCurExpr->GetObjId() )
                break;
            pCurExpr = m_pRight;
            continue;
        }

        ppe = pCurExpr;
        if( ppe )
        {
            ret = ppe->IsSymbolResolved();
            if( !ret )
                return ret;
            if( m_pRight->GetObjId() ==
                pCurExpr->GetObjId() )
                break;
            pCurExpr = m_pRight;
            continue;
        }
        break;
    }while( 1 );
    return false;
}


gint32 CStUnaryExpr::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    if( m_pOperand.IsEmpty() )
        return ERROR_FALSE;
    if( true )
    {
        CStUnaryExpr* pue;
        CStBinaryExpr* pbe;
        CStPrimaryExprNode* ppe;
        pue = m_pOperand;
        if( pue )
            return pue->ResolveSymbol(
                pctx, iKind);

        pbe = m_pOperand;
        if( pbe )
            return pbe->ResolveSymbol(
                pctx, iKind);

        ppe = m_pOperand;
        if( ppe )
            return ppe->ResolveSymbol(
                pctx, iKind);
    }
    return false;
}

gint32 CStBinaryExpr::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    if( m_pLeft.IsEmpty() || m_pRight.IsEmpty() )
        return false;
    ObjPtr pCurExpr = m_pLeft;
    do{
        bool ret;
        CStUnaryExpr* pue;
        CStBinaryExpr* pbe;
        CStPrimaryExprNode* ppe;
        pue = pCurExpr;
        if( pue )
        {
            ret = pue->ResolveSymbol(
                pctx, iKind );
            if( !ret )
                return ret;
            if( m_pRight->GetObjId() ==
                pCurExpr->GetObjId() )
                break;
            pCurExpr = m_pRight;
            continue;
        }
        pbe = pCurExpr;
        if( pbe )
        {
            ret = pbe->ResolveSymbol(
                pctx, iKind );
            if( !ret )
                return ret;
            if( m_pRight->GetObjId() ==
                pCurExpr->GetObjId() )
                break;
            pCurExpr = m_pRight;
            continue;
        }

        ppe = pCurExpr;
        if( ppe )
        {
            ret = ppe->ResolveSymbol(
                pctx, iKind );
            if( !ret )
                return ret;
            if( m_pRight->GetObjId() ==
                pCurExpr->GetObjId() )
                break;
            pCurExpr = m_pRight;
            continue;
        }
        break;
    }while( 1 );
    return false;
}

gint32 CStExpressionNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    if( m_pSubExpr.IsEmpty() )
        return ERROR_FALSE;
    ObjPtr pCurExpr = m_pSubExpr;
    if( true )
    {
        CStUnaryExpr* pue;
        CStBinaryExpr* pbe;
        CStPrimaryExprNode* ppe;
        pue = pCurExpr;
        if( pue )
            return pue->ResolveSymbol(
                pctx, iKind);

        pbe = pCurExpr;
        if( pbe )
            return pbe->ResolveSymbol(
                pctx, iKind);

        ppe = pCurExpr;
        if( ppe )
            return ppe->ResolveSymbol(
                pctx, iKind);
    }
    return false;
}
// ========================================================================
// CStSubrangeNode Implementation
// ========================================================================

bool CStSubrangeNode::IsSymbolResolved() const
{ 
    if( m_pStart.IsEmpty() && m_pEnd.IsEmpty() )
        return true;

    CStExpressionNode* pExprS = m_pStart;
    CStExpressionNode* pExprE = m_pEnd;
    if( pExprS )
        if( !pExprS->IsSymbolResolved() )
            return false;
    if( pExprE )
        if( !pExprE->IsSymbolResolved() )
            return false;
    return true;
}

gint32 CStSubrangeNode::ResolveSymbol(
        void* pctx, enumSymKind iKind )
{
    if( m_pStart.IsEmpty() && m_pEnd.IsEmpty() )
        return 0;

    gint32 ret = 0;
    CStExpressionNode* pExprS = m_pStart;
    CStExpressionNode* pExprE = m_pEnd;
    if( pExprS )
    {
        ret = pExprS->ResolveSymbol(
            pctx, iKind );
        if( ERROR( ret ) )
            return ret;
    }
    if( pExprE )
    {
        ret = pExprE->ResolveSymbol(
            pctx, iKind );
        if( ERROR( ret ) )
            return ret;
    }
    return ret;
}

std::string CStSubrangeNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "Subrange{ ";
    if( !m_pStart.IsEmpty() )
        oss << "expr";
    else
        oss << "none";
    oss << "..";
    if( !m_pEnd.IsEmpty() )
        oss << "expr";
    else
        oss << "none";
    oss << " }";
    return oss.str();
}

// ========================================================================
// CStSubrangeListNode Implementation
// ========================================================================

bool CStSubrangeListNode::IsSymbolResolved() const
{ 
    if( m_vecRanges.empty() )
        return true;
    for( auto& elem : m_vecRanges )
    {
        auto pSubRange = ( CStSubrangeNode* )elem;
        if( !pSubRange )
            continue;
        if( !pSubRange->IsSymbolResolved() )
            return false;
    }
    return true;
}

gint32 CStSubrangeListNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{ 
    if( m_vecRanges.empty() )
        return 0;
    gint32 ret = 0;
    for( auto& elem : m_vecRanges )
    {
        auto pSubRange = ( CStSubrangeNode* )elem;
        if( !pSubRange )
            continue;
        ret = pSubRange->ResolveSymbol(
            pctx, iKind );
        if( ERROR( ret ) )
            return ret;
    }
    return ret;
}

std::string CStSubrangeListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "SubrangeList{ dims: " << m_vecRanges.size() << " }";
    return oss.str();
}

// Helpers shared by the array initializer nodes. An initial
// value can be an expression, a nested array initializer, a
// repeated element, or a struct initializer.
namespace array_init
{

static bool IsStInitValueResolved( const ObjPtr& pVal )
{
    if( pVal.IsEmpty() )
        return true;

    gint32 iClsid = pVal->GetClsid();
    if( iClsid == clsid( CStArrayInitNode ) )
    {
        CStArrayInitNode* pai = pVal;
        return ( pai != nullptr && pai->IsSymbolResolved() );
    }
    if( iClsid == clsid( CStArrayRepeatNode ) )
    {
        CStArrayRepeatNode* par = pVal;
        return ( par != nullptr && par->IsSymbolResolved() );
    }
    if( iClsid == clsid( CStStructInitNode ) )
    {
        // TODO: struct initializer resolution
        return true;
    }

    CStExprBase* pExpr = pVal;
    if( pExpr != nullptr )
        return pExpr->IsSymbolResolved();
    return true;
}

static gint32 ResolveStInitValue(
    void* pctx, enumSymKind iKind, const ObjPtr& pVal )
{
    if( pVal.IsEmpty() )
        return 0;

    gint32 iClsid = pVal->GetClsid();
    if( iClsid == clsid( CStArrayInitNode ) )
    {
        CStArrayInitNode* pai = pVal;
        if( pai != nullptr )
            return pai->ResolveSymbol( pctx, iKind );
        return 0;
    }
    if( iClsid == clsid( CStArrayRepeatNode ) )
    {
        CStArrayRepeatNode* par = pVal;
        if( par != nullptr )
            return par->ResolveSymbol( pctx, iKind );
        return 0;
    }
    if( iClsid == clsid( CStStructInitNode ) )
    {
        // TODO: struct initializer resolution
        return 0;
    }

    CStExprBase* pExpr = pVal;
    if( pExpr != nullptr )
        return pExpr->ResolveSymbol( pctx, iKind );
    return 0;
}

}  // namespace array_init

bool CStArrayTypeDeclNode::IsSymbolResolved() const
{
    // never report a node that is being resolved, or one that
    // turned out to be cyclic, as resolved
    if( m_iState == tsInProgress || m_iState == tsInvalid )
        return false;

    if( !IsDirectType( m_iDirectType ) &&
        m_iDirectType != stparser::TOK_ARRAY )
        return false;

    // the dimension bound expressions
    if( !m_pTypeData.IsEmpty() )
    {
        CStSubrangeListNode* psrl = m_pTypeData;
        if( psrl == nullptr || !psrl->IsSymbolResolved() )
            return false;
    }

    // the initializer
    if( !m_pTypeInit.IsEmpty() )
    {
        CStArrayInitNode* pai = m_pTypeInit;
        if( pai != nullptr && !pai->IsSymbolResolved() )
            return false;
    }

    if( !IsElementaryType( m_iDirectType ) )
    {
        if( m_pDirectType.IsEmpty() )
            return false;
        CStTypeDeclNode* pdt = m_pDirectType;
        if( !pdt )
            return false;
        if( pdt->GetClsid() == clsid( CStArrayTypeDeclNode ) )
        {
            // the element may point back at this node; rely on
            // its recorded state instead of recursing
            if( pdt->GetTypeState() != tsDone )
                return false;
        }
        else if( !pdt->IsSymbolResolved() )
            return false;
    }

    return true;
}

gint32 CStArrayTypeDeclNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    // the state guard turns a cyclic element type, such as
    // 'a : ARRAY [1..2] OF a', into a permanent error instead
    // of an endless recursion
    gint32 ret = BeginResolve();
    if( ERROR( ret ) )
        return ret;
    if( ret == 1 )
        return 0;

    // resolve the element type: m_oTypeRef.first
    // names either an elementary type or another
    // named type
    ret = 0;
    do{
        stdstr strType = this->m_oTypeRef.first;
        auto pCtx = reinterpret_cast
            < CStParseContext* >( pctx );
        auto& ost = pCtx->m_oSymTab;
        std::vector< stdstr > vecComps =
            ost.BreakdownQualified( strType );

        auto pScope = ost.GetCurrentScope();
        SymPtr pSym;
        gint32 iType = -1;

        pSym = this->m_oTypeRef.second;
        // an elementary element type, including a string, is
        // already recorded at build time and has no name to
        // look up
        if( !IsElementaryType( m_iDirectType ) &&
            ( pSym.IsEmpty() ||
            this->m_iDirectType == -1 ) )
        {
            ObjPtr pDirectType;
            pSym.Clear();
            ret = ResolveTypeToDirectType(
                pCtx, vecComps, iKind,
                pScope, pSym, iType, pDirectType );

            if( ERROR( ret ) )
                break;

            if( SUCCEEDED( ret ) &&
                !pDirectType.IsEmpty() )
                m_pDirectType = pDirectType;

            if( !pSym.IsEmpty() )
                this->m_oTypeRef.second = pSym;

            if( iType >= 0 )
                this->m_iDirectType = iType;

            if( m_pDirectType.IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
        }

        CStTypeDeclNode* pType = m_pDirectType;
        if( pType )
        {
            ret = pType->ResolveSymbol( pctx, iKind );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            // TODO: emit unexpected error
        }

        CStTypeDeclNode* pElemType = m_pElemType;
        if( pElemType )
        {
            ret = pElemType->ResolveSymbol(
                pctx, iKind );
            if( ERROR( ret ) )
                break;
        }

        if( !m_pTypeData.IsEmpty() )
        {
            CStSubrangeListNode* psrl = m_pTypeData;
            if( psrl != nullptr )
            {
                ret = psrl->ResolveSymbol( pctx, iKind );
                if( ERROR( ret ) )
                    break;
            }
        }

        if( !m_pTypeInit.IsEmpty() )
        {
            CStArrayInitNode* pai = m_pTypeInit;
            if( pai != nullptr )
            {
                ret = pai->ResolveSymbol( pctx, iKind );
                if( ERROR( ret ) )
                    break;
            }
        }

    }while( 0 );

    EndResolve( ret );
    return ret;
}

gint32 CStStructTypeDeclNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    // the state guard also catches a struct that contains
    // itself by value, e.g. 's : STRUCT x : s; END_STRUCT',
    // and a mutual struct cycle
    gint32 ret = BeginResolve();
    if( ERROR( ret ) )
        return ret;
    if( ret == 1 )
        return 0;

    do{
        for( auto& member : m_vecMembers )
        {
            CStTypeDeclNode* ptd = member.m_pType;
            if( ptd == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            ret = ptd->ResolveSymbol( pctx, iKind );
            if( ERROR( ret ) )
                break;

            // a member that resolves back to this struct by
            // value is a cyclic declaration. A reference to
            // this struct, e.g. 'r : REF_TO s', adds no
            // inline storage and is legal
            if( ptd->GetClsid() ==
                    clsid( CStRefTypeDeclNode ) ||
                ptd->m_iTypeId == stparser::TOK_REF_TO )
                continue;

            CStTypeDeclNode* pDirect = ptd->m_pDirectType;
            if( pDirect == ( CStTypeDeclNode* )this )
            {
                ret = -EINVAL;   // cyclic struct declaration
                break;
            }
        }
    }while( 0 );

    EndResolve( ret );
    return ret;
}

gint32 CStStructTypeDeclNode::LookupMember(
    const stdstr& strName, ObjPtr& pType ) const
{
    for( const auto& member : m_vecMembers )
    {
        if( member.m_strName != strName )
            continue;
        pType = member.m_pType;
        // the member is declared, but its type
        // node is not built yet: let the caller
        // retry
        return pType.IsEmpty() ? -ENOENT : 0;
    }
    // members of a declared struct are fixed;
    // retrying will never make the name appear
    return -EINVAL;
}

bool CStSubrangeTypeDeclNode::IsSymbolResolved() const
{
    if( m_iDirectType != stparser::RuleSubrange_type_decl )
        return false;

    if( !m_pTypeData.IsEmpty() )
    {
        CStSubrangeNode* psr = m_pTypeData;
        if( psr == nullptr || !psr->IsSymbolResolved() )
            return false;
    }

    if( !m_pTypeInit.IsEmpty() )
    {
        CStExprBase* pExpr = m_pTypeInit;
        if( pExpr != nullptr && !pExpr->IsSymbolResolved() )
            return false;
    }

    return true;
}

gint32 CStSubrangeTypeDeclNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    gint32 ret = BeginResolve();
    if( ERROR( ret ) )
        return ret;
    if( ret == 1 )
        return 0;

    do{
        // a named (derived) base type must be resolved first;
        // an elementary base is already recorded at parse time
        if( m_iDirectType != stparser::RuleSubrange_type_decl )
        {
            ret = ResolveOwnSymbol( pctx, iKind );
            if( ERROR( ret ) )
                break;
        }

        if( !m_pTypeData.IsEmpty() )
        {
            CStSubrangeNode* psr = m_pTypeData;
            if( psr != nullptr )
            {
                ret = psr->ResolveSymbol( pctx, iKind );
                if( ERROR( ret ) )
                    break;
            }
        }

        if( !m_pTypeInit.IsEmpty() )
        {
            CStExprBase* pExpr = m_pTypeInit;
            if( pExpr != nullptr )
            {
                ret = pExpr->ResolveSymbol( pctx, iKind );
                if( ERROR( ret ) )
                    break;
            }
        }

        // the subrange type is a type of its own, not an alias
        // of its base type
        m_iDirectType = stparser::RuleSubrange_type_decl;

    }while( 0 );

    EndResolve( ret );
    return ret;
}

// ========================================================================
// CStStmtListNode Implementation
// ========================================================================

std::string CStStmtListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "StmtList{ stmts: " << m_vecStatements.size() << " }";
    return oss.str();
}

// ========================================================================
// CStIfBranchListNode Implementation
// ========================================================================

std::string CStIfBranchListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "IfBranchList{ branches: " << m_vecBranches.size() << " }";
    return oss.str();
}

// ========================================================================
// CStCallExpr Implementation
// ========================================================================

std::string CStCallExpr::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "CallExpr{ positional: " << m_vecArgs.size()
        << ", named: " << m_vecNamedArgs.size() << " }";
    return oss.str();
}

// ========================================================================
// CStArgListNode Implementation
// ========================================================================

std::string CStArgListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "ArgList{ positional: " << m_vecArgs.size()
        << ", named: " << m_vecNamed.size() << " }";
    return oss.str();
}

// ========================================================================
// CStArrayInitNode Implementation
// ========================================================================

std::string CStArrayInitNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "ArrayInit{ values: " << m_vecValues.size() << " }";
    return oss.str();
}

bool CStArrayInitNode::IsSymbolResolved() const
{
    for( const auto& pVal : m_vecValues )
    {
        if( !array_init::IsStInitValueResolved( pVal ) )
            return false;
    }
    return true;
}

gint32 CStArrayInitNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    for( const auto& pVal : m_vecValues )
    {
        gint32 ret = array_init::ResolveStInitValue(
            pctx, iKind, pVal );
        if( ERROR( ret ) )
            return ret;
    }
    return 0;
}

// ========================================================================
// CStStructInitNode Implementation
// ========================================================================

std::string CStStructInitNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "StructInit{ members: " << m_vecElemInits.size() << " }";
    return oss.str();
}

// ========================================================================
// CStIdentifierListNode Implementation
// ========================================================================

std::string CStIdentifierListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "IdentifierList{ " << JoinStrings( m_vecIdentifiers, ", " ) << " }";
    return oss.str();
}

std::string CStVarDeclListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "VarDeclList{ decls: " << m_vecVarDecls.size() << " }";
    return oss.str();
}

std::string CStVarConfigListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "VarConfigList{ configs: " << m_vecConfigs.size() << " }";
    return oss.str();
}

std::string CStProgConfigListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "ProgConfigList{ progs: " << m_vecProgs.size() << " }";
    return oss.str();
}

std::string CStProgConfigNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "ProgConfig{ name: " << m_strProgName << ", params: " << m_vecParams.size() << " }";
    return oss.str();
}

std::string CStSymbolicVariableRef::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "SymbolicVar{ this: " << ( m_bThis ? "yes" : "no" )
        << ", super: " << ( m_bSuper ? "yes" : "no" ) << " }";
    return oss.str();
}

bool CStSymbolicVariableRef::IsSymbolResolved() const
{
    if( m_bNull || m_bConstant )
        return true;

    if( m_pDeclNode.IsEmpty() )
        return false;

    // the declaration itself must be resolved
    CStSymbolDeclNode* pDecl = m_pDeclNode;
    if( pDecl != nullptr && !pDecl->IsSymbolResolved() )
        return false;
    // TODO: CStVarDeclNode does not expose IsSymbolResolved()
    // yet; check its type and initializer here once it does.

    // every element of the access chain must be resolved too
    for( const auto& pElem : m_vecMultiElemChain )
    {
        if( pElem.IsEmpty() )
            continue;

        if( pElem->GetClsid() ==
            clsid( CStMemberAccessExpr ) )
        {
            CStMemberAccessExpr* pMember = pElem;
            if( pMember != nullptr &&
                !pMember->IsSymbolResolved() )
                return false;
            // TODO: the member's resulting type is not
            // determined yet; validate the next chain element
            // against it once type resolution is available.
            continue;
        }

        // a subscript group is a CStlObjVector holding the
        // index expressions
        CStSubscriptList* paae = pElem;
        if( paae != nullptr )
        {
            for( const auto& pIndex : paae->m_vecIndices )
            {
                if( pIndex.IsEmpty() )
                    continue;
                CStExprBase* pExpr = pIndex;
                if( pExpr != nullptr &&
                    !pExpr->IsSymbolResolved() )
                    return false;
            }
            // TODO: the indexed element's type is not
            // determined yet.
            continue;
        }

        // TODO: unknown multi-element item; keep it
        // unresolved until its type can be determined
        return false;
    }

    return true;
}

gint32 CStSymbolicVariableRef::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    return primary_expr::ResolveStSymbolicVariableRef(
        reinterpret_cast< CStParseContext* >( pctx ),
        this, iKind );
}

bool CStMemberAccessExpr::IsSymbolResolved() const
{
    if( m_pDeclNode.IsEmpty() )
        return false;

    CStSymbolDeclNode* pDecl = m_pDeclNode;
    if( pDecl != nullptr && !pDecl->IsSymbolResolved() )
        return false;

    // TODO: verify the member against the object's type once
    // member lookup is implemented
    return true;
}

gint32 CStMemberAccessExpr::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    // TODO: resolve the member in the object's type
    ( void )pctx;
    ( void )iKind;
    if( m_pDeclNode.IsEmpty() )
        return -ENOENT;
    return 0;
}

std::string CStFbTaskNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "FbTask{ carets: " << m_iCaretCount << " }";
    return oss.str();
}

std::string CStProgCnxnNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "ProgCnxn{}";
    return oss.str();
}

std::string CStProgConfElemNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "ProgConfElem{ type: " << ( m_eType == elemFbTask ? "fb_task" : "prog_cnxn" ) << " }";
    return oss.str();
}

std::string CStMethodDeclListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "MethodList{ methods: " << m_vecMethods.size() << " }";
    return oss.str();
}

std::string CStCaseBranchListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "CaseBranchList{ branches: " << m_vecBranches.size() << " }";
    return oss.str();
}

std::string CStDirectAddressNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "DirectAddr{ " << m_strAddress;
    switch( m_eAddrType )
    {
        case datRpcf:
            oss << ", rpcf";
            break;
        case datPeripheral:
            oss << ", peripheral";
            break;
        case datPeripheralOffset:
            oss << ", peripheral+offset";
            break;
        default:
            break;
    }
    if( !m_pIndex.IsEmpty() )
        oss << "[expr]";
    oss << " }";
    return oss.str();
}

std::string CStDirectAddressNode::GetSignature() const
{ return m_strAddress; }

std::string CStArrayRepeatNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "Repeat{ count: " << m_iCount << ", element ";
    if( !m_pElement.IsEmpty() )
        oss << "set";
    else
        oss << "none";
    oss << " }";
    return oss.str();
}

bool CStArrayRepeatNode::IsSymbolResolved() const
{
    return array_init::IsStInitValueResolved( m_pElement );
}

gint32 CStArrayRepeatNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    return array_init::ResolveStInitValue(
        pctx, iKind, m_pElement );
}

bool CStNamespaceDecl::IsGlobalScope() const 
{ return m_pSymbol->m_strName.empty(); }

bool CStNamespaceDecl::IsNamedNamespace() const
{ return !m_pSymbol->m_strName.empty(); }

gint32 CStNamespaceDecl::ResolveUsingNs( void* pctx )
{
    CStScope* pScope = m_pSymbol->m_pDefScope;
    return stc::ResolveUsingNs(
        pctx, ObjPtr( pScope ) );
}


ObjPtr CStNamespaceDecl::ResolvePath(
    const std::vector<std::string>& vecNames,
    void* pctx )
{
    size_t iPos;
    enumPathStop eReason = pathStopResolved;
    auto pSym = ResolveNsPrefix(
        vecNames, pctx, iPos, eReason );
    if( pSym.IsEmpty() ) 
        return pSym;
    if( eReason != pathStopResolved )
        return ObjPtr();
    return pSym;
}


gint32 CStEnumTypeDeclNode::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    if( iKind != symDataType )
        return ERROR_FALSE;
    if( pctx == nullptr )
        return -EINVAL;
    if( m_pTypeInit.IsEmpty() )
        return 0;
    CStPrimaryExprNode* pExpr = m_pTypeInit;
    if( pExpr == nullptr )
        return 0;
    return pExpr->ResolveSymbol(
        pctx, symEnumValue );
}

bool CStEnumTypeDeclNode::IsSymbolResolved() const
{
    if( !IsDirectType( m_iDirectType ) )
        return false;
    if( m_pTypeInit.IsEmpty() )
        return true;
    CStPrimaryExprNode* pExpr = m_pTypeInit;
    if( pExpr == nullptr )
        return true;
    return pExpr->IsSymbolResolved();
}

gint32 CStEnumValueRef::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    if( iKind != symEnumValue &&
        iKind != symNamedValue )
        return ERROR_FALSE;
    if( pctx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( IsSymbolResolved() )
            break;
        auto pCtx = reinterpret_cast
            < CStParseContext* >( pctx );
        auto& ost = pCtx->m_oSymTab;
        if( m_strQualified.size() )
        {
            std::vector< stdstr > vecComps =
                ost.BreakdownQualified( m_strQualified );

            auto pScope = ost.GetCurrentScope();
            SymPtr pSym;
            gint32 iType = -1;

            ObjPtr pDirectType;
            ret = ResolveTypeToDirectType(
                pCtx, vecComps, symDataType,
                pScope, pSym, iType, pDirectType );
            if( ERROR( ret ) )
                break;

            if( m_pDeclNode == nullptr )
                m_pDeclNode = pDirectType;
            else if( m_pDeclNode->GetObjId() !=
                pDirectType->GetObjId() )
            {
                ret = ERROR_FALSE;
                break;
            }

            CStEnumTypeDeclNode* petd = m_pDeclNode;
            auto& vecNames = petd->m_vecValNames;
            size_t i = 0;
            for( ; i < vecNames.size(); i++ )
            {
                if( petd->m_vecValNames[ i ] ==
                    m_strValue )
                    break;
            }
            if( i == vecNames.size() )
            {
                ret = ERROR_FALSE;
                break;
            }
            m_iIndex = i;
        }
    }while( 0 );
    return ret;
}

gint32 CStNamedvalValueRef::ResolveSymbol(
    void* pctx, enumSymKind iKind )
{
    if( iKind != symNamedValue )
        return ERROR_FALSE;
    if( pctx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( IsSymbolResolved() )
            break;
        auto pCtx = reinterpret_cast
            < CStParseContext* >( pctx );
        auto& ost = pCtx->m_oSymTab;
        if( m_strQualified.size() )
        {
            std::vector< stdstr > vecComps =
                ost.BreakdownQualified( m_strQualified );

            auto pScope = ost.GetCurrentScope();
            SymPtr pSym;
            gint32 iType = -1;

            ObjPtr pDirectType;
            ret = ResolveTypeToDirectType(
                pCtx, vecComps, symDataType,
                pScope, pSym, iType, pDirectType );
            if( ERROR( ret ) )
                break;

            if( m_pDeclNode == nullptr )
                m_pDeclNode = pDirectType;
            else if( m_pDeclNode->GetObjId() !=
                pDirectType->GetObjId() )
            {
                ret = ERROR_FALSE;
                break;
            }

            CStNamedvalTypeDeclNode* pntd = m_pDeclNode;
            auto& vecNames = pntd->m_vecValNames;
            size_t i = 0;
            for( ; i < vecNames.size(); i++ )
            {
                if( pntd->m_vecValNames[ i ] ==
                    m_strValue )
                    break;
            }
            if( i == vecNames.size() )
            {
                ret = ERROR_FALSE;
                break;
            }
            m_iIndex = i;
        }
    }while( 0 );
    return ret;
}

// ========================================================================
// Default implementations for AST nodes
// These use base class dummy implementation via override
// ========================================================================

#define IMPL_GETNODEINFO(ClsName) \
std::string ClsName::GetNodeInfo() const \
{ \
    return ""; \
}

#define IMPL_GETSIGNATURE(ClsName) \
std::string ClsName::GetSignature() const \
{ \
    return ""; \
}

// Expression nodes
IMPL_GETNODEINFO( CStExprBase )
IMPL_GETSIGNATURE( CStExprBase )
IMPL_GETNODEINFO( CStLiteralExpr )
// CStIdentifierExpr has inline impl in header
IMPL_GETNODEINFO( CStBinaryExpr )
IMPL_GETNODEINFO( CStUnaryExpr )
// CStCallExpr has explicit implementation
IMPL_GETNODEINFO( CStSubscriptList )
IMPL_GETNODEINFO( CStMemberAccessExpr )
IMPL_GETNODEINFO( CStDereferenceExpr )
IMPL_GETNODEINFO( CStPointerMemberExpr )

// L-value wrapper nodes (explicit implementations above)
// CStLValueNode has explicit implementation
// CStLValueExtNode has explicit implementation

// Type nodes
IMPL_GETNODEINFO( CStTypeNode )
IMPL_GETSIGNATURE( CStTypeNode )
IMPL_GETNODEINFO( CStConstValueNode )
IMPL_GETNODEINFO( CStEnumValueRef )
IMPL_GETNODEINFO( CStNamedvalValueRef )
IMPL_GETNODEINFO( CStAmbiguousValueRef )
IMPL_GETNODEINFO( CStArrayTypeDeclNode )
IMPL_GETNODEINFO( CStStructTypeDeclNode )
// CStDataTypeDeclNode has explicit implementation
// CStTypeDeclNode has explicit implementation
IMPL_GETNODEINFO( CStPointerTypeNode )
IMPL_GETNODEINFO( CStReferenceTypeNode )
IMPL_GETNODEINFO( CStDerivedTypeNode )

// Variable declaration
IMPL_GETNODEINFO( CStVarDeclNode )

// Statement nodes
IMPL_GETNODEINFO( CStStmtNode )
IMPL_GETNODEINFO( CStAssignStmt )
IMPL_GETNODEINFO( CStCallStmt )
IMPL_GETNODEINFO( CStIfStmt )
IMPL_GETNODEINFO( CStForStmt )
IMPL_GETNODEINFO( CStWhileStmt )
IMPL_GETNODEINFO( CStRepeatStmt )
IMPL_GETNODEINFO( CStCaseStmt )
IMPL_GETNODEINFO( CStPragmaStmt )

// POU declaration nodes
IMPL_GETNODEINFO( CStPouDeclNode )
IMPL_GETNODEINFO( CStProgramDecl )
IMPL_GETNODEINFO( CStFunctionBlockDecl )
IMPL_GETNODEINFO( CStFunctionBlockHeaderNode )
IMPL_GETNODEINFO( CStFunctionDecl )
IMPL_GETNODEINFO( CStMethodDecl )

// Other declaration nodes
IMPL_GETNODEINFO( CStNamespaceDecl )
IMPL_GETNODEINFO( CStInterfaceDecl )
IMPL_GETNODEINFO( CStVarConfigDecl )
IMPL_GETNODEINFO( CStTaskConfigNode )
IMPL_GETNODEINFO( CStTaskConfigListNode )
IMPL_GETNODEINFO( CStUsingDirective )
IMPL_GETNODEINFO( CStAccessDeclNode )
IMPL_GETNODEINFO( CStAccessDeclListNode )
IMPL_GETNODEINFO( CStAccessDeclsNode )
IMPL_GETNODEINFO( CStGlobalVarDeclListNode )
IMPL_GETNODEINFO( CStConfigDeclNode )
IMPL_GETNODEINFO( CStDeclsNode )
IMPL_GETNODEINFO( CStSingleResourceDeclNode )
IMPL_GETNODEINFO( CStResourceDeclNode )
IMPL_GETNODEINFO( CStResourceDeclListNode )
IMPL_GETNODEINFO( CStResourceSectionNode )

#undef IMPL_GETNODEINFO

} // namespace stc
