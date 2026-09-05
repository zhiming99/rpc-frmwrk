/*
 * =====================================================================================
 *
 *       Filename:  stparser.y
 *
 *    Description:  The grammar parser for Structured Text Language
 *
 *        Version:  1.0
 *        Created:  04/10/2026 12:00:00 PM
 *       Revision:  none
 *       Compiler:  Bison
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

%locations
%define api.pure full
%define api.push-pull push
%require "3.0"

%{
#include <rpc.h>
#include "stlexer.h"
#include "parsrctx.h"
#include "stclsids.h"
#include "astnodes.h"
#include "astfactory.h"
#include "astbuilder.h"

using namespace rpcf;

extern std::shared_ptr< rpcf::CSTParserContext > g_pParserCtx;
extern ObjPtr g_pAstRoot;  // Global root AST node

// Helper to create AST nodes using NewObj pattern
// MUST be defined before use in grammar rules
template <typename Invalid >
ObjPtr CreateAstNode()
{
    ObjPtr pNode; 
    pNode.NewObj( clsid( Invalid ) );
    return pNode;
}

//#define CreateAstNode(T) MakeAstNode( T )

// Helper to check if YYSTYPE (shared_ptr<YYSPAIR>) contains ObjPtr
inline bool IsObjPtrVal( YYSTYPE yylval )
{
    if( !yylval ) return false;
    return yylval->first.GetTypeId() == typeObj;
}

// Helper to extract ObjPtr from YYSTYPE
inline ObjPtr ToObjPtrVal( YYSTYPE yylval )
{
    if( !yylval ) return ObjPtr();
    const Variant& var = yylval->first;
    if( var.GetTypeId() != typeObj ) return ObjPtr();
    // Use the implicit conversion operator
    return var;
}

// Helper to check if YYSTYPE contains vector of ObjPtr
inline bool IsVecObjVal( YYSTYPE yylval )
{
    if( !yylval ) return false;
    return yylval->first.GetTypeId() == typeObj;
}

// Helper to check if YYSTYPE contains vector of strings
inline bool IsVecStrVal( YYSTYPE yylval )
{
    if( !yylval ) return false;
    // VecStr type ID check
    return yylval->first.GetTypeId() == typeObj;
}

// Helper to extract vector of strings from YYSTYPE
inline std::vector< std::string > ToVecStrVal( YYSTYPE yylval )
{
    if( !yylval ) return std::vector< std::string >();
    // Placeholder - actual implementation depends on Variant type
    return std::vector< std::string >();
}

// Helper to get Variant from YYSTYPE
inline Variant& GetVariantVal( YYSTYPE yylval )
{
    static Variant dummy;
    if( !yylval ) return dummy;
    return yylval->first;
}

// Helper to create empty YYSTYPE
inline YYSTYPE MakeEmptyVal()
{
    return std::make_shared< YYSPAIR >(
        std::piecewise_construct,
        std::forward_as_tuple(),
        std::forward_as_tuple() );
}

#define MAKE_EMPTY() MakeEmptyVal()

%}

%code requires {

#include <rpc.h>
#include "parsrctx.h"
using namespace rpcf;

extern void ParserPrint(
    const char* szFile,
    gint32 iLineNo,
    const char* strMsg,
    bool bErr );

struct yypstate;
extern int GetParserState( yypstate* ps );

void yyerror (YYLTYPE* yyloc,
    rpcf::CSTParserContext* pCtx,
    const char* yymsgp);
}

%token TOK_PROGRAM TOK_VAR TOK_END_VAR TOK_IF TOK_THEN TOK_ELSE TOK_ELSIF TOK_END_IF TOK_FOR TOK_TO TOK_DO TOK_END_FOR TOK_WHILE TOK_END_WHILE TOK_REPEAT TOK_UNTIL TOK_END_REPEAT
%token TOK_TON TOK_TON_VALUE TOK_STRING TOK_WSTRING TOK_INT TOK_REAL TOK_LREAL TOK_BOOL TOK_TRUE TOK_FALSE TOK_TIME TOK_LTIME TOK_TYPED_LITERAL TOK_TYPE TOK_END_TYPE TOK_STRUCT TOK_END_STRUCT
%token TOK_UINT TOK_DINT TOK_UDINT TOK_SINT TOK_USINT TOK_BYTE TOK_WORD TOK_DWORD TOK_ULINT TOK_LINT TOK_LWORD TOK_LDWORD

%token TOK_ID TOK_NUMBER TOK_ASSIGN TOK_SEMICOLON TOK_COLON TOK_COMMA TOK_ARRAY TOK_RANGE TOK_DOT
%token TOK_ADD TOK_MINUS TOK_MUL TOK_DIV TOK_MOD TOK_NOT TOK_AND TOK_OR TOK_XOR TOK_DATE TOK_TIME_OF_DAY TOK_DATE_TIME TOK_ABS_ADDR_PERIPHERAL TOK_ABS_ADDR_BIT TOK_ABS_ADDR_BLOCK

%token TOK_EQUAL TOK_POWER TOK_LBRACKET TOK_RBRACKET TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_LE TOK_GT TOK_NEQU TOK_NLE TOK_NGT
%token TOK_EOF TOK_NAMESPACE TOK_END_NAMESPACE TOK_USING
               
%token TOK_FUNCTION_BLOCK TOK_FUNCTION TOK_END_FUNCTION_BLOCK TOK_END_FUNCTION TOK_END_PROGRAM TOK_INCLUDE TOK_INTERFACE TOK_END_INTERFACE
%token TOK_VAR_INPUT TOK_VAR_OUTPUT TOK_VAR_IN_OUT TOK_VAR_GLOBAL TOK_CONSTANT TOK_PUNC TOK_VAR_TEMP TOK_AT TOK_VAR_EXTERNAL TOK_RETAIN TOK_PERSISTENT TOK_VAR_CONFIG TOK_CARET TOK_POINTER TOK_VAR_STAT

%token TOK_TIME_TYPE TOK_TIME_OF_DAY_TYPE TOK_DATE_TYPE TOK_STRING_TYPE TOK_WSTRING_TYPE TOK_COMMENT TOK_BY TOK_CASE TOK_END_CASE TOK_OF TOK_ABSTRACT TOK_FINAL TOK_EXTENDS TOK_IMPLEMENTS TOK_SUPER TOK_THIS TOK_PRIVATE TOK_PUBLIC TOK_INTERNAL TOK_PROTECTED TOK_REFERENCE TOK_REF_TO TOK_METHOD TOK_END_METHOD TOK_ATTRIBUTE TOK_INFO TOK_REGION TOK_END_REGION TOK_RPCF_ADDR TOK_OUTPUT_ASSIGN
// virtual tokens
%token TOK_VSTART_MAIN TOK_VSTART_PRAGMA TOK_VCASE_SEP TOK_VPUNC TOK_VSEMICOLON TOK_VSUB TOK_VSTART_CASESEL

 /*%glr-parser*/

%start start_point
%parse-param { rpcf::CSTParserContext *pCtx }

%define parse.error verbose
%define parse.lac full

%left TOK_OR
%left TOK_XOR
%left TOK_AND
%left TOK_EQUAL TOK_NEQU TOK_LT TOK_LE TOK_GT TOK_NLE TOK_NGT
%left TOK_ADD TOK_VSUB
%left TOK_MUL TOK_DIV TOK_MOD
%right TOK_NOT

%%

start_point:
    TOK_VSTART_MAIN source_file
    {
        // Build the root AST node
        ObjPtr pSrcNode = ToObjPtrVal( $2 );
        CStRootNode* pSrcRoot = pSrcNode;
        if( pSrcRoot != nullptr )
        {
            ObjPtr pRoot;
            pRoot.NewObj( clsid( CStRootNode ) );
            CStRootNode* pDestRoot = pRoot;
            pDestRoot->m_vecChildren = pSrcRoot->m_vecChildren;
            pDestRoot->SetLocation( LOC_RANGE($1, $2) );
            g_pAstRoot = pRoot;
        }
        $$ = $1;
    }
    | conditional_pragma
    | TOK_EOF
    | case_selector_check
    ;

source_file:
    /* empty */
    {
        ObjPtr pNode = CreateAstNode< CStRootNode >();
        $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
    }
    | namespace_elements
    {
        ObjPtr pNode = CreateAstNode< CStRootNode >();
        CStRootNode* pRoot = pNode;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            // Get declarations from namespace_elements
            ObjPtr pNamespaceContent = ToObjPtrVal( $1 );
            CStRootNode* pNsContent = pNamespaceContent;
            if( pNsContent != nullptr )
            {
                // Create a virtual global scope node with empty name
                // This node serves as the starting point for '::abc' resolution
                ObjPtr pGlobalScope = GetAstFactory( pCtx )->CreateGlobalScope(
                    pNsContent->m_vecChildren, LOC($1) );
                
                // Attach global scope to root
                pRoot->m_vecDeclarations.push_back( pGlobalScope );
                CSTAstNodeBase* pScope = pGlobalScope;
                if( pScope != nullptr )
                    pScope->SetParent( pRoot );
            }
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

namespace_name : TOK_ID 
    { $$=$1; }
    ;

namespace_elements : namespace_element
      {
        ObjPtr pNode = CreateAstNode< CStRootNode >();
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            CStRootNode* pRoot = pNode;
            pRoot->m_vecChildren.push_back( ToObjPtrVal( $1 ) );
        }
        else
        {
            pCtx->IncError();
            stdstr strCurFile = basename(
                pCtx->GetCurFileName().c_str() );
            ParserPrint( strCurFile.c_str(),
                @1.last_line,
                "invalid namespace element", true );
            yyerrok;
            break;
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }

    | namespace_elements namespace_element
      {
        ObjPtr pNode = CreateAstNode< CStRootNode >();
        CStRootNode* pRoot = pNode;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            CStRootNode* pRootL = (ObjPtr&)$1->first;
            if( pRootL != nullptr )
                pRoot->m_vecChildren = pRootL->m_vecChildren;
        }
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            pRoot->m_vecChildren.push_back( ( ObjPtr& )$2->first );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
      }
    ;

namespace_element :
    pou_declaration
    | type_definition_block
    | var_config_declaration
    | pragma_statement
    | namespace_declaration
    | global_var
    | using_directive
    ;

namespace_declaration
    : TOK_NAMESPACE namespace_name namespace_elements TOK_END_NAMESPACE
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< ObjPtr > vecDecls;
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStRootNode* pRoot = ToObjPtrVal( $3 );
              if( pRoot != nullptr )
                  vecDecls = pRoot->m_vecChildren;
          }

          ObjPtr pNamespace = pFactory->CreateNamespaceDecl(
              ID($2), vecDecls, LOC_RANGE($1, $4) );

          $$ = MAKE_VALUE( Variant( pNamespace ), LOC_RANGE($1, $4) );
      }
    ;

global_var:
    TOK_VAR_GLOBAL opt_qualifier var_list TOK_END_VAR 
    ;

pou_declaration:
    program
    | function_block
    | function
    ;

