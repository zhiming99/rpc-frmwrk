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

#include "stlexer.h"
#include "stclsids.h"
#include "astnodes.h"
#include <sstream>
#include <string_view>
#include "stsymtab.h"
#include "st_parser_ext.h"

namespace rpcf
{

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

gint32 ResolveStSymbolicVariableRef( CStParseContext* pCtx,
    CStSymbolicVariableRef* pRef, enumSymKind iKind )
{
    ( void )iKind;
    if( pCtx == nullptr || pRef == nullptr )
        return -EINVAL;

    // 'NULL' is a constant reference and needs no lookup
    if( pRef->m_bNull || pRef->m_bConstant )
        return 0;

    std::string strName;
    if( !pRef->m_strQualified.empty() )
    {
        strName = pRef->m_strQualified;
        if( !pRef->m_strVarName.empty() )
            strName += "." + pRef->m_strVarName;
    }
    else
        strName = pRef->m_strVarName;

    if( strName.empty() )
        return -EINVAL;

    SymPtr pSym;
    gint32 iOffset = -1;
    gint32 ret = ResolveSymbolicRef(
        pCtx, strName, pSym, iOffset );
    if( ERROR( ret ) )
    {
        // -ENOENT with a bound prefix means the remaining
        // components are member accesses: keep the prefix and
        // its kind so IsFbInstance/IsClassInstance can answer.
        if( ret != -ENOENT || pSym.IsEmpty() )
            return ret;
    }

    pRef->m_pDeclNode = pSym->m_pDeclNode;
    pRef->m_bFb = ( pSym->m_eKind == symFunctionBlock );
    pRef->m_bClass = ( pSym->m_eKind == symClass );
    return 0;
}

}  // namespace primary_expr

}  // namespace rpcf
