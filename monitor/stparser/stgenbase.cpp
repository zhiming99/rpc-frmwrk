/*
 * =====================================================================================
 *
 *       Filename:  stgenbase.cpp
 *
 *    Description:  Implementations of ST to C++ code generator
 *
 *        Version:  1.0
 *        Created:  09/15/2026
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
#include "stgenbase.h"
#include <set>

namespace rpcf
{

// ========================================================================
// Entry Points
// ========================================================================

std::string CStCodeGenerator::Generate( ObjPtr pRoot )
{
    m_strOutput.clear();
    return VisitRoot( pRoot );
}

std::string CStCodeGenerator::GenerateHeader( ObjPtr pRoot )
{
    m_strOutput.clear();
    m_oCtx.m_bIsHeader = true;
    return VisitRoot( pRoot );
}

std::string CStCodeGenerator::GenerateImpl( ObjPtr pRoot )
{
    m_strOutput.clear();
    m_oCtx.m_bIsHeader = false;
    // TODO: Generate implementation (without type definitions)
    return "";
}

// ========================================================================
// Declaration Visitors
// ========================================================================

std::string CStCodeGenerator::VisitRoot( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStRootNode* pRoot = dynamic_cast< CStRootNode* >( &*pNode );
    if( pRoot == nullptr )
        return "";

    // Generate header guard
    AppendRawLine( "#pragma once" );
    AppendRawLine( "" );

    // Generate includes
    AppendRawLine( "#include \"rpc.h\"" );
    AppendRawLine( "" );

    // Visit each declaration
    for( const auto& pDecl : pRoot->m_vecDeclarations )
    {
        Visit( pDecl );
    }

    return m_strOutput;
}

std::string CStCodeGenerator::VisitProgramDecl( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStProgramDecl* p = dynamic_cast< CStProgramDecl* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = MakeValidId( p->m_strName );

    // Program class declaration
    AppendRawLine( "class " + strName + " : public CStProgramBase" );
    AppendRawLine( "{" );
    AppendLine( "public:" );
    m_oCtx.PushIndent();

    // Constructor
    AppendLine( strName + "();" );
    AppendLine( "virtual ~" + strName + "();" );

    // TODO: Add method declarations from m_vecStatements

    m_oCtx.PopIndent();
    AppendRawLine( "};" );
    AppendRawLine( "" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitFunctionBlockDecl( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStFunctionBlockDecl* p = dynamic_cast< CStFunctionBlockDecl* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = MakeValidId( p->m_strName );

    // Determine class modifiers
    std::string strModifier = "";
    if( p->m_eModifier == CStFunctionBlockDecl::fbmAbstract )
        strModifier = " abstract";
    else if( p->m_eModifier == CStFunctionBlockDecl::fbmFinal )
        strModifier = " final";

    // Inheritance
    std::string strExtends = "";
    if( !p->m_strExtends.empty() )
        strExtends = " : public " + MakeValidId( p->m_strExtends );

    AppendRawLine( "class " + strName + strExtends + strModifier );
    AppendRawLine( "{" );
    AppendLine( "public:" );
    m_oCtx.PushIndent();

    // Constructor
    AppendLine( strName + "();" );
    AppendLine( "virtual ~" + strName + "();" );

    // Variable declarations (as member variables)
    if( !p->m_vecInputVars.empty() || !p->m_vecOutputVars.empty() ||
        !p->m_vecInOutVars.empty() || !p->m_vecLocalVars.empty() )
    {
        AppendRawLine( "" );
        AppendLine( "// Input variables" );
        for( const auto& pVar : p->m_vecInputVars )
            VisitVarDecl( pVar );

        AppendLine( "// Output variables" );
        for( const auto& pVar : p->m_vecOutputVars )
            VisitVarDecl( pVar );

        AppendLine( "// Local variables" );
        for( const auto& pVar : p->m_vecLocalVars )
            VisitVarDecl( pVar );
    }

    // Methods
    for( const auto& pMethod : p->m_vecMethods )
        VisitMethodDecl( pMethod );

    m_oCtx.PopIndent();
    AppendRawLine( "};" );
    AppendRawLine( "" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitFunctionDecl( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStFunctionDecl* p = dynamic_cast< CStFunctionDecl* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = MakeValidId( p->m_strName );

    // Return type
    std::string strRetType = "void";
    if( !p->m_pReturnType.IsEmpty() )
        strRetType = VisitType( p->m_pReturnType );

    // Function declaration
    AppendLine( strRetType + " " + strName + "(" );

    // Parameters
    bool bFirst = true;
    for( const auto& pVar : p->m_vecInputVars )
    {
        if( !bFirst )
            AppendRaw( ", " );
        // TODO: Generate parameter
        bFirst = false;
    }
    AppendRawLine( ");" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitMethodDecl( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStMethodDecl* p = dynamic_cast< CStMethodDecl* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = MakeValidId( p->m_strName );

    // Access modifier
    std::string strAccess = "public";
    if( p->m_eAccessModifier == CStMethodDecl::amPrivate )
        strAccess = "private";
    else if( p->m_eAccessModifier == CStMethodDecl::amProtected )
        strAccess = "protected";

    // Return type
    std::string strRetType = "void";
    if( !p->m_pReturnType.IsEmpty() )
        strRetType = VisitType( p->m_pReturnType );

    AppendLine( strAccess + ":" );
    AppendLine( strRetType + " " + strName + "();" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitTypeDecl( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStTypeDecl* p = dynamic_cast< CStTypeDecl* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = MakeValidId( p->m_strName );
    AppendLine( "// Type: " + strName );

    if( !p->m_pTypeDefinition.IsEmpty() )
        Visit( p->m_pTypeDefinition );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitVarDecl( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStVarDeclNode* p = dynamic_cast< CStVarDeclNode* >( &*pNode );
    if( p == nullptr )
        return "";

    // Type
    std::string strType = "auto";
    if( !p->m_pType.IsEmpty() )
        strType = VisitType( p->m_pType );

    // Qualifier
    std::string strQual;
    if( p->m_eQualifier == CStVarDeclNode::vqConstant )
        strQual = "const ";
    else if( p->m_eQualifier == CStVarDeclNode::vqRetain )
        strQual = "/*retain*/ ";
    else if( p->m_eQualifier == CStVarDeclNode::vqPersistent )
        strQual = "/*persistent*/ ";

    // Variable names and initial values
    for( size_t i = 0; i < p->m_vecNames.size(); i++ )
    {
        std::string strName = MakeValidId( p->m_vecNames[i] );
        std::string strInit;

        if( !p->m_pInitialValue.IsEmpty() )
        {
            strInit = " = " + Visit( p->m_pInitialValue );
        }

        if( p->m_bAtDirectAddress )
        {
            AppendLine( "// AT " + p->m_strDirectAddress );
        }

        AppendLine( strQual + strType + " " + strName + strInit + ";" );
    }

    return m_strOutput;
}