program:
    TOK_PROGRAM TOK_ID program_unit TOK_END_PROGRAM
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($2);

          // Parse program_unit to extract components
          std::vector< ObjPtr > vecStatements;
          std::vector< ObjPtr > vecInput, vecOutput, vecInOut;
          std::vector< ObjPtr > vecLocal, vecTemp;
          std::vector< std::string > vecUsing;

          /* The statement list comes from the program unit body;
             the var/using extraction is still pending */
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              ObjPtr pList = ToObjPtrVal( $3 );
              CStStmtListNode* pStmtList = pList;
              if( pStmtList != nullptr )
                  vecStatements = pStmtList->m_vecStatements;
          }
          // TODO: Parse program_unit properly to extract var groups

          ObjPtr pProgram = pFactory->CreateProgramDecl(
              strName, vecInput, vecOutput, vecInOut,
              vecLocal, vecTemp, vecStatements, vecUsing,
              LOC_RANGE($1, $4) );

          $$ = MAKE_VALUE( Variant( pProgram ), LOC_RANGE($1, $4) );
      }
    ;

program_unit:
    using_directive_list var_declarations body
      {
          /* Propagate the body statement list; var extraction pending */
          $$ = $3;
      }
    | var_declarations body
      {
          $$ = $2;
      }

body:
    /* empty */
      {
          /* Empty body - provide an empty accumulator */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pNode = pFactory->CreateStmtListNode( YYLTYPE2() );
          $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
      }
    | block_statements
      {
          $$ = $1;
      }

type_definition_block:
      TOK_TYPE type_assignments TOK_END_TYPE
      {
          /* Wrap all type assignments in a TypeDefinitionBlock node */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pBlock = pFactory->CreateTypeDefinitionBlockNode( LOC_RANGE($1, $3) );
          CStTypeDefinitionBlockNode* pBlockNode = pBlock;
          if( pBlockNode != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              ObjPtr pDecls = ToObjPtrVal( $2 );
              CStTypeDefinitionBlockNode* pDeclsBlock = pDecls;
              if( pDeclsBlock != nullptr )
              {
                  pBlockNode->m_vecTypeDecls =
                      pDeclsBlock->m_vecTypeDecls;
                  for( size_t i = 0;
                      i < pBlockNode->m_vecTypeDecls.size(); i++ )
                  {
                      /* the accumulated block is discarded, so the
                         declarations must point to this final container */
                      CSTAstNodeBase* pDecl =
                          pBlockNode->m_vecTypeDecls[ i ];
                      if( pDecl != nullptr )
                          pDecl->SetParent( pBlockNode );
                  }
              }
          }
          $$ = MAKE_VALUE( Variant( pBlock ), LOC_RANGE($1, $3) );
      }
    ;

type_assignments:
      type_assignment
      {
          /* First type assignment - create block container */
          ObjPtr pBlock;
          pBlock.NewObj( clsid( CStTypeDefinitionBlockNode ) );
          CStTypeDefinitionBlockNode* pBlockNode = pBlock;
          if( pBlockNode != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pBlockNode->m_vecTypeDecls.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pBlock ), LOC($1) );
      }
    | type_assignments type_assignment
      {
          /* Accumulate type assignments */
          ObjPtr pBlock;
          pBlock.NewObj( clsid( CStTypeDefinitionBlockNode ) );
          CStTypeDefinitionBlockNode* pBlockNode = pBlock;
          if( pBlockNode != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStTypeDefinitionBlockNode* pPrevBlock = pPrev;
              if( pPrevBlock != nullptr )
                  pBlockNode->m_vecTypeDecls = pPrevBlock->m_vecTypeDecls;
          }
          if( pBlockNode != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
              pBlockNode->m_vecTypeDecls.push_back( ToObjPtrVal( $2 ) );
          $$ = MAKE_VALUE( Variant( pBlock ), LOC_RANGE($1, $2) );
      }
    /* error recovery for type definitions */
    | type_assignments error
      {
          pCtx->IncError();
          stdstr strCurFile = basename(
              pCtx->GetCurFileName().c_str() );
          ParserPrint( strCurFile.c_str(),
              @2.last_line,
              "invalid type definition, skipping", true );
          yyerrok;
          /* Return the previous block unchanged */
          $$ = $1;
      }
    ;

enum_value_list:
      enum_value
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStEnumValueListNode ) );
          CStEnumValueListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pList->m_vecValues.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | enum_value_list TOK_COMMA enum_value
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStEnumValueListNode ) );
          CStEnumValueListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStEnumValueListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pList->m_vecValues = pPrevList->m_vecValues;
          }
          if( pList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
              pList->m_vecValues.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    /* error recovery for enum values */
    | enum_value_list error enum_value
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @2.last_line,
              "invalid enum value, skipping", true );
          yyerrok;
      }
    | enum_value_list TOK_COMMA error
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @3.last_line,
              "invalid enum value, skipping", true );
          yyerrok;
      }
    ;

enum_value:
      TOK_ID
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStEnumValueNode ) );
          CStEnumValueNode* pVal = pNode;
          if( pVal != nullptr )
          {
              pVal->m_strName = ID($1);
              // m_pExplicitValue remains empty = auto-assign
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | TOK_ID TOK_ASSIGN full_expression
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStEnumValueNode ) );
          CStEnumValueNode* pVal = pNode;
          if( pVal != nullptr )
          {
              pVal->m_strName = ID($1);
              if( $3 != nullptr && IsObjPtrVal( $3 ) )
              {
                  pVal->m_pExplicitValue = ToObjPtrVal( $3 );
                  CSTAstNodeBase* pChild = pVal->m_pExplicitValue;
                  if( pChild != nullptr )
                      pChild->SetParent( pVal );
              }
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

opt_base_type:
    /* empty */ 
    | int_type

 /* the default initialization for variables of this enum */
opt_assign_enum_val:
    /* empty */ 
    | TOK_ID /* one of the enum value */
    | TOK_ID TOK_PUNC TOK_ID /* enum_type_name#member */

enum_type_head:
    TOK_ID TOK_COLON TOK_LPAREN enum_value_list TOK_RPAREN
    {
        // Store the enum type name in the enum_value_list node
        ObjPtr pList = ToObjPtrVal( $4 );
        CStEnumValueListNode* pValList = pList;
        if( pValList != nullptr )
        {
            pValList->m_strTypeName = ID($1);
        }
        $$ = $4;
    }

enum_type_definition:
      enum_type_head TOK_ASSIGN opt_base_type opt_assign_enum_val semicolons
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strTypeName = "";
          std::vector< ObjPtr > vecValues;

          // Extract from enum_value_list in $1 (CStEnumValueListNode)
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pList = ToObjPtrVal( $1 );
              CStEnumValueListNode* pValList = pList;
              if( pValList != nullptr )
              {
                  strTypeName = pValList->m_strTypeName;
                  vecValues = pValList->m_vecValues;
              }
          }

          ObjPtr pBaseType = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          std::string strDefaultInit = STR($4);

          ObjPtr pNode = pFactory->CreateEnumTypeNode(
              strTypeName, vecValues, pBaseType, strDefaultInit,
              LOC_RANGE($1, $5) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }

type_assignment:
      TOK_ID TOK_COLON type_spec semicolons {
        /* Alias: TYPE MyInt : INT; */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($1);
        ObjPtr pTypeDef = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
            ToObjPtrVal( $3 ) : nullptr;
        ObjPtr pTypeDecl = pFactory->CreateTypeDecl(
            strName, pTypeDef, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pTypeDecl ), LOC_RANGE($1, $4) );
      }

    | TOK_ID TOK_COLON struct_definition semicolons   {
        /* Struct: TYPE Motor : STRUCT... */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($1);
        ObjPtr pStructType = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
            ToObjPtrVal( $3 ) : nullptr;
        ObjPtr pTypeDecl = pFactory->CreateTypeDecl(
            strName, pStructType, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pTypeDecl ), LOC_RANGE($1, $4) );
    }
    | enum_type_definition
    {
        $$ = $1;
    }
    ;

struct_definition:
      TOK_STRUCT member_list TOK_END_STRUCT
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strTypeName = "";
          std::vector< CStStructTypeNode::CStructMember > vecMembers;
          if( $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              CStStructTypeNode* pStruct = ToObjPtrVal( $2 );
              if( pStruct != nullptr )
              {
                  strTypeName = pStruct->m_strTypeName;
                  vecMembers = pStruct->m_vecMembers;
              }
          }
          ObjPtr pNode = pFactory->CreateStructTypeNode(
              strTypeName, vecMembers, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

member_list:
      member_declaration semicolons
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< CStStructTypeNode::CStructMember > vecMembers;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStStructTypeNode::CStructMember member;
              member.m_pType = ToObjPtrVal( $1 );
              vecMembers.push_back(member);
          }
          ObjPtr pNode = pFactory->CreateStructTypeNode(
              "", vecMembers, LOC($1) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | member_list member_declaration semicolons
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< CStStructTypeNode::CStructMember > vecMembers;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStStructTypeNode* pStruct = ToObjPtrVal( $1 );
              if( pStruct != nullptr )
                  vecMembers = pStruct->m_vecMembers;
          }
          if( $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              CStStructTypeNode::CStructMember member;
              member.m_pType = ToObjPtrVal( $2 );
              vecMembers.push_back(member);
          }
          ObjPtr pNode = pFactory->CreateStructTypeNode(
              "", vecMembers, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

member_declaration:
      TOK_ID TOK_COLON type_spec
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($1);
          ObjPtr pType = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          // Create a struct member node
          CStStructTypeNode::CStructMember member;
          member.m_strName = strName;
          member.m_pType = pType;
          ObjPtr pNode = pFactory->CreateStructTypeNode(
              "", {member}, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

var_decl_type:
    TOK_VAR
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcLocal, LOC($1) );
      }
    | TOK_VAR_TEMP
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcTemp, LOC($1) );
      }
    | TOK_VAR_INPUT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcInput, LOC($1) );
      }
    | TOK_VAR_OUTPUT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcOutput, LOC($1) );
      }
    | TOK_VAR_IN_OUT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcInOut, LOC($1) );
      }
    | TOK_VAR_STAT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcStat, LOC($1) );
      }
    | TOK_VAR_EXTERNAL
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vcExternal, LOC($1) );
      }
    ;

var_declarations:
    /* empty */
      {
          /* no VAR block: an empty accumulator, split by the POU
             rules into the var vectors */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStVarDeclListNode ) );
          $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
      }
    | var_declarations var_decl_type declaration TOK_END_VAR
      {
          /* Rebuild-and-copy: the declarations of the previous
             blocks plus this one; the block keyword decides the
             category of every declaration it contains, e.g.
             VAR_INPUT ... END_VAR marks them as inputs */
          CStVarDeclNode::enumVarCategory eCategory =
              ( CStVarDeclNode::enumVarCategory )NUM( $2 );
          ObjPtr pNode;
          pNode.NewObj( clsid( CStVarDeclListNode ) );
          CStVarDeclListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStVarDeclListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pList->m_vecVarDecls = pPrevList->m_vecVarDecls;
          }
          if( pList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              ObjPtr pBlock = ToObjPtrVal( $3 );
              CStVarDeclListNode* pBlockList = pBlock;
              if( pBlockList != nullptr )
              {
                  for( guint32 i = 0;
                      i < pBlockList->m_vecVarDecls.size(); ++i )
                  {
                      CStVarDeclNode* pVar =
                          pBlockList->m_vecVarDecls[ i ];
                      if( pVar != nullptr )
                          pVar->m_eCategory = eCategory;
                      pList->m_vecVarDecls.push_back(
                          pBlockList->m_vecVarDecls[ i ] );
                  }
              }
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
      }
    ;

