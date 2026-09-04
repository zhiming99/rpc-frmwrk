/*
 * =====================================================================================
 *
 *       Filename:  stclsids.h
 *
 *    Description:  declarations of classids for ST parser classes 
 *
 *        Version:  1.0
 *        Created:  05/15/2026 11:11:38 AM
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
#include "objfctry.h"
namespace rpcf
{ 

typedef enum 
{
    DECL_CLSID( CLValueVariableInstPath ) = clsid( ClassFactoryStart ) + 100,
    DECL_CLSID( CLValueVariableDataMember ),
    DECL_CLSID( CLValueVariableDefPtr ),
    DECL_CLSID( CLValueVariableArrayAccess ),

    // AST Expression Nodes
    DECL_CLSID( CStLiteralExpr ),
    DECL_CLSID( CStIdentifierExpr ),
    DECL_CLSID( CStBinaryExpr ),
    DECL_CLSID( CStUnaryExpr ),
    DECL_CLSID( CStCallExpr ),
    DECL_CLSID( CStArgListNode ),
    DECL_CLSID( CStArrayAccessExpr ),
    DECL_CLSID( CStMemberAccessExpr ),
    DECL_CLSID( CStDereferenceExpr ),
    DECL_CLSID( CStPointerMemberExpr ),
    DECL_CLSID( CStLValueNode ),
    DECL_CLSID( CStLValueExtNode ),
    DECL_CLSID( CStInstancePathNode ),
    DECL_CLSID( CStFullExpressionNode ),
    DECL_CLSID( CStSubrangeNode ),
    DECL_CLSID( CStSubrangeListNode ),
    DECL_CLSID( CStStmtListNode ),
    DECL_CLSID( CStIfBranchListNode ),

    // AST Type Nodes
    DECL_CLSID( CStBasicTypeNode ),
    DECL_CLSID( CStArrayTypeNode ),
    DECL_CLSID( CStStructTypeNode ),
    DECL_CLSID( CStEnumTypeNode ),
    DECL_CLSID( CStEnumValueNode ),
    DECL_CLSID( CStEnumValueListNode ),
    DECL_CLSID( CStDataTypeSpecNode ),
    DECL_CLSID( CStTypeSpecNode ),
    DECL_CLSID( CStTypeDefinitionBlockNode ),
    DECL_CLSID( CStPointerTypeNode ),
    DECL_CLSID( CStReferenceTypeNode ),
    DECL_CLSID( CStDerivedTypeNode ),

    // AST Variable Declaration
    DECL_CLSID( CStVarDeclNode ),

    // AST Statement Nodes
    DECL_CLSID( CStAssignStmt ),
    DECL_CLSID( CStCallStmt ),
    DECL_CLSID( CStIfStmt ),
    DECL_CLSID( CStForStmt ),
    DECL_CLSID( CStWhileStmt ),
    DECL_CLSID( CStRepeatStmt ),
    DECL_CLSID( CStCaseStmt ),
    DECL_CLSID( CStPragmaStmt ),

    // AST POU Declaration Nodes
    DECL_CLSID( CStProgramDecl ),
    DECL_CLSID( CStFunctionBlockDecl ),
    DECL_CLSID( CStFunctionDecl ),
    DECL_CLSID( CStMethodDecl ),

    // AST Other Declaration Nodes
    DECL_CLSID( CStNamespaceDecl ),
    DECL_CLSID( CStInterfaceDecl ),
    DECL_CLSID( CStTypeDecl ),
    DECL_CLSID( CStVarConfigDecl ),
    DECL_CLSID( CStUsingDirective ),
    DECL_CLSID( CStInitialValueNode ),
    DECL_CLSID( CStArrayInitNode ),
    DECL_CLSID( CStStructInitNode ),
    DECL_CLSID( CStIdentifierListNode ),

    // AST Root Node
    DECL_CLSID( CStRootNode ),

}EnumSTParserClsid;

}