std::string CStCodeGenerator::VisitNamespace( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStNamespaceDecl* p = dynamic_cast< CStNamespaceDecl* >( &*pNode );
    if( p == nullptr )
        return "";

    // Empty name means global scope boundary - no wrapping needed
    if( p->IsGlobalScope() )
    {
        for( const auto& pDecl : p->m_vecDeclarations )
            Visit( pDecl );
        return m_strOutput;
    }

    AppendRawLine( "namespace " + p->m_strName + " {" );
    AppendRawLine( "" );

    for( const auto& pDecl : p->m_vecDeclarations )
        Visit( pDecl );

    AppendRawLine( "} // namespace " + p->m_strName );
    AppendRawLine( "" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitUsingDirective( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStUsingDirective* p = dynamic_cast< CStUsingDirective* >( &*pNode );
    if( p == nullptr )
        return "";

    // Generate #include for using directive
    std::string strPath;
    for( const auto& str : p->m_vecNamespace )
    {
        if( !strPath.empty() )
            strPath += "::";
        strPath += str;
    }
    AppendLine( "#include \"" + strPath + ".h\"" );

    return m_strOutput;
}

// ========================================================================
// Statement Visitors
// ========================================================================

std::string CStCodeGenerator::VisitStatement( ObjPtr pNode )
{
    return Visit( pNode );
}

std::string CStCodeGenerator::VisitAssignStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStAssignStmt* p = dynamic_cast< CStAssignStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strLVal = Visit( p->m_pLValue );
    std::string strRVal = Visit( p->m_pRValue );

    AppendLine( strLVal + " = " + strRVal + ";" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitIfStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStIfStmt* p = dynamic_cast< CStIfStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    // IF condition
    std::string strCond = Visit( p->m_pCondition );
    AppendLine( "if ( " + strCond + " ) {" );
    m_oCtx.PushIndent();

    // THEN statements
    for( const auto& pStmt : p->m_vecThenStatements )
        Visit( pStmt );

    m_oCtx.PopIndent();

    // ELSIF branches
    for( const auto& branch : p->m_vecElseIfBranches )
    {
        AppendLine( "} else if ( " + Visit( branch.m_pCondition ) + " ) {" );
        m_oCtx.PushIndent();
        for( const auto& pStmt : branch.m_vecStatements )
            Visit( pStmt );
        m_oCtx.PopIndent();
    }

    // ELSE statements
    if( !p->m_vecElseStatements.empty() )
    {
        AppendLine( "} else {" );
        m_oCtx.PushIndent();
        for( const auto& pStmt : p->m_vecElseStatements )
            Visit( pStmt );
        m_oCtx.PopIndent();
    }

    AppendLine( "}" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitForStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStForStmt* p = dynamic_cast< CStForStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strVar = MakeValidId( p->m_strLoopVar );
    std::string strStart = Visit( p->m_pStartValue );
    std::string strEnd = Visit( p->m_pEndValue );

    AppendLine( "for ( int " + strVar + " = " + strStart + "; " +
                strVar + " <= " + strEnd + "; " );

    if( !p->m_pStepValue.IsEmpty() )
    {
        std::string strStep = Visit( p->m_pStepValue );
        AppendRaw( strVar + " += " + strStep );
    }
    else
    {
        AppendRaw( strVar + "++" );
    }
    AppendRawLine( " ) {" );

    m_oCtx.PushIndent();
    for( const auto& pStmt : p->m_vecBody )
        Visit( pStmt );
    m_oCtx.PopIndent();

    AppendLine( "}" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitWhileStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStWhileStmt* p = dynamic_cast< CStWhileStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strCond = Visit( p->m_pCondition );

    AppendLine( "while ( " + strCond + " ) {" );
    m_oCtx.PushIndent();

    for( const auto& pStmt : p->m_vecBody )
        Visit( pStmt );

    m_oCtx.PopIndent();
    AppendLine( "}" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitRepeatStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStRepeatStmt* p = dynamic_cast< CStRepeatStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    AppendLine( "do {" );
    m_oCtx.PushIndent();

    for( const auto& pStmt : p->m_vecBody )
        Visit( pStmt );

    m_oCtx.PopIndent();

    std::string strCond = Visit( p->m_pCondition );
    AppendLine( "} while ( " + strCond + " );" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitCaseStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStCaseStmt* p = dynamic_cast< CStCaseStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strExpr = Visit( p->m_pExpression );

    AppendLine( "switch ( " + strExpr + " ) {" );
    m_oCtx.PushIndent();

    // CASE branches
    for( const auto& branch : p->m_vecBranches )
    {
        for( const auto& sel : branch.m_vecSelectors )
        {
            std::string strStart = Visit( sel.m_pStartValue );
            if( !sel.m_pEndValue.IsEmpty() )
            {
                std::string strEnd = Visit( sel.m_pEndValue );
                AppendLine( "case " + strStart + " ... " + strEnd + ":" );
            }
            else
            {
                AppendLine( "case " + strStart + ":" );
            }
        }

        m_oCtx.PushIndent();
        for( const auto& pStmt : branch.m_vecStatements )
            Visit( pStmt );
        AppendLine( "break;" );
        m_oCtx.PopIndent();
    }

    // ELSE branch
    if( !p->m_vecElseStatements.empty() )
    {
        AppendLine( "default:" );
        m_oCtx.PushIndent();
        for( const auto& pStmt : p->m_vecElseStatements )
            Visit( pStmt );
        m_oCtx.PopIndent();
    }

    m_oCtx.PopIndent();
    AppendLine( "}" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitCallStmt( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStCallStmt* p = dynamic_cast< CStCallStmt* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strCall = VisitCallExpr( p->m_pCallExpr );
    AppendLine( strCall + ";" );

    return m_strOutput;
}

// ========================================================================
// Expression Visitors
// ========================================================================

std::string CStCodeGenerator::VisitExpression( ObjPtr pNode )
{
    return Visit( pNode );
}

std::string CStCodeGenerator::VisitInitialValue( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStInitialValueNode* p = dynamic_cast< CStInitialValueNode* >( &*pNode );
    if( p == nullptr )
        return "";

    switch( p->m_eInitType )
    {
        case CStInitialValueNode::initExpression:
            // For expressions, visit the inner value directly
            if( !p->m_pValue.IsEmpty() )
                return Visit( p->m_pValue );
            return "";

        case CStInitialValueNode::initArray:
            {
                // Array init: visit inner CStArrayInitNode
                CStArrayInitNode* pArr = dynamic_cast< CStArrayInitNode* >( &*p->m_pValue );
                if( pArr == nullptr )
                    return "{}";

                std::string strResult = "{ ";
                bool bFirst = true;
                for( const auto& pVal : pArr->m_vecValues )
                {
                    if( !bFirst )
                        strResult += ", ";
                    strResult += Visit( pVal );
                    bFirst = false;
                }
                strResult += " }";
                return strResult;
            }

        case CStInitialValueNode::initStruct:
            {
                // Struct init: visit inner CStStructInitNode
                CStStructInitNode* pStruct = dynamic_cast< CStStructInitNode* >( &*p->m_pValue );
                if( pStruct == nullptr )
                    return "{}";

                std::string strResult = "{ ";
                bool bFirst = true;
                for( size_t i = 0; i < pStruct->m_vecMembers.size() && i < pStruct->m_vecValues.size(); i++ )
                {
                    if( !bFirst )
                        strResult += ", ";
                    strResult += ".";
                    strResult += MakeValidId( pStruct->m_vecMembers[i] );
                    strResult += " = ";
                    strResult += Visit( pStruct->m_vecValues[i] );
                    bFirst = false;
                }
                strResult += " }";
                return strResult;
            }
    }

    return "";
}

std::string CStCodeGenerator::VisitLiteral( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStLiteralExpr* p = dynamic_cast< CStLiteralExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    // Simplified literal handling - just return placeholder
    switch( p->m_eLiteralType )
    {
        case CStLiteralExpr::ltNumber:
            return "/* number literal */";
        case CStLiteralExpr::ltBool:
            return "/* bool literal */";
        case CStLiteralExpr::ltString:
        case CStLiteralExpr::ltWString:
            return "/* string literal */";
        default:
            return "/* TODO: literal */";
    }
}

std::string CStCodeGenerator::VisitIdentifier( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStIdentifierExpr* p = dynamic_cast< CStIdentifierExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    return MakeValidId( p->m_strName );
}

std::string CStCodeGenerator::VisitBinaryExpr( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStBinaryExpr* p = dynamic_cast< CStBinaryExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strLeft = Visit( p->m_pLeft );
    std::string strRight = Visit( p->m_pRight );
    std::string strOp = BinaryOpToString( p->m_eOperator );

    return "( " + strLeft + " " + strOp + " " + strRight + " )";
}

std::string CStCodeGenerator::VisitUnaryExpr( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStUnaryExpr* p = dynamic_cast< CStUnaryExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strOperand = Visit( p->m_pOperand );
    std::string strOp = UnaryOpToString( p->m_eOperator );

    return strOp + strOperand;
}

std::string CStCodeGenerator::VisitCallExpr( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStCallExpr* p = dynamic_cast< CStCallExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strCallee = Visit( p->m_pCallee );
    std::string strArgs;

    bool bFirst = true;
    for( const auto& pArg : p->m_vecArgs )
    {
        if( !bFirst )
            strArgs += ", ";
        strArgs += Visit( pArg );
        bFirst = false;
    }

    return strCallee + "( " + strArgs + " )";
}

std::string CStCodeGenerator::VisitArrayAccess( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStArrayAccessExpr* p = dynamic_cast< CStArrayAccessExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strArray = Visit( p->m_pArray );
    std::string strIndex;

    bool bFirst = true;
    for( const auto& pIdx : p->m_vecIndices )
    {
        if( !bFirst )
            strIndex += "][";
        strIndex += Visit( pIdx );
        bFirst = false;
    }

    return strArray + "[ " + strIndex + " ]";
}

std::string CStCodeGenerator::VisitMemberAccess( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStMemberAccessExpr* p = dynamic_cast< CStMemberAccessExpr* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strObj = Visit( p->m_pObject );

    return strObj + "." + MakeValidId( p->m_strMember );
}

// ========================================================================
// Type Visitors
// ========================================================================

std::string CStCodeGenerator::VisitType( ObjPtr pNode )
{
    return Visit( pNode );
}

std::string CStCodeGenerator::VisitBasicType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "int";

    CStBasicTypeNode* p = dynamic_cast< CStBasicTypeNode* >( &*pNode );
    if( p == nullptr )
        return "int";

    return BasicTypeToString( p->m_eBasicType );
}

std::string CStCodeGenerator::VisitArrayType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStArrayTypeNode* p = dynamic_cast< CStArrayTypeNode* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strElemType = Visit( p->m_pElementType );
    std::string strDims;

    for( const auto& dim : p->m_vecDims )
    {
        strDims += "[" + std::to_string( dim.m_iEnd - dim.m_iStart + 1 ) + "]";
    }

    return strElemType + strDims;
}

std::string CStCodeGenerator::VisitStructType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStStructTypeNode* p = dynamic_cast< CStStructTypeNode* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = p->m_strTypeName;
    if( strName.empty() )
        strName = "/* anonymous struct */";

    AppendRawLine( "struct " + MakeValidId( strName ) + " {" );
    m_oCtx.PushIndent();

    for( const auto& member : p->m_vecMembers )
    {
        std::string strType = Visit( member.m_pType );
        AppendLine( strType + " " + MakeValidId( member.m_strName ) + ";" );
    }

    m_oCtx.PopIndent();
    AppendRawLine( "};" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitEnumType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStEnumTypeNode* p = dynamic_cast< CStEnumTypeNode* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName = p->m_strTypeName;
    if( strName.empty() )
        strName = "/* anonymous enum */";

    AppendRawLine( "enum class " + MakeValidId( strName ) + " {" );
    m_oCtx.PushIndent();

    bool bFirst = true;
    for( const auto& val : p->m_vecValues )
    {
        if( !bFirst )
            AppendRaw( ", " );
        CStEnumValueNode* pVal = val;
        AppendRaw( MakeValidId( pVal->m_strName ) );
        bFirst = false;
    }

    AppendRawLine( "" );
    m_oCtx.PopIndent();
    AppendRawLine( "};" );

    return m_strOutput;
}

std::string CStCodeGenerator::VisitPointerType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStPointerTypeNode* p = dynamic_cast< CStPointerTypeNode* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strTarget = Visit( p->m_pTargetType );
    return strTarget + "*";
}

std::string CStCodeGenerator::VisitReferenceType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStReferenceTypeNode* p = dynamic_cast< CStReferenceTypeNode* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strTarget = Visit( p->m_pTargetType );
    return strTarget + "&";
}

std::string CStCodeGenerator::VisitDerivedType( ObjPtr pNode )
{
    if( pNode.IsEmpty() )
        return "";

    CStDerivedTypeNode* p = dynamic_cast< CStDerivedTypeNode* >( &*pNode );
    if( p == nullptr )
        return "";

    std::string strName;
    for( const auto& part : p->m_vecQualifiedName )
    {
        if( !strName.empty() )
            strName += "::";
        strName += part;
    }

    return strName;
}

// ========================================================================
// Helper Methods
// ========================================================================

std::string CStCodeGenerator::BasicTypeToString( CStBasicTypeNode::enumBasicType eType )
{
    switch( eType )
    {
        case CStBasicTypeNode::btInt:      return "int";
        case CStBasicTypeNode::btDInt:     return "int32_t";
        case CStBasicTypeNode::btSInt:     return "int8_t";
        case CStBasicTypeNode::btUInt:     return "uint32_t";
        case CStBasicTypeNode::btUDInt:    return "uint32_t";
        case CStBasicTypeNode::btUSInt:    return "uint8_t";
        case CStBasicTypeNode::btByte:     return "uint8_t";
        case CStBasicTypeNode::btWord:     return "uint16_t";
        case CStBasicTypeNode::btDWord:    return "uint32_t";
        case CStBasicTypeNode::btLWord:    return "uint64_t";
        case CStBasicTypeNode::btLInt:     return "int64_t";
        case CStBasicTypeNode::btULInt:    return "uint64_t";
        case CStBasicTypeNode::btReal:     return "float";
        case CStBasicTypeNode::btLReal:    return "double";
        case CStBasicTypeNode::btBool:     return "bool";
        case CStBasicTypeNode::btString:   return "std::string";
        case CStBasicTypeNode::btWString:  return "std::wstring";
        case CStBasicTypeNode::btTime:     return "int64_t";  // Time in ms
        case CStBasicTypeNode::btLTime:    return "int64_t";  // LTime in ns
        default: return "int";
    }
}

std::string CStCodeGenerator::BinaryOpToString( CStBinaryExpr::enumBinaryOp eOp )
{
    switch( eOp )
    {
        case CStBinaryExpr::boAdd:         return "+";
        case CStBinaryExpr::boSub:         return "-";
        case CStBinaryExpr::boMul:         return "*";
        case CStBinaryExpr::boDiv:         return "/";
        case CStBinaryExpr::boMod:         return "%";
        case CStBinaryExpr::boAnd:         return "&&";
        case CStBinaryExpr::boOr:          return "||";
        case CStBinaryExpr::boXor:         return "^";
        case CStBinaryExpr::boEqual:       return "==";
        case CStBinaryExpr::boNotEqual:    return "!=";
        case CStBinaryExpr::boLessThan:    return "<";
        case CStBinaryExpr::boLessEqual:   return "<=";
        case CStBinaryExpr::boGreaterThan: return ">";
        case CStBinaryExpr::boGreaterEqual: return ">=";
        case CStBinaryExpr::boPower:       return "**";  // TODO: pow()
        default: return "/* unknown op */";
    }
}

std::string CStCodeGenerator::UnaryOpToString( CStUnaryExpr::enumUnaryOp eOp )
{
    switch( eOp )
    {
        case CStUnaryExpr::uoNot:  return "!";
        case CStUnaryExpr::uoNeg:  return "-";
        default: return "/* unknown op */";
    }
}

std::string CStCodeGenerator::MakeValidId( const std::string& strName )
{
    if( strName.empty() )
        return "_";

    std::string strResult;
    for( char c : strName )
    {
        if( isalnum( c ) || c == '_' )
            strResult += c;
        else
            strResult += '_';
    }

    // C++ keywords
    static const std::set< std::string > keywords = {
        "class", "struct", "enum", "union", "if", "else", "for", "while",
        "do", "switch", "case", "default", "break", "continue", "return",
        "void", "int", "float", "double", "char", "bool", "true", "false",
        "namespace", "using", "public", "private", "protected", "virtual",
        "override", "final", "const", "static", "new", "delete", "this"
    };

    if( keywords.count( strResult ) )
        strResult += "_";

    return strResult;
}

std::string CStCodeGenerator::GenerateArrayInit(
    const std::vector< ObjPtr >& vecValues )
{
    std::string strResult = "{ ";
    bool bFirst = true;
    for( const auto& pVal : vecValues )
    {
        if( !bFirst )
            strResult += ", ";
        strResult += Visit( pVal );
        bFirst = false;
    }
    strResult += " }";
    return strResult;
}

std::string CStCodeGenerator::GenerateStructInit(
    const std::vector< std::pair< std::string, ObjPtr > >& vecInits )
{
    std::string strResult = "{ ";
    bool bFirst = true;
    for( const auto& pair : vecInits )
    {
        if( !bFirst )
            strResult += ", ";
        strResult += ".";
        strResult += MakeValidId( pair.first );
        strResult += " = ";
        strResult += Visit( pair.second );
        bFirst = false;
    }
    strResult += " }";
    return strResult;
}

} // namespace rpcf
