#include <iostream>
#include <fstream>
#include "rpc.h"
#include "stlexer.h"
#include "stclsids.h"
#include "astnodes.h"
#include "astfactory.h"

using namespace rpcf;

// Global AST root
ObjPtr g_pAstRoot;

/**
 * @brief Print AST tree recursively
 */
void PrintAstNode( CObjBase* pNode, gint32 iDepth = 0 )
{
    if( pNode == nullptr )
        return;

    // Print indentation
    for( gint32 i = 0; i < iDepth; i++ )
        std::cout << "  ";

    // Print node type
    std::cout << "[" << pNode->GetClsid() << "] ";

    // Print node-specific information based on class ID
    EnumClsid eClsid = pNode->GetClsid();
    
    if( eClsid == clsid( CStLiteralExpr ) )
    {
        CStLiteralExpr* p = dynamic_cast< CStLiteralExpr* >( pNode );
        std::cout << "Literal: type=" << p->m_eLiteralType;
    }
    else if( eClsid == clsid( CStIdentifierExpr ) )
    {
        CStIdentifierExpr* p = dynamic_cast< CStIdentifierExpr* >( pNode );
        std::cout << "Identifier: " << p->m_strName;
    }
    else if( eClsid == clsid( CStBinaryExpr ) )
    {
        CStBinaryExpr* p = dynamic_cast< CStBinaryExpr* >( pNode );
        std::cout << "Binary: op=" << p->m_eOperator;
    }
    else if( eClsid == clsid( CStAssignStmt ) )
    {
        std::cout << "Assignment";
    }
    else if( eClsid == clsid( CStProgramDecl ) )
    {
        CStProgramDecl* p = dynamic_cast< CStProgramDecl* >( pNode );
        std::cout << "Program: " << p->m_strName;
    }
    else if( eClsid == clsid( CStRootNode ) )
    {
        std::cout << "Root";
    }

    std::cout << std::endl;
}

/**
 * @brief Main test function
 */
gint32 main( gint32 argc, char* argv[] )
{
    std::cout << "ST Parser AST Building Test" << std::endl;
    std::cout << "================================" << std::endl;

    // Create a factory
    CSTParserContext oCtx;
    CStAstFactory factory( &oCtx );

    YYLTYPE2 oLoc;

    // Create a simple program for testing
    // Program: TestProgram with statement x := 10 + 5;
    std::vector< ObjPtr > vecInput, vecOutput, vecInOut, vecLocal, vecTemp;
    std::vector< ObjPtr > vecStmts;

    // Create literal 10
    ObjPtr pLit10 = factory.CreateLiteralExpr(
        CStLiteralExpr::ltNumber, Variant( 10 ), oLoc );

    // Create literal 5
    ObjPtr pLit5 = factory.CreateLiteralExpr(
        CStLiteralExpr::ltNumber, Variant( 5 ), oLoc );

    // Create binary expression: 10 + 5
    ObjPtr pAdd = factory.CreateBinaryExpr(
        CStBinaryExpr::boAdd, pLit10, pLit5, oLoc );

    // Create identifier "x"
    ObjPtr pX = factory.CreateIdentifierExpr( "x", oLoc );

    // Create assignment statement: x := 10 + 5;
    ObjPtr pAssign = factory.CreateAssignStmt( pX, pAdd, oLoc );
    vecStmts.push_back( pAssign );

    // Create program
    ObjPtr pTestProg = factory.CreateProgramDecl(
        "TestProgram", vecInput, vecOutput, vecInOut,
        vecLocal, vecTemp, vecStmts, {}, oLoc );

    // Print the AST
    std::cout << "\nGenerated AST for Program TestProgram:" << std::endl;
    PrintAstNode( pTestProg );

    std::cout << "\nTest completed successfully!" << std::endl;

    return 0;
}