opt_qualifier:
    /* empty */
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqNone, YYLTYPE2() );
      }
    | TOK_RETAIN
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqRetain, LOC($1) );
      }
    | TOK_PERSISTENT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqPersistent, LOC($1) );
      }
    | TOK_RETAIN TOK_PERSISTENT
      {
          /* PERSISTENT implies RETAIN */
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqPersistent,
              LOC_RANGE($1, $2) );
      }
    | TOK_PERSISTENT TOK_RETAIN
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqPersistent,
              LOC_RANGE($1, $2) );
      }
    | TOK_CONSTANT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqConst, LOC($1) );
      }
    ;

declaration:
    opt_qualifier var_list semicolons
      {
          /* Apply the block-level qualifier, e.g. the RETAIN of
             'VAR RETAIN ... END_VAR', to every declaration of the
             block */
          ObjPtr pList = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
              ToObjPtrVal( $2 ) : nullptr;
          CStVarDeclListNode* pNode = pList;
          if( pNode != nullptr )
          {
              CStVarDeclNode::enumVarQualifier eQualifier =
                  ( CStVarDeclNode::enumVarQualifier )NUM( $1 );
              for( guint32 i = 0;
                  i < pNode->m_vecVarDecls.size(); ++i )
              {
                  CStVarDeclNode* pVar = pNode->m_vecVarDecls[ i ];
                  if( pVar == nullptr )
                      continue;
                  pVar->m_eQualifier = eQualifier;
                  if( eQualifier == CStVarDeclNode::vqConst )
                      pVar->m_bConstant = true;
              }
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }
    ;

var_list:
    var_declaration
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStVarDeclListNode ) );
          CStVarDeclListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pList->m_vecVarDecls.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | var_list semicolons  var_declaration
      {
          /* rebuild-and-copy: the previous declarations plus the
             new one */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStVarDeclListNode ) );
          CStVarDeclListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStVarDeclListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pList->m_vecVarDecls = pPrevList->m_vecVarDecls;
          }
          if( pList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
              pList->m_vecVarDecls.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    /* error recovery for variable declarations */
    | var_list semicolons error
      {
          pCtx->IncError();
          stdstr strCurFile = basename(
              pCtx->GetCurFileName().c_str() );
          ParserPrint( strCurFile.c_str(),
              @3.last_line,
              "invalid variable declaration, skipping", true );
          yyerrok;
          /* Keep the declarations accumulated so far */
          $$ = $1;
      }
    ;

initial_value:
      full_expression                    /* Simple: int := 10; */
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStInitialValueNode ) );
          CStInitialValueNode* pInit = pNode;
          if( pInit != nullptr )
          {
              pInit->m_eInitType = CStInitialValueNode::initExpression;
              if( $1 != nullptr && IsObjPtrVal( $1 ) )
              {
                  pInit->m_pValue = ToObjPtrVal( $1 );
                  CSTAstNodeBase* pChild = pInit->m_pValue;
                  if( pChild != nullptr )
                      pChild->SetParent( pInit );
              }
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | TOK_LBRACKET array_init_list TOK_RBRACKET             /* Array:int := [1, 2, 3]; */
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStInitialValueNode ) );
          CStInitialValueNode* pInit = pNode;
          if( pInit != nullptr )
          {
              pInit->m_eInitType = CStInitialValueNode::initArray;
              if( $2 != nullptr && IsObjPtrVal( $2 ) )
              {
                  pInit->m_pValue = ToObjPtrVal( $2 );
                  CSTAstNodeBase* pChild = pInit->m_pValue;
                  if( pChild != nullptr )
                      pChild->SetParent( pInit );
              }
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    | TOK_LPAREN struct_init_list TOK_RPAREN      /* Struct: := (Speed := 10, Run := TRUE); */
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStInitialValueNode ) );
          CStInitialValueNode* pInit = pNode;
          if( pInit != nullptr )
          {
              pInit->m_eInitType = CStInitialValueNode::initStruct;
              if( $2 != nullptr && IsObjPtrVal( $2 ) )
              {
                  pInit->m_pValue = ToObjPtrVal( $2 );
                  CSTAstNodeBase* pChild = pInit->m_pValue;
                  if( pChild != nullptr )
                      pChild->SetParent( pInit );
              }
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

array_elem_init:
      TOK_NUMBER TOK_LPAREN initial_value TOK_RPAREN
      {
          /* Keep the repetition unexpanded, e.g. 3(5): expanding
             '10000(5)' at parse time would create 10000 AST
             entries; the semantic phase expands the node when
             materializing the initial values */
          gint32 iCount = NUM($1);
          ObjPtr pElement = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pNode = pFactory->CreateArrayRepeatNode(
              iCount, pElement, LOC_RANGE($1, $4) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
      }
    | TOK_NUMBER TOK_LPAREN array_elem_init TOK_RPAREN
      {
          /* Flatten nested repetitions at parse time, e.g.
             3(5("h")) becomes 15("h"): the repetition count is
             multiplicative and neither the C++ nor the wasm
             translator cares about the nesting, so the parser
             multiplies the counts instead of growing the node
             tree. Expanding to individual entries is still
             postponed to the semantic phase, e.g. '10000(5)'
             remains a single node */
          gint32 iCount = NUM($1);
          ObjPtr pInnerNode = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          CStArrayRepeatNode* pInner = pInnerNode;
          gint32 iTotal = iCount;
          ObjPtr pElement = pInnerNode;
          if( pInner != nullptr )
          {
              gint64 llCount = ( gint64 )iCount * pInner->m_iCount;
              if( llCount <= INT32_MAX )
              {
                  iTotal = ( gint32 )llCount;
                  pElement = pInner->m_pElement;
              }
              else
              {
                  pCtx->IncError();
                  ParserPrint(
                      basename( pCtx->GetCurFileName().c_str() ),
                      @1.last_line,
                      "repeated element count exceeds the limit",
                      true );
                  /* keep the nested node so the count is not
                     silently wrapped around */
              }
          }
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pNode = pFactory->CreateArrayRepeatNode(
              iTotal, pElement, LOC_RANGE($1, $4) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
      }
    ;

array_init_list:
      initial_value
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStArrayInitNode ) );
          CStArrayInitNode* pInit = pNode;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
              pInit->m_vecValues.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | array_init_list TOK_COMMA initial_value
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStArrayInitNode ) );
          CStArrayInitNode* pInit = pNode;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStArrayInitNode* pPrevInit = pPrev;
              if( pPrevInit != nullptr )
                  pInit->m_vecValues = pPrevInit->m_vecValues;
          }
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
              pInit->m_vecValues.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    /* ST also supports 'n(value)' for repeating array elements */
    | array_init_list TOK_COMMA array_elem_init
      {
          /* Keep the repetition unexpanded: expanding '10000(5)'
             here would create 10000 AST entries; the semantic
             phase expands it when materializing the values */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStArrayInitNode ) );
          CStArrayInitNode* pInit = pNode;
          if( pInit != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStArrayInitNode* pPrevInit = pPrev;
              if( pPrevInit != nullptr )
                  pInit->m_vecValues = pPrevInit->m_vecValues;
          }
          if( pInit != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
              pInit->m_vecValues.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    /* error recovery for array initialization */
    | array_init_list TOK_COMMA error
    {
        pCtx->IncError();
        ParserPrint( basename(pCtx->GetCurFileName().c_str()),
            @3.last_line,
            "invalid array element, skipping", true );
        yyerrok;
    }
    ;

struct_init_list:
      TOK_ID TOK_ASSIGN initial_value
      {
          // Store as a map-like structure in Variant
          // For now, use a simplified representation
          std::string strMember = ID($1);
          ObjPtr pValue = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          // Use CStStructInitNode to store member initializations
          ObjPtr pNode;
          pNode.NewObj( clsid( CStStructInitNode ) );
          CStStructInitNode* pInit = pNode;
          pInit->m_vecMembers.push_back( strMember );
          pInit->m_vecValues.push_back( pValue );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    | struct_init_list TOK_COMMA TOK_ID TOK_ASSIGN initial_value
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStStructInitNode ) );
          CStStructInitNode* pInit = pNode;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStStructInitNode* pPrevInit = pPrev;
              if( pPrevInit != nullptr )
              {
                  pInit->m_vecMembers = pPrevInit->m_vecMembers;
                  pInit->m_vecValues = pPrevInit->m_vecValues;
              }
          }
          std::string strMember = ID($3);
          ObjPtr pValue = ( $5 != nullptr && IsObjPtrVal( $5 ) ) ?
              ToObjPtrVal( $5 ) : nullptr;
          pInit->m_vecMembers.push_back( strMember );
          pInit->m_vecValues.push_back( pValue );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }
    /* error recovery for struct initialization */
    | struct_init_list TOK_COMMA error
    {
        pCtx->IncError();
        ParserPrint( basename(pCtx->GetCurFileName().c_str()),
            @3.last_line,
            "invalid struct member initializer, skipping", true );
        yyerrok;
    }
    ;

