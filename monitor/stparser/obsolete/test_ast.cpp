#include <iostream>
#include <fstream>
#include "rpc.h"
#include "stlexer.h"
#include "stclsids.h"
#include "astnodes.h"
#include "astfactory.h"
#include "astdebug.h"

using namespace rpcf;

// Global AST root
ObjPtr g_pAstRoot;

/**
 * @brief Helper: create an enum value node with the given name
 */
static ObjPtr MakeEnumValue( const std::string& strName, const YYLTYPE2& oLoc )
{
    ObjPtr pNode;
    pNode.NewObj( clsid( CStEnumValueNode ) );
    CStEnumValueNode* pVal = pNode;
    if( pVal != nullptr )
    {
        pVal->m_strName = strName;
        pVal->SetLocation( oLoc );
    }
    return pNode;
}

/**
 * @brief Test type definition block
 */
void TestTypeDefinitionBlock( CStAstFactory& factory, YYLTYPE2& oLoc )
{
    std::cout << "\n=== Testing Type Definition Block ===" << std::endl;

    // Create a basic type node
    ObjPtr pIntType = factory.CreateBasicTypeNode(
        CStBasicTypeNode::btInt, 0, oLoc );

    // Create a data type spec wrapper
    ObjPtr pDataTypeSpec = factory.CreateDataTypeSpecNode( pIntType, oLoc );

    // Create a type spec wrapper
    ObjPtr pTypeSpec = factory.CreateTypeSpecNode( pDataTypeSpec, oLoc );

    // Create a type declaration for MyInt : INT;
    ObjPtr pTypeDecl = factory.CreateTypeDecl( "MyInt", pTypeSpec, oLoc );

    // Create type definition block
    ObjPtr pTypeBlock = factory.CreateTypeDefinitionBlockNode( oLoc );
    CStTypeDefinitionBlockNode* pBlock = pTypeBlock;
    if( pBlock )
    {
        pBlock->m_vecTypeDecls.push_back( pTypeDecl );
    }

    DumpAstTree( pTypeBlock );
}

/**
 * @brief Test enum type
 */
void TestEnumType( CStAstFactory& factory, YYLTYPE2& oLoc )
{
    std::cout << "\n=== Testing Enum Type ===" << std::endl;

    // Create enum values
    std::vector< ObjPtr > vecValues;
    vecValues.push_back( MakeEnumValue( "Red", oLoc ) );
    vecValues.push_back( MakeEnumValue( "Green", oLoc ) );
    vecValues.push_back( MakeEnumValue( "Blue", oLoc ) );

    // Create enum type node
    ObjPtr pEnumType = factory.CreateEnumTypeNode(
        "Color", vecValues, nullptr, "", oLoc );

    DumpAstTree( pEnumType );
}

/**
 * @brief Main test function
 */
gint32 main( gint32 argc, char* argv[] )
{
    std::cout << "ST Parser AST Debug Utility Test" << std::endl;
    std::cout << "=================================" << std::endl;

    // Create a factory
    CSTParserContext oCtx;
    CStAstFactory factory( &oCtx );

    YYLTYPE2 oLoc;

    // Test 1: Simple program with expression
    std::cout << "\n=== Test 1: Simple Program ===" << std::endl;
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

    // Print the AST using the new utility
    DumpAstTree( pTestProg );

    // Test 2: Type definition block
    TestTypeDefinitionBlock( factory, oLoc );

    // Test 3: Enum type
    TestEnumType( factory, oLoc );

    // Test 4: String literal
    std::cout << "\n=== Test 4: String Literal ===" << std::endl;
    ObjPtr pStringLit = factory.CreateLiteralExpr(
        CStLiteralExpr::ltString, Variant( "Hello, ST!" ), oLoc );
    DumpAstTree( pStringLit );

    // Test 5: Array type
    std::cout << "\n=== Test 5: Array Type ===" << std::endl;
    ObjPtr pElemType = factory.CreateBasicTypeNode(
        CStBasicTypeNode::btInt, 0, oLoc );
    CStArrayTypeNode::CArrayDim oDim;
    oDim.m_iStart = 0;
    oDim.m_iEnd = 9;
    std::vector< CStArrayTypeNode::CArrayDim > vecDims;
    vecDims.push_back( oDim );
    ObjPtr pArrayType = factory.CreateArrayTypeNode(
        pElemType, vecDims, oLoc );
    DumpAstTree( pArrayType );

    // Test 6: Reference type
    std::cout << "\n=== Test 6: Reference Type ===" << std::endl;
    ObjPtr pRefType = factory.CreateReferenceTypeNode( pElemType, oLoc );
    DumpAstTree( pRefType );

    std::cout << "\nAll tests completed successfully!" << std::endl;

    return 0;
}