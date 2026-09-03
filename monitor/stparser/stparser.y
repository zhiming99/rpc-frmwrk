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

          // For now, we'll simplify and set empty vectors
          // TODO: Parse program_unit properly to extract these

          ObjPtr pProgram = pFactory->CreateProgramDecl(
              strName, vecInput, vecOutput, vecInOut,
              vecLocal, vecTemp, vecStatements, vecUsing,
              LOC_RANGE($1, $4) );

          $$ = MAKE_VALUE( Variant( pProgram ), LOC_RANGE($1, $4) );
      }
    ;

program_unit:
    using_directive_list var_declarations body
    | var_declarations body

body:
    /* empty */
    | block_statements

type_definition_block:
      TOK_TYPE type_assignments TOK_END_TYPE
    ;

type_assignments:
      type_assignment
    | type_assignments type_assignment
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
                  pVal->m_pExplicitValue = ToObjPtrVal( $3 );
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
        // This rule just passes through to enum_value_list
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
                  vecValues = pValList->m_vecValues;
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

var_decl_type: TOK_VAR
    | TOK_VAR_TEMP
    | TOK_VAR_INPUT
    | TOK_VAR_OUTPUT
    | TOK_VAR_IN_OUT
    | TOK_VAR_STAT
    | TOK_VAR_EXTERNAL
    ;
    
var_declarations:
    /* empty */
    | var_declarations var_decl_type declaration TOK_END_VAR
    ;

opt_qualifier:
    /* empty */
    | TOK_RETAIN
    | TOK_PERSISTENT
    | TOK_RETAIN TOK_PERSISTENT
    | TOK_PERSISTENT TOK_RETAIN
    | TOK_CONSTANT
    ;

declaration:
      opt_qualifier var_list semicolons
    ;