var_declaration:
      identifier_list TOK_COLON type_spec
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< std::string > vecNames;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pIdList = ToObjPtrVal( $1 );
              CStIdentifierListNode* pIds = pIdList;
              if( pIds != nullptr )
                  vecNames = pIds->m_vecIdentifiers;
          }
          ObjPtr pType = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          ObjPtr pNode = pFactory->CreateVarDeclNode(
              vecNames, pType, CStVarDeclNode::vcLocal,
              CStVarDeclNode::vqNone, nullptr, "", false, nullptr,
              LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    | identifier_list TOK_COLON type_spec TOK_ASSIGN initial_value
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< std::string > vecNames;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pIdList = ToObjPtrVal( $1 );
              CStIdentifierListNode* pIds = pIdList;
              if( pIds != nullptr )
                  vecNames = pIds->m_vecIdentifiers;
          }
          ObjPtr pType = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          ObjPtr pInit = ( $5 != nullptr && IsObjPtrVal( $5 ) ) ?
              ToObjPtrVal( $5 ) : nullptr;
          ObjPtr pNode = pFactory->CreateVarDeclNode(
              vecNames, pType, CStVarDeclNode::vcLocal,
              CStVarDeclNode::vqNone, pInit, "", false, nullptr,
              LOC_RANGE($1, $5) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }
    | TOK_ID TOK_AT direct_address
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($1);
          std::string strAddr = "";
          ObjPtr pDirectAddr = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          if( !pDirectAddr.IsEmpty() )
          {
              /* Keep the wrapper node as a marker for the
                 translator, which emits different code for direct
                 addresses; the text stays for display */
              CStDirectAddressNode* pDirect = pDirectAddr;
              if( pDirect != nullptr )
                  strAddr = pDirect->m_strAddress;
          }
          ObjPtr pNode = pFactory->CreateVarDeclNode(
              std::vector<std::string>{strName}, nullptr, CStVarDeclNode::vcLocal,
              CStVarDeclNode::vqNone, nullptr, strAddr, true,
              pDirectAddr, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    | TOK_ID TOK_AT direct_address TOK_ASSIGN initial_value
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($1);
          std::string strAddr = "";
          ObjPtr pDirectAddr = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;
          if( !pDirectAddr.IsEmpty() )
          {
              /* Keep the wrapper node as a marker for the
                 translator, which emits different code for direct
                 addresses; the text stays for display */
              CStDirectAddressNode* pDirect = pDirectAddr;
              if( pDirect != nullptr )
                  strAddr = pDirect->m_strAddress;
          }
          ObjPtr pInit = ( $5 != nullptr && IsObjPtrVal( $5 ) ) ?
              ToObjPtrVal( $5 ) : nullptr;
          ObjPtr pNode = pFactory->CreateVarDeclNode(
              std::vector<std::string>{strName}, nullptr, CStVarDeclNode::vcLocal,
              CStVarDeclNode::vqNone, pInit, strAddr, true,
              pDirectAddr, LOC_RANGE($1, $5) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }
    ;


direct_address:
    TOK_RPCF_ADDR
      {
          /* rpcf direct address, e.g. @IBx.I0:value; the lexer
             already parsed it into a StrVecPtr */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strAddr = ( $1 != nullptr ) ?
              $1->second.text : "";
          ObjPtr pParsed = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pAddr = pFactory->CreateDirectAddressNode(
              strAddr, CStDirectAddressNode::datRpcf,
              pParsed, nullptr, LOC($1) );
          $$ = MAKE_VALUE( Variant( pAddr ), LOC($1) );
      }
    | TOK_ABS_ADDR_PERIPHERAL
      {
          /* Absolute peripheral address, e.g. %IW0.1, %Q*; the
             lexer already parsed it into an IntVecPtr */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strAddr = ( $1 != nullptr ) ?
              $1->second.text : "";
          ObjPtr pParsed = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pAddr = pFactory->CreateDirectAddressNode(
              strAddr, CStDirectAddressNode::datPeripheral,
              pParsed, nullptr, LOC($1) );
          $$ = MAKE_VALUE( Variant( pAddr ), LOC($1) );
      }
    | TOK_ABS_ADDR_PERIPHERAL TOK_LBRACKET full_expression TOK_RBRACKET
      {
          /* Absolute peripheral address with index, e.g. %IW[expr];
             the index is kept in the node for the translator */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strAddr = ( $1 != nullptr ) ?
              $1->second.text : "";
          ObjPtr pParsed = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pIndex = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              UnwrapFullExpression( ToObjPtrVal( $3 ) ) : nullptr;
          ObjPtr pAddr = pFactory->CreateDirectAddressNode(
              strAddr, CStDirectAddressNode::datPeripheralOffset,
              pParsed, pIndex, LOC_RANGE($1, $4) );
          $$ = MAKE_VALUE( Variant( pAddr ), LOC_RANGE($1, $4) );
      }
    ;

identifier_list:
      TOK_ID
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          CStIdentifierListNode* pIdList = pNode;
          if( pIdList != nullptr )
              pIdList->m_vecIdentifiers.push_back( ID($1) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | identifier_list TOK_COMMA TOK_ID
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          CStIdentifierListNode* pIdList = pNode;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStIdentifierListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pIdList->m_vecIdentifiers = pPrevList->m_vecIdentifiers;
          }
          pIdList->m_vecIdentifiers.push_back( ID($3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

int_type:
    TOK_INT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_BOOL
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btBool, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_WORD
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btWord, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_UINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_DINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btDInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_UDINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUDInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_SINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btSInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_USINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUSInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_BYTE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btByte, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_DWORD
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btDWord, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_ULINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btULInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_LINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btLInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_LWORD
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btLWord, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }

time_type:
    TOK_TIME_TYPE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btTime, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_TIME_OF_DAY_TYPE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btTimeOfDay, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_DATE_TYPE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btDate, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }


array_type:
    TOK_ARRAY TOK_LBRACKET range_list TOK_RBRACKET TOK_OF type_spec
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pElementType = nullptr;

        // Extract element type from type_spec
        if( $6 != nullptr && IsObjPtrVal( $6 ) )
        {
            ObjPtr pTypeSpec = ToObjPtrVal( $6 );
            CStDataTypeSpecNode* pSpecNode = pTypeSpec;
            if( pSpecNode != nullptr )
                pElementType = pSpecNode->m_pTypeSpec;
        }

        /* Flatten the subrange list into array dimensions */
        std::vector< CStArrayTypeNode::CArrayDim > vecDims;
        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            ObjPtr pList = ToObjPtrVal( $3 );
            CStSubrangeListNode* pRangeList = pList;
            if( pRangeList != nullptr )
            {
                for( size_t i = 0;
                    i < pRangeList->m_vecRanges.size(); i++ )
                {
                    CStSubrangeNode* pRange = pRangeList->m_vecRanges[ i ];
                    if( pRange == nullptr )
                        continue;
                    /* Keep the bound expressions; the numeric bounds
                       are evaluated later by the semantic phase */
                    CStArrayTypeNode::CArrayDim dim;
                    dim.m_pStart = pRange->m_pStart;
                    dim.m_pEnd = pRange->m_pEnd;
                    vecDims.push_back( dim );
                }
            }
        }

        ObjPtr pNode = pFactory->CreateArrayTypeNode(
            pElementType, vecDims, LOC_RANGE($1, $6) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }
    ;

