/*
 * =====================================================================================
 *
 *       Filename:  stgenbase.h
 *
 *    Description:  ST to C++ code generator base classes
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
#pragma once

#include "rpc.h"
#include "astnodes.h"

namespace rpcf
{

// ========================================================================
// Code Generation Context
// ========================================================================

/**
 * @brief Context for code generation
 *
 * Tracks the current scope during code generation:
 * - Current namespace
 * - Current class (for FB/Method)
 * - Indentation level
 * - Whether we're in a header or implementation
 */
class CStGenContext
{
public:
    std::string m_strNamespace;
    std::string m_strClassName;
    std::string m_strIndent;
    bool m_bIsHeader;           // true = .h, false = .cpp
    bool m_bInMemberContext;    // true = inside class/struct

    CStGenContext()
        : m_bIsHeader( true ), m_bInMemberContext( false )
    {}

    void PushIndent() { m_strIndent += "    "; }
    void PopIndent() 
    { 
        if( m_strIndent.size() >= 4 ) 
            m_strIndent.resize( m_strIndent.size() - 4 ); 
    }
};

// ========================================================================
// Main Code Generator
// ========================================================================

/**
 * @brief Main ST to C++ code generator
 *
 * Uses recursive visitor pattern with context stack.
 */
class CStCodeGenerator
{
protected:
    std::string m_strOutput;
    CStGenContext m_oCtx;

public:
    CStCodeGenerator()
    {}

    virtual ~CStCodeGenerator() {}

    // ========================================================================
    // Entry points
    // ========================================================================

    /**
     * @brief Generate C++ code from AST root
     */
    virtual std::string Generate( ObjPtr pRoot );

    /**
     * @brief Generate header file content
     */
    std::string GenerateHeader( ObjPtr pRoot );

    /**
     * @brief Generate implementation file content
     */
    std::string GenerateImpl( ObjPtr pRoot );

    // ========================================================================
    // Declaration Visitors
    // ========================================================================

    std::string VisitRoot( ObjPtr pNode );
    std::string VisitProgramDecl( ObjPtr pNode );
    std::string VisitFunctionBlockDecl( ObjPtr pNode );
    std::string VisitFunctionDecl( ObjPtr pNode );
    std::string VisitMethodDecl( ObjPtr pNode );
    std::string VisitTypeDecl( ObjPtr pNode );
    std::string VisitVarDecl( ObjPtr pNode );
    std::string VisitNamespace( ObjPtr pNode );
    std::string VisitUsingDirective( ObjPtr pNode );

    // ========================================================================
    // Statement Visitors
    // ========================================================================

    std::string VisitStatement( ObjPtr pNode );
    std::string VisitAssignStmt( ObjPtr pNode );
    std::string VisitIfStmt( ObjPtr pNode );
    std::string VisitForStmt( ObjPtr pNode );
    std::string VisitWhileStmt( ObjPtr pNode );
    std::string VisitRepeatStmt( ObjPtr pNode );
    std::string VisitCaseStmt( ObjPtr pNode );
    std::string VisitCallStmt( ObjPtr pNode );

    // ========================================================================
    // Expression Visitors
    // ========================================================================

    std::string VisitExpression( ObjPtr pNode );
    std::string VisitInitialValue( ObjPtr pNode );
    std::string VisitLiteral( ObjPtr pNode );
    std::string VisitIdentifier( ObjPtr pNode );
    std::string VisitBinaryExpr( ObjPtr pNode );
    std::string VisitUnaryExpr( ObjPtr pNode );
    std::string VisitCallExpr( ObjPtr pNode );
    std::string VisitArrayAccess( ObjPtr pNode );
    std::string VisitMemberAccess( ObjPtr pNode );

    // ========================================================================
    // Type Visitors
    // ========================================================================

    std::string VisitType( ObjPtr pNode );
    std::string VisitBasicType( ObjPtr pNode );
    std::string VisitArrayType( ObjPtr pNode );
    std::string VisitStructType( ObjPtr pNode );
    std::string VisitEnumType( ObjPtr pNode );
    std::string VisitPointerType( ObjPtr pNode );
    std::string VisitReferenceType( ObjPtr pNode );
    std::string VisitDerivedType( ObjPtr pNode );

    // ========================================================================
    // Helper methods
    // ========================================================================

    /**
     * @brief Get class ID from node
     */
    EnumClsid GetClsid( ObjPtr pNode )
    {
        if( pNode.IsEmpty() )
            return clsid( CObjBase );
        CObjBase* p = pNode;
        return p->GetClsid();
    }

    /**
     * @brief Convert basic type enum to C++ string
     */
    std::string BasicTypeToString( CStBasicTypeNode::enumBasicType eType );

    /**
     * @brief Convert binary operator to C++ string
     */
    std::string BinaryOpToString( CStBinaryExpr::enumBinaryOp eOp );

    /**
     * @brief Convert unary operator to C++ string
     */
    std::string UnaryOpToString( CStUnaryExpr::enumUnaryOp eOp );

    /**
     * @brief Make valid C++ identifier
     */
    std::string MakeValidId( const std::string& strName );

    /**
     * @brief Get the generated output string
     */
    const std::string& GetOutput() const
    { return m_strOutput; }

    /**
     * @brief Clear the output buffer
     */
    void Clear()
    { m_strOutput.clear(); m_oCtx.m_strIndent.clear(); }

    /**
     * @brief Generate array initializer from vector of expressions
     */
    std::string GenerateArrayInit( const std::vector< ObjPtr >& vecValues );

    /**
     * @brief Generate struct initializer from vector of (name, value) pairs
     */
    std::string GenerateStructInit(
        const std::vector< std::pair< std::string, ObjPtr > >& vecInits );