var_list:
    var_declaration
    | var_list semicolons  var_declaration {  }
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
                  pInit->m_pValue = ToObjPtrVal( $1 );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | TOK_LBRACKET init_list TOK_RBRACKET             /* Array:int := [1, 2, 3]; */
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStInitialValueNode ) );
          CStInitialValueNode* pInit = pNode;
          if( pInit != nullptr )
          {
              pInit->m_eInitType = CStInitialValueNode::initArray;
              if( $2 != nullptr && IsObjPtrVal( $2 ) )
                  pInit->m_pValue = ToObjPtrVal( $2 );
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
                  pInit->m_pValue = ToObjPtrVal( $2 );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

init_list:
      initial_value
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStArrayInitNode ) );
          CStArrayInitNode* pInit = pNode;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
              pInit->m_vecValues.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | init_list TOK_COMMA initial_value
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
    | init_list TOK_COMMA TOK_NUMBER TOK_LPAREN initial_value TOK_RPAREN
      {
          // Handle repeated element syntax: 3(value)
          // This creates an array with the value repeated N times
          gint32 iCount = NUM($3);
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
          if( $5 != nullptr && IsObjPtrVal( $5 ) )
          {
              for( gint32 i = 0; i < iCount; i++ )
                  pInit->m_vecValues.push_back( ToObjPtrVal( $5 ) );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
      }
    /* error recovery for array initialization */
    | init_list TOK_COMMA error
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
              CStVarDeclNode::vqNone, nullptr, "", false,
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
              CStVarDeclNode::vqNone, pInit, "", false,
              LOC_RANGE($1, $5) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }
    | TOK_ID TOK_AT direct_address
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($1);
          std::string strAddr = STR($3);
          ObjPtr pNode = pFactory->CreateVarDeclNode(
              std::vector<std::string>{strName}, nullptr, CStVarDeclNode::vcLocal,
              CStVarDeclNode::vqNone, nullptr, strAddr, true,
              LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    | TOK_ID TOK_AT direct_address TOK_ASSIGN initial_value
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($1);
          std::string strAddr = STR($3);
          ObjPtr pInit = ( $5 != nullptr && IsObjPtrVal( $5 ) ) ?
              ToObjPtrVal( $5 ) : nullptr;
          ObjPtr pNode = pFactory->CreateVarDeclNode(
              std::vector<std::string>{strName}, nullptr, CStVarDeclNode::vcLocal,
              CStVarDeclNode::vqNone, pInit, strAddr, true,
              LOC_RANGE($1, $5) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
      }
    ;


direct_address:
    TOK_RPCF_ADDR
    | TOK_ABS_ADDR_PERIPHERAL
    | TOK_ABS_ADDR_PERIPHERAL TOK_LBRACKET full_expression TOK_RBRACKET
  

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
    | TOK_BOOL
    | TOK_WORD
    | TOK_UINT
    | TOK_DINT
    | TOK_UDINT
    | TOK_SINT
    | TOK_USINT
    | TOK_BYTE
    | TOK_DWORD
    | TOK_ULINT
    | TOK_LINT
    | TOK_LWORD

time_type:
    TOK_TIME_TYPE
    | TOK_TIME_OF_DAY_TYPE
    | TOK_DATE_TYPE


array_type:
    TOK_ARRAY TOK_LBRACKET range_list TOK_RBRACKET TOK_OF type_spec
    ;

range_list:
      range
    | full_expression TOK_COMMA range  /* Supports multi-dimensional arrays */
    ;

range:
    TOK_NUMBER TOK_RANGE TOK_NUMBER    /* e.g., 1..10 */
    ;

string_type:
    TOK_STRING_TYPE TOK_LPAREN TOK_NUMBER TOK_RPAREN    {  }  /* Specific length */
    | TOK_STRING_TYPE TOK_LBRACKET TOK_NUMBER TOK_RBRACKET    {  }  /* Specific length */
    | TOK_STRING_TYPE                    {  }  /* Default length is 80 */
    | TOK_WSTRING_TYPE TOK_LPAREN TOK_NUMBER TOK_RPAREN   {  }
    | TOK_WSTRING_TYPE TOK_LBRACKET TOK_NUMBER TOK_RBRACKET    {  }  /* Specific length */
    | TOK_WSTRING_TYPE                   {  } /* Wide string (UTF-16) */
    ;

pointer_type:
    TOK_CARET type_spec
    TOK_POINTER TOK_TO type_spec

reference_type:
    TOK_REFERENCE TOK_TO type_spec
    | TOK_REF_TO type_spec
    ;

other_elementry_type:
    time_type
    | TOK_REAL
    | TOK_LREAL
    ;
    
data_type_spec:
    int_type
    | int_type TOK_LPAREN range TOK_RPAREN
    | other_elementry_type
    | string_type
    /* implicit enum */
    | TOK_LPAREN enum_value_list TOK_RPAREN
    ;

type_spec: 
    data_type_spec
    | array_type
    | reference_type
    | pointer_type
    | derived_type
    ;

derived_type: instance_path
    | TOK_DOT instance_path
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
          // TODO: Extract from arg_list
          
          ObjPtr pNode = pFactory->CreateCallStmt(
              pCallee, LOC_RANGE($1, $4) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
      }
    ;

/* L-Value rule: Strictly limited to writable memory locations */
l_value:
    l_value_var
    | TOK_DOT l_value_var
    | TOK_SUPER pointer l_value_var
    | TOK_THIS pointer l_value_var
    ;

l_value_ext:
    l_value
    /* %Q and %M can be l_value */
    | direct_address
    // actually waht the lexer sees is a TOK_DOT 
    // and stmain will replaced it with TOK_VPUNC
    | l_value TOK_VPUNC TOK_NUMBER
    ;

pointer:
    TOK_CARET TOK_DOT

l_value_var:
    /* simple variable or struct field*/
    instance_path {  }
    /* array element */
    | l_value_var TOK_LBRACKET full_expression TOK_RBRACKET {  }
    /* access data member via a pointer */
    | l_value_var TOK_CARET TOK_DOT instance_path {  }
    /* dereference a pointer */
    | l_value_var TOK_CARET {  }
    ; 

full_expression:
    and_expression
      {
          $$ = $1;  // Pass through
      }
    | full_expression TOK_OR and_expression
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boOr, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
      }
    | full_expression TOK_XOR and_expression
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          ObjPtr pLeft = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
              ToObjPtrVal( $1 ) : nullptr;
          ObjPtr pRight = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
              ToObjPtrVal( $3 ) : nullptr;

          ObjPtr pExpr = pFactory->CreateBinaryExpr(
              CStBinaryExpr::boXor, pLeft, pRight, LOC_RANGE($1, $3) );

          $$ = MAKE_VALUE( Variant( pExpr ), LOC_RANGE($1, $3) );
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
    | TOK_NEQU
    | TOK_LE
    | TOK_GT
    | TOK_NLE
    | TOK_NGT

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
              std::string strOp = STR($2);
              if( strOp == "<>" ) eOp = CStBinaryExpr::boNotEqual;
              else if( strOp == "<" ) eOp = CStBinaryExpr::boLessThan;
              else if( strOp == "<=" ) eOp = CStBinaryExpr::boLessEqual;
              else if( strOp == ">" ) eOp = CStBinaryExpr::boGreaterThan;
              else if( strOp == ">=" ) eOp = CStBinaryExpr::boGreaterEqual;
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
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              // TODO: Extract arguments from arg_list
          }

          ObjPtr pCall = pFactory->CreateCallExpr(
              pCallee, vecArgs, LOC_RANGE($1, $4) );

          $$ = MAKE_VALUE( Variant( pCall ), LOC_RANGE($1, $4) );
      }
    | l_value_ext
      {
          $$ = $1;  // Pass through l_value expression
      }
    | TOK_LPAREN full_expression TOK_RPAREN
      {
          $$ = $2;  // Pass through parenthesized expression
      }
    ;