range_list:
      range
      {
          /* First dimension - create the accumulator */
          ObjPtr pList;
          pList.NewObj( clsid( CStSubrangeListNode ) );
          CStSubrangeListNode* pRangeList = pList;
          if( pRangeList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pRangeList->m_vecRanges.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
      }
    | range_list TOK_COMMA range  /* Supports multi-dimensional arrays */
      {
          /* Accumulate dimensions */
          ObjPtr pList;
          pList.NewObj( clsid( CStSubrangeListNode ) );
          CStSubrangeListNode* pRangeList = pList;
          if( pRangeList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStSubrangeListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pRangeList->m_vecRanges = pPrevList->m_vecRanges;
          }
          if( pRangeList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
              pRangeList->m_vecRanges.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }
    ;

range:
    full_expression TOK_RANGE full_expression    /* e.g., 1..10 */
    {
        /* Per the spec the bounds are constant expressions. They may
           reference named constants or enum values and involve
           arithmetic, which requires the variable tables of the
           semantic phase, so the parser keeps the bound expressions
           as-is and performs no numeric evaluation. */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);

        ObjPtr pNode = pFactory->CreateSubrangeNode(
            $1->first, $3->first, LOC_RANGE($1, $3) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
    }
    ;

string_type:
    TOK_STRING_TYPE TOK_LPAREN full_expression TOK_RPAREN
    {  /* Specific length: the length is a constant expression, kept
          as parsed; the numeric length is evaluated by the semantic
          phase */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btString, 0, LOC_RANGE($1, $4) );
        CStBasicTypeNode* pType = pNode;
        if( pType != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            pType->m_pStringLength = UnwrapFullExpression(
                ToObjPtrVal( $3 ) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | TOK_STRING_TYPE TOK_LBRACKET full_expression TOK_RBRACKET
    {  /* Specific length: constant expression, see above */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btString, 0, LOC_RANGE($1, $4) );
        CStBasicTypeNode* pType = pNode;
        if( pType != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            pType->m_pStringLength = UnwrapFullExpression(
                ToObjPtrVal( $3 ) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | TOK_STRING_TYPE
    {  /* Default length is 80 */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btString, 80, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_WSTRING_TYPE TOK_LPAREN full_expression TOK_RPAREN
    {  /* Wide string with specific length: constant expression,
          see above */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btWString, 0, LOC_RANGE($1, $4) );
        CStBasicTypeNode* pType = pNode;
        if( pType != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            pType->m_pStringLength = UnwrapFullExpression(
                ToObjPtrVal( $3 ) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | TOK_WSTRING_TYPE TOK_LBRACKET full_expression TOK_RBRACKET
    {  /* Wide string with specific length: constant expression,
          see above */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btWString, 0, LOC_RANGE($1, $4) );
        CStBasicTypeNode* pType = pNode;
        if( pType != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            pType->m_pStringLength = UnwrapFullExpression(
                ToObjPtrVal( $3 ) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | TOK_WSTRING_TYPE
    {  /* Wide string (UTF-16), default length 0 */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btWString, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

pointer_type:
    TOK_CARET type_spec
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTargetType = nullptr;

        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            ObjPtr pTypeSpec = ToObjPtrVal( $2 );
            CStDataTypeSpecNode* pSpecNode = pTypeSpec;
            if( pSpecNode != nullptr )
                pTargetType = pSpecNode->m_pTypeSpec;
        }

        ObjPtr pNode = pFactory->CreatePointerTypeNode(
            pTargetType, LOC_RANGE($1, $2) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    | TOK_POINTER TOK_TO type_spec
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTargetType = nullptr;

        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            ObjPtr pTypeSpec = ToObjPtrVal( $3 );
            CStDataTypeSpecNode* pSpecNode = pTypeSpec;
            if( pSpecNode != nullptr )
                pTargetType = pSpecNode->m_pTypeSpec;
        }

        ObjPtr pNode = pFactory->CreatePointerTypeNode(
            pTargetType, LOC_RANGE($1, $3) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
    }

reference_type:
    TOK_REFERENCE TOK_TO type_spec
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTargetType = nullptr;

        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            ObjPtr pTypeSpec = ToObjPtrVal( $3 );
            CStDataTypeSpecNode* pSpecNode = pTypeSpec;
            if( pSpecNode != nullptr )
                pTargetType = pSpecNode->m_pTypeSpec;
        }

        ObjPtr pNode = pFactory->CreateReferenceTypeNode(
            pTargetType, LOC_RANGE($1, $3) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
    }
    | TOK_REF_TO type_spec
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTargetType = nullptr;

        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            ObjPtr pTypeSpec = ToObjPtrVal( $2 );
            CStDataTypeSpecNode* pSpecNode = pTypeSpec;
            if( pSpecNode != nullptr )
                pTargetType = pSpecNode->m_pTypeSpec;
        }

        ObjPtr pNode = pFactory->CreateReferenceTypeNode(
            pTargetType, LOC_RANGE($1, $2) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    ;

other_elementry_type:
    time_type
    { $$ = $1; }
    | TOK_REAL
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btReal, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_LREAL
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btLReal, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;
    
data_type_spec:
    int_type
    {
        // Wrap int_type result in CStDataTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | int_type TOK_LPAREN range TOK_RPAREN
    {
        // TODO: Handle ranged integer types
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | other_elementry_type
    {
        // Wrap other_elementry_type result in CStDataTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | string_type
    {
        // Wrap string_type result in CStDataTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    /* implicit enum - anonymous enum type */
    | TOK_LPAREN enum_value_list TOK_RPAREN
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::vector< ObjPtr > vecValues;
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            ObjPtr pList = ToObjPtrVal( $2 );
            CStEnumValueListNode* pValList = pList;
            if( pValList != nullptr )
                vecValues = pValList->m_vecValues;
        }
        // Create anonymous enum type with empty name
        ObjPtr pEnumType = pFactory->CreateEnumTypeNode(
            "", vecValues, nullptr, "", LOC_RANGE($1, $3) );
        // Wrap in CStDataTypeSpecNode
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pEnumType, LOC_RANGE($1, $3) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
    }
    ;

type_spec:
    data_type_spec
    {
        // Wrap data_type_spec in CStTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pType = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateTypeSpecNode( pType, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | array_type
    {
        // Wrap array_type in CStTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pType = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateTypeSpecNode( pType, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | reference_type
    {
        // Wrap reference_type in CStTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pType = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateTypeSpecNode( pType, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | pointer_type
    {
        // Wrap pointer_type in CStTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pType = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateTypeSpecNode( pType, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | derived_type
    {
        // Wrap derived_type in CStTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pType = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateTypeSpecNode( pType, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

derived_type: instance_path
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        // Extract the qualified name from the instance path wrapper
        std::vector< std::string > vecQualifiedName;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            CStInstancePathNode* pPath = ToObjPtrVal( $1 );
            if( pPath != nullptr )
                vecQualifiedName = pPath->m_vecNameComponents;
        }
        if( vecQualifiedName.empty() )
            vecQualifiedName.push_back( "DerivedType" );  // Fallback on error recovery

        ObjPtr pNode = pFactory->CreateDerivedTypeNode(
            vecQualifiedName, false, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_DOT instance_path
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        // Global namespace type: components come from the path wrapper
        std::vector< std::string > vecQualifiedName;
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            CStInstancePathNode* pPath = ToObjPtrVal( $2 );
            if( pPath != nullptr )
                vecQualifiedName = pPath->m_vecNameComponents;
        }
        if( vecQualifiedName.empty() )
            vecQualifiedName.push_back( "GlobalType" );  // Fallback on error recovery

        ObjPtr pNode = pFactory->CreateDerivedTypeNode(
            vecQualifiedName, true, LOC_RANGE($1, $2) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    ;

/*statements:
    statement
    | statements semicolons statement
    ;
    */


statement:
    assignment_statement 
    | if_statement
    | for_statement
    | while_statement
    | repeat_statement
    | function_call_statement
    | case_statement
    ;

pragma_statement:
    TOK_LBRACE TOK_REGION TOK_STRING TOK_RBRACE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = STR($3);
        ObjPtr pNode = pFactory->CreatePragmaStmt(
            CStPragmaStmt::ptRegion, strName, "",
            nullptr, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | TOK_LBRACE TOK_END_REGION TOK_RBRACE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreatePragmaStmt(
            CStPragmaStmt::ptEndRegion, "", "",
            nullptr, LOC_RANGE($1, $3) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
    }

conditional_pragma:
    TOK_VSTART_PRAGMA TOK_IF full_expression TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_ELSIF full_expression TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_ELSE TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_END_IF TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_INFO TOK_STRING TOK_RBRACE {
        // Access string value from $3 which is YYSTYPE (shared_ptr<YYSPAIR>)
        stdstr strMsg = GetVariantVal( $3 );
        YYLTYPE2 oLoc;
        ParserPrint( pCtx->GetCurFileName().c_str(),
            oLoc.first_line, strMsg.c_str(), true );
    }
    | TOK_VSTART_PRAGMA TOK_INCLUDE TOK_STRING TOK_RBRACE {

        yyscan_t yyscanner = pCtx->GetScanner();
        YYLTYPE2 oLoc;
        std::string strFile = GetVariantVal( $3 );
        stdstr strCurFile = basename(
            pCtx->GetFileName(
                oLoc.fidx).c_str() );
        if( strFile.empty() )
        {
            if( strCurFile.size() )
                ParserPrint( 
                    strCurFile.c_str(),
                    oLoc.first_line,
                    "error, expecting file name",
                    true );
            pCtx->IncSemError();
            YYERROR;
        }

        stdstr strFullPath;
        FILE* pIncl = pCtx->TryOpenFile(
            strFile.c_str(), strFullPath );
        if ( !pIncl )
        {
            ParserPrint(
                strCurFile.c_str(),
                oLoc.first_line,
                strerror( errno ), true );
            pCtx->IncSemError();
            YYERROR;
        }
        if( pCtx->IsFileOnStack( strFullPath ) )
        {
            stdstr strMsg =
                "error, cyclic inclusion of files ";
            strMsg += strFile;
            ParserPrint(
                strCurFile.c_str(),
                oLoc.first_line,
                strMsg.c_str(), true );
            pCtx->IncSemError();
            YYERROR;
        }

        FILECTX2* pfc = new FILECTX2();
        pfc->m_strPath = pCtx->GetCurFileName();
        pfc->m_fp = yyget_in( yyscanner );
        ( ( YYLTYPE& ) pfc->m_oLocation ) =
            *yyget_lloc( pCtx->GetScanner() );
        pCtx->m_vecFileStack.push_back(
            std::unique_ptr< FILECTX2 >( pfc ) );
        yypush_buffer_state(
            yy_create_buffer( pIncl, YY_BUF_SIZE, yyscanner ),
            yyscanner );
        pCtx->GetCurFileName() = strFullPath;
        yyset_lineno( 1, yyscanner );
        yyset_column( 1, yyscanner );
    }
    | TOK_VSTART_PRAGMA TOK_ATTRIBUTE TOK_STRING opt_attr_values TOK_RBRACE {}
    ;

string_list:
    TOK_STRING
    | string_list TOK_COMMA TOK_STRING

opt_attr_values :
    /* empty */
    | TOK_ASSIGN string_list

    
/* Rule for Assignments: Only allows memory locations on the LHS */
assignment_statement:
      l_value_ext TOK_ASSIGN full_expression
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLVal = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRVal = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pAssign = pFactory->CreateAssignStmt(
              pLVal, pRVal, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pAssign ), LOC_RANGE($1, $3) );
      }
    ;

/* Rule for Standalone Calls: Used for functions/methods that return void or whose return is ignored */
function_call_statement:
      l_value TOK_LPAREN arg_list TOK_RPAREN
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pCallee = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;

          std::vector< ObjPtr > vecArgs;
          std::vector< CStCallExpr::CNamedArg > vecNamed;
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              ObjPtr pArgList = ToObjPtrVal( $3 );
              CStArgListNode* pArgListNode = pArgList;
              if( pArgListNode != nullptr )
              {
                  vecArgs = pArgListNode->m_vecArgs;
                  vecNamed = pArgListNode->m_vecNamed;
              }
          }

          ObjPtr pCall = pFactory->CreateCallExpr(
              pCallee, vecArgs, LOC_RANGE($1, $4), vecNamed );
          ObjPtr pNode = pFactory->CreateCallStmt(
              pCall, LOC_RANGE($1, $4) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
      }
    ;

/* L-Value rule: Strictly limited to writable memory locations */
l_value:
    l_value_var
      {
          /* Wrap standard l_value in CStLValueNode */
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pExpr = ToObjPtrVal( $1 );
              ObjPtr pLValue = pFactory->CreateLValueNode( pExpr, LOC($1) );
              $$ = MAKE_VALUE( Variant( pLValue ), LOC($1) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC($1) );
          }
      }
    | TOK_DOT l_value_var
      {
          /* Leading dot - access member of implied 'this' */
          if( $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pExpr = ToObjPtrVal( $2 );
              ObjPtr pLValue = pFactory->CreateLValueNode( pExpr, LOC_RANGE($1, $2) );
              $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $2) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $2) );
          }
      }
    | TOK_SUPER pointer l_value_var
      {
          /* Super class member access via pointer */
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pSuper = pFactory->CreateIdentifierExpr( "super", LOC($1) );
              ObjPtr pMember = ToObjPtrVal( $3 );

              /* Build member access: super.member */
              CStMemberAccessExpr* pMemberExpr = dynamic_cast< CStMemberAccessExpr* >( ( CObjBase* )pMember );
              if( pMemberExpr != nullptr )
              {
                  ObjPtr pAccess = pFactory->CreateMemberAccessExpr(
                      CStMemberAccessExpr::atDot, pSuper, pMemberExpr->m_strMember, LOC_RANGE($1, $3) );
                  ObjPtr pLValue = pFactory->CreateLValueNode( pAccess, LOC_RANGE($1, $3) );
                  $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $3) );
              }
              else
              {
                  /* Fallback: wrap the member as is */
                  ObjPtr pLValue = pFactory->CreateLValueNode( pMember, LOC_RANGE($1, $3) );
                  $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $3) );
              }
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $3) );
          }
      }
    | TOK_THIS pointer l_value_var
      {
          /* This member access via pointer */
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pThis = pFactory->CreateIdentifierExpr( "this", LOC($1) );
              ObjPtr pMember = ToObjPtrVal( $3 );

              /* Build member access: this.member */
              CStMemberAccessExpr* pMemberExpr = dynamic_cast< CStMemberAccessExpr* >( ( CObjBase* )pMember );
              if( pMemberExpr != nullptr )
              {
                  ObjPtr pAccess = pFactory->CreateMemberAccessExpr(
                      CStMemberAccessExpr::atDot, pThis, pMemberExpr->m_strMember, LOC_RANGE($1, $3) );
                  ObjPtr pLValue = pFactory->CreateLValueNode( pAccess, LOC_RANGE($1, $3) );
                  $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $3) );
              }
              else
              {
                  /* Fallback: wrap the member as is */
                  ObjPtr pLValue = pFactory->CreateLValueNode( pMember, LOC_RANGE($1, $3) );
                  $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $3) );
              }
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $3) );
          }
      }
    ;

l_value_ext:
    l_value
      {
          /* Wrap standard l_value in extended l_value node */
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pLVal = ToObjPtrVal( $1 );
              ObjPtr pLValueExt = pFactory->CreateLValueExtNode( pLVal, LOC($1) );
              $$ = MAKE_VALUE( Variant( pLValueExt ), LOC($1) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC($1) );
          }
      }
    /* %Q and %M can be l_value */
    | direct_address
      {
          /* Direct address like %Q0.0 or %MW100 */
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pExpr = ToObjPtrVal( $1 );
              ObjPtr pLValueExt = pFactory->CreateLValueExtNode( pExpr, LOC($1) );
              $$ = MAKE_VALUE( Variant( pLValueExt ), LOC($1) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC($1) );
          }
      }
    // actually what the lexer sees is a TOK_DOT
    // and stmain will replaced it with TOK_VPUNC
    | l_value TOK_VPUNC TOK_NUMBER
      {
          /* Bit access like MyVar.0 or MyArray[5].3 */
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pLVal = ToObjPtrVal( $1 );

              /* Create a member access for the bit index */
              std::string strBitIndex = ID($3);
              ObjPtr pBitAccess = pFactory->CreateMemberAccessExpr(
                  CStMemberAccessExpr::atDot, pLVal, strBitIndex, LOC_RANGE($1, $3) );

              ObjPtr pLValueExt = pFactory->CreateLValueExtNode( pBitAccess, LOC_RANGE($1, $3) );
              $$ = MAKE_VALUE( Variant( pLValueExt ), LOC_RANGE($1, $3) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $3) );
          }
      }
    ;

pointer:
    TOK_CARET TOK_DOT
    /* This is just syntactic - the actual node creation happens in the parent rule */
    ;