    /**
     * @brief Append to output with proper indentation
     */
    void Append( const std::string& str )
    { m_strOutput += m_oCtx.m_strIndent + str; }

    void AppendLine( const std::string& str )
    { m_strOutput += m_oCtx.m_strIndent + str + "\n"; }

    void AppendRaw( const std::string& str )
    { m_strOutput += str; }

    void AppendRawLine( const std::string& str )
    { m_strOutput += str + "\n"; }

    // ========================================================================
    // Dispatcher
    // ========================================================================

    /**
     * @brief Dispatch to appropriate visitor based on node type
     */
    std::string Visit( ObjPtr pNode )
    {
        if( pNode.IsEmpty() )
            return "";

        // Root
        if( dynamic_cast< CStProgramDecl* >( &*pNode ) )
            return VisitProgramDecl( pNode );
        if( dynamic_cast< CStFunctionBlockDecl* >( &*pNode ) )
            return VisitFunctionBlockDecl( pNode );
        if( dynamic_cast< CStFunctionDecl* >( &*pNode ) )
            return VisitFunctionDecl( pNode );
        if( dynamic_cast< CStMethodDecl* >( &*pNode ) )
            return VisitMethodDecl( pNode );
        if( dynamic_cast< CStTypeDecl* >( &*pNode ) )
            return VisitTypeDecl( pNode );
        if( dynamic_cast< CStVarDeclNode* >( &*pNode ) )
            return VisitVarDecl( pNode );
        if( dynamic_cast< CStNamespaceDecl* >( &*pNode ) )
            return VisitNamespace( pNode );
        if( dynamic_cast< CStUsingDirective* >( &*pNode ) )
            return VisitUsingDirective( pNode );

        // Statements
        if( dynamic_cast< CStAssignStmt* >( &*pNode ) )
            return VisitAssignStmt( pNode );
        if( dynamic_cast< CStIfStmt* >( &*pNode ) )
            return VisitIfStmt( pNode );
        if( dynamic_cast< CStForStmt* >( &*pNode ) )
            return VisitForStmt( pNode );
        if( dynamic_cast< CStWhileStmt* >( &*pNode ) )
            return VisitWhileStmt( pNode );
        if( dynamic_cast< CStRepeatStmt* >( &*pNode ) )
            return VisitRepeatStmt( pNode );
        if( dynamic_cast< CStCaseStmt* >( &*pNode ) )
            return VisitCaseStmt( pNode );
        if( dynamic_cast< CStCallStmt* >( &*pNode ) )
            return VisitCallStmt( pNode );

        // Expressions
        if( dynamic_cast< CStInitialValueNode* >( &*pNode ) )
            return VisitInitialValue( pNode );
        if( dynamic_cast< CStLiteralExpr* >( &*pNode ) )
            return VisitLiteral( pNode );
        if( dynamic_cast< CStIdentifierExpr* >( &*pNode ) )
            return VisitIdentifier( pNode );
        if( dynamic_cast< CStBinaryExpr* >( &*pNode ) )
            return VisitBinaryExpr( pNode );
        if( dynamic_cast< CStUnaryExpr* >( &*pNode ) )
            return VisitUnaryExpr( pNode );
        if( dynamic_cast< CStCallExpr* >( &*pNode ) )
            return VisitCallExpr( pNode );
        if( dynamic_cast< CStArrayAccessExpr* >( &*pNode ) )
            return VisitArrayAccess( pNode );
        if( dynamic_cast< CStMemberAccessExpr* >( &*pNode ) )
            return VisitMemberAccess( pNode );

        // Types
        if( dynamic_cast< CStBasicTypeNode* >( &*pNode ) )
            return VisitBasicType( pNode );
        if( dynamic_cast< CStArrayTypeNode* >( &*pNode ) )
            return VisitArrayType( pNode );
        if( dynamic_cast< CStStructTypeNode* >( &*pNode ) )
            return VisitStructType( pNode );
        if( dynamic_cast< CStEnumTypeNode* >( &*pNode ) )
            return VisitEnumType( pNode );
        if( dynamic_cast< CStPointerTypeNode* >( &*pNode ) )
            return VisitPointerType( pNode );
        if( dynamic_cast< CStReferenceTypeNode* >( &*pNode ) )
            return VisitReferenceType( pNode );
        if( dynamic_cast< CStDerivedTypeNode* >( &*pNode ) )
            return VisitDerivedType( pNode );

        // Unknown node type
        return "";
    }

    // ========================================================================
    // Factory function
    // ========================================================================

    /**
     * @brief Create a code generator and generate C++ from ST AST
     * @param pRoot Root AST node
     * @param bHeader If true, generate header file; otherwise implementation
     * @return Generated C++ code string
     */
    inline std::string GenerateStCpp(
        ObjPtr pRoot,
        bool bHeader = true )
    {
        CStCodeGenerator oGen;
        return bHeader ? oGen.GenerateHeader( pRoot )
                       : oGen.GenerateImpl( pRoot );
    }

    /**
     * @brief Generate complete .h file with header guard
     */
    inline std::string GenerateStHeaderFile(
        ObjPtr pRoot,
        const std::string& strGuard )
    {
        CStCodeGenerator oGen;
        std::string strOutput;

        // Header guard
        std::string strUpperGuard = strGuard;
        for( char& c : strUpperGuard )
            c = toupper( c );

        strOutput += "#ifndef " + strUpperGuard + "\n";
        strOutput += "#define " + strUpperGuard + "\n\n";

        // Generated content
        strOutput += oGen.GenerateHeader( pRoot );

        strOutput += "#endif // " + strUpperGuard + "\n";

        return strOutput;
    }
};

} // namespace rpcf
