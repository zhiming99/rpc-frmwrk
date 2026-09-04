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

#include "stlexer.h"
#include "parsrctx.h"
#include "stclsids.h"
#include "astnodes.h"
#include <sstream>

namespace rpcf
{

// Helper function to join vector of strings
static std::string JoinStrings( const std::vector< std::string >& vec,
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
// CStEnumValueNode Implementation
// ========================================================================

std::string CStEnumValueNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "EnumValue{ name: " << m_strName;
    if( !m_pExplicitValue.IsEmpty() )
        oss << ", explicit";
    oss << " }";
    return oss.str();
}

// ========================================================================
// CStEnumValueListNode Implementation
// ========================================================================

std::string CStEnumValueListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "EnumValueList{ values: " << m_vecValues.size() << " }";
    return oss.str();
}

// ========================================================================
// CStDataTypeSpecNode Implementation
// ========================================================================

std::string CStDataTypeSpecNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "DataTypeSpec{ ";
    if( !m_pTypeSpec.IsEmpty() )
        oss << "type_spec set";
    else
        oss << "no type_spec";
    oss << " }";
    return oss.str();
}

// ========================================================================
// CStTypeSpecNode Implementation
// ========================================================================

std::string CStTypeSpecNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "TypeSpec{ ";
    if( !m_pType.IsEmpty() )
        oss << "type set";
    else
        oss << "no type";
    oss << " }";
    return oss.str();
}

std::string CStTypeDefinitionBlockNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "TypeDefinitionBlock{ " << m_vecTypeDecls.size() << " declarations }";
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
        CSTAstNodeBase* pBase = dynamic_cast< CSTAstNodeBase* >(
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
        CSTAstNodeBase* pBase = dynamic_cast< CSTAstNodeBase* >(
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

std::string CStFullExpressionNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "FullExpr{ ";
    if( !m_pExpression.IsEmpty() )
        oss << "expr set";
    else
        oss << "no expr";
    oss << " }";
    return oss.str();
}

std::string CStFullExpressionNode::GetSignature() const
{
    CStExprNode* pInner = m_pExpression;
    if( pInner != nullptr )
        return pInner->GetSignature();
    return std::string();
}

// ========================================================================
// CStSubrangeNode Implementation
// ========================================================================

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

std::string CStSubrangeListNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "SubrangeList{ dims: " << m_vecRanges.size() << " }";
    return oss.str();
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

// ========================================================================
// CStStructInitNode Implementation
// ========================================================================

std::string CStStructInitNode::GetNodeInfo() const
{
    std::ostringstream oss;
    oss << "StructInit{ members: " << m_vecMembers.size() << " }";
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
IMPL_GETNODEINFO( CStExprNode )
IMPL_GETSIGNATURE( CStExprNode )
IMPL_GETNODEINFO( CStLiteralExpr )
// CStIdentifierExpr has inline impl in header
IMPL_GETNODEINFO( CStBinaryExpr )
IMPL_GETNODEINFO( CStUnaryExpr )
// CStCallExpr has explicit implementation
IMPL_GETNODEINFO( CStArrayAccessExpr )
IMPL_GETNODEINFO( CStMemberAccessExpr )
IMPL_GETNODEINFO( CStDereferenceExpr )
IMPL_GETNODEINFO( CStPointerMemberExpr )

// L-value wrapper nodes (explicit implementations above)
// CStLValueNode has explicit implementation
// CStLValueExtNode has explicit implementation

// Type nodes
IMPL_GETNODEINFO( CStTypeNode )
IMPL_GETSIGNATURE( CStTypeNode )
IMPL_GETNODEINFO( CStBasicTypeNode )
IMPL_GETNODEINFO( CStArrayTypeNode )
IMPL_GETNODEINFO( CStStructTypeNode )
IMPL_GETNODEINFO( CStEnumTypeNode )
// CStDataTypeSpecNode has explicit implementation
// CStTypeSpecNode has explicit implementation
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
IMPL_GETNODEINFO( CStFunctionDecl )
IMPL_GETNODEINFO( CStMethodDecl )

// Other declaration nodes
IMPL_GETNODEINFO( CStNamespaceDecl )
IMPL_GETNODEINFO( CStInterfaceDecl )
IMPL_GETNODEINFO( CStTypeDecl )
IMPL_GETNODEINFO( CStVarConfigDecl )
IMPL_GETNODEINFO( CStUsingDirective )

#undef IMPL_GETNODEINFO

} // namespace rpcf