l_value_var:
    /* simple variable or struct field*/
    instance_path
      {
          /* Unwrap the path: l-value boundary holds the expression form,
             while the name components stay available via the wrapper in
             name-extraction contexts (derived_type, using_directive, ...) */
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPath = ToObjPtrVal( $1 );
              CStInstancePathNode* pInstancePath = pPath;
              if( pInstancePath != nullptr )
              {
                  $$ = MAKE_VALUE( Variant( pInstancePath->m_pExpression ),
                      LOC($1) );
              }
              else
              {
                  $$ = $1;
              }
          }
          else
          {
              $$ = $1;
          }
      }
    /* array element */
    | l_value_var TOK_LBRACKET full_expression TOK_RBRACKET
      {
          if( $1 != nullptr && IsObjPtrVal( $1 ) && $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pArray = ToObjPtrVal( $1 );
              ObjPtr pIndex = UnwrapFullExpression( ToObjPtrVal( $3 ) );
              std::vector< ObjPtr > vecIndices;
              vecIndices.push_back( pIndex );
              ObjPtr pArrayAccess = pFactory->CreateArrayAccessExpr(
                  pArray, vecIndices, LOC_RANGE($1, $4) );
              $$ = MAKE_VALUE( Variant( pArrayAccess ), LOC_RANGE($1, $4) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $4) );
          }
      }
    /* access data member via a pointer */
    | l_value_var TOK_CARET TOK_DOT instance_path
      {
          if( $1 != nullptr && IsObjPtrVal( $1 ) && $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pPointer = ToObjPtrVal( $1 );
              ObjPtr pMember = ToObjPtrVal( $4 );

              /* If instance_path is an identifier, extract the name */
              CStIdentifierExpr* pIdent = dynamic_cast< CStIdentifierExpr* >( ( CObjBase* )pMember );
              if( pIdent != nullptr )
              {
                  ObjPtr pPointerMember = pFactory->CreatePointerMemberExpr(
                      pPointer, pIdent->m_strName, LOC_RANGE($1, $4) );
                  $$ = MAKE_VALUE( Variant( pPointerMember ), LOC_RANGE($1, $4) );
              }
              else
              {
                  /* instance_path could be a member access chain itself */
                  $$ = $4;
              }
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $4) );
          }
      }
    /* dereference a pointer */
    | l_value_var TOK_CARET
      {
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pPointer = ToObjPtrVal( $1 );
              ObjPtr pDeref = pFactory->CreateDereferenceExpr( pPointer, LOC_RANGE($1, $2) );
              $$ = MAKE_VALUE( Variant( pDeref ), LOC_RANGE($1, $2) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $2) );
          }
      }
    ; 

full_expression:
    and_expression
      {
          /* Wrap in a boundary node: marks the root of an expression in
             statement position (condition, loop bound, case selector,
             initializer, call argument) */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pInner = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pNode = pFactory->CreateFullExpressionNode(
              pInner, LOC($1) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | full_expression TOK_OR and_expression
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              UnwrapFullExpression( ToObjPtrVal( $1 ) ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boOr, pLeft, pRight, LOC_RANGE($1, $3) );

          /* the result is again a statement-position expression */
          ObjPtr pNode = pFactory->CreateFullExpressionNode(
              pExpr, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    | full_expression TOK_XOR and_expression
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              UnwrapFullExpression( ToObjPtrVal( $1 ) ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boXor, pLeft, pRight, LOC_RANGE($1, $3) );

          /* the result is again a statement-position expression */
          ObjPtr pNode = pFactory->CreateFullExpressionNode(
              pExpr, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

and_expression:
    comparison_expression
      {
          $$ = $1;  // Pass through
      }
    | and_expression TOK_AND comparison_expression
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boAnd, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    ;

/* 3. COMPARISON (=, <>, <, >, <=, >=) */
comp_op:
    TOK_EQUAL
    { 
        Variant oVar = TOK_EQUAL;
        MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_NEQU
    { 
        Variant oVar = TOK_NEQU;
        MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_LE
    { 
        Variant oVar = TOK_LE;
        MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_GT
    { 
        Variant oVar = TOK_GT;
        MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_NLE
    { 
        Variant oVar = TOK_NLE;
        MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_NGT
    { 
        Variant oVar = TOK_NGT;
        MAKE_VALUE( oVar, LOC($1) );
    }

comparison_expression:
      arithmetic_expr
      {
          $$ = $1;  // Pass through
      }
    | arithmetic_expr comp_op arithmetic_expr
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          // Determine comparison operator
          CStBinaryExpr::enumBinaryOp eOp = CStBinaryExpr::boEqual;
          if( $2 != nullptr )
          {
              gint32 iOp = NUM($2);
              if( iOp == TOK_EQUAL ) eOp = CStBinaryExpr::boEqual;
              else if( iOp == TOK_NEQU ) eOp = CStBinaryExpr::boNotEqual;
              else if( iOp == TOK_LE ) eOp = CStBinaryExpr::boLessThan;
              else if( iOp == TOK_NGT ) eOp = CStBinaryExpr::boLessEqual;
              else if( iOp == TOK_GT ) eOp = CStBinaryExpr::boGreaterThan;
              else if( iOp == TOK_NLE ) eOp = CStBinaryExpr::boGreaterEqual;
          }

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              eOp, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    ;

/* 4. ADDITIVE (+, -) */
arithmetic_expr:
      term
      {
          $$ = $1;
      }
    | arithmetic_expr TOK_ADD term
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boAdd, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    | arithmetic_expr TOK_VSUB term
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boSub, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    ;

/* 5. MULTIPLICATIVE (*, /, MOD) */
term:
      unary_expr
      {
          $$ = $1;
      }
    | term TOK_MUL unary_expr
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boMul, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    | term TOK_DIV unary_expr
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boDiv, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    | term TOK_MOD unary_expr
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boMod, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    ;

/* 6. UNARY (NOT, -) */
unary_expr:
      power_expr
      {
          $$ = $1;
      }
    | TOK_MINUS power_expr
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pOperand = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
              ToObjPtrVal( $2 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateUnaryExpr(
              CStUnaryExpr::uoNeg, pOperand, LOC_RANGE($1, $2) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $2) );
      }
    | TOK_NOT power_expr
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pOperand = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
              ToObjPtrVal( $2 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateUnaryExpr(
              CStUnaryExpr::uoNot, pOperand, LOC_RANGE($1, $2) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $2) );
      }
    ;

/* 7. POWER (**) */
power_expr:
      factor
      {
          $$ = $1;
      }
    | factor TOK_POWER factor
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boPower, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    ;

/* 8. PRIMARY (Highest Precedence) */
factor:
      TOK_NUMBER
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltNumber, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_STRING
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltString, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_WSTRING
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltWString, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_TIME
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltTime, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_LTIME
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltLTime, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_DATE
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltDate, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_DATE_TIME
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltDateTime, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_TIME_OF_DAY
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue;
          if( $1 != nullptr )
              oValue = $1->first;

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltTimeOfDay, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_TRUE
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue( true );

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltBool, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | TOK_FALSE
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          Variant oValue( false );

          ObjPtr pLiteral = pFactory->CreateLiteralExpr(
              CStLiteralExpr::ltBool, oValue, LOC($1) );

          $$ = MAKE_VALUE( Variant( pLiteral ), LOC($1) );
      }
    | l_value TOK_LPAREN arg_list TOK_RPAREN
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pCallee = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;

          std::vector< ObjPtr > vecArgs;
          std::vector< CStCallExpr::CNamedArg > vecNamed;
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              ObjPtr pArgList = ToObjPtrVal( $3 );
              CStArgListNode* pArgListNode = pArgList;
              if( pArgListNode != nullptr )
              {
                  vecArgs = pArgListNode->m_vecArgs;
                  vecNamed = pArgListNode->m_vecNamed;
              }
          }

          ObjPtr pCall = pFactory->CreateCallExpr(
              pCallee, vecArgs, LOC_RANGE($1, $4), vecNamed );

          $$ = MAKE_VALUE( Variant( pCall ), LOC_RANGE($1, $4) );
      }
    | l_value_ext
      {
          $$ = $1;  // Pass through l_value expression
      }
    | TOK_LPAREN full_expression TOK_RPAREN
      {
          /* Pass through the parenthesized expression, unwrapped from
             the full-expression boundary node */
          ObjPtr pInner = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
              UnwrapFullExpression( ToObjPtrVal( $2 ) ) : nullptr;
          $$ = MAKE_VALUE( Variant( pInner ), LOC_RANGE($1, $3) );
      }
    ;

elseif_branch:
    /* empty */
      {
          /* No else-if branches - provide an empty accumulator */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pNode = pFactory->CreateIfBranchListNode( YYLTYPE2() );
          $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
      }
    | elseif_branch TOK_ELSIF full_expression TOK_THEN block_statements
      {
          /* Accumulate the else-if branch */
          ObjPtr pList;
          pList.NewObj( clsid( CStIfBranchListNode ) );
          CStIfBranchListNode* pBranchList = pList;
          if( pBranchList != nullptr )
          {
              CStIfStmt::CIfBranch oBranch;
              if( $3 != nullptr && IsObjPtrVal( $3 ) )
                  oBranch.m_pCondition = ToObjPtrVal( $3 );
              if( $5 != nullptr && IsObjPtrVal( $5 ) )
              {
                  ObjPtr pStmts = ToObjPtrVal( $5 );
                  CStStmtListNode* pStmtList = pStmts;
                  if( pStmtList != nullptr )
                      oBranch.m_vecStatements =
                          pStmtList->m_vecStatements;
              }
              if( $1 != nullptr && IsObjPtrVal( $1 ) )
              {
                  ObjPtr pPrev = ToObjPtrVal( $1 );
                  CStIfBranchListNode* pPrevList = pPrev;
                  if( pPrevList != nullptr )
                      pBranchList->m_vecBranches =
                          pPrevList->m_vecBranches;
              }
              pBranchList->m_vecBranches.push_back( oBranch );
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $5) );
      }
    ;

else_branch:
    /* empty */
      {
          /* No else statements - provide an empty accumulator */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pNode = pFactory->CreateStmtListNode( YYLTYPE2() );
          $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
      }
    | TOK_ELSE block_statements
      {
          /* the block accumulator carries the else statements */
          $$ = $2;
      }
    ;

opt_semicolons:
    /* empty */
    | semicolons

if_statement:
    TOK_IF full_expression TOK_THEN block_statements elseif_branch else_branch TOK_END_IF
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pCondition = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        
        std::vector< ObjPtr > vecThen;
        if( $4 != nullptr && IsObjPtrVal( $4 ) )
        {
            /* Extract the then-branch statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $4 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecThen = pStmtList->m_vecStatements;
        }
        std::vector< CStIfStmt::CIfBranch > vecElseIf;
        if( $5 != nullptr && IsObjPtrVal( $5 ) )
        {
            /* Extract the else-if branches from the accumulator */
            ObjPtr pList = ToObjPtrVal( $5 );
            CStIfBranchListNode* pBranchList = pList;
            if( pBranchList != nullptr )
                vecElseIf = pBranchList->m_vecBranches;
        }
        std::vector< ObjPtr > vecElse;
        if( $6 != nullptr && IsObjPtrVal( $6 ) )
        {
            /* Extract the else statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $6 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecElse = pStmtList->m_vecStatements;
        }
        
        ObjPtr pNode = pFactory->CreateIfStmt(
            pCondition, vecThen, vecElseIf, vecElse,
            LOC_RANGE($1, $7) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $7) );
    }
    ;

opt_by_step:
    /* empty */
    | TOK_BY full_expression

block_statements_1:
    statement
      {
          /* First statement - create the accumulator */
          ObjPtr pList;
          pList.NewObj( clsid( CStStmtListNode ) );
          CStStmtListNode* pStmtList = pList;
          if( pStmtList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pStmtList->m_vecStatements.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
      }
    | block_statements_1 semicolons statement
      {
          /* Accumulate statements */
          ObjPtr pList;
          pList.NewObj( clsid( CStStmtListNode ) );
          CStStmtListNode* pStmtList = pList;
          if( pStmtList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStStmtListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pStmtList->m_vecStatements = pPrevList->m_vecStatements;
          }
          if( pStmtList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
              pStmtList->m_vecStatements.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }
    ;
block_statements:
    block_statements_1 opt_semicolons

for_statement:
    TOK_FOR instance_path TOK_ASSIGN full_expression TOK_TO full_expression opt_by_step TOK_DO block_statements TOK_END_FOR
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strVar;
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            CStInstancePathNode* pPath = ToObjPtrVal( $2 );
            if( pPath != nullptr )
                strVar = pPath->GetDottedName();
        }
        ObjPtr pStart = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        ObjPtr pEnd = ( $6 != nullptr && IsObjPtrVal( $6 ) ) ?
            ToObjPtrVal( $6 ) : nullptr;
        ObjPtr pStep = ( $7 != nullptr && IsObjPtrVal( $7 ) ) ?
            ToObjPtrVal( $7 ) : nullptr;
        std::vector< ObjPtr > vecBody;
        if( $9 != nullptr && IsObjPtrVal( $9 ) )
        {
            /* Extract the loop body statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $9 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecBody = pStmtList->m_vecStatements;
        }
        
        ObjPtr pNode = pFactory->CreateForStmt(
            strVar, pStart, pEnd, pStep, vecBody,
            LOC_RANGE($1, $10) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $10) );
    }
    ;

while_statement:
    TOK_WHILE full_expression TOK_DO block_statements TOK_END_WHILE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pCondition = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        std::vector< ObjPtr > vecBody;
        if( $4 != nullptr && IsObjPtrVal( $4 ) )
        {
            /* Extract the loop body statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $4 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecBody = pStmtList->m_vecStatements;
        }
        
        ObjPtr pNode = pFactory->CreateWhileStmt(
            pCondition, vecBody, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }
    ;

repeat_statement:
    TOK_REPEAT block_statements TOK_UNTIL full_expression TOK_END_REPEAT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::vector< ObjPtr > vecBody;
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            /* Extract the loop body statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $2 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecBody = pStmtList->m_vecStatements;
        }
        ObjPtr pCondition = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        
        ObjPtr pNode = pFactory->CreateRepeatStmt(
            vecBody, pCondition, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }
    ;

positional_args:
    full_expression
      {
          /* First positional argument - create accumulator */
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pArgList->m_vecArgs.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
      }
    | positional_args TOK_COMMA full_expression
      {
          /* Accumulate positional arguments */
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStArgListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
              {
                  pArgList->m_vecArgs = pPrevList->m_vecArgs;
                  pArgList->m_vecNamed = pPrevList->m_vecNamed;
              }
          }
          if( pArgList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
              pArgList->m_vecArgs.push_back( ToObjPtrVal( $3 ) );
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }

arg_list:
      positional_args
    | param_assignments
    ;

param_assignments:
    param_assignment
      {
          $$ = $1;
      }
    | param_assignments TOK_COMMA param_assignment
      {
          /* Accumulate named arguments */
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStArgListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
              {
                  pArgList->m_vecArgs = pPrevList->m_vecArgs;
                  pArgList->m_vecNamed = pPrevList->m_vecNamed;
              }
          }
          if( pArgList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              ObjPtr pCur = ToObjPtrVal( $3 );
              CStArgListNode* pCurList = pCur;
              if( pCurList != nullptr )
              {
                  for( auto& arg : pCurList->m_vecArgs )
                      pArgList->m_vecArgs.push_back( arg );
                  for( auto& arg : pCurList->m_vecNamed )
                      pArgList->m_vecNamed.push_back( arg );
              }
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }
    /* error recovery: skip the bad token before the parameter */
    | param_assignments TOK_COMMA error param_assignment
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @3.last_line, "invalid parameter, skipping", true );
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStArgListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
              {
                  pArgList->m_vecArgs = pPrevList->m_vecArgs;
                  pArgList->m_vecNamed = pPrevList->m_vecNamed;
              }
          }
          if( pArgList != nullptr && $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              ObjPtr pCur = ToObjPtrVal( $4 );
              CStArgListNode* pCurList = pCur;
              if( pCurList != nullptr )
              {
                  for( auto& arg : pCurList->m_vecArgs )
                      pArgList->m_vecArgs.push_back( arg );
                  for( auto& arg : pCurList->m_vecNamed )
                      pArgList->m_vecNamed.push_back( arg );
              }
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $4) );
      }
    /* error recovery: skip the bad token between parameters */
    | param_assignments error TOK_COMMA param_assignment
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @2.last_line, "invalid parameter list, skipping", true );
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStArgListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
              {
                  pArgList->m_vecArgs = pPrevList->m_vecArgs;
                  pArgList->m_vecNamed = pPrevList->m_vecNamed;
              }
          }
          if( pArgList != nullptr && $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              ObjPtr pCur = ToObjPtrVal( $4 );
              CStArgListNode* pCurList = pCur;
              if( pCurList != nullptr )
              {
                  for( auto& arg : pCurList->m_vecArgs )
                      pArgList->m_vecArgs.push_back( arg );
                  for( auto& arg : pCurList->m_vecNamed )
                      pArgList->m_vecNamed.push_back( arg );
              }
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $4) );
      }
    ;

