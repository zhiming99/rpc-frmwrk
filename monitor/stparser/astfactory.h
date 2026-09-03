/*
 * =====================================================================================
 *
 *       Filename:  astfactory.h
 *
 *    Description:  Factory class for creating AST nodes
 *
 *        Version:  1.0
 *        Created:  09/01/2026
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

#include "astnodes.h"
#include "stclsids.h"
#include "nonterm.h"
#include "parsrctx.h"

namespace rpcf
{

/**
 * @brief Factory class for creating AST nodes
 * Uses rpc-frmwrk's NewObj pattern for object creation
 */
class CStAstFactory
{
public:
    CStAstFactory( CSTParserContext* pCtx )
        : m_pCtx( pCtx )
    {}

    // Root node
    ObjPtr CreateRootNode( const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStRootNode ) );
        CStRootNode* pRoot = pNode;
        if( pRoot != nullptr )
            pRoot->SetLocation( oLoc );
        return pNode;
    }

    // Expression nodes
    ObjPtr CreateLiteralExpr( CStLiteralExpr::enumLiteralType eType,
        const Variant& oValue, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStLiteralExpr ) );
        CStLiteralExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_eLiteralType = eType;
            p->m_oLiteralValue = oValue;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateIdentifierExpr( const std::string& strName, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStIdentifierExpr ) );
        CStIdentifierExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateBinaryExpr( CStBinaryExpr::enumBinaryOp eOp,
        ObjPtr pLeft, ObjPtr pRight, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStBinaryExpr ) );
        CStBinaryExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_eOperator = eOp;
            p->m_pLeft = pLeft;
            p->m_pRight = pRight;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateUnaryExpr( CStUnaryExpr::enumUnaryOp eOp,
        ObjPtr pOperand, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStUnaryExpr ) );
        CStUnaryExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_eOperator = eOp;
            p->m_pOperand = pOperand;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateCallExpr( ObjPtr pCallee,
        const std::vector< ObjPtr >& vecArgs, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStCallExpr ) );
        CStCallExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_pCallee = pCallee;
            p->m_vecArgs = vecArgs;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateArrayAccessExpr( ObjPtr pArray,
        const std::vector< ObjPtr >& vecIndices, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStArrayAccessExpr ) );
        CStArrayAccessExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_pArray = pArray;
            p->m_vecIndices = vecIndices;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateMemberAccessExpr( CStMemberAccessExpr::enumAccessType eType,
        ObjPtr pObject, const std::string& strMember, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStMemberAccessExpr ) );
        CStMemberAccessExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_eAccessType = eType;
            p->m_pObject = pObject;
            p->m_strMember = strMember;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateDereferenceExpr( ObjPtr pPointer, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStDereferenceExpr ) );
        CStDereferenceExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_pPointer = pPointer;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreatePointerMemberExpr( ObjPtr pPointer,
        const std::string& strMember, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStPointerMemberExpr ) );
        CStPointerMemberExpr* p = pNode;
        if( p != nullptr )
        {
            p->m_pPointer = pPointer;
            p->m_strMember = strMember;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    // Type nodes
    ObjPtr CreateBasicTypeNode( CStBasicTypeNode::enumBasicType eType,
        gint32 iStringLength, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStBasicTypeNode ) );
        CStBasicTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_eBasicType = eType;
            p->m_iStringLength = iStringLength;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateArrayTypeNode( ObjPtr pElementType,
        const std::vector< CStArrayTypeNode::CArrayDim >& vecDims,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStArrayTypeNode ) );
        CStArrayTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_pElementType = pElementType;
            p->m_vecDims = vecDims;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateStructTypeNode( const std::string& strName,
        const std::vector< CStStructTypeNode::CStructMember >& vecMembers,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStStructTypeNode ) );
        CStStructTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_vecMembers = vecMembers;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateEnumTypeNode( const std::string& strName,
        const std::vector< ObjPtr >& vecValues,
        ObjPtr pBaseType, const std::string& strDefaultInit,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStEnumTypeNode ) );
        CStEnumTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_vecValues = vecValues;
            p->m_pBaseType = pBaseType;
            p->m_strDefaultInit = strDefaultInit;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreatePointerTypeNode( ObjPtr pTargetType, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStPointerTypeNode ) );
        CStPointerTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_pTargetType = pTargetType;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateReferenceTypeNode( ObjPtr pTargetType, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStReferenceTypeNode ) );
        CStReferenceTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_pTargetType = pTargetType;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateDerivedTypeNode( const std::vector< std::string >& vecQualifiedName,
        bool bGlobalNamespace, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStDerivedTypeNode ) );
        CStDerivedTypeNode* p = pNode;
        if( p != nullptr )
        {
            p->m_vecQualifiedName = vecQualifiedName;
            p->m_bGlobalNamespace = bGlobalNamespace;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    // Variable declaration
    ObjPtr CreateVarDeclNode( const std::vector< std::string >& vecNames,
        ObjPtr pType, CStVarDeclNode::enumVarCategory eCategory,
        CStVarDeclNode::enumVarQualifier eQualifier,
        ObjPtr pInitialValue, const std::string& strDirectAddress,
        bool bAtDirectAddress, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStVarDeclNode ) );
        CStVarDeclNode* p = pNode;
        if( p != nullptr )
        {
            p->m_vecNames = vecNames;
            if( !vecNames.empty() )
                p->m_strName = vecNames[ 0 ];
            p->m_pType = pType;
            p->m_eCategory = eCategory;
            p->m_eQualifier = eQualifier;
            p->m_pInitialValue = pInitialValue;
            p->m_strDirectAddress = strDirectAddress;
            p->m_bAtDirectAddress = bAtDirectAddress;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    // Statement nodes
    ObjPtr CreateAssignStmt( ObjPtr pLValue, ObjPtr pRValue, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStAssignStmt ) );
        CStAssignStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_pLValue = pLValue;
            p->m_pRValue = pRValue;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateCallStmt( ObjPtr pCallExpr, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStCallStmt ) );
        CStCallStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_pCallExpr = pCallExpr;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateIfStmt( ObjPtr pCondition,
        const std::vector< ObjPtr >& vecThen,
        const std::vector< CStIfStmt::CIfBranch >& vecElseIf,
        const std::vector< ObjPtr >& vecElse,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStIfStmt ) );
        CStIfStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_pCondition = pCondition;
            p->m_vecThenStatements = vecThen;
            p->m_vecElseIfBranches = vecElseIf;
            p->m_vecElseStatements = vecElse;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateForStmt( const std::string& strLoopVar,
        ObjPtr pStart, ObjPtr pEnd, ObjPtr pStep,
        const std::vector< ObjPtr >& vecBody,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStForStmt ) );
        CStForStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_strLoopVar = strLoopVar;
            p->m_pStartValue = pStart;
            p->m_pEndValue = pEnd;
            p->m_pStepValue = pStep;
            p->m_vecBody = vecBody;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateWhileStmt( ObjPtr pCondition,
        const std::vector< ObjPtr >& vecBody,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStWhileStmt ) );
        CStWhileStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_pCondition = pCondition;
            p->m_vecBody = vecBody;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateRepeatStmt( const std::vector< ObjPtr >& vecBody,
        ObjPtr pCondition, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStRepeatStmt ) );
        CStRepeatStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_vecBody = vecBody;
            p->m_pCondition = pCondition;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateCaseStmt( ObjPtr pExpression,
        const std::vector< CStCaseStmt::CCaseBranch >& vecBranches,
        const std::vector< ObjPtr >& vecElse,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStCaseStmt ) );
        CStCaseStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_pExpression = pExpression;
            p->m_vecBranches = vecBranches;
            p->m_vecElseStatements = vecElse;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreatePragmaStmt( CStPragmaStmt::enumPragmaType eType,
        const std::string& strName, const std::string& strValue,
        ObjPtr pCondition, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStPragmaStmt ) );
        CStPragmaStmt* p = pNode;
        if( p != nullptr )
        {
            p->m_eType = eType;
            p->m_strName = strName;
            p->m_strValue = strValue;
            p->m_pCondition = pCondition;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    // POU declaration nodes
    ObjPtr CreateProgramDecl( const std::string& strName,
        const std::vector< ObjPtr >& vecInput,
        const std::vector< ObjPtr >& vecOutput,
        const std::vector< ObjPtr >& vecInOut,
        const std::vector< ObjPtr >& vecLocal,
        const std::vector< ObjPtr >& vecTemp,
        const std::vector< ObjPtr >& vecStmts,
        const std::vector< std::string >& vecUsing,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStProgramDecl ) );
        CStProgramDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_vecInputVars = vecInput;
            p->m_vecOutputVars = vecOutput;
            p->m_vecInOutVars = vecInOut;
            p->m_vecLocalVars = vecLocal;
            p->m_vecTempVars = vecTemp;
            p->m_vecStatements = vecStmts;
            p->m_vecUsingNamespaces = vecUsing;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateFunctionBlockDecl( const std::string& strName,
        CStFunctionBlockDecl::enumFbModifier eModifier,
        const std::string& strExtends,
        const std::vector< std::string >& vecImplements,
        const std::vector< ObjPtr >& vecInput,
        const std::vector< ObjPtr >& vecOutput,
        const std::vector< ObjPtr >& vecInOut,
        const std::vector< ObjPtr >& vecLocal,
        const std::vector< ObjPtr >& vecTemp,
        const std::vector< ObjPtr >& vecMethods,
        const std::vector< ObjPtr >& vecStatements,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStFunctionBlockDecl ) );
        CStFunctionBlockDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_eModifier = eModifier;
            p->m_strExtends = strExtends;
            p->m_vecImplements = vecImplements;
            p->m_vecInputVars = vecInput;
            p->m_vecOutputVars = vecOutput;
            p->m_vecInOutVars = vecInOut;
            p->m_vecLocalVars = vecLocal;
            p->m_vecTempVars = vecTemp;
            p->m_vecMethods = vecMethods;
            p->m_vecStatements = vecStatements;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateFunctionDecl( const std::string& strName,
        ObjPtr pReturnType,
        const std::vector< ObjPtr >& vecInput,
        const std::vector< ObjPtr >& vecOutput,
        const std::vector< ObjPtr >& vecInOut,
        const std::vector< ObjPtr >& vecLocal,
        const std::vector< ObjPtr >& vecTemp,
        const std::vector< ObjPtr >& vecStmts,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStFunctionDecl ) );
        CStFunctionDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_pReturnType = pReturnType;
            p->m_vecInputVars = vecInput;
            p->m_vecOutputVars = vecOutput;
            p->m_vecInOutVars = vecInOut;
            p->m_vecLocalVars = vecLocal;
            p->m_vecTempVars = vecTemp;
            p->m_vecStatements = vecStmts;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateMethodDecl( const std::string& strName,
        ObjPtr pReturnType,
        const std::vector< ObjPtr >& vecVars,
        const std::vector< ObjPtr >& vecStmts,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStMethodDecl ) );
        CStMethodDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_pReturnType = pReturnType;
            p->m_vecVariables = vecVars;
            p->m_vecStatements = vecStmts;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    // Other declaration nodes
    ObjPtr CreateNamespaceDecl( const std::string& strName,
        const std::vector< ObjPtr >& vecDecls,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStNamespaceDecl ) );
        CStNamespaceDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_vecDeclarations = vecDecls;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    /**
     * @brief Create a virtual global scope namespace node
     * 
     * This creates a CStNamespaceDecl with empty name that represents
     * the global scope boundary. It contains all top-level declarations
     * from the root node and is used as the starting point when
     * resolving absolute paths like '::abc'.
     * 
     * @param vecDecls The top-level declarations to include in global scope
     * @param oLoc Location info (typically empty for virtual nodes)
     * @return ObjPtr pointing to CStNamespaceDecl with empty name
     */
    ObjPtr CreateGlobalScope( const std::vector< ObjPtr >& vecDecls,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStNamespaceDecl ) );
        CStNamespaceDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName.clear();  // Empty name = global scope
            p->m_vecDeclarations = vecDecls;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateInterfaceDecl( const std::string& strName,
        const std::vector< ObjPtr >& vecMethods,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStInterfaceDecl ) );
        CStInterfaceDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_vecMethodDecls = vecMethods;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateTypeDecl( const std::string& strName,
        ObjPtr pTypeDef,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStTypeDecl ) );
        CStTypeDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strName = strName;
            p->m_pTypeDefinition = pTypeDef;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateVarConfigDecl( const std::string& strPath,
        ObjPtr pType, const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStVarConfigDecl ) );
        CStVarConfigDecl* p = pNode;
        if( p != nullptr )
        {
            p->m_strInstancePath = strPath;
            p->m_pType = pType;
            p->SetLocation( oLoc );
        }
        return pNode;
    }

    ObjPtr CreateUsingDirective( const std::vector< std::string >& vecNamespace,
        const YYLTYPE2& oLoc )
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStUsingDirective ) );
        CStUsingDirective* p = pNode;
        if( p != nullptr )
        {
            p->m_vecNamespace = vecNamespace;
            if( !vecNamespace.empty() )
                p->m_strNamespace = vecNamespace.back();
            p->SetLocation( oLoc );
        }
        return pNode;
    }

private:
    CSTParserContext* m_pCtx;
};

// Get factory from context
inline CStAstFactory* GetAstFactory( CSTParserContext* pCtx )
{
    static CStAstFactory oFactory( pCtx );
    return &oFactory;
}

} // namespace rpcf
