/*
 * =====================================================================================
 *
 *       Filename:  astdebug.cpp
 *
 *    Description:  Utility functions for debugging and dumping AST trees
 *
 *        Version:  1.0
 *        Created:  01/15/2026
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *   Organization:
 *
 *      Copyright:
 *
 * =====================================================================================
 */

#include "astdebug.h"
#include <sstream>
#include <iomanip>

using namespace rpcf;

namespace rpcf
{

// ============================================================================
// Helper functions
// ============================================================================

static std::string GetLiteralTypeName( CStLiteralExpr::enumLiteralType eType )
{
    switch( eType )
    {
        case CStLiteralExpr::ltNumber:   return "Number";
        case CStLiteralExpr::ltString:   return "String";
        case CStLiteralExpr::ltWString:  return "WString";
        case CStLiteralExpr::ltBool:     return "Bool";
        case CStLiteralExpr::ltTime:     return "Time";
        case CStLiteralExpr::ltLTime:    return "LTime";
        case CStLiteralExpr::ltDate:     return "Date";
        case CStLiteralExpr::ltDateTime: return "DateTime";
        case CStLiteralExpr::ltTimeOfDay:return "TimeOfDay";
        default: return "Unknown";
    }
}

static std::string GetBinaryOpName( CStBinaryExpr::enumBinaryOp eOp )
{
    switch( eOp )
    {
        case CStBinaryExpr::boAdd:         return "+";
        case CStBinaryExpr::boSub:         return "-";
        case CStBinaryExpr::boMul:         return "*";
        case CStBinaryExpr::boDiv:         return "/";
        case CStBinaryExpr::boMod:         return "MOD";
        case CStBinaryExpr::boAnd:         return "AND";
        case CStBinaryExpr::boOr:          return "OR";
        case CStBinaryExpr::boXor:         return "XOR";
        case CStBinaryExpr::boEqual:       return "=";
        case CStBinaryExpr::boNotEqual:    return "<>";
        case CStBinaryExpr::boLessThan:    return "<";
        case CStBinaryExpr::boLessEqual:   return "<=";
        case CStBinaryExpr::boGreaterThan: return ">";
        case CStBinaryExpr::boGreaterEqual:return ">=";
        case CStBinaryExpr::boPower:       return "**";
        default: return "Unknown";
    }
}

static std::string GetUnaryOpName( CStUnaryExpr::enumUnaryOp eOp )
{
    switch( eOp )
    {
        case CStUnaryExpr::uoNot: return "NOT";
        case CStUnaryExpr::uoNeg: return "-";
        default: return "Unknown";
    }
}

static std::string VariantToStr( const Variant& oVar )
{
    std::ostringstream oss;
    switch( oVar.GetTypeId() )
    {
        case typeString:
            oss << "\"" << ( const std::string& )oVar << "\"";
            break;
        case typeByte:
            oss << ( ( const guint8& )oVar ? 1 : 0 );
            break;
        case typeUInt16:
            oss << ( guint32 )( const guint16& )oVar;
            break;
        case typeUInt32:
            oss << ( guint32 )( const guint32& )oVar;
            break;
        case typeUInt64:
            oss << ( guint64 )( const guint64& )oVar;
            break;
        case typeFloat:
            oss << ( double )( const float& )oVar;
            break;
        case typeDouble:
            oss << ( double )( const double& )oVar;
            break;
        case typeObj:
            oss << "<obj>";
            break;
        default:
            oss << "<none>";
            break;
    }
    return oss.str();
}

static std::string GetBasicTypeName( CStBasicTypeNode::enumBasicType eType )
{
    switch( eType )
    {
        case CStBasicTypeNode::btBool:      return "BOOL";
        case CStBasicTypeNode::btByte:      return "BYTE";
        case CStBasicTypeNode::btWord:      return "WORD";
        case CStBasicTypeNode::btDWord:     return "DWORD";
        case CStBasicTypeNode::btLWord:     return "LWORD";
        case CStBasicTypeNode::btSInt:      return "SINT";
        case CStBasicTypeNode::btInt:       return "INT";
        case CStBasicTypeNode::btDInt:      return "DINT";
        case CStBasicTypeNode::btLInt:      return "LINT";
        case CStBasicTypeNode::btUSInt:     return "USINT";
        case CStBasicTypeNode::btUInt:      return "UINT";
        case CStBasicTypeNode::btUDInt:     return "UDINT";
        case CStBasicTypeNode::btULInt:     return "ULINT";
        case CStBasicTypeNode::btInt8:      return "INT8";
        case CStBasicTypeNode::btInt16:     return "INT16";
        case CStBasicTypeNode::btInt32:     return "INT32";
        case CStBasicTypeNode::btInt64:     return "INT64";
        case CStBasicTypeNode::btUint8:     return "UINT8";
        case CStBasicTypeNode::btUint16:    return "UINT16";
        case CStBasicTypeNode::btUint32:    return "UINT32";
        case CStBasicTypeNode::btUint64:    return "UINT64";
        case CStBasicTypeNode::btReal:      return "REAL";
        case CStBasicTypeNode::btLReal:     return "LREAL";
        case CStBasicTypeNode::btTime:      return "TIME";
        case CStBasicTypeNode::btLTime:     return "LTIME";
        case CStBasicTypeNode::btDate:      return "DATE";
        case CStBasicTypeNode::btTimeOfDay: return "TIME_OF_DAY";
        case CStBasicTypeNode::btDateTime:  return "DATE_AND_TIME";
        case CStBasicTypeNode::btString:    return "STRING";
        case CStBasicTypeNode::btWString:   return "WSTRING";
        default: return "Unknown";
    }
}

// ============================================================================
// Public interface functions
// ============================================================================

std::string GetNodeTypeName( EnumClsid eClsid )
{
    if( eClsid == clsid( CStRootNode ) )
        return "RootNode";
    if( eClsid == clsid( CStProgramDecl ) )
        return "ProgramDecl";
    if( eClsid == clsid( CStFunctionDecl ) )
        return "FunctionDecl";
    if( eClsid == clsid( CStFunctionBlockDecl ) )
        return "FunctionBlockDecl";
    if( eClsid == clsid( CStFunctionBlockHeaderNode ) )
        return "FunctionBlockHeaderNode";
    if( eClsid == clsid( CStLiteralExpr ) )
        return "LiteralExpr";
    if( eClsid == clsid( CStIdentifierExpr ) )
        return "IdentifierExpr";
    if( eClsid == clsid( CStDirectAddressNode ) )
        return "DirectAddressNode";
    if( eClsid == clsid( CStBinaryExpr ) )
        return "BinaryExpr";
    if( eClsid == clsid( CStUnaryExpr ) )
        return "UnaryExpr";
    if( eClsid == clsid( CStCallExpr ) )
        return "CallExpr";
    if( eClsid == clsid( CStArgListNode ) )
        return "ArgListNode";
    if( eClsid == clsid( CStArrayAccessExpr ) )
        return "ArrayAccessExpr";
    if( eClsid == clsid( CStArrayInitNode ) )
        return "ArrayInitNode";
    if( eClsid == clsid( CStArrayRepeatNode ) )
        return "ArrayRepeatNode";
    if( eClsid == clsid( CStMemberAccessExpr ) )
        return "MemberAccessExpr";
    if( eClsid == clsid( CStDereferenceExpr ) )
        return "DereferenceExpr";
    if( eClsid == clsid( CStPointerMemberExpr ) )
        return "PointerMemberExpr";
    if( eClsid == clsid( CStLValueNode ) )
        return "LValueNode";
    if( eClsid == clsid( CStLValueExtNode ) )
        return "LValueExtNode";
    if( eClsid == clsid( CStInstancePathNode ) )
        return "InstancePathNode";
    if( eClsid == clsid( CStFullExpressionNode ) )
        return "FullExpressionNode";
    if( eClsid == clsid( CStSubrangeNode ) )
        return "SubrangeNode";
    if( eClsid == clsid( CStSubrangeListNode ) )
        return "SubrangeListNode";
    if( eClsid == clsid( CStStmtListNode ) )
        return "StmtListNode";
    if( eClsid == clsid( CStIfBranchListNode ) )
        return "IfBranchListNode";
    if( eClsid == clsid( CStVarDeclListNode ) )
        return "VarDeclListNode";
    if( eClsid == clsid( CStVarDeclNode ) )
        return "VarDeclNode";
    if( eClsid == clsid( CStBasicTypeNode ) )
        return "BasicTypeNode";
    if( eClsid == clsid( CStArrayTypeNode ) )
        return "ArrayTypeNode";
    if( eClsid == clsid( CStPointerTypeNode ) )
        return "PointerTypeNode";
    if( eClsid == clsid( CStReferenceTypeNode ) )
        return "ReferenceTypeNode";
    if( eClsid == clsid( CStDerivedTypeNode ) )
        return "DerivedTypeNode";
    if( eClsid == clsid( CStDataTypeSpecNode ) )
        return "DataTypeSpecNode";
    if( eClsid == clsid( CStTypeSpecNode ) )
        return "TypeSpecNode";
    if( eClsid == clsid( CStTypeDefinitionBlockNode ) )
        return "TypeDefinitionBlockNode";
    if( eClsid == clsid( CStEnumTypeNode ) )
        return "EnumTypeNode";
    if( eClsid == clsid( CStEnumValueNode ) )
        return "EnumValueNode";
    if( eClsid == clsid( CStEnumValueListNode ) )
        return "EnumValueListNode";
    if( eClsid == clsid( CStIfStmt ) )
        return "IfStmt";
    if( eClsid == clsid( CStForStmt ) )
        return "ForStmt";
    if( eClsid == clsid( CStWhileStmt ) )
        return "WhileStmt";
    if( eClsid == clsid( CStRepeatStmt ) )
        return "RepeatStmt";
    if( eClsid == clsid( CStCaseStmt ) )
        return "CaseStmt";
    if( eClsid == clsid( CStAssignStmt ) )
        return "AssignStmt";
    if( eClsid == clsid( CStCallStmt ) )
        return "CallStmt";

    return std::string( "Unknown(" ) + std::to_string( (gint32)eClsid ) + ")";
}

std::string GetNodeDebugInfo( const ObjPtr& pNode )
{
    if( pNode.IsEmpty() )
        return "null";

    std::ostringstream oss;
    CObjBase* pBase = ( CObjBase* )pNode;

    EnumClsid eClsid = pBase->GetClsid();

    if( eClsid == clsid( CStLiteralExpr ) )
    {
        CStLiteralExpr* p = dynamic_cast< CStLiteralExpr* >( pBase );
        if( p )
        {
            oss << "type=" << GetLiteralTypeName( p->m_eLiteralType ) << ", ";
            oss << "value=" << VariantToStr( p->m_oLiteralValue );
        }
    }
    else if( eClsid == clsid( CStIdentifierExpr ) )
    {
        CStIdentifierExpr* p = dynamic_cast< CStIdentifierExpr* >( pBase );
        if( p )
            oss << "name=" << p->m_strName;
    }
    else if( eClsid == clsid( CStDirectAddressNode ) )
    {
        CStDirectAddressNode* p =
            dynamic_cast< CStDirectAddressNode* >( pBase );
        if( p )
        {
            oss << "addr=" << p->m_strAddress;
            if( p->m_eAddrType == CStDirectAddressNode::datRpcf )
                oss << ", rpcf";
            else if( p->m_eAddrType ==
                CStDirectAddressNode::datPeripheral )
                oss << ", peripheral";
            else if( p->m_eAddrType ==
                CStDirectAddressNode::datPeripheralOffset )
                oss << ", peripheral+offset";
            if( !p->m_pIndex.IsEmpty() )
                oss << ", indexed";
        }
    }
    else if( eClsid == clsid( CStArrayInitNode ) )
    {
        CStArrayInitNode* p = dynamic_cast< CStArrayInitNode* >( pBase );
        if( p )
            oss << "values=" << p->m_vecValues.size();
    }
    else if( eClsid == clsid( CStArrayRepeatNode ) )
    {
        CStArrayRepeatNode* p =
            dynamic_cast< CStArrayRepeatNode* >( pBase );
        if( p )
            oss << "count=" << p->m_iCount;
    }
    else if( eClsid == clsid( CStBinaryExpr ) )
    {
        CStBinaryExpr* p = dynamic_cast< CStBinaryExpr* >( pBase );
        if( p )
            oss << "op=" << GetBinaryOpName( p->m_eOperator );
    }
    else if( eClsid == clsid( CStUnaryExpr ) )
    {
        CStUnaryExpr* p = dynamic_cast< CStUnaryExpr* >( pBase );
        if( p )
            oss << "op=" << GetUnaryOpName( p->m_eOperator );
    }
    else if( eClsid == clsid( CStCallExpr ) )
    {
        CStCallExpr* p = dynamic_cast< CStCallExpr* >( pBase );
        if( p )
            oss << "positional=" << p->m_vecArgs.size()
                << ", named=" << p->m_vecNamedArgs.size();
    }
    else if( eClsid == clsid( CStArgListNode ) )
    {
        CStArgListNode* p = dynamic_cast< CStArgListNode* >( pBase );
        if( p )
            oss << "positional=" << p->m_vecArgs.size()
                << ", named=" << p->m_vecNamed.size();
    }
    else if( eClsid == clsid( CStArrayAccessExpr ) )
    {
        CStArrayAccessExpr* p = dynamic_cast< CStArrayAccessExpr* >( pBase );
        if( p )
            oss << "indices=" << p->m_vecIndices.size();
    }
    else if( eClsid == clsid( CStMemberAccessExpr ) )
    {
        CStMemberAccessExpr* p = dynamic_cast< CStMemberAccessExpr* >( pBase );
        if( p )
        {
            oss << "access=";
            if( p->m_eAccessType == CStMemberAccessExpr::atDot )
                oss << ".";
            else
                oss << "->";
            oss << ", member=" << p->m_strMember;
        }
    }
    else if( eClsid == clsid( CStDereferenceExpr ) )
    {
        oss << "deref";
    }
    else if( eClsid == clsid( CStPointerMemberExpr ) )
    {
        CStPointerMemberExpr* p = dynamic_cast< CStPointerMemberExpr* >( pBase );
        if( p )
            oss << "member=" << p->m_strMember;
    }
    else if( eClsid == clsid( CStLValueNode ) )
    {
        oss << "expr";
    }
    else if( eClsid == clsid( CStLValueExtNode ) )
    {
        oss << "ext_expr";
    }
    else if( eClsid == clsid( CStInstancePathNode ) )
    {
        CStInstancePathNode* p = dynamic_cast< CStInstancePathNode* >( pBase );
        if( p )
            oss << "path=" << p->GetDottedName();
    }
    else if( eClsid == clsid( CStFullExpressionNode ) )
    {
        oss << "full_expr";
    }
    else if( eClsid == clsid( CStSubrangeNode ) )
    {
        oss << "range";
    }
    else if( eClsid == clsid( CStSubrangeListNode ) )
    {
        CStSubrangeListNode* p = dynamic_cast< CStSubrangeListNode* >( pBase );
        if( p )
            oss << "dims=" << p->m_vecRanges.size();
    }
    else if( eClsid == clsid( CStStmtListNode ) )
    {
        CStStmtListNode* p = dynamic_cast< CStStmtListNode* >( pBase );
        if( p )
            oss << "stmts=" << p->m_vecStatements.size();
    }
    else if( eClsid == clsid( CStVarDeclListNode ) )
    {
        CStVarDeclListNode* p =
            dynamic_cast< CStVarDeclListNode* >( pBase );
        if( p )
            oss << "varDecls=" << p->m_vecVarDecls.size();
    }
    else if( eClsid == clsid( CStIfBranchListNode ) )
    {
        CStIfBranchListNode* p =
            dynamic_cast< CStIfBranchListNode* >( pBase );
        if( p )
            oss << "branches=" << p->m_vecBranches.size();
    }
    else if( eClsid == clsid( CStProgramDecl ) )
    {
        CStProgramDecl* p = dynamic_cast< CStProgramDecl* >( pBase );
        if( p )
        {
            oss << "name=" << p->m_strName << ", ";
            oss << "inputs=" << p->m_vecInputVars.size() << ", ";
            oss << "outputs=" << p->m_vecOutputVars.size() << ", ";
            oss << "stmts=" << p->m_vecStatements.size();
        }
    }
    else if( eClsid == clsid( CStFunctionDecl ) )
    {
        CStFunctionDecl* p = dynamic_cast< CStFunctionDecl* >( pBase );
        if( p )
            oss << "name=" << p->m_strName;
    }
    else if( eClsid == clsid( CStFunctionBlockDecl ) )
    {
        CStFunctionBlockDecl* p = dynamic_cast< CStFunctionBlockDecl* >( pBase );
        if( p )
            oss << "name=" << p->m_strName;
    }
    else if( eClsid == clsid( CStFunctionBlockHeaderNode ) )
    {
        CStFunctionBlockHeaderNode* p =
            dynamic_cast< CStFunctionBlockHeaderNode* >( pBase );
        if( p )
        {
            oss << "name=" << p->m_strName;
            if( !p->m_strExtends.empty() )
                oss << ", extends=" << p->m_strExtends;
            if( !p->m_vecImplements.empty() )
                oss << ", implements=" << p->m_vecImplements.size();
        }
    }
    else if( eClsid == clsid( CStVarDeclNode ) )
    {
        CStVarDeclNode* p = dynamic_cast< CStVarDeclNode* >( pBase );
        if( p )
        {
            oss << "name=" << p->m_strName;
            if( !p->m_strDirectAddress.empty() )
                oss << ", at=" << p->m_strDirectAddress;
        }
    }
    else if( eClsid == clsid( CStBasicTypeNode ) )
    {
        CStBasicTypeNode* p = dynamic_cast< CStBasicTypeNode* >( pBase );
        if( p )
        {
            oss << "type=" << GetBasicTypeName( p->m_eBasicType );
            if( p->m_iStringLength > 0 )
                oss << ", len=" << p->m_iStringLength;
            else if( !p->m_pStringLength.IsEmpty() )
                oss << ", len=expr";
        }
    }
    else if( eClsid == clsid( CStArrayTypeNode ) )
    {
        CStArrayTypeNode* p = dynamic_cast< CStArrayTypeNode* >( pBase );
        if( p )
            oss << "dims=" << p->m_vecDims.size();
    }
    else if( eClsid == clsid( CStPointerTypeNode ) )
    {
        CStPointerTypeNode* p = dynamic_cast< CStPointerTypeNode* >( pBase );
        if( p )
        {
            if( !p->m_pTargetType.IsEmpty() )
                oss << "target=set";
            else
                oss << "target=null";
        }
    }
    else if( eClsid == clsid( CStReferenceTypeNode ) )
    {
        CStReferenceTypeNode* p = dynamic_cast< CStReferenceTypeNode* >( pBase );
        if( p )
        {
            if( !p->m_pTargetType.IsEmpty() )
                oss << "target=set";
            else
                oss << "target=null";
        }
    }
    else if( eClsid == clsid( CStDerivedTypeNode ) )
    {
        CStDerivedTypeNode* p = dynamic_cast< CStDerivedTypeNode* >( pBase );
        if( p )
        {
            if( !p->m_vecQualifiedName.empty() )
            {
                oss << "name=";
                for( size_t i = 0; i < p->m_vecQualifiedName.size(); i++ )
                {
                    if( i > 0 )
                        oss << ".";
                    oss << p->m_vecQualifiedName[i];
                }
            }
        }
    }
    else if( eClsid == clsid( CStTypeDefinitionBlockNode ) )
    {
        CStTypeDefinitionBlockNode* p = dynamic_cast< CStTypeDefinitionBlockNode* >( pBase );
        if( p )
            oss << "decls=" << p->m_vecTypeDecls.size();
    }
    else if( eClsid == clsid( CStEnumTypeNode ) )
    {
        CStEnumTypeNode* p = dynamic_cast< CStEnumTypeNode* >( pBase );
        if( p )
            oss << "name=" << p->m_strName;
    }
    else if( eClsid == clsid( CStEnumValueNode ) )
    {
        CStEnumValueNode* p = dynamic_cast< CStEnumValueNode* >( pBase );
        if( p )
            oss << "name=" << p->m_strName;
    }
    else if( eClsid == clsid( CStEnumValueListNode ) )
    {
        CStEnumValueListNode* p = dynamic_cast< CStEnumValueListNode* >( pBase );
        if( p )
            oss << "values=" << p->m_vecValues.size();
        if( !p->m_strTypeName.empty() )
            oss << ", type_name=" << p->m_strTypeName;
    }
    else
    {
        // Generic node info
        CSTAstNodeBase* p = dynamic_cast< CSTAstNodeBase* >( pBase );
        if( p )
            oss << p->GetNodeInfo();
    }

    return oss.str();
}

void DumpAstTree(
    const ObjPtr& pNode,
    std::ostream& os,
    gint32 iDepth,
    const std::string& strIndent )
{
    if( pNode.IsEmpty() )
        return;

    CObjBase* pBase = ( CObjBase* )pNode;
    EnumClsid eClsid = pBase->GetClsid();

    // Print indentation
    for( gint32 i = 0; i < iDepth; i++ )
        os << strIndent;

    // Print node type and info
    os << "[" << GetNodeTypeName( eClsid ) << "] ";
    os << GetNodeDebugInfo( pNode );
    os << std::endl;

    // Recursively dump children based on node type
    if( eClsid == clsid( CStBinaryExpr ) )
    {
        CStBinaryExpr* p = dynamic_cast< CStBinaryExpr* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pLeft, os, iDepth + 1, strIndent );
            DumpAstTree( p->m_pRight, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStUnaryExpr ) )
    {
        CStUnaryExpr* p = dynamic_cast< CStUnaryExpr* >( pBase );
        if( p )
            DumpAstTree( p->m_pOperand, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStCallExpr ) )
    {
        CStCallExpr* p = dynamic_cast< CStCallExpr* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pCallee, os, iDepth + 1, strIndent );
            for( const auto& arg : p->m_vecArgs )
                DumpAstTree( arg, os, iDepth + 1, strIndent );
            for( const auto& arg : p->m_vecNamedArgs )
                DumpAstTree( arg.m_pValue, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStArgListNode ) )
    {
        CStArgListNode* p = dynamic_cast< CStArgListNode* >( pBase );
        if( p )
        {
            for( const auto& arg : p->m_vecArgs )
                DumpAstTree( arg, os, iDepth + 1, strIndent );
            for( const auto& arg : p->m_vecNamed )
                DumpAstTree( arg.m_pValue, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStArrayAccessExpr ) )
    {
        CStArrayAccessExpr* p = dynamic_cast< CStArrayAccessExpr* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pArray, os, iDepth + 1, strIndent );
            for( const auto& index : p->m_vecIndices )
                DumpAstTree( index, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStDirectAddressNode ) )
    {
        CStDirectAddressNode* p =
            dynamic_cast< CStDirectAddressNode* >( pBase );
        if( p && !p->m_pIndex.IsEmpty() )
        {
            DumpAstTree( p->m_pIndex, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStArrayInitNode ) )
    {
        CStArrayInitNode* p = dynamic_cast< CStArrayInitNode* >( pBase );
        if( p )
        {
            for( const auto& val : p->m_vecValues )
                DumpAstTree( val, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStArrayRepeatNode ) )
    {
        CStArrayRepeatNode* p =
            dynamic_cast< CStArrayRepeatNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pElement, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStMemberAccessExpr ) )
    {
        CStMemberAccessExpr* p = dynamic_cast< CStMemberAccessExpr* >( pBase );
        if( p )
            DumpAstTree( p->m_pObject, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStDereferenceExpr ) )
    {
        CStDereferenceExpr* p = dynamic_cast< CStDereferenceExpr* >( pBase );
        if( p )
            DumpAstTree( p->m_pPointer, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStPointerMemberExpr ) )
    {
        CStPointerMemberExpr* p = dynamic_cast< CStPointerMemberExpr* >( pBase );
        if( p )
            DumpAstTree( p->m_pPointer, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStLValueNode ) )
    {
        CStLValueNode* p = dynamic_cast< CStLValueNode* >( pBase );
        if( p )
            DumpAstTree( p->m_pExpression, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStLValueExtNode ) )
    {
        CStLValueExtNode* p = dynamic_cast< CStLValueExtNode* >( pBase );
        if( p )
            DumpAstTree( p->m_pExpression, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStInstancePathNode ) )
    {
        CStInstancePathNode* p = dynamic_cast< CStInstancePathNode* >( pBase );
        if( p )
            DumpAstTree( p->m_pExpression, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStFullExpressionNode ) )
    {
        CStFullExpressionNode* p = dynamic_cast< CStFullExpressionNode* >( pBase );
        if( p )
            DumpAstTree( p->m_pExpression, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStSubrangeNode ) )
    {
        CStSubrangeNode* p = dynamic_cast< CStSubrangeNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pStart, os, iDepth + 1, strIndent );
            DumpAstTree( p->m_pEnd, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStSubrangeListNode ) )
    {
        CStSubrangeListNode* p = dynamic_cast< CStSubrangeListNode* >( pBase );
        if( p )
        {
            for( size_t i = 0; i < p->m_vecRanges.size(); i++ )
                DumpAstTree( p->m_vecRanges[ i ], os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStStmtListNode ) )
    {
        CStStmtListNode* p = dynamic_cast< CStStmtListNode* >( pBase );
        if( p )
        {
            for( size_t i = 0; i < p->m_vecStatements.size(); i++ )
                DumpAstTree( p->m_vecStatements[ i ], os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStVarDeclListNode ) )
    {
        CStVarDeclListNode* p =
            dynamic_cast< CStVarDeclListNode* >( pBase );
        if( p )
        {
            for( size_t i = 0; i < p->m_vecVarDecls.size(); i++ )
                DumpAstTree( p->m_vecVarDecls[ i ], os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStIfBranchListNode ) )
    {
        CStIfBranchListNode* p =
            dynamic_cast< CStIfBranchListNode* >( pBase );
        if( p )
        {
            for( size_t i = 0; i < p->m_vecBranches.size(); i++ )
            {
                DumpAstTree( p->m_vecBranches[ i ].m_pCondition,
                    os, iDepth + 1, strIndent );
                const std::vector< ObjPtr >& vecStmts =
                    p->m_vecBranches[ i ].m_vecStatements;
                for( size_t j = 0; j < vecStmts.size(); j++ )
                    DumpAstTree( vecStmts[ j ], os, iDepth + 2, strIndent );
            }
        }
    }
    else if( eClsid == clsid( CStProgramDecl ) )
    {
        CStProgramDecl* p = dynamic_cast< CStProgramDecl* >( pBase );
        if( p )
        {
            for( const auto& decl : p->m_vecInputVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : p->m_vecOutputVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : p->m_vecInOutVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : p->m_vecLocalVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : p->m_vecTempVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& stmt : p->m_vecStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStFunctionBlockDecl ) )
    {
        CStFunctionBlockDecl* pFB =
            dynamic_cast< CStFunctionBlockDecl* >( pBase );
        if( pFB )
        {
            for( const auto& decl : pFB->m_vecInputVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFB->m_vecOutputVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFB->m_vecInOutVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFB->m_vecLocalVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFB->m_vecTempVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFB->m_vecVariables )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFB->m_vecMethods )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& stmt : pFB->m_vecStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStFunctionDecl ) )
    {
        CStFunctionDecl* pFunc = dynamic_cast< CStFunctionDecl* >( pBase );
        if( pFunc )
        {
            for( const auto& decl : pFunc->m_vecInputVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFunc->m_vecOutputVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFunc->m_vecInOutVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFunc->m_vecLocalVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& decl : pFunc->m_vecTempVars )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
            for( const auto& stmt : pFunc->m_vecStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStIfStmt ) )
    {
        CStIfStmt* p = dynamic_cast< CStIfStmt* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pCondition, os, iDepth + 1, strIndent );
            for( const auto& stmt : p->m_vecThenStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
            for( const auto& branch : p->m_vecElseIfBranches )
            {
                DumpAstTree( branch.m_pCondition, os, iDepth + 1, strIndent );
                for( const auto& stmt : branch.m_vecStatements )
                    DumpAstTree( stmt, os, iDepth + 2, strIndent );
            }
            for( const auto& stmt : p->m_vecElseStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStForStmt ) )
    {
        CStForStmt* p = dynamic_cast< CStForStmt* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pInitialValue, os, iDepth + 1, strIndent );
            DumpAstTree( p->m_pStartValue, os, iDepth + 1, strIndent );
            DumpAstTree( p->m_pEndValue, os, iDepth + 1, strIndent );
            if( !p->m_pStepValue.IsEmpty() )
                DumpAstTree( p->m_pStepValue, os, iDepth + 1, strIndent );
            for( const auto& stmt : p->m_vecStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStWhileStmt ) )
    {
        CStWhileStmt* p = dynamic_cast< CStWhileStmt* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pCondition, os, iDepth + 1, strIndent );
            for( const auto& stmt : p->m_vecStatements )
                DumpAstTree( stmt, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStAssignStmt ) )
    {
        CStAssignStmt* p = dynamic_cast< CStAssignStmt* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pLValue, os, iDepth + 1, strIndent );
            DumpAstTree( p->m_pRValue, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStCallStmt ) )
    {
        CStCallStmt* p = dynamic_cast< CStCallStmt* >( pBase );
        if( p )
            DumpAstTree( p->m_pCallExpr, os, iDepth + 1, strIndent );
    }
    else if( eClsid == clsid( CStArrayTypeNode ) )
    {
        CStArrayTypeNode* p = dynamic_cast< CStArrayTypeNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pElementType, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStPointerTypeNode ) )
    {
        CStPointerTypeNode* p = dynamic_cast< CStPointerTypeNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pTargetType, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStReferenceTypeNode ) )
    {
        CStReferenceTypeNode* p = dynamic_cast< CStReferenceTypeNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pTargetType, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStDataTypeSpecNode ) )
    {
        CStDataTypeSpecNode* p = dynamic_cast< CStDataTypeSpecNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pTypeSpec, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStTypeSpecNode ) )
    {
        CStTypeSpecNode* p = dynamic_cast< CStTypeSpecNode* >( pBase );
        if( p )
        {
            DumpAstTree( p->m_pType, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStTypeDefinitionBlockNode ) )
    {
        CStTypeDefinitionBlockNode* p = dynamic_cast< CStTypeDefinitionBlockNode* >( pBase );
        if( p )
        {
            for( const auto& decl : p->m_vecTypeDecls )
                DumpAstTree( decl, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStEnumTypeNode ) )
    {
        CStEnumTypeNode* p = dynamic_cast< CStEnumTypeNode* >( pBase );
        if( p )
        {
            for( const auto& val : p->m_vecValues )
                DumpAstTree( val, os, iDepth + 1, strIndent );
            if( !p->m_pBaseType.IsEmpty() )
                DumpAstTree( p->m_pBaseType, os, iDepth + 1, strIndent );
        }
    }
    else if( eClsid == clsid( CStEnumValueListNode ) )
    {
        CStEnumValueListNode* p = dynamic_cast< CStEnumValueListNode* >( pBase );
        if( p )
        {
            for( const auto& val : p->m_vecValues )
                DumpAstTree( val, os, iDepth + 1, strIndent );
        }
    }
}

std::string AstTreeToString( const ObjPtr& pNode )
{
    std::ostringstream oss;
    DumpAstTree( pNode, oss );
    return oss.str();
}

} // namespace rpcf