param_assignment:
    TOK_ID TOK_ASSIGN full_expression  /* Formal Input: IN := True */
      {
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStCallExpr::CNamedArg oArg;
              oArg.m_strName = ID($1);
              oArg.m_pValue = ToObjPtrVal( $3 );
              oArg.m_bOutput = false;
              pArgList->m_vecNamed.push_back( oArg );
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }
    | TOK_ID TOK_OUTPUT_ASSIGN l_value_ext           /* Formal Output: Q => MyLamp */
      {
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStCallExpr::CNamedArg oArg;
              oArg.m_strName = ID($1);
              oArg.m_pValue = ToObjPtrVal( $3 );
              oArg.m_bOutput = true;
              pArgList->m_vecNamed.push_back( oArg );
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
      }
    | TOK_ID TOK_OUTPUT_ASSIGN error l_value_ext           /* Formal Output: Q => MyLamp */
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @3.last_line, "invalid output parameter, skipping", true );
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              CStCallExpr::CNamedArg oArg;
              oArg.m_strName = ID($1);
              oArg.m_pValue = ToObjPtrVal( $4 );
              oArg.m_bOutput = true;
              pArgList->m_vecNamed.push_back( oArg );
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $4) );
      }
    | TOK_ID TOK_ASSIGN error full_expression  /* Formal Input: IN := True */
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @3.last_line, "invalid input parameter, skipping", true );
          ObjPtr pList;
          pList.NewObj( clsid( CStArgListNode ) );
          CStArgListNode* pArgList = pList;
          if( pArgList != nullptr && $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              CStCallExpr::CNamedArg oArg;
              oArg.m_strName = ID($1);
              oArg.m_pValue = ToObjPtrVal( $4 );
              oArg.m_bOutput = false;
              pArgList->m_vecNamed.push_back( oArg );
          }
          $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $4) );
      }
    ;

method_declaration_list:
    method_declaration
    | method_declaration_list method_declaration
    /* error recovery for method declarations */
    | method_declaration_list error
    {
        pCtx->IncError();
        ParserPrint( basename(pCtx->GetCurFileName().c_str()),
            @2.last_line,
            "invalid method declaration, skipping", true );
        yyerrok;
    }
    ;

opt_global_namespace:
    /* empty */
    | TOK_DOT
    ;

method_declaration:
    TOK_METHOD opt_access_modifier TOK_ID TOK_COLON data_type_spec
        var_declarations
        block_statements
    TOK_END_METHOD
    /*| TOK_METHOD opt_access_modifier TOK_ID TOK_COLON derived_type
        var_declarations
        block_statements
    TOK_END_METHOD*/
    /* TOK_VSEMICOLON is a virtual token*/
    | TOK_METHOD opt_access_modifier TOK_ID TOK_COLON opt_global_namespace instance_path TOK_VSEMICOLON
        var_declarations
        block_statements
    TOK_END_METHOD
    ;

opt_access_modifier:
    /* empty - defaults to PUBLIC in most ST dialects */

    | TOK_PUBLIC
    | TOK_PROTECTED
    | TOK_PRIVATE
    | TOK_INTERNAL
    ;

function_block:
    function_block_header using_directive_list var_declarations method_declaration_list block_statements TOK_END_FUNCTION_BLOCK
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = "";
        CStFunctionBlockDecl::enumFbModifier eModifier =
            CStFunctionBlockDecl::fbmNone;
        std::string strExtends = "";
        std::vector< std::string > vecImplements;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            /* Extract the header information from the accumulator */
            ObjPtr pHeader = ToObjPtrVal( $1 );
            CStFunctionBlockHeaderNode* pFbHeader = pHeader;
            if( pFbHeader != nullptr )
            {
                strName = pFbHeader->m_strName;
                eModifier = pFbHeader->m_eModifier;
                strExtends = pFbHeader->m_strExtends;
                vecImplements = pFbHeader->m_vecImplements;
            }
        }
        /* method extraction is pending its accumulator */
        std::vector< ObjPtr > vecInput, vecOutput, vecInOut, vecLocal, vecTemp;
        std::vector< ObjPtr > vecMethods, vecStatements;
        std::vector< ObjPtr > vecOther;
        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            /* Split the accumulated declarations by category */
            SplitVarDeclList( ToObjPtrVal( $3 ),
                vecInput, vecOutput, vecInOut,
                vecLocal, vecTemp, vecOther );
        }
        if( $5 != nullptr && IsObjPtrVal( $5 ) )
        {
            /* Extract the body statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $5 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecStatements = pStmtList->m_vecStatements;
        }
        
        ObjPtr pNode = pFactory->CreateFunctionBlockDecl(
            strName, eModifier, strExtends, vecImplements,
            vecInput, vecOutput, vecInOut, vecLocal, vecTemp,
            vecMethods, vecStatements, LOC_RANGE($1, $6) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }
    | function_block_header var_declarations method_declaration_list block_statements TOK_END_FUNCTION_BLOCK
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = "";
        CStFunctionBlockDecl::enumFbModifier eModifier =
            CStFunctionBlockDecl::fbmNone;
        std::string strExtends = "";
        std::vector< std::string > vecImplements;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            /* Extract the header information from the accumulator */
            ObjPtr pHeader = ToObjPtrVal( $1 );
            CStFunctionBlockHeaderNode* pFbHeader = pHeader;
            if( pFbHeader != nullptr )
            {
                strName = pFbHeader->m_strName;
                eModifier = pFbHeader->m_eModifier;
                strExtends = pFbHeader->m_strExtends;
                vecImplements = pFbHeader->m_vecImplements;
            }
        }
        /* method extraction is pending its accumulator */
        std::vector< ObjPtr > vecInput, vecOutput, vecInOut, vecLocal, vecTemp;
        std::vector< ObjPtr > vecStatements;
        std::vector< ObjPtr > vecOther;
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            /* Split the accumulated declarations by category */
            SplitVarDeclList( ToObjPtrVal( $2 ),
                vecInput, vecOutput, vecInOut,
                vecLocal, vecTemp, vecOther );
        }
        if( $4 != nullptr && IsObjPtrVal( $4 ) )
        {
            /* Extract the body statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $4 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecStatements = pStmtList->m_vecStatements;
        }

        ObjPtr pNode = pFactory->CreateFunctionBlockDecl(
            strName, eModifier, strExtends, vecImplements,
            vecInput, vecOutput, vecInOut, vecLocal, vecTemp,
            {}, vecStatements, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }

function_block_header:
    TOK_FUNCTION_BLOCK opt_fb_modifier TOK_ID opt_extends_clause opt_implements_clause
      {
          /* Accumulate the header information; consumed by the
             function_block rule to build the final declaration */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($3);
          CStFunctionBlockDecl::enumFbModifier eModifier =
              ( CStFunctionBlockDecl::enumFbModifier )NUM($2);
          std::string strExtends; 
          if( $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              CStInstancePathNode* pInstPath = ( ObjPtr& )$4->first;
              strExtends = pInstPath->GetDottedName();
          }
          std::vector< std::string > vecImplements;
          if( $5 != nullptr && IsObjPtrVal( $5 ) )
          {
              ObjPtr pList = ToObjPtrVal( $5 );
              CStIdentifierListNode* pIdList = pList;
              if( pIdList != nullptr )
                  vecImplements = pIdList->m_vecIdentifiers;
          }
          ObjPtr pNode = pFactory->CreateFunctionBlockHeaderNode(
              strName, eModifier, strExtends, vecImplements,
              LOC_RANGE($1, $5) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }
    ;

/* Handles ABSTRACT or FINAL keywords */
opt_fb_modifier:
    /* empty */
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStFunctionBlockDecl::fbmNone, YYLTYPE2() );
      }
    | TOK_ABSTRACT
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStFunctionBlockDecl::fbmAbstract, LOC($1) );
      }
    | TOK_FINAL
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStFunctionBlockDecl::fbmFinal, LOC($1) );
      }
    ;

