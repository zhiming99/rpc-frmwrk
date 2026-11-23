/*
 * =====================================================================================
 *
 *       Filename:  primexpr.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/14/2026 05:38:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
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

namespace primary_expr
{

static gint32 FindStValueIndex(
    const std::vector< stdstr >& vecNames,
    const stdstr& strValue )
{
    for( size_t i = 0; i < vecNames.size(); i++ )
    {
        if( vecNames[ i ] == strValue )
            return ( gint32 )i;
    }
    return -1;
}

static ObjPtr MakeStAmbiguousValueRef( const char* szRefText,
    antlr4::Token* pStart, antlr4::Token* pStop )
{
    ObjPtr pObj;
    pObj.NewObj( clsid( CStAmbiguousValueRef ) );
    CParamList oParams;
    oParams.Push( std::string(
        szRefText != nullptr ? szRefText : "" ) );
    oParams.SetIntProp( propStRuleId,
        ( stparser::RulePrimary_expr << 16 ) +
        stparser::RuleEnum_value );
    CStAmbiguousValueRef* pavr = pObj;
    pavr->m_pRefInfo = oParams.GetCfg();
    pavr->SetTokens( pStart, pStop );
    return pObj;
}

ObjPtr BuildStValueRefNode( CStParseContext* pCtx, ObjPtr pRef,
    const char* szRefText,
    antlr4::Token* pStart, antlr4::Token* pStop )
{
    if( pCtx == nullptr || pRef.IsEmpty() )
        return ObjPtr();

    CStEnumValueRef* pref = pRef;
    if( pref == nullptr )
        return ObjPtr();

    auto& ost = pCtx->m_oSymTab;
    SymPtr pSym;
    gint32 iType = -1;
    gint32 ret = 0;

    ObjPtr pDirectType;
    if( !pref->m_strQualified.empty() )
    {
        // 'Type#name': the qualifier must name a data type
        std::vector< stdstr > vecComps =
            ost.BreakdownQualified( pref->m_strQualified );
        auto pScope = ost.GetCurrentScope();
        ret = ResolveTypeToDirectType( pCtx, vecComps,
            symDataType, pScope, pSym, iType,
            pDirectType );
        if( ERROR( ret ) || pSym.IsEmpty() )
            return MakeStAmbiguousValueRef(
                szRefText, pStart, pStop );
    }
    else
    {
        // bare name: could be an enum value, a named value or
        // a variable
        gint32 iOffset = -1;
        ret = ResolveSymbolicRef( pCtx, pref->m_strValue,
            pSym, iOffset );
        if( ERROR( ret ) || pSym.IsEmpty() ||
            pSym->m_pDeclNode.IsEmpty() )
            return MakeStAmbiguousValueRef(
                szRefText, pStart, pStop );

        gint32 iClsid = pSym->m_pDeclNode->GetClsid();
        if( iClsid == clsid( CStEnumTypeDeclNode ) )
            iType = stparser::RuleEnum_type_decl;
        else if( iClsid == clsid( CStNamedvalTypeDeclNode ) )
            iType = stparser::RuleNamedval_type_decl;
        else
        {
            // TODO: a variable or another kind of symbol

            return MakeStAmbiguousValueRef(
                szRefText, pStart, pStop );
        }
    }

    if( iType == stparser::RuleEnum_type_decl )
    {
        CStEnumTypeDeclNode* petd = pSym->m_pDeclNode;
        if( petd == nullptr )
            return MakeStAmbiguousValueRef(
                szRefText, pStart, pStop );

        gint32 i = FindStValueIndex(
            petd->m_vecValNames, pref->m_strValue );
        if( i < 0 )
        {
            pCtx->LogError( pStart,
                "Error undeclared enum value '%s'.",
                szRefText != nullptr ? szRefText : "" );
            return ObjPtr();
        }
        pref->m_pDeclNode = pSym->m_pDeclNode;
        pref->m_iIndex = i;
        pref->SetTokens( pStart, pStop );
        return pRef;
    }

    if( iType == stparser::RuleNamedval_type_decl )
    {
        CStNamedvalTypeDeclNode* pntd = pSym->m_pDeclNode;
        if( pntd == nullptr )
            return MakeStAmbiguousValueRef(
                szRefText, pStart, pStop );

        gint32 i = FindStValueIndex(
            pntd->m_vecValNames, pref->m_strValue );
        if( i < 0 )
        {
            pCtx->LogError( pStart,
                "Error undeclared namedval '%s'.",
                szRefText != nullptr ? szRefText : "" );
            return ObjPtr();
        }

        ObjPtr pObj;
        pObj.NewObj( clsid( CStNamedvalValueRef ) );
        CStNamedvalValueRef* pnref = pObj;
        pnref->m_strValue = pref->m_strValue;
        pnref->m_strQualified = pref->m_strQualified;
        pnref->m_pDeclNode = pSym->m_pDeclNode;
        pnref->m_iIndex = i;
        pnref->SetTokens( pStart, pStop );
        return pObj;
    }

    pCtx->LogError( pStart, "Error unknown type for '%s'.",
        szRefText != nullptr ? szRefText : "" );
    return ObjPtr();
}

// Resolve one '.member' step against pCurType and return the
// member's type node in pMemberType.
static gint32 ResolveStMemberStep( CStParseContext* pCtx,
    CStTypeDeclNode* pCurType, const std::string& strMember,
    CStTypeDeclNode*& pMemberType )
{
    pMemberType = nullptr;
    if( pCtx == nullptr || pCurType == nullptr ||
        strMember.empty() )
        return -EINVAL;

    if( pCurType->GetClsid() !=
        clsid( CStStructTypeDeclNode ) )
    {
        // TODO: emit error type mismatch
        return -EINVAL;
    }

    auto pst = static_cast
        < CStStructTypeDeclNode* >( pCurType );
    ObjPtr pType;
    gint32 ret = pst->LookupMember( strMember, pType );
    if( ERROR( ret ) )
        return ret;

    CStTypeDeclNode* ptd = pType;
    if( ptd == nullptr )
        return -ENOENT;

    if( !ptd->IsSymbolResolved() )
        return -ENOENT;

    pMemberType = ptd;
    return 0;
}

gint32 ResolveStSymbolicVariableRef( CStParseContext* pCtx,
    CStSymbolicVariableRef* pRef, enumSymKind iKind )
{
    ( void )iKind;
    if( pCtx == nullptr || pRef == nullptr )
        return -EINVAL;

    // 'NULL' is a constant reference and needs no
    // lookup
    if( pRef->m_bNull || pRef->m_bConstant )
        return 0;
    gint32 ret = 0;

    do{
        std::string strName;
        // 'THIS' and 'SUPER' are not stored by name;
        // prefix them so that ResolveSymbolicRef
        // starts the lookup at the enclosing unit or
        // its base type instead of the current scope
        if( pRef->m_bThis )
            strName = "THIS";
        else if( pRef->m_bSuper )
            strName = "SUPER";
        if( !pRef->m_strQualified.empty() )
        {
            if( !strName.empty() )
                strName += ".";
            strName += pRef->m_strQualified;
        }
        if( !pRef->m_strVarName.empty() )
        {
            if( !strName.empty() )
                strName += ".";
            strName += pRef->m_strVarName;
        }

        if( strName.empty() )
        {
            ret = -EINVAL;
            break;
        }

        SymPtr pSym;
        gint32 iOffset = -1;

        auto& ost = pCtx->m_oSymTab;
        std::vector< std::string > vecNames =
            ost.BreakdownQualified( strName );
        if( vecNames.empty() )
        {
            ret = -EINVAL;
            break;
        }

        CStScope* pScope = ost.GetCurrentScope();
        ret = ResolveSymbolicRef( pCtx,
            vecNames, pSym, iOffset, pScope );

        // The components the scope lookup could not bind. A
        // bound prefix with leftover components is a member
        // path, not a forward reference: 'obj.member' is parsed
        // as the qualified name 'obj' plus the name 'member',
        // and 'member' lives in obj's type, not in a scope.
        std::vector< std::string > vecMembers;
        if( ERROR( ret ) )
        {
            if( ret != -ENOENT || pSym.IsEmpty() )
            {
                // fatal error: permanent, or nothing bound
                break;
            }
            if( pSym->m_eKind == symNamespace )
            {
                // a namespace member may still be declared
                // after this reference
                ret = -ENOENT;
                break;
            }
            if( iOffset < 0 ||
                ( size_t )iOffset >= vecNames.size() )
            {
                // partial match without a position: the
                // lookup and this caller disagree
                ret = -EINVAL;
                break;
            }
            vecMembers.insert( vecMembers.end(),
                vecNames.begin() + iOffset, vecNames.end() );
            ret = 0;
        }

        // the type the remaining path starts from
        CStTypeDeclNode* pCurType = nullptr;
        if( pSym->m_eKind == symVariable )
        {
            if( !pSym->m_pDeclNode.IsEmpty() &&
                pSym->m_pDeclNode->GetClsid() ==
                    clsid( CStVarDeclNode ) )
            {
                CStVarDeclNode* pVar = pSym->m_pDeclNode;
                if( !pVar->m_pType.IsEmpty() )
                    pCurType = pVar->m_pType;
            }
        }
        else
        {
            pCurType = pSym->m_pDeclNode;
        }

        // the dotted components the scope lookup could not bind
        // come first, then the parsed multi-element chain
        auto& vecChain = pRef->m_vecMultiElemChain;
        size_t iMember = 0;
        size_t iElem = 0;
        while( iMember < vecMembers.size() ||
            iElem < vecChain.size() )
        {
            if( pCurType == nullptr )
            {
                // TODO:emit error type mismatch
                ret = -EINVAL;
                break;
            }
            if( IsDerivedType( pCurType->m_iTypeId ) )
                pCurType = pCurType->m_pDirectType;
            if( pCurType == nullptr )
            {
                // TODO:emit unexpected error
                ret = -EINVAL;
                break;
            }
            if( !pCurType->IsSymbolResolved() )
            {
                ret = -ENOENT;
                break;
            }

            if( iMember < vecMembers.size() )
            {
                CStTypeDeclNode* pType = nullptr;
                ret = ResolveStMemberStep( pCtx, pCurType,
                    vecMembers[ iMember ], pType );
                if( ERROR( ret ) )
                    break;
                pCurType = pType;
                iMember++;
                continue;
            }

            ObjPtr pElem = vecChain[ iElem++ ];
            if( pElem->GetClsid() ==
                clsid( CStSubscriptList ) )
            {
                if( pCurType->GetClsid() !=
                    clsid( CStArrayTypeDeclNode ) )
                {
                    // TODO:emit error type mismatch
                    ret = -EINVAL;
                    break;
                }

                if( pCurType->m_iDirectType == -1 )
                {
                    // unresolved symbol
                    ret = -ENOENT;
                    break;
                }
                auto pRangeList = ( CStSubrangeListNode* )
                    pCurType->m_pTypeData;
                if( pRangeList == nullptr )
                {
                    // TODO: unexpected error
                    ret = -EFAULT;
                    break;
                }
                auto psl = ( CStSubscriptList* )pElem;

                if( !psl->IsSymbolResolved() )
                {
                    ret = -ENOENT;
                    break;
                }

                if( pRangeList->m_vecRanges.size() !=
                    psl->m_vecIndices.size() )
                {
                    // TODO: emit array dimension mismatch.
                    ret = -EINVAL;
                    break;
                }
                pCurType = pCurType->m_pDirectType;
            }
            else if( pElem->GetClsid() ==
                clsid( CStMemberAccessExpr ) )
            {
                auto pelem = ( CStMemberAccessExpr* )pElem;
                CStTypeDeclNode* pType = nullptr;
                ret = ResolveStMemberStep( pCtx, pCurType,
                    pelem->m_strMember, pType );
                if( ERROR( ret ) )
                    break;
                // what CStMemberAccessExpr::IsSymbolResolved
                // and the later passes look at
                pelem->m_pDeclNode = pType;
                pCurType = pType;
            }
            else
            {
                // TODO: emit unexpected error
                ret = -EINVAL;
                break;
            }
        }
        if( ERROR( ret ) )
            break;
        pRef->m_pDeclNode = pSym->m_pDeclNode;
        pRef->m_bFb = ( pSym->m_eKind == symFunctionBlock );
        pRef->m_bClass = ( pSym->m_eKind == symClass );

    }while( 0 );

    return ret;
}

}  // namespace primary_expr

}  // namespace stc
