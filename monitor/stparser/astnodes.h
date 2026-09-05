/*
 * =====================================================================================
 *
 *       Filename:  astnodes.h
 *
 *    Description:  Complete AST node definitions for Structured Text Language
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

#include <vector>
#include <string>
#include <memory>
#include "rpc.h"
#include "stast.h"

namespace rpcf
{

// Forward declarations
class CStDeclNode;
class CStStmtNode;
struct CStExprNode;
class CStTypeNode;
class CStVarDeclNode;

// ============================================================================
// Expression Nodes
// ============================================================================

/**
 * @brief Base class for all expression nodes
 */
struct CStExprNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    CStExprNode() : super()
    {
        m_iToken = YYUNDEF;
    }

    virtual std::string GetNodeInfo() const override;
    virtual std::string GetSignature() const override;
};

/**
 * @brief Literal expression (numbers, strings, booleans, etc.)
 */
struct CStLiteralExpr : public CStExprNode
{
    typedef CStExprNode super;

    enum enumLiteralType {
        ltNumber,
        ltString,
        ltWString,
        ltBool,
        ltTime,
        ltLTime,
        ltDate,
        ltDateTime,
        ltTimeOfDay
    };

    enumLiteralType m_eLiteralType;
    Variant m_oLiteralValue;

    CStLiteralExpr() : super(),
        m_eLiteralType(ltNumber)
    { SetClassId( clsid( CStLiteralExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Identifier expression (variable or function name)
 */
struct CStIdentifierExpr : public CStExprNode
{
    typedef CStExprNode super;

    std::string m_strName;

    CStIdentifierExpr() : super()
    { SetClassId( clsid( CStIdentifierExpr ) ); }

    virtual std::string GetNodeInfo() const override
    { return m_strName; }

    virtual std::string GetSignature() const override
    { return m_strName; }
};

/**
 * @brief Direct address wrapper node, e.g. %IW0.1, %Q*, @IBx.I0
 *
 * Direct addresses are translated quite differently from normal
 * identifiers, so they get a dedicated node instead of reusing
 * CStIdentifierExpr. The node carries both views of the address:
 *  - m_strAddress: the address source text
 *  - m_eAddrType:  the address kind (rpcf, peripheral, or
 *                  peripheral address with a runtime offset)
 *  - m_pParsed:    the parsed address object from the lexer
 *                  (IntVecPtr for peripheral addresses, StrVecPtr
 *                  for rpcf addresses)
 *  - m_pIndex:     the index expression of the indexed form
 *                  '%IW[expr]', empty if not indexed
 */
struct CStDirectAddressNode : public CStExprNode
{
    typedef CStExprNode super;

    enum enumAddrType
    {
        datRpcf,             // '@' rpcf address
        datPeripheral,       // '%' peripheral address
        datPeripheralOffset  // '%' address with a runtime offset, '%IW[expr]'
    };

    std::string m_strAddress;
    enumAddrType m_eAddrType;
    ObjPtr m_pParsed;
    ObjPtr m_pIndex;

    CStDirectAddressNode() : super(),
        m_eAddrType( datRpcf )
    { SetClassId( clsid( CStDirectAddressNode ) ); }

    virtual std::string GetNodeInfo() const override;
    virtual std::string GetSignature() const override;
};

/**
 * @brief Binary expression (arithmetic, logical, comparison)
 */
struct CStBinaryExpr : public CStExprNode
{
    typedef CStExprNode super;

    enum enumBinaryOp {
        boAdd, boSub, boMul, boDiv, boMod,
        boAnd, boOr, boXor, boEqual, boNotEqual,
        boLessThan, boLessEqual, boGreaterThan, boGreaterEqual, boPower
    };

    enumBinaryOp m_eOperator;
    ObjPtr m_pLeft;
    ObjPtr m_pRight;

    CStBinaryExpr() : super(),
        m_eOperator(boAdd)
    { SetClassId( clsid( CStBinaryExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Unary expression (NOT, -)
 */
struct CStUnaryExpr : public CStExprNode
{
    typedef CStExprNode super;

    enum enumUnaryOp {
        uoNot,
        uoNeg
    };

    enumUnaryOp m_eOperator;
    ObjPtr m_pOperand;

    CStUnaryExpr() : super(),
        m_eOperator(uoNot)
    { SetClassId( clsid( CStUnaryExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Function call expression
 */
struct CStCallExpr : public CStExprNode
{
    typedef CStExprNode super;

    struct CNamedArg
    {
        std::string m_strName;   // formal parameter name
        ObjPtr m_pValue;         // argument expression
        bool m_bOutput;          // true for formal output params (Q => lvalue)

        CNamedArg() : m_bOutput( false ){}
    };

    ObjPtr m_pCallee;
    std::vector< ObjPtr > m_vecArgs;         // positional arguments
    std::vector< CNamedArg > m_vecNamedArgs; // named arguments

    CStCallExpr() : super()
    { SetClassId( clsid( CStCallExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Argument list node (parser accumulator)
 *
 * Collects positional and named arguments during parsing. Its contents
 * are flattened into CStCallExpr when the enclosing call is reduced,
 * so this node is transient and does not appear in the final AST.
 */
struct CStArgListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecArgs;                  // positional
    std::vector< CStCallExpr::CNamedArg > m_vecNamed; // named

    CStArgListNode() : super()
    { SetClassId( clsid( CStArgListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Array access expression
 */
struct CStArrayAccessExpr : public CStExprNode
{
    typedef CStExprNode super;

    ObjPtr m_pArray;
    ObjPtr m_pIndex;
    std::vector< ObjPtr > m_vecIndices;

    CStArrayAccessExpr() : super()
    { SetClassId( clsid( CStArrayAccessExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Member access expression (dot operator)
 */
struct CStMemberAccessExpr : public CStExprNode
{
    typedef CStExprNode super;

    enum enumAccessType {
        atDot,
        atArrow
    };

    enumAccessType m_eAccessType;
    ObjPtr m_pObject;
    std::string m_strMember;

    CStMemberAccessExpr() : super(),
        m_eAccessType(atDot)
    { SetClassId( clsid( CStMemberAccessExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Dereference expression (*ptr)
 */
struct CStDereferenceExpr : public CStExprNode
{
    typedef CStExprNode super;

    ObjPtr m_pPointer;

    CStDereferenceExpr() : super()
    { SetClassId( clsid( CStDereferenceExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Pointer member access (->)
 */
struct CStPointerMemberExpr : public CStExprNode
{
    typedef CStExprNode super;

    ObjPtr m_pPointer;
    std::string m_strMember;

    CStPointerMemberExpr() : super()
    { SetClassId( clsid( CStPointerMemberExpr ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief L-Value wrapper node
 *
 * Wraps expressions that are valid l-values (writable memory locations).
 * This provides a clear boundary for semantic analysis to distinguish
 * l-values from r-values.
 */
struct CStLValueNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    ObjPtr m_pExpression;  // The actual l-value expression (identifier, member access, etc.)

    CStLValueNode() : super()
    { SetClassId( clsid( CStLValueNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Extended L-Value wrapper node
 *
 * Wraps extended l-values that include direct addresses and bit access.
 * This provides a clear boundary for semantic analysis to distinguish
 * extended l-values from standard l-values.
 */
struct CStLValueExtNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    ObjPtr m_pExpression;  // The actual extended l-value expression

    CStLValueExtNode() : super()
    { SetClassId( clsid( CStLValueExtNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Instance path wrapper node
 *
 * Wraps a dotted instance path (e.g. MainProg.Motor1.SensorIn) and carries
 * both views of it:
 *  - m_pExpression: the expression form (CStIdentifierExpr, or a
 *    CStMemberAccessExpr chain) for use in value/l-value contexts;
 *  - m_vecNameComponents: the path components for name-extraction contexts
 *    (derived types, USING namespaces, FOR loop variables).
 */
struct CStInstancePathNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< std::string > m_vecNameComponents;
    ObjPtr m_pExpression;

    CStInstancePathNode() : super()
    { SetClassId( clsid( CStInstancePathNode ) ); }

    /**
     * @brief Join the path components with '.' (e.g. "MainProg.Motor1")
     */
    std::string GetDottedName() const
    {
        std::string strName;
        for( size_t i = 0; i < m_vecNameComponents.size(); i++ )
        {
            if( i > 0 )
                strName += ".";
            strName += m_vecNameComponents[ i ];
        }
        return strName;
    }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Boundary wrapper for a full expression
 *
 * Marks the root of an expression used in statement position (conditions,
 * loop bounds, case selectors, initializers, call arguments). Like the
 * l-value boundary nodes, it gives the semantic checker a uniform handle
 * on 'a complete expression' and a place to hang result-type info.
 * Expression-internal consumers (parenthesized factors, array indices,
 * subrange bounds) unwrap it via UnwrapFullExpression().
 */
struct CStFullExpressionNode : public CStExprNode
{
    typedef CStExprNode super;

    ObjPtr m_pExpression;

    CStFullExpressionNode() : super()
    { SetClassId( clsid( CStFullExpressionNode ) ); }

    virtual std::string GetNodeInfo() const override;
    virtual std::string GetSignature() const override;
};

/**
 * @brief One array dimension: 'start .. end'
 *
 * Holds the bound expressions as parsed. Per the spec the bounds are
 * constant expressions, but they may reference named constants or
 * enum values and involve arithmetic, so evaluating them requires
 * the variable tables of the semantic phase. The parser performs
 * no numeric evaluation.
 */
struct CStSubrangeNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    ObjPtr m_pStart;
    ObjPtr m_pEnd;

    CStSubrangeNode() : super()
    { SetClassId( clsid( CStSubrangeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Subrange list node (parser accumulator)
 *
 * Collects the dimensions of an array type during parsing. Its contents
 * are flattened into CStArrayTypeNode::m_vecDims when the array type is
 * reduced, so this node is transient and does not appear in the final AST.
 */
struct CStSubrangeListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecRanges;  // CStSubrangeNode items

    CStSubrangeListNode() : super()
    { SetClassId( clsid( CStSubrangeListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Statement list node (parser accumulator)
 *
 * Collects the statements of a block (if/for/while/repeat bodies, POU
 * bodies) during parsing. Its contents are flattened into the owning
 * statement/declaration node when the block is reduced, so this node
 * is transient and does not appear in the final AST.
 */
struct CStStmtListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecStatements;

    CStStmtListNode() : super()
    { SetClassId( clsid( CStStmtListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

// ============================================================================
// Type Nodes
// ============================================================================

/**
 * @brief Base class for type nodes
 */
struct CStTypeNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    CStTypeNode() : super()
    {}

    virtual std::string GetNodeInfo() const override;
    virtual std::string GetSignature() const override;
};

/**
 * @brief Basic type node (int, bool, string, etc.)
 */
struct CStBasicTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    enum enumBasicType {
        btInt, btBool, btString, btWString,
        btByte, btWord, btDWord, btLWord,
        btInt8, btInt16, btInt32, btInt64,
        btUint8, btUint16, btUint32, btUint64,
        btReal, btLReal,
        btTime, btDate, btDateTime, btTimeOfDay,
        // Additional types for compatibility
        btDInt, btSInt, btUInt, btUDInt, btUSInt,
        btLInt, btULInt, btLTime
    };

    enumBasicType m_eBasicType;
    bool m_bSigned;
    gint32 m_iStringLength;
    // String length as a constant expression for the STRING( n ) /
    // WSTRING( n ) forms. The parser keeps the expression; the
    // numeric m_iStringLength is filled by the semantic phase after
    // constant evaluation.
    ObjPtr m_pStringLength;

    CStBasicTypeNode() : super(),
        m_eBasicType(btInt),
        m_bSigned(true),
        m_iStringLength(0)
    { SetClassId( clsid( CStBasicTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Array type node
 */
struct CStArrayTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    // Array dimension
    struct CArrayDim
    {
        // Evaluated bounds, filled in by the semantic phase once the
        // bound expressions are resolved against the variable tables
        // (named constants, enum values, arithmetic). The parser
        // does not evaluate them; they stay 0 here.
        gint32 m_iStart = 0;
        gint32 m_iEnd = 0;
        // The bound expressions as parsed (start..end)
        ObjPtr m_pStart;
        ObjPtr m_pEnd;
    };

    ObjPtr m_pElementType;
    std::vector< CArrayDim > m_vecDims;

    CStArrayTypeNode() : super()
    { SetClassId( clsid( CStArrayTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Struct type node
 */
struct CStStructTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    // Struct member
    struct CStructMember
    {
        std::string m_strName;
        ObjPtr m_pType;
        ObjPtr m_pInitialValue;
    };

    std::string m_strName;
    std::string m_strTypeName;
    std::vector< CStructMember > m_vecMembers;

    CStStructTypeNode() : super()
    { SetClassId( clsid( CStStructTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Enum type node
 */
struct CStEnumTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    std::string m_strName;
    std::string m_strTypeName;
    ObjPtr m_pBaseType;
    std::string m_strDefaultInit;
    std::vector< ObjPtr > m_vecValues;  // Contains CStEnumValueNode

    CStEnumTypeNode() : super()
    { SetClassId( clsid( CStEnumTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Enum value node - represents a single enum value
 *
 * Each enum value has a name and an optional explicit value.
 * When no explicit value is given, the compiler assigns the next
 * sequential value (auto-increment from last value or 0).
 */
struct CStEnumValueNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::string m_strName;      // Name of the enum value
    ObjPtr m_pExplicitValue;    // Optional explicit value expression

    CStEnumValueNode() : super()
    { SetClassId( clsid( CStEnumValueNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Enum value list node - container for all enum values
 *
 * This node holds the list of enum values and can be used by
 * backends to process all values together.
 */
struct CStEnumValueListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    // Temporary storage for enum type name during parsing
    // This is extracted and stored in CStEnumTypeNode.m_strName later
    std::string m_strTypeName;
    std::vector< ObjPtr > m_vecValues;  // Contains CStEnumValueNode

    CStEnumValueListNode() : super()
    { SetClassId( clsid( CStEnumValueListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Data type specification wrapper node
 *
 * This node wraps various type specifications (basic types, strings, implicit enums)
 * into a uniform node for use in type_spec. This allows type_spec to have a
 * consistent child type instead of handling multiple node type variants.
 */
struct CStDataTypeSpecNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    ObjPtr m_pTypeSpec;  // Can be CStBasicTypeNode, CStEnumTypeNode, etc.

    CStDataTypeSpecNode() : super()
    { SetClassId( clsid( CStDataTypeSpecNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Type spec wrapper node
 *
 * Wraps all type_spec alternatives (data_type_spec, array_type,
 * reference_type, pointer_type, derived_type) into a uniform node type.
 * This allows the parent rules to handle a consistent node type.
 */
struct CStTypeSpecNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    ObjPtr m_pType;  // Can be CStDataTypeSpecNode, CStArrayTypeNode, CStReferenceTypeNode, etc.

    CStTypeSpecNode() : super()
    { SetClassId( clsid( CStTypeSpecNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Type definition block node
 *
 * Contains all type declarations within a TYPE ... END_TYPE block.
 * This wraps the type_assignments to provide a uniform container.
 */
struct CStTypeDefinitionBlockNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecTypeDecls;  // CStTypeDeclNode or CStEnumTypeNode, etc.

    CStTypeDefinitionBlockNode() : super()
    { SetClassId( clsid( CStTypeDefinitionBlockNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Pointer type node
 */
struct CStPointerTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    ObjPtr m_pTargetType;

    CStPointerTypeNode() : super()
    { SetClassId( clsid( CStPointerTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Reference type node
 */
struct CStReferenceTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    ObjPtr m_pTargetType;

    CStReferenceTypeNode() : super()
    { SetClassId( clsid( CStReferenceTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Derived type node (type alias)
 */
struct CStDerivedTypeNode : public CStTypeNode
{
    typedef CStTypeNode super;

    std::string m_strName;
    std::vector< std::string > m_vecQualifiedName;
    bool m_bGlobalNamespace;
    ObjPtr m_pBaseType;

    CStDerivedTypeNode() : super(),
        m_bGlobalNamespace(false)
    { SetClassId( clsid( CStDerivedTypeNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

// ============================================================================
// Variable Declaration
// ============================================================================

/**
 * @brief Variable declaration node
 */
struct CStVarDeclNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    enum enumVarCategory {
        vcInput,
        vcOutput,
        vcInOut,
        vcLocal,
        vcTemp,
        vcGlobal,
        vcExternal,
        vcStat
    };

    enum enumVarQualifier {
        vqNone,
        vqConst,
        vqConstant = vqConst,
        vqRetain,
        vqNonRetain,
        vqPersistent
    };

    std::string m_strName;
    std::vector< std::string > m_vecNames;
    ObjPtr m_pType;
    ObjPtr m_pInitialValue;
    enumVarCategory m_eCategory;
    enumVarQualifier m_eQualifier;
    bool m_bConstant;
    bool m_bExternal;
    std::string m_strDirectAddress;
    bool m_bAtDirectAddress;
    // The CStDirectAddressNode wrapper; kept as a marker for the
    // translator, which emits different code for direct addresses
    ObjPtr m_pDirectAddr;

    CStVarDeclNode() : super(),
        m_eCategory(vcLocal),
        m_eQualifier(vqNone),
        m_bConstant(false),
        m_bExternal(false),
        m_bAtDirectAddress(false)
    { SetClassId( clsid( CStVarDeclNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

// ============================================================================
// Statement Nodes
// ============================================================================

/**
 * @brief Base class for statement nodes
 */
struct CStStmtNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    CStStmtNode() : super()
    {}

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Assignment statement
 */
struct CStAssignStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    ObjPtr m_pLValue;
    ObjPtr m_pRValue;

    CStAssignStmt() : super()
    { SetClassId( clsid( CStAssignStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Function/Method call statement
 */
struct CStCallStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    ObjPtr m_pCallExpr;

    CStCallStmt() : super()
    { SetClassId( clsid( CStCallStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief If statement
 */
struct CStIfStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    struct CIfBranch
    {
        ObjPtr m_pCondition;
        std::vector< ObjPtr > m_vecStatements;
    };

    ObjPtr m_pCondition;
    std::vector< ObjPtr > m_vecThenStatements;
    std::vector< CIfBranch > m_vecElseIfBranches;
    std::vector< ObjPtr > m_vecElseStatements;
    std::vector< CIfBranch > m_vecElseIf;
    std::vector< ObjPtr > m_vecElse;

    CStIfStmt() : super()
    { SetClassId( clsid( CStIfStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief If-branch list node (parser accumulator)
 *
 * Collects the else-if branches of an if statement during parsing.
 * Its contents are flattened into CStIfStmt::m_vecElseIfBranches when
 * the if statement is reduced, so this node is transient and does not
 * appear in the final AST.
 */
struct CStIfBranchListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< CStIfStmt::CIfBranch > m_vecBranches;

    CStIfBranchListNode() : super()
    { SetClassId( clsid( CStIfBranchListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief For statement
 */
struct CStForStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    std::string m_strIterator;
    std::string m_strLoopVar;
    ObjPtr m_pInitialValue;
    ObjPtr m_pStartValue;
    ObjPtr m_pEndValue;
    ObjPtr m_pStepValue;
    std::vector< ObjPtr > m_vecStatements;
    std::vector< ObjPtr > m_vecBody;

    CStForStmt() : super()
    { SetClassId( clsid( CStForStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief While statement
 */
struct CStWhileStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    ObjPtr m_pCondition;
    std::vector< ObjPtr > m_vecStatements;
    std::vector< ObjPtr > m_vecBody;

    CStWhileStmt() : super()
    { SetClassId( clsid( CStWhileStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Repeat statement
 */
struct CStRepeatStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    ObjPtr m_pCondition;
    std::vector< ObjPtr > m_vecStatements;
    std::vector< ObjPtr > m_vecBody;

    CStRepeatStmt() : super()
    { SetClassId( clsid( CStRepeatStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Case statement
 */
struct CStCaseStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    struct CSelectRange
    {
        ObjPtr m_pStartValue;
        ObjPtr m_pEndValue;
    };

    struct CCaseBranch
    {
        std::vector< CSelectRange > m_vecSelectors;
        std::vector< ObjPtr > m_vecStatements;
    };

    ObjPtr m_pExpression;
    std::vector< CCaseBranch > m_vecBranches;
    std::vector< ObjPtr > m_vecElse;
    std::vector< ObjPtr > m_vecElseStatements;

    CStCaseStmt() : super()
    { SetClassId( clsid( CStCaseStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Pragma statement
 */
struct CStPragmaStmt : public CStStmtNode
{
    typedef CStStmtNode super;

    enum enumPragmaType {
        ptRegion,
        ptEndRegion,
        ptCodeSection,
        data
    };

    enumPragmaType m_eType;
    std::string m_strName;
    std::string m_strValue;
    ObjPtr m_pCondition;

    CStPragmaStmt() : super(),
        m_eType(ptRegion)
    { SetClassId( clsid( CStPragmaStmt ) ); }

    virtual std::string GetNodeInfo() const override;
};

// ============================================================================
// POU (Program, Function Block, Function) Declaration Nodes
// ============================================================================

/**
 * @brief Base class for POU declaration nodes
 */
struct CStPouDeclNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    enum enumPouType {
        ptProgram,
        ptFunctionBlock,
        ptFunction
    };

    enumPouType m_ePouType;
    std::string m_strName;

    CStPouDeclNode() : super()
    {}

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Program declaration
 */
struct CStProgramDecl : public CStPouDeclNode
{
    typedef CStPouDeclNode super;

    std::vector< ObjPtr > m_vecVariables;
    std::vector< ObjPtr > m_vecStatements;
    std::vector< ObjPtr > m_vecInputVars;
    std::vector< ObjPtr > m_vecOutputVars;
    std::vector< ObjPtr > m_vecInOutVars;
    std::vector< ObjPtr > m_vecLocalVars;
    std::vector< ObjPtr > m_vecTempVars;
    std::vector< std::string > m_vecUsingNamespaces;

    CStProgramDecl() : super()
    { m_ePouType = ptProgram; SetClassId( clsid( CStProgramDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Function block declaration
 */
struct CStFunctionBlockDecl : public CStPouDeclNode
{
    typedef CStPouDeclNode super;

    enum enumModifier {
        fbmNone,
        fbmStatic,
        fbmAbstract,
        fbmFinal
    };

    // Alias for compatibility with astfactory.h
    typedef enumModifier enumFbModifier;

    enumModifier m_eModifier;
    std::string m_strExtends;
    std::vector< ObjPtr > m_vecVariables;
    std::vector< ObjPtr > m_vecMethods;
    std::vector< ObjPtr > m_vecInputVars;
    std::vector< ObjPtr > m_vecOutputVars;
    std::vector< ObjPtr > m_vecInOutVars;
    std::vector< ObjPtr > m_vecLocalVars;
    std::vector< ObjPtr > m_vecTempVars;
    std::vector< std::string > m_vecImplements;
    std::vector< ObjPtr > m_vecStatements;

    CStFunctionBlockDecl() : super(),
        m_eModifier(fbmNone)
    { m_ePouType = ptFunctionBlock; SetClassId( clsid( CStFunctionBlockDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Transient accumulator for the function block header
 *
 * Holds the header information (name, modifier, EXTENDS and
 * IMPLEMENTS clauses) until the function_block rule builds the
 * final CStFunctionBlockDecl.
 */
struct CStFunctionBlockHeaderNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::string m_strName;
    CStFunctionBlockDecl::enumModifier m_eModifier;
    std::string m_strExtends;
    std::vector< std::string > m_vecImplements;

    CStFunctionBlockHeaderNode() : super(),
        m_eModifier( CStFunctionBlockDecl::fbmNone )
    { SetClassId( clsid( CStFunctionBlockHeaderNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Function declaration
 */
struct CStFunctionDecl : public CStPouDeclNode
{
    typedef CStPouDeclNode super;

    ObjPtr m_pReturnType;
    std::vector< ObjPtr > m_vecVariables;
    std::vector< ObjPtr > m_vecStatements;
    std::vector< ObjPtr > m_vecInputVars;
    std::vector< ObjPtr > m_vecOutputVars;
    std::vector< ObjPtr > m_vecInOutVars;
    std::vector< ObjPtr > m_vecLocalVars;
    std::vector< ObjPtr > m_vecTempVars;

    CStFunctionDecl() : super()
    { m_ePouType = ptFunction; SetClassId( clsid( CStFunctionDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Method declaration within function block
 */
struct CStMethodDecl : public CStPouDeclNode
{
    typedef CStPouDeclNode super;

    enum enumAccessModifier {
        amPublic,
        amProtected,
        amPrivate
    };

    enumAccessModifier m_eAccessModifier;
    bool m_bGlobalNamespace;
    ObjPtr m_pReturnType;
    std::vector< std::string > m_vecQualifiedName;
    std::vector< ObjPtr > m_vecVariables;
    std::vector< ObjPtr > m_vecStatements;
    std::vector< ObjPtr > m_vecInputVars;
    std::vector< ObjPtr > m_vecOutputVars;
    std::vector< ObjPtr > m_vecInOutVars;
    std::vector< ObjPtr > m_vecLocalVars;
    std::vector< ObjPtr > m_vecTempVars;

    CStMethodDecl() : super(),
        m_eAccessModifier(amPublic),
        m_bGlobalNamespace(false)
    { m_ePouType = ptFunction; SetClassId( clsid( CStMethodDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

// ============================================================================
// Other Declaration Nodes
// ============================================================================

/**
 * @brief Namespace declaration
 * 
 * Represents both named namespaces and global scope boundary.
 * When m_strName is empty, this node represents the global scope,
 * which can be used to explicitly mark the boundary between global
 * declarations and backend-specific constructs.
 */
struct CStNamespaceDecl : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::string m_strName;
    std::vector< ObjPtr > m_vecElements;
    std::vector< ObjPtr > m_vecDeclarations;

    CStNamespaceDecl() : super()
    { SetClassId( clsid( CStNamespaceDecl ) ); }

    /**
     * @brief Check if this is the global scope boundary
     * @return true if m_strName is empty (global scope)
     */
    bool IsGlobalScope() const 
    { return m_strName.empty(); }

    /**
     * @brief Check if this is a named namespace
     * @return true if m_strName is not empty
     */
    bool IsNamedNamespace() const
    { return !m_strName.empty(); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Interface declaration
 */
struct CStInterfaceDecl : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::string m_strName;
    std::vector< ObjPtr > m_vecMethods;
    std::vector< ObjPtr > m_vecMethodDecls;

    CStInterfaceDecl() : super()
    { SetClassId( clsid( CStInterfaceDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Type declaration
 */
struct CStTypeDecl : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::string m_strName;
    ObjPtr m_pType;
    ObjPtr m_pTypeDefinition;

    CStTypeDecl() : super()
    { SetClassId( clsid( CStTypeDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Variable configuration declaration
 */
struct CStVarConfigDecl : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    // Instance configuration
    struct CInstanceConfig
    {
        std::string m_strPath;
        ObjPtr m_pValue;
    };

    std::string m_strInstancePath;
    ObjPtr m_pType;
    std::vector< CInstanceConfig > m_vecConfigs;

    CStVarConfigDecl() : super()
    { SetClassId( clsid( CStVarConfigDecl ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Using directive
 */
struct CStUsingDirective : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::string m_strNamespace;
    std::vector< std::string > m_vecNamespace;

    CStUsingDirective() : super()
    { SetClassId( clsid( CStUsingDirective ) ); }

    virtual std::string GetNodeInfo() const override;
};

// ============================================================================
// Root Node
// ============================================================================

/**
 * @brief Initial value node - wraps initial values with explicit type tag
 *
 * This node provides a uniform way to represent initial values with
 * explicit type information, making the AST backend-neutral for both
 * C++ and WASM code generation.
 */
struct CStInitialValueNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    enum enumInitType {
        initExpression,  // Simple expression: x := 10;
        initArray,       // Array initializer: arr := [1, 2, 3];
        initStruct       // Struct initializer: s := (a := 1, b := 2);
    };

    enumInitType m_eInitType;
    ObjPtr m_pValue;  // Expression, CStArrayInitNode, or CStStructInitNode

    CStInitialValueNode() : super(),
        m_eInitType( initExpression )
    { SetClassId( clsid( CStInitialValueNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Array initialization node
 */
struct CStArrayInitNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecValues;

    CStArrayInitNode() : super()
    { SetClassId( clsid( CStArrayInitNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Repeated array element, e.g. '3(5)' in [ 1, 3(5), 2 ]
 *
 * The repetition is kept unexpanded: expanding '10000(5)' at parse
 * time would create 10000 AST entries. The semantic phase expands
 * the node when it materializes the runtime initial values. Nested
 * repetitions never occur: the parser multiplies the counts, e.g.
 * 3(5("h")) becomes 15("h"), since neither the C++ nor the wasm
 * translator cares about the nesting. This node is an element of
 * CStArrayInitNode::m_vecValues.
 */
struct CStArrayRepeatNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    gint32 m_iCount;    // repeat count, from TOK_NUMBER
    ObjPtr m_pElement;  // the repeated element: an initial value
                        // expression

    CStArrayRepeatNode() : super(),
        m_iCount( 0 )
    { SetClassId( clsid( CStArrayRepeatNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Struct initialization node
 */
struct CStStructInitNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< std::string > m_vecMembers;
    std::vector< ObjPtr > m_vecValues;

    CStStructInitNode() : super()
    { SetClassId( clsid( CStStructInitNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Identifier list node
 */
struct CStIdentifierListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< std::string > m_vecIdentifiers;

    CStIdentifierListNode() : super()
    { SetClassId( clsid( CStIdentifierListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Transient accumulator for var_declarations
 *
 * Collects the variable declarations of the VAR...END_VAR blocks of
 * a POU. Each CStVarDeclNode in the list carries its category
 * (input, output, ...) applied at block level, so the consumers can
 * split the list into the input/output/local/temp vectors of the
 * POU declaration.
 */
struct CStVarDeclListNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecVarDecls;

    CStVarDeclListNode() : super()
    { SetClassId( clsid( CStVarDeclListNode ) ); }

    virtual std::string GetNodeInfo() const override;
};

/**
 * @brief Root node for the entire AST
 *
 * The root node contains all top-level declarations and provides
 * access to the global namespace scope for absolute path resolution.
 */
struct CStRootNode : public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;

    std::vector< ObjPtr > m_vecChildren;
    std::vector< ObjPtr > m_vecDeclarations;

    CStRootNode() : super()
    { SetClassId( clsid( CStRootNode ) ); }

    /**
     * @brief Get the global namespace scope for absolute path resolution
     * 
     * Returns a virtual namespace node with empty name that contains
     * all top-level declarations. This node is used as the starting
     * point when resolving absolute paths like '::abc'.
     * 
     * The returned node is a child of the root but not part of
     * m_vecDeclarations to avoid duplication in code generation.
     * 
     * @return ObjPtr pointing to CStNamespaceDecl with empty name
     */
    ObjPtr GetGlobalScope() const;

    /**
     * @brief Get debug info about this node
     *
     * Returns a string representation of the node for debugging purposes.
     * This is NOT meant to produce valid ST code - use the code generator
     * for that purpose.
     *
     * @return std::string with debug information
     */
    std::string GetNodeInfo() const;
};

} // namespace rpcf