/* Handles EXTENDS <Parent> */
opt_extends_clause:
    /* empty */
      {
          $$ = MAKE_VALUE( std::string( "" ), YYLTYPE2() );
      }
    | TOK_EXTENDS instance_path
     {
        if( $2 != nullptr )
              $$ = MAKE_VALUE( $2->first, LOC_RANGE($1, $2) );
        else
        {
            pCtx->IncError();
            stdstr strCurFile = basename(
                pCtx->GetCurFileName().c_str() );
            ParserPrint( strCurFile.c_str(),
                @1.last_line,
                "class or fb name is expected", true );
            yyerrok;
        }
     }
    | TOK_EXTENDS error
    {
    }
    ;

/* Handles IMPLEMENTS <Interface1, Interface2...> */
opt_implements_clause:
    /* empty */
      {
          /* no IMPLEMENTS clause: an empty identifier list */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
      }
    | TOK_IMPLEMENTS interface_list
      {
          $$ = $2;
      }
    ;

interface_list:
    instance_path
      {
          /* Accumulate the interface names as dotted paths, e.g.
             'Robotics.Interfaces.IActuator'; each element is an
             instance_path, flattened to its dotted name here */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          CStIdentifierListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPath = ToObjPtrVal( $1 );
              CStInstancePathNode* pInstPath = pPath;
              if( pInstPath != nullptr )
                  pList->m_vecIdentifiers.push_back(
                      pInstPath->GetDottedName() );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | interface_list TOK_COMMA instance_path
      {
          /* rebuild-and-copy: the previous entries plus the new
             interface path */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          CStIdentifierListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStIdentifierListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pList->m_vecIdentifiers =
                      pPrevList->m_vecIdentifiers;
          }
          if( pList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              ObjPtr pPath = ToObjPtrVal( $3 );
              CStInstancePathNode* pInstPath = pPath;
              if( pInstPath != nullptr )
                  pList->m_vecIdentifiers.push_back(
                      pInstPath->GetDottedName() );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

function:
    TOK_FUNCTION TOK_ID var_declarations block_statements TOK_END_FUNCTION
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($2);
        
        std::vector< ObjPtr > vecInput, vecOutput, vecInOut;
        std::vector< ObjPtr > vecLocal, vecTemp, vecStatements;
        std::vector< ObjPtr > vecOther;
        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            /* Split the accumulated declarations by category */
            SplitVarDeclList( ToObjPtrVal( $3 ),
                vecInput, vecOutput, vecInOut,
                vecLocal, vecTemp, vecOther );
        }
        if( $4 != nullptr && IsObjPtrVal( $4 ) )
        {
            /* Extract the body statements from the accumulator */
            ObjPtr pList = ToObjPtrVal( $4 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecStatements = pStmtList->m_vecStatements;
        }
        
        ObjPtr pNode = pFactory->CreateFunctionDecl(
            strName, nullptr, vecInput, vecOutput, vecInOut,
            vecLocal, vecTemp, vecStatements, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }
    ;

case_statement:
    TOK_CASE full_expression TOK_OF
        case_element_list
        opt_else_statement
    TOK_END_CASE
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pExpr = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        
        std::vector< CStCaseStmt::CCaseBranch > vecBranches;
        std::vector< ObjPtr > vecElse;
        // TODO: Extract from case_element_list and opt_else_statement
        
        ObjPtr pNode = pFactory->CreateCaseStmt(
            pExpr, vecBranches, vecElse, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }
    ;

opt_else_statement:
    /* empty */
    |TOK_ELSE block_statements
    ;

case_element_list:
    case_element
    | case_element_list case_element 
    ;

semicolons:
    TOK_SEMICOLON
    | TOK_SEMICOLON semicolons

cinner_statements_1:
    statement
    | cinner_statements_1 TOK_SEMICOLON statement
    {}
    | cinner_statements_1 TOK_SEMICOLON
    /* error check */
    | cinner_statements_1 error statement
    { 
        pCtx->IncError();
        stdstr strCurFile = basename(
            pCtx->GetCurFileName().c_str() );
        ParserPrint( strCurFile.c_str(),
            @2.last_line,
            "';' is expected", true );
        yyerrok;
    }
    ;


cinner_statements: cinner_statements_1 

case_element:
    // case_list_selector cinner_statements TOK_SEMICOLON
    // TOK_VCASE_SEP does not come from source, it is used to resolve the
    // shift/reduce conflict
    case_list_selector cinner_statements TOK_VCASE_SEP
    ;

case_check_statement:
    assignment_statement
    | function_call_statement

case_selector_check:
    TOK_VSTART_CASESEL case_list_selector
    { pCtx->m_iLastCaseChk = 0; }
    | TOK_VSTART_CASESEL case_check_statement
    {
        pCtx->m_iLastCaseChk = 1;
        if( yychar != YYEOF )
        {
            // insert a TOK_VCASE_SEP
            auto current_tok = yychar;
            // and tell the checker to stop
            yychar = YYEOF;

            auto casep_lloc = yylval->second;
            casep_lloc.text = ";";
            casep_lloc.last_column = casep_lloc.first_column;
            YYSTYPE casep_lval = MAKE_EMPTY();

            // current_tok must not be semicolon, which
            // is filtered off by StartCaseSelectorCheck.
            // let's insert one to mark the end of the
            // statement. 
            // inserting a VCASE_SEP instead of
            // SEMICOLON is because at this point, the
            // only possible correct case is that the
            // next line is case selector, or end_case
            // where the current line is not required
            // to end up with a SEMICOLON. 
            pCtx->PushToken(
                 { casep_lloc, TOK_VCASE_SEP, casep_lval } );

            auto current_lloc = yylval->second;
            auto current_lval = yylval;
            pCtx->PushToken(
                { current_lloc, current_tok, current_lval } );
        }
    }
    | TOK_VSTART_CASESEL case_check_statement error
    {
        yychar = YYEOF;
        yyerrok;
    }

case_list_selector:
    case_selector TOK_COLON
    | case_list_selector TOK_COMMA case_selector TOK_COLON
    /* error recovery for case selectors */
    | case_list_selector TOK_COMMA error TOK_COLON
    {
        pCtx->IncError();
        ParserPrint( basename(pCtx->GetCurFileName().c_str()),
            @3.last_line,
            "invalid case selector, skipping", true );
        yyerrok;
    }
    ;

case_selector:
    case_constant_expression

    | case_constant_expression TOK_RANGE case_constant_expression
    ;

case_constant_expression:
    full_expression { /* evaluate the full_expression to get a constant value */ }
    ;

var_config_declaration:
    TOK_VAR_CONFIG
        instance_specific_init_list
    TOK_END_VAR
    ;

instance_specific_init_list:
    /* empty */
    | instance_specific_init_list instance_specific_init
    ;

instance_specific_init:
    instance_path TOK_AT direct_address TOK_COLON type_spec semicolons
    |instance_path TOK_AT direct_address TOK_COLON type_spec TOK_ASSIGN initial_value semicolons
    ;

instance_path : TOK_ID
      {
          /* Single identifier - wrap in CStInstancePathNode */
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($1);
          std::vector< std::string > vecComponents;
          vecComponents.push_back( strName );

          ObjPtr pExpr = pFactory->CreateIdentifierExpr(
            strName, LOC($1) );
          ObjPtr pPath = pFactory->CreateInstancePathNode(
            vecComponents, pExpr, LOC($1) );
          $$ = MAKE_VALUE( Variant( pPath ), LOC($1) );
      }
    | instance_path TOK_DOT TOK_ID  /* e.g., MainProg.Motor1.SensorIn */
      {
          /* Append component and extend the member access chain */
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              std::string strMember = ID($3);
              ObjPtr pPrevPath = ToObjPtrVal( $1 );
              CStInstancePathNode* pPrev = pPrevPath;

              std::vector< std::string > vecComponents;
              ObjPtr pBaseExpr;
              if( pPrev != nullptr )
              {
                  vecComponents = pPrev->m_vecNameComponents;
                  pBaseExpr = pPrev->m_pExpression;
              }
              vecComponents.push_back( strMember );

              ObjPtr pMemberAccess = pFactory->CreateMemberAccessExpr(
                  CStMemberAccessExpr::atDot, pBaseExpr, strMember, LOC_RANGE($1, $3) );
              ObjPtr pPath = pFactory->CreateInstancePathNode(
                  vecComponents, pMemberAccess, LOC_RANGE($1, $3) );
              $$ = MAKE_VALUE( Variant( pPath ), LOC_RANGE($1, $3) );
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $3) );
          }
      }
    ;

using_directive_list : using_directive
      {  }

    | using_directive_list using_directive
      {
          
          $$ = $1;
      }
    ;
using_directive : TOK_USING instance_path TOK_VSEMICOLON
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< std::string > vecNamespace;
          // Namespace components come from the path wrapper
          if( $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              CStInstancePathNode* pPath = ToObjPtrVal( $2 );
              if( pPath != nullptr )
                  vecNamespace = pPath->m_vecNameComponents;
          }

          ObjPtr pNode = pFactory->CreateUsingDirective(
              vecNamespace, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

%%

void yyerror (YYLTYPE* yyloc,
    rpcf::CSTParserContext* pCtx,
    const char* yymsgp)
{
    if( pCtx->GetTokenCount() &&
        !pCtx->UseQueuedToken() )
    {
        return;
    }
    stdstr strCurFile = basename(
        pCtx->GetCurFileName().c_str() );
    bool bErr = false;
    stdstr strMsg = yymsgp;
    if( strMsg.substr( 0, 12 ) == "syntax error" ||
        strMsg.substr( 0, 5 ) == "error" ||
        strMsg.substr( 0, 7 ) == "warning" )
        bErr = true;

    if( strCurFile.empty() )
        strCurFile = " ";

    yypstate* ps = reinterpret_cast< yypstate* >
        ( pCtx->GetParser() );

    YYSTYPE pVal = *ps->yyvsp;
    if( pVal )
    {
        strMsg = "Parser found error at '";
        strMsg += pVal->second.text + "'";
        ParserPrint(
            strCurFile.c_str(),
            pVal->second.first_line,
            strMsg.c_str(), true );
    }
    else
    {
        ParserPrint( 
            strCurFile.c_str(),
            yyloc->first_line,
            yymsgp,
            bErr );
    }
}

int GetParserState( yypstate* ps )
{ return ps->yystate; }