elseif_branch:
    /* empty */
    | elseif_branch TOK_ELSIF full_expression TOK_THEN block_statements

else_branch:
    /* empty */
    | TOK_ELSE block_statements

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
        std::vector< CStIfStmt::CIfBranch > vecElseIf;
        std::vector< ObjPtr > vecElse;
        // TODO: Extract from block_statements, elseif_branch, else_branch
        
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
    | block_statements_1 semicolons statement
    ;
block_statements:
    block_statements_1 opt_semicolons

for_statement:
    TOK_FOR instance_path TOK_ASSIGN full_expression TOK_TO full_expression opt_by_step TOK_DO block_statements TOK_END_FOR
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strVar = "";  // TODO: Extract from instance_path
        ObjPtr pStart = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        ObjPtr pEnd = ( $6 != nullptr && IsObjPtrVal( $6 ) ) ?
            ToObjPtrVal( $6 ) : nullptr;
        ObjPtr pStep = ( $7 != nullptr && IsObjPtrVal( $7 ) ) ?
            ToObjPtrVal( $7 ) : nullptr;
        std::vector< ObjPtr > vecBody;
        // TODO: Extract from block_statements
        
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
        // TODO: Extract from block_statements
        
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
        // TODO: Extract from block_statements
        ObjPtr pCondition = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        
        ObjPtr pNode = pFactory->CreateRepeatStmt(
            vecBody, pCondition, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }
    ;

positional_args:
    full_expression
    | positional_args TOK_COMMA full_expression

arg_list:
      positional_args
    | param_assignments
    ;

param_assignments:
    param_assignment
    | param_assignments TOK_COMMA param_assignment
    | param_assignments TOK_COMMA error param_assignment
    | param_assignments error TOK_COMMA param_assignment
    ;

param_assignment:
    TOK_ID TOK_ASSIGN full_expression  /* Formal Input: IN := True */
    | TOK_ID TOK_OUTPUT_ASSIGN l_value_ext           /* Formal Output: Q => MyLamp */
    | TOK_ID TOK_OUTPUT_ASSIGN error l_value_ext           /* Formal Output: Q => MyLamp */
    | TOK_ID TOK_ASSIGN error full_expression  /* Formal Input: IN := True */
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
    function_block_header using_directive_list var_declaration method_declaration_list block_statements TOK_END_FUNCTION_BLOCK
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = "";
        CStFunctionBlockDecl::enumFbModifier eModifier = CStFunctionBlockDecl::fbmNone;
        std::string strExtends = "";
        std::vector< std::string > vecImplements;
        // TODO: Extract from header and other components
        std::vector< ObjPtr > vecInput, vecOutput, vecInOut, vecLocal, vecTemp;
        std::vector< ObjPtr > vecMethods, vecStatements;
        
        ObjPtr pNode = pFactory->CreateFunctionBlockDecl(
            strName, eModifier, strExtends, vecImplements,
            vecInput, vecOutput, vecInOut, vecLocal, vecTemp,
            vecMethods, vecStatements, LOC_RANGE($1, $6) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }
    | function_block_header var_declaration method_declaration_list block_statements TOK_END_FUNCTION_BLOCK
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = "";
        CStFunctionBlockDecl::enumFbModifier eModifier = CStFunctionBlockDecl::fbmNone;
        std::string strExtends = "";
        std::vector< std::string > vecImplements;
        
        ObjPtr pNode = pFactory->CreateFunctionBlockDecl(
            strName, eModifier, strExtends, vecImplements,
            {}, {}, {}, {}, {},
            {}, {}, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }

function_block_header:
    TOK_FUNCTION_BLOCK opt_fb_modifier TOK_ID opt_extends_clause opt_implements_clause
    ;

/* Handles ABSTRACT or FINAL keywords */
opt_fb_modifier:
    /* empty */
    | TOK_ABSTRACT
    | TOK_FINAL
    ;

/* Handles EXTENDS <Parent> */
opt_extends_clause:
    /* empty */
    | TOK_EXTENDS TOK_ID
    ;

/* Handles IMPLEMENTS <Interface1, Interface2...> */
opt_implements_clause:
    /* empty */
    | TOK_IMPLEMENTS interface_list
    ;

interface_list:
    identifier_list
    ;

function:
    TOK_FUNCTION TOK_ID var_declarations block_statements TOK_END_FUNCTION
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($2);
        
        // TODO: Extract var_declarations into input/output/local vectors
        std::vector< ObjPtr > vecInput, vecOutput, vecInOut;
        std::vector< ObjPtr > vecLocal, vecTemp, vecStatements;
        
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


opt_semicolon:
    /* empty */
    | TOK_SEMICOLON

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

instance_path : TOK_ID{ }
    | instance_path TOK_DOT TOK_ID  /* e.g., MainProg.Motor1.SensorIn */
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
          // TODO: Extract namespace from instance_path
          
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

