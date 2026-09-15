/*
 * =====================================================================================
 *
 *       Filename:  stparser.yy
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
#include "astnodes.h"
#include "astfactory.h"
#include "astbuilder.h"

using namespace rpcf;

extern std::shared_ptr< rpcf::CSTParserContext > g_pParserCtx;
extern ObjPtr g_pAstRoot;  // Global root AST node

template <typename T >
ObjPtr CreateAstNode()
{
    ObjPtr pNode; 
    pNode.NewObj( ( EnumClsid )AstTraits<T>::id );
    return pNode;
}


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

%token TOK_PROGRAM TOK_VAR TOK_END_VAR TOK_IF TOK_THEN TOK_ELSE TOK_ELSIF TOK_END_IF TOK_FOR TOK_TO TOK_DO TOK_END_FOR TOK_WHILE TOK_END_WHILE TOK_REPEAT TOK_UNTIL TOK_END_REPEAT TOK_CONFIGURATION TOK_END_CONFIGURATION TOK_TASK TOK_SINGLE TOK_INTERVAL TOK_PRIORITY TOK_RESOURCE TOK_END_RESOURCE TOK_ON TOK_READ_ONLY TOK_READ_WRITE
%token TOK_TON TOK_TON_VALUE TOK_STRING TOK_WSTRING TOK_USTRING TOK_INT TOK_REAL TOK_LREAL TOK_BOOL TOK_TRUE TOK_FALSE TOK_TIME TOK_LTIME TOK_TYPED_LITERAL TOK_TYPE TOK_END_TYPE TOK_STRUCT TOK_END_STRUCT TOK_CHAR TOK_WCHAR TOK_UCHAR
%token TOK_UINT TOK_DINT TOK_UDINT TOK_SINT TOK_USINT TOK_BYTE TOK_WORD TOK_DWORD TOK_ULINT TOK_LINT TOK_LWORD TOK_LDWORD

%token TOK_ID TOK_NUMBER TOK_ASSIGN TOK_SEMICOLON TOK_COLON TOK_COMMA TOK_ARRAY TOK_RANGE TOK_DOT TOK_UNDERSCORE
%token TOK_ADD TOK_MINUS TOK_MUL TOK_DIV TOK_MOD TOK_NOT TOK_AND TOK_OR TOK_XOR TOK_DATE TOK_TIME_OF_DAY TOK_DATE_TIME TOK_ABS_ADDR_PERIPHERAL TOK_ABS_ADDR_BIT TOK_ABS_ADDR_BLOCK

%token TOK_EQUAL TOK_POWER TOK_LBRACKET TOK_RBRACKET TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_LE TOK_GT TOK_NEQU TOK_NLE TOK_NGT
%token TOK_EOF TOK_NAMESPACE TOK_END_NAMESPACE TOK_USING
               
%token TOK_FUNCTION_BLOCK TOK_FUNCTION TOK_END_FUNCTION_BLOCK TOK_END_FUNCTION TOK_END_PROGRAM TOK_INCLUDE TOK_INTERFACE TOK_END_INTERFACE
%token TOK_VAR_INPUT TOK_VAR_OUTPUT TOK_VAR_IN_OUT TOK_VAR_GLOBAL TOK_CONSTANT TOK_PUNC TOK_VAR_TEMP TOK_AT TOK_VAR_EXTERNAL TOK_RETAIN TOK_PERSISTENT TOK_VAR_CONFIG TOK_CARET TOK_POINTER TOK_VAR_STAT TOK_OVERLAP TOK_NON_RETAIN TOK_WITH TOK_VAR_ACCESS

%token TOK_TIME_TYPE TOK_TIME_OF_DAY_TYPE TOK_DATE_TYPE TOK_STRING_TYPE TOK_WSTRING_TYPE TOK_USTRING_TYPE TOK_COMMENT TOK_BY TOK_CASE TOK_END_CASE TOK_OF TOK_ABSTRACT TOK_FINAL TOK_EXTENDS TOK_IMPLEMENTS TOK_SUPER TOK_THIS TOK_PRIVATE TOK_PUBLIC TOK_INTERNAL TOK_PROTECTED TOK_REFERENCE TOK_REF_TO TOK_METHOD TOK_END_METHOD TOK_ATTRIBUTE TOK_INFO TOK_REGION TOK_END_REGION TOK_RPCF_ADDR TOK_OUTPUT_ASSIGN TOK_OVERRIDE TOK_REF TOK_NULL
// virtual tokens
%token TOK_VSTART_MAIN TOK_VSTART_PRAGMA TOK_VCASE_SEP TOK_VPUNC TOK_VSEMICOLON TOK_VSUB TOK_VSTART_CASESEL
%token TOK_VSTRUCT TOK_VSIMPLE TOK_VSUBRANGE TOK_VARRAY TOK_VSEP TOK_VSEP_MULTI_PART
%token TOK_PROPERTY_GET TOK_PROPERTY_SET TOK_PROPERTY TOK_END_PROPERTY TOK_MULTPART_ACCESS TOK_PART_ADDR

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
%left TOK_DOT
%right TOK_ASSIGN

%%

start_point:
    TOK_VSTART_MAIN Decls
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
    /* this rule applies to the second push-parser */
    | conditional_pragma
    { $$ = $1; }
    | TOK_EOF
    { $$ = $1; }
    /* this rule applies to the second push-parser */
    | case_selector_check
    { $$ = $1; }
    ;


Decls:
    namespace_elements
    {
        /* Build declarations list from namespace_elements */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStDeclsNode ) );
        CStDeclsNode* pDecls = pNode;
        if( pDecls != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            // Get children from namespace_elements (CStRootNode)
            ObjPtr pNamespaceContent = ToObjPtrVal( $1 );
            CStRootNode* pNsContent = pNamespaceContent;
            if( pNsContent != nullptr )
            {
                // Add all children as declarations
                for( size_t i = 0; i < pNsContent->m_vecChildren.size(); ++i )
                    pDecls->m_vecDecls.push_back( pNsContent->m_vecChildren[ i ] );
            }
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | config_declaration
    {
        /* Create declarations list with config_declaration */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStDeclsNode ) );
        CStDeclsNode* pDecls = pNode;
        if( pDecls != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pDecls->m_vecDecls.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | access_decls
    {
        /* Create declarations list with access_decls */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStDeclsNode ) );
        CStDeclsNode* pDecls = pNode;
        if( pDecls != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pDecls->m_vecDecls.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | Decls config_declaration
    {
        /* Accumulate config_declaration - modify $1 directly */
        if( $1 != nullptr && IsObjPtrVal( $1 ) && $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStDeclsNode* pPrevDecls = pPrev;
            if( pPrevDecls != nullptr )
                pPrevDecls->m_vecDecls.push_back( ToObjPtrVal( $2 ) );
        }
        $$ = $1;
    }
    | Decls access_decls
    {
        /* Accumulate access_decls - modify $1 directly */
        if( $1 != nullptr && IsObjPtrVal( $1 ) && $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStDeclsNode* pPrevDecls = pPrev;
            if( pPrevDecls != nullptr )
                pPrevDecls->m_vecDecls.push_back( ToObjPtrVal( $2 ) );
        }
        $$ = $1;
    }
    | Decls config_declaration namespace_elements
    {
        /* Accumulate config_declaration and namespace_elements - modify $1 directly */
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStDeclsNode* pPrevDecls = pPrev;
            if( pPrevDecls != nullptr )
            {
                if( $2 != nullptr && IsObjPtrVal( $2 ) )
                    pPrevDecls->m_vecDecls.push_back( ToObjPtrVal( $2 ) );
                if( $3 != nullptr && IsObjPtrVal( $3 ) )
                {
                    // Get children from namespace_elements
                    ObjPtr pNamespaceContent = ToObjPtrVal( $3 );
                    CStRootNode* pNsContent = pNamespaceContent;
                    if( pNsContent != nullptr )
                    {
                        for( size_t i = 0; i < pNsContent->m_vecChildren.size(); ++i )
                            pPrevDecls->m_vecDecls.push_back( pNsContent->m_vecChildren[ i ] );
                    }
                }
            }
        }
        $$ = $1;
    }
    | Decls access_decls namespace_elements
    {
        /* Accumulate access_decls and namespace_elements - modify $1 directly */
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStDeclsNode* pPrevDecls = pPrev;
            if( pPrevDecls != nullptr )
            {
                if( $2 != nullptr && IsObjPtrVal( $2 ) )
                    pPrevDecls->m_vecDecls.push_back( ToObjPtrVal( $2 ) );
                if( $3 != nullptr && IsObjPtrVal( $3 ) )
                {
                    // Get children from namespace_elements
                    ObjPtr pNamespaceContent = ToObjPtrVal( $3 );
                    CStRootNode* pNsContent = pNamespaceContent;
                    if( pNsContent != nullptr )
                    {
                        for( size_t i = 0; i < pNsContent->m_vecChildren.size(); ++i )
                            pPrevDecls->m_vecDecls.push_back( pNsContent->m_vecChildren[ i ] );
                    }
                }
            }
        }
        $$ = $1;
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
    global_var_decls
    { $$ = $1; }
    | data_type_decl
    { $$ = $1; }
    | pragma_statement
    { $$ = $1; }
    | func_decl
    { $$ = $1; }
    | prog_decl
    { $$ = $1; }
    | fb_decl
    { $$ = $1; }
    | class_decl
    { $$ = $1; }
    | interface_decl
    { $$ = $1; }
    | namespace_decl
    { $$ = $1; }
    ;

/* 1. Main Configuration Rule */
config_declaration :
    TOK_CONFIGURATION TOK_ID
    opt_global_var_decls
    resource_section
    access_decls_opt
    opt_config_init
    TOK_END_CONFIGURATION
    {
        /* Build configuration declaration node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStConfigDeclNode ) );
        CStConfigDeclNode* pConfig = pNode;
        if( pConfig != nullptr )
        {
            pConfig->m_strConfigName = ID($2);
            if( $3 != nullptr && IsObjPtrVal( $3 ) )
                pConfig->m_pGlobalVars = ToObjPtrVal( $3 );
            if( $4 != nullptr && IsObjPtrVal( $4 ) )
                pConfig->m_pResourceSection = ToObjPtrVal( $4 );
            if( $5 != nullptr && IsObjPtrVal( $5 ) )
                pConfig->m_pAccessDecls = ToObjPtrVal( $5 );
            if( $6 != nullptr && IsObjPtrVal( $6 ) )
                pConfig->m_pConfigInit = ToObjPtrVal( $6 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $7) );
    }
    ;

/* 2. Handles Global_Var_Decls* (Zero or More) */
opt_global_var_decls :
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | global_var_decls
    ;

single_resource_decl:
    task_config_list prog_config_list
    {
        /* Build single resource node with task and prog lists */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStSingleResourceDeclNode ) );
        CStSingleResourceDeclNode* pSingle = pNode;
        if( pSingle != nullptr )
        {
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pSingle->m_pTaskList = ToObjPtrVal( $1 );
            if( $2 != nullptr && IsObjPtrVal( $2 ) )
                pSingle->m_pProgList = ToObjPtrVal( $2 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    | prog_config_list
    {
        /* Build single resource node with only prog list */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStSingleResourceDeclNode ) );
        CStSingleResourceDeclNode* pSingle = pNode;
        if( pSingle != nullptr )
        {
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pSingle->m_pProgList = ToObjPtrVal( $1 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }

task_config_list:
    task_config TOK_SEMICOLON
    {
        /* First task - create accumulator */
        ObjPtr pList;
        pList.NewObj( clsid( CStTaskConfigListNode ) );
        CStTaskConfigListNode* pTaskList = pList;
        if( pTaskList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pTaskList->m_vecTasks.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
    }
    | task_config_list task_config TOK_SEMICOLON
    {
        /* Accumulate tasks */
        ObjPtr pList;
        pList.NewObj( clsid( CStTaskConfigListNode ) );
        CStTaskConfigListNode* pTaskList = pList;
        if( pTaskList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStTaskConfigListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pTaskList->m_vecTasks = pPrevList->m_vecTasks;
        }
        if( pTaskList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
            pTaskList->m_vecTasks.push_back( ToObjPtrVal( $2 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $2) );
    }
    ;

task_config:
    TOK_TASK TOK_ID task_init
    {
        /* Attach task name to the task config node */
        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            ObjPtr pTask = ToObjPtrVal( $3 );
            CStTaskConfigNode* pNode = pTask;
            if( pNode != nullptr )
                pNode->m_strTaskName = ID($2);
        }
        $$ = $3;
    }

task_init:
    TOK_LPAREN opt_single_init opt_interval_init priority_init TOK_RPAREN
    {
        /* Build a task configuration node with single, interval, and priority */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStTaskConfigNode ) );
        CStTaskConfigNode* pTask = pNode;
        if( pTask != nullptr )
        {
            pTask->m_pSingle = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
                ToObjPtrVal( $2 ) : nullptr;
            pTask->m_pInterval = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
                ToObjPtrVal( $3 ) : nullptr;
            pTask->m_pPriority = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
                ToObjPtrVal( $4 ) : nullptr;
            pTask->SetLocation( LOC_RANGE($1, $5) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }

opt_single_init:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_SINGLE TOK_ASSIGN data_source TOK_COMMA
    {
        $$ = $3;
    }

opt_interval_init:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_INTERVAL TOK_ASSIGN data_source TOK_COMMA
    {
        $$ = $3;
    }

priority_init:
    TOK_PRIORITY TOK_ASSIGN TOK_NUMBER
    {
        /* priority is an unsigned int number (type node), not a value */
        Variant oVar = NUM( $3 );
        $$ = MAKE_VALUE( oVar, LOC_RANGE($1, $3) );
    }

data_source:
    /* semantic checks needed */
    constant
    { $$ = $1; }
    | global_var_access
    | direct_variable
    | prog_output_access
    ;

array_var_decl:
    variable_list TOK_DOT array_spec
    ;

struct_var_decl:
    variable_list TOK_DOT struct_type_access

variable_list:
    identifier_list
    ;

struct_type_access:
    instance_path

interface_var_decl:
    variable_list TOK_DOT instance_path

prog_output_access:
    tokid_dot symbolic_var

prog_config_list:
    prog_config TOK_SEMICOLON
    {
        /* First program config - create accumulator */
        ObjPtr pList;
        pList.NewObj( clsid( CStProgConfigListNode ) );
        CStProgConfigListNode* pProgList = pList;
        if( pProgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pProgList->m_vecProgs.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
    }
    | prog_config_list prog_config TOK_SEMICOLON
    {
        /* Accumulate program configs */
        ObjPtr pList;
        pList.NewObj( clsid( CStProgConfigListNode ) );
        CStProgConfigListNode* pProgList = pList;
        if( pProgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStProgConfigListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pProgList->m_vecProgs = pPrevList->m_vecProgs;
        }
        if( pProgList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
            pProgList->m_vecProgs.push_back( ToObjPtrVal( $2 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $2) );
    }
    ;

prog_config:
    TOK_PROGRAM opt_retain TOK_ID opt_with_task TOK_COLON prog_type_access opt_prog_conf_elems
    {
        /* Build a program configuration node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStProgConfigNode ) );
        CStProgConfigNode* pProg = pNode;
        if( pProg != nullptr )
        {
            // Program name from TOK_ID
            pProg->m_strProgName = ID($3);

            // Retain type from opt_retain
            pProg->m_eRetain = 
                ( decltype( CStProgConfigNode::retainNone ) )NUM($2);

            // Task name from opt_with_task
            if( $4 != nullptr && IsObjPtrVal( $4 ) )
            {
                // opt_with_task returns the TOK_ID token when present
                pProg->m_strTaskName = ID($4);
            }

            // Program type (instance_path) from prog_type_access
            if( $6 != nullptr && IsObjPtrVal( $6 ) )
                pProg->m_pProgType = ToObjPtrVal( $6 );

            // Params from opt_prog_conf_elems
            if( $7 != nullptr && IsObjPtrVal( $7 ) )
            {
                ObjPtr pParams = ToObjPtrVal( $7 );
                CStProgConfigListNode* pParamList = pParams;
                if( pParamList != nullptr )
                    pProg->m_vecParams = pParamList->m_vecProgs;
            }

            pProg->SetLocation( LOC_RANGE($1, $7) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $7) );
    }

opt_retain:
    /* empty */
    {
        $$ = MAKE_VALUE( ( guint32 )0, YYLTYPE2() );  // retainNone
    }
    | TOK_RETAIN
    {
        $$ = MAKE_VALUE( ( guint32 )1, LOC($1) );  // retainRetain
    }
    | TOK_NON_RETAIN
    {
        $$ = MAKE_VALUE( ( guint32 )2, LOC($1) );  // retainNonRetain
    }

opt_with_task:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_WITH TOK_ID
    {
        $$ = $2;
    }

prog_type_access:
    /* semantic checks needed */
    instance_path
    {
        $$ = $1;
    }

opt_prog_conf_elems:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_LPAREN prog_conf_elems TOK_RPAREN
    {
        $$ = $2;
    }
    ;
prog_conf_elems: prog_conf_elem
    {
        /* First param - create accumulator */
        ObjPtr pList;
        pList.NewObj( clsid( CStProgConfigListNode ) );
        CStProgConfigListNode* pParamList = pList;
        if( pParamList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pParamList->m_vecProgs.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
    }
    | prog_conf_elems TOK_COMMA prog_conf_elem
    {
        /* Accumulate more params */
        ObjPtr pList;
        pList.NewObj( clsid( CStProgConfigListNode ) );
        CStProgConfigListNode* pParamList = pList;
        if( pParamList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStProgConfigListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pParamList->m_vecProgs = pPrevList->m_vecProgs;
        }
        if( pParamList != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
            pParamList->m_vecProgs.push_back( ToObjPtrVal( $3 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
    }

prog_conf_elem: fb_task
    {
        /* Wrap fb_task in prog_conf_elem node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStProgConfElemNode ) );
        CStProgConfElemNode* pElem = pNode;
        if( pElem != nullptr )
        {
            pElem->m_eType = CStProgConfElemNode::elemFbTask;
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pElem->m_pContent = ToObjPtrVal( $1 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | prog_cnxn
    {
        /* Wrap prog_cnxn in prog_conf_elem node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStProgConfElemNode ) );
        CStProgConfElemNode* pElem = pNode;
        if( pElem != nullptr )
        {
            pElem->m_eType = CStProgConfElemNode::elemProgCnxn;
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pElem->m_pContent = ToObjPtrVal( $1 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }

fb_task: fb_instance_name TOK_WITH TOK_ID

fb_instance_name: instance_path
    {
        /* Build fb_task node with instance_path */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStFbTaskNode ) );
        CStFbTaskNode* pFbTask = pNode;
        if( pFbTask != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            pFbTask->m_pInstancePath = ToObjPtrVal( $1 );
            pFbTask->m_iCaretCount = 0;
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | instance_path caret_list
    {
        /* Build fb_task node with instance_path and caret count */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStFbTaskNode ) );
        CStFbTaskNode* pFbTask = pNode;
        if( pFbTask != nullptr )
        {
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pFbTask->m_pInstancePath = ToObjPtrVal( $1 );
            if( $2 != nullptr )
                pFbTask->m_iCaretCount = NUM($2);
            else
                pFbTask->m_iCaretCount = 0;
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }

caret_list: TOK_CARET
    {
        /* Accumulate caret count */
        $$ = MAKE_VALUE( ( guint32 )1, LOC($1) );
    }
    | caret_list TOK_CARET
    {
        /* Increment caret count */
        guint32 iCount = NUM($1);
        $$ = MAKE_VALUE( ( guint32 )( iCount + 1 ), LOC_RANGE($1, $2) );
    }
    ;

prog_cnxn:
    symbolic_var TOK_ASSIGN full_expression TOK_OUTPUT_ASSIGN data_sink
    {
        /* Build prog_cnxn node with all components */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStProgCnxnNode ) );
        CStProgCnxnNode* pCnxn = pNode;
        if( pCnxn != nullptr )
        {
            // Store symbolic_var ($1)
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pCnxn->m_pInputVar = ToObjPtrVal( $1 );
            // Store full_expression ($3)
            if( $3 != nullptr && IsObjPtrVal( $3 ) )
                pCnxn->m_pExpression = ToObjPtrVal( $3 );
            // Store data_sink ($5)
            if( $5 != nullptr && IsObjPtrVal( $5 ) )
                pCnxn->m_pOutputSink = ToObjPtrVal( $5 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
    }

data_sink:
    /* semantic checks needed */
    global_var_access
    { $$ = $1; }
    | direct_variable
    { $$ = $1; }

tokid_dot:
    TOK_ID TOK_DOT/* one of the enum value */
    {
        $$ = $1;
    }
    ;

opt_tokid_dot:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | tokid_dot
    {
        $$ = $1;
    }
    ;

opt_dot_tokid:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | dot_tokid
    {
        $$ = $1;
    }
    ;

dot_tokid:
    TOK_ID TOK_DOT
    {
        $$ = $1;
    }

global_var_access:
    opt_tokid_dot TOK_ID opt_dot_tokid
    ;

symbolic_variable:
    var_access_or_multi_elem
    | this_notation var_access_or_multi_elem
    | instance_path TOK_DOT var_access_or_multi_elem
    ;

var_access_or_multi_elem:
    multi_elem_var
    | var_access

var_access:
    ref_deref
    | variable_name
    ;

variable_name:
    TOK_ID
    ;

multi_elem_var:
    var_access multi_elem_chain

multi_elem_chain:
    multi_elem_arr_struct
    | multi_elem_chain multi_elem_arr_struct 


multi_elem_arr_struct:
    subscript_list
    | struct_variable
    ;

struct_variable:
    TOK_DOT struct_elem_select

struct_elem_select:
    var_access
    
subscript_list:
    TOK_LBRACKET expr_list TOK_RBRACKET
    {
        /* Pass through the expr_list */
        $$ = $2;
    }

expr_list:
    full_expression
    {
        /* First expression - create list */
        ObjPtr pList;
        pList.NewObj( clsid( CStArgListNode ) );
        CStArgListNode* pArgs = pList;
        if( pArgs != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pArgs->m_vecArgs.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
    }
    | expr_list TOK_COMMA full_expression
    {
        /* Accumulate more expressions */
        ObjPtr pList;
        pList.NewObj( clsid( CStArgListNode ) );
        CStArgListNode* pArgs = pList;
        if( pArgs != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStArgListNode* pPrevArgs = pPrev;
            if( pPrevArgs != nullptr )
                pArgs->m_vecArgs = pPrevArgs->m_vecArgs;
        }
        if( pArgs != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
            pArgs->m_vecArgs.push_back( ToObjPtrVal( $3 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $3) );
    }

this_notation:
    TOK_THIS TOK_DOT
    { $$ = $1; }

symbolic_var:
    symbolic_term_list

symbolic_term_list:
    symbolic_term
    | symbolic_term_list TOK_DOT symbolic_term
    ;
    
symbolic_term_postfix:
    subscript_list
    | caret_list
    ;

symbolic_term:
    identifier_dot_list
    | identifier_dot_list symbolic_term_postfix
    ;

/* 3. Handles (Single_Resource_Decl | Resource_Decl+) */
resource_section
    : single_resource_decl
    {
        /* Wrap single resource in resource section node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStResourceSectionNode ) );
        CStResourceSectionNode* pSection = pNode;
        if( pSection != nullptr )
        {
            pSection->m_eType = CStResourceSectionNode::rtSingle;
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pSection->m_pContent = ToObjPtrVal( $1 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | resource_decl_list
    {
        /* Wrap resource list in resource section node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStResourceSectionNode ) );
        CStResourceSectionNode* pSection = pNode;
        if( pSection != nullptr )
        {
            pSection->m_eType = CStResourceSectionNode::rtList;
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
                pSection->m_pContent = ToObjPtrVal( $1 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

/* Helper for Resource_Decl+ (One or More Explicit Resources) */
resource_decl_list :
    resource_declaration
    {
        /* First resource - create accumulator */
        ObjPtr pList;
        pList.NewObj( clsid( CStResourceDeclListNode ) );
        CStResourceDeclListNode* pResList = pList;
        if( pResList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pResList->m_vecResources.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
    }
    | resource_decl_list resource_declaration
    {
        /* Accumulate resources */
        ObjPtr pList;
        pList.NewObj( clsid( CStResourceDeclListNode ) );
        CStResourceDeclListNode* pResList = pList;
        if( pResList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStResourceDeclListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pResList->m_vecResources = pPrevList->m_vecResources;
        }
        if( pResList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
            pResList->m_vecResources.push_back( ToObjPtrVal( $2 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $2) );
    }
    ;

resource_declaration:
    TOK_RESOURCE TOK_ID TOK_ON TOK_ID opt_global_var_decls single_resource_decl TOK_END_RESOURCE
    {
        /* Build resource declaration node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStResourceDeclNode ) );
        CStResourceDeclNode* pRes = pNode;
        if( pRes != nullptr )
        {
            pRes->m_strResourceName = ID($2);
            pRes->m_strResourceType = ID($4);
            if( $5 != nullptr && IsObjPtrVal( $5 ) )
                pRes->m_pGlobalVars = ToObjPtrVal( $5 );
            if( $6 != nullptr && IsObjPtrVal( $6 ) )
                pRes->m_pSingleResource = ToObjPtrVal( $6 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $7) );
    }

/* 4. Handles Access_Decls? (Optional) */
access_decls_opt
    : /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | access_decls
    {
        /* Pass through access_decls result */
        $$ = $1;
    }
    ;

access_decls:
    TOK_VAR_ACCESS TOK_END_VAR
    {
        /* Empty access declarations - create wrapper with empty list */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStAccessDeclsNode ) );
        CStAccessDeclsNode* pDecls = pNode;
        if( pDecls != nullptr )
        {
            // Create empty list
            ObjPtr pList;
            pList.NewObj( clsid( CStAccessDeclListNode ) );
            pDecls->m_pAccessDeclList = pList;
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    | TOK_VAR_ACCESS access_decl_list TOK_END_VAR
    {
        /* Wrap access_decl_list in access_decls node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStAccessDeclsNode ) );
        CStAccessDeclsNode* pDecls = pNode;
        if( pDecls != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
            pDecls->m_pAccessDeclList = ToObjPtrVal( $2 );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
    }

access_decl_list:
    access_decl TOK_SEMICOLON
    {
        /* First access declaration - create accumulator */
        ObjPtr pList;
        pList.NewObj( clsid( CStAccessDeclListNode ) );
        CStAccessDeclListNode* pDeclList = pList;
        if( pDeclList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pDeclList->m_vecAccessDecls.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC($1) );
    }
    | access_decl_list access_decl TOK_SEMICOLON
    {
        /* Accumulate access declarations */
        ObjPtr pList;
        pList.NewObj( clsid( CStAccessDeclListNode ) );
        CStAccessDeclListNode* pDeclList = pList;
        if( pDeclList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStAccessDeclListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pDeclList->m_vecAccessDecls = pPrevList->m_vecAccessDecls;
        }
        if( pDeclList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
            pDeclList->m_vecAccessDecls.push_back( ToObjPtrVal( $2 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $2) );
    }

access_decl:
    TOK_ID TOK_COLON access_path TOK_COLON type_spec opt_access_dir
    {
        /* Build access declaration node */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode;
        pNode.NewObj( clsid( CStAccessDeclNode ) );
        CStAccessDeclNode* pDecl = pNode;
        if( pDecl != nullptr )
        {
            pDecl->m_strId = ID($1);
            if( $3 != nullptr && IsObjPtrVal( $3 ) )
                pDecl->m_pAccessPath = ToObjPtrVal( $3 );
            if( $5 != nullptr && IsObjPtrVal( $5 ) )
                pDecl->m_pTypeSpec = ToObjPtrVal( $5 );
            // opt_access_dir: 0 = none, 1 = READ_ONLY, 2 = READ_WRITE
            guint32 nDir = NUM($6);
            pDecl->m_bReadOnly = ( nDir == 1 );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }

access_path:
    /* semantic checks needed */
    instance_path
    { $$ = $1; }

opt_access_dir:
    /* empty */
    {
        $$ = MAKE_VALUE( ( guint32 )0, YYLTYPE2() );  // none
    }
    | TOK_READ_ONLY
    {
        $$ = MAKE_VALUE( ( guint32 )1, LOC($1) );  // READ_ONLY
    }
    | TOK_READ_WRITE
    {
        $$ = MAKE_VALUE( ( guint32 )2, LOC($1) );  // READ_WRITE
    }

/* 5. Handles Config_Init? (Optional) */
opt_config_init
    : /* empty */
    {
        $$ = MAKE_VALUE( ( guint32 )0, YYLTYPE2() );  // none
    }
    /* semantic check with the EBNF rule config_inst_init in the IEC 61131-3(2025) spec. */
    | config_init
    { $$ = $1; }
    ;

namespace_decl
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

opt_retain_or_constant:
    /* empty */
    |TOK_RETAIN
    | TOK_CONSTANT
    ;

global_var_decls:
    TOK_VAR_GLOBAL opt_internal opt_retain_or_constant opt_global_var_decl_list TOK_END_VAR
    ;

opt_global_var_decl_list:
    /* empty */
    | global_var_decl_list
    ;

global_var_decl_list:
    global_var_decl TOK_SEMICOLON
    | global_var_decl_list global_var_decl
    ;

global_var_decl:
    global_var_spec TOK_COLON global_var_type_set

global_var_spec:
    identifier_list
    | TOK_ID locate_at
    ;

global_var_type_set:
    loc_var_spec_init
    | fb_type_access
    ;


program:
    TOK_PROGRAM TOK_ID opt_using_directive_list var_declarations body
        TOK_END_PROGRAM
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($2);

          /* The using directives, the var declarations and the
             body are all visible directly; no carrier needed */
          std::vector< ObjPtr > vecStatements;
          std::vector< ObjPtr > vecInput, vecOutput, vecInOut;
          std::vector< ObjPtr > vecLocal, vecTemp;
          std::vector< ObjPtr > vecOther;
          std::vector< std::string > vecUsing;

          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              /* the accumulated namespace names, e.g. 'A.B' */
              ObjPtr pList = ToObjPtrVal( $3 );
              CStIdentifierListNode* pIdList = pList;
              if( pIdList != nullptr )
                  vecUsing = pIdList->m_vecIdentifiers;
          }
          if( $4 != nullptr && IsObjPtrVal( $4 ) )
          {
              /* Split the accumulated declarations by category */
              SplitVarDeclList( ToObjPtrVal( $4 ),
                  vecInput, vecOutput, vecInOut,
                  vecLocal, vecTemp, vecOther );
          }
          if( $5 != nullptr && IsObjPtrVal( $5 ) )
          {
              ObjPtr pList = ToObjPtrVal( $5 );
              CStStmtListNode* pStmtList = pList;
              if( pStmtList != nullptr )
                  vecStatements = pStmtList->m_vecStatements;
          }

          ObjPtr pProgram = pFactory->CreateProgramDecl(
              strName, vecInput, vecOutput, vecInOut,
              vecLocal, vecTemp, vecStatements, vecUsing,
              LOC_RANGE($1, $6) );

          $$ = MAKE_VALUE( Variant( pProgram ), LOC_RANGE($1, $6) );
      }
    ;

/* Handles the optional USING directives of a program */
opt_using_directive_list:
    /* empty */
      {
          /* no USING clause: an empty identifier list */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
      }
    | using_directive_list
      {
          $$ = $1;
      }
    ;

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

data_type_decl:
      TOK_TYPE type_decl TOK_SEMICOLON TOK_END_TYPE
      | TOK_TYPE TOK_OVERRIDE type_decl TOK_SEMICOLON TOK_END_TYPE
      | TOK_TYPE using_directive_list type_decl TOK_SEMICOLON TOK_END_TYPE
      | TOK_TYPE TOK_OVERRIDE using_directive_list type_decl TOK_SEMICOLON TOK_END_TYPE
      ;

type_decl:
    simple_type_decl
    | subrange_type_decl
    | enum_type_decl
    | namedval_type_decl
    | array_type_decl
    | struct_type_decl_init
    | string_type_decl
    | ref_type_decl
    | ambiguous_type_decl
    ;

spec_type_accesses:
    simple_type_access
    {
        // FIXME:  must be aware the derived type can be one of the simple type or
        // array type, or subrange type
    }
    | struct_spec_init
    ;

ambiguous_type_decl:
    TOK_ID TOK_COLON instance_path 
    | TOK_ID TOK_COLON instance_path TOK_ASSIGN spec_type_accesses
    

simple_type_decl:
    TOK_ID TOK_COLON simple_spec_init
    ;
simple_spec_init:
    simple_spec 
    | simple_spec TOK_ASSIGN full_expression
    ;
simple_spec:
    elem_type_name
//    | simple_type_access
    ;
simple_type_access:
    instance_path
    ;

subrange_spec:
    int_type_name TOK_LPAREN range TOK_RPAREN 
//    | subrange_type_access
    ;

// subrange_type_access:
//     instance_path
    ;
subrange_spec_init:
    subrange_spec
    | subrange_spec TOK_ASSIGN TOK_NUMBER
    { /* TOK_NUMBER must be an int */ }
    ;

subrange_type_decl:
    TOK_ID TOK_COLON subrange_spec_init
    ;
namedval_type_decl: 
    TOK_ID TOK_COLON multibits_type_name namedval_spec_init
    TOK_ID TOK_COLON int_type_name namedval_spec_init
    ;

namedval_spec_init:
    TOK_LPAREN namedval_spec_list TOK_RPAREN
    | TOK_LPAREN namedval_spec_list TOK_RPAREN TOK_ASSIGN full_expression
    ;
namedval_spec_list:
    namedval_spec
    | namedval_spec_list TOK_COMMA namedval_spec
    ;

namedval_spec:
    TOK_ID TOK_ASSIGN full_expression
    ;

array_type_decl:
    TOK_ID TOK_COLON array_spec_init
    ;
array_spec_init:
    array_spec
    | array_spec TOK_ASSIGN array_init
    ;
array_spec:
    TOK_ARRAY TOK_LBRACKET range_list TOK_RBRACKET TOK_OF data_type_access
    // | array_type_access
    ;
// array_type_access:
//     instance_path

array_init:
    TOK_LBRACKET array_elem_init_list TOK_RBRACKET
    ;
array_elem_init_list:
    array_elem_init
    | array_elem_init_list TOK_COMMA array_elem_init
    ;
array_elem_init:
    array_elem_init_value
    | TOK_NUMBER TOK_LPAREN array_elem_init_list TOK_RPAREN
    { /* TOK_NUMBER must be uint*/ }
    ; 
array_elem_init_value:
    initial_value
    | instance_path TOK_PUNC TOK_ID
    {
        /* TODO: check enum value */
    }
    ;

struct_type_decl:
    TOK_ID TOK_COLON struct_decl
    
struct_type_decl_init:
    TOK_ID TOK_COLON struct_spec
    ;

struct_spec:
    struct_decl
//    | struct_spec_init
    ;

struct_spec_init:
    instance_path TOK_ASSIGN struct_init
    ;

struct_init:
    TOK_LPAREN struct_elem_init_list TOK_RPAREN

ref_type_decl:
    TOK_ID TOK_COLON ref_spec_init

ref_spec_init:
    ref_spec 
    | ref_spec TOK_ASSIGN ref_value

ref_spec: TOK_REF_TO data_type_access
ref_value:
    ref_addr
    | TOK_NULL

ref_addr:
    TOK_REF TOK_RPAREN symbolic_var TOK_RPAREN

ref_name : TOK_ID
    ;

ref_deref:
    ref_name caret_list

/*ref_assign:
    ref_name TOK_ASSIGN ref_name 
    | ref_name TOK_ASSIGN ref_value
    | ref_name TOK_ASSIGN ref_deref
    ;
    */

ref_var_decl:
    identifier_list TOK_COLON ref_spec

fb_var_decls:
    fb_io_var_decls
    | func_var_decls
    | temp_var_decls
    | other_var_decls
    ;

other_var_decls:
    retain_var_decls
    | no_retain_var_decls
    ;

retain_var_decls:
    TOK_VAR TOK_RETAIN opt_access_spec opt_var_decls_init_set TOK_END_VAR

no_retain_var_decls:
    TOK_VAR TOK_NON_RETAIN opt_access_spec opt_var_decls_init_set TOK_END_VAR

fb_io_var_decls:
    fb_input_decls
    | fb_output_decls
    | in_out_decls
    ;

fb_input_decls:
    TOK_VAR_INPUT opt_retain opt_fb_input_decl_list TOK_END_VAR

opt_fb_input_decl_list:
    /* empty */
    | fb_input_decl_list

fb_input_decl_list:
    fb_input_decl TOK_SEMICOLON
    | fb_input_decl_list fb_input_decl
    ;

fb_input_decl:
    var_decl_init_set

fb_output_decls:
    TOK_VAR_OUTPUT opt_retain opt_fb_output_decl_list TOK_END_VAR

opt_fb_output_decl_list:
    /* empty */
    | fb_output_decl_list
    ;

fb_output_decl_list:
    fb_output_decl TOK_SEMICOLON
    | fb_output_decl_list fb_output_decl

fb_output_decl:
    var_decl_init_set

in_out_decls:
    TOK_VAR_IN_OUT opt_in_out_var_decl_list TOK_END_VAR

opt_in_out_var_decl_list:
    /* empty */
    | in_out_var_decl_list
    ;

in_out_var_decl_list:
    in_out_var_decl TOK_SEMICOLON
    | in_out_var_decl_list in_out_var_decl
    ;

in_out_var_decl:
    var_decl
    | fb_decl_no_init
    | array_conform_decl
    ;

array_conform_decl:
    variable_list TOK_COLON array_conformand

array_conformand:
    TOK_ARRAY TOK_LBRACKET varied_length_dim_list TOK_RBRACKET TOK_OF data_type_access    
    
    
method_or_property:
    method_declaration_list
    | property_decl
    ;

method_or_property_list:
    method_or_property
    | method_or_property_list method_or_property
    ;

fb_decl:
    TOK_FUNCTION_BLOCK TOK_INTERNAL opt_fb_modifier derived_fb_name opt_using_directive_list
    opt_extends_clause opt_implements_clause fb_var_decls 
    method_or_property_list opt_body TOK_END_FUNCTION_BLOCK
    | TOK_FUNCTION_BLOCK opt_fb_modifier derived_fb_name opt_using_directive_list
    opt_extends_clause opt_implements_clause fb_var_decls 
    method_or_property_list opt_body TOK_END_FUNCTION_BLOCK

derived_fb_name:
    TOK_ID

setter_or_getter:
    TOK_PROPERTY_SET
    | TOK_PROPERTY_GET
    ;

property_decl:
    setter_or_getter opt_access_spec opt_fb_modifier opt_override TOK_ID TOK_COLON data_type_access 
    opt_property_var_decl opt_body TOK_END_PROPERTY
    ;

opt_property_var_decl:
    func_var_decls
    | temp_var_decls
    ;

opt_data_type_access:
    /* empty */
    | TOK_COLON data_type_access
    ;

opt_internal:
    /* empty */
    | TOK_INTERNAL
    ;

func_var_decl_set:
    io_var_decls
    | func_var_decls
    | temp_var_decls
    ;

io_var_decls:
    input_decls
    | output_decls
    | in_out_decls

input_decls:
    TOK_VAR_INPUT opt_retain opt_input_decl_list TOK_END_VAR

opt_input_decl_list :
    /* empty */
    | input_decl_list
    ;

input_decl_list:
    input_decl TOK_SEMICOLON
    | input_decl_list input_decl
    ;

input_decl:
    var_decl_init 
    | array_conform_decl

output_decls:
    TOK_VAR_OUTPUT opt_retain opt_output_decl_list TOK_END_VAR

opt_output_decl_list :
    opt_input_decl_list
    ;

prog_decl:
    TOK_PROGRAM opt_internal TOK_ID opt_prog_var_decls_set body TOK_END_PROGRAM


opt_prog_var_decls_set:
    /* empty */
    | prog_var_decls_set_list

prog_var_decls_set_list:
    prog_var_decls_set TOK_SEMICOLON
    | prog_var_decls_set_list prog_var_decls_set
    ;

prog_var_decls_set:
    fb_io_var_decls
    | func_var_decls
    | global_var_decls
    | temp_var_decls
    | other_var_decls
    | prog_access_decls
    ;

prog_access_decls:
    TOK_VAR_ACCESS opt_prog_access_decl_list TOK_END_VAR

opt_prog_access_decl_list:
    /* empty */
    | prog_access_decl_list
    ;

prog_access_decl_list:
    prog_access_decl TOK_SEMICOLON
    | prog_access_decl_list prog_access_decl

prog_access_decl:
    TOK_ID TOK_COLON symbolic_var TOK_DOT multi_part_access


func_decl:
    TOK_FUNCTION opt_internal derived_func_name opt_data_type_access opt_using_directive_list 
    ;

func_var_decls:
    external_var_decls
    | var_decls
    ;

temp_var_member:
    var_decl
    | ref_var_decl
    | interface_var_decl

temp_var_member_list:
    temp_var_member TOK_SEMICOLON
    | temp_var_member_list temp_var_member

opt_temp_var_member_list:
    /* empty */
    | temp_var_member_list

temp_var_decls:
    TOK_VAR_TEMP opt_temp_var_member_list TOK_END_VAR

opt_constant:
    /* empty */
    | TOK_CONSTANT
    ;

var_decl:
    identifier_list TOK_COLON var_member_init

var_member_init:
    simple_spec
    | string_spec
    | array_var_decl
    | struct_var_decl
    | struct_type_decl

opt_var_decls_init_set:
    /* empty */
    | var_decls_init_set
    ;

var_decls_init_set:
    var_decl_init
    | loc_var_decl
    | loc_partly_var
    ;

loc_var_decl:
    TOK_ID locate_at TOK_COLON loc_var_spec_init
    ;

loc_var_spec_init: 
    simple_spec_init
    | array_spec_init
    | struct_spec_init
    | string_spec_init
    ;

loc_partly_var:
    TOK_ID TOK_AT TOK_PART_ADDR

var_decls:
    TOK_VAR opt_constant opt_access_spec opt_var_decls_init_set TOK_END_VAR

var_decl_init:
    variable_list TOK_COLON var_decl_init_set
    ;

var_decl_init_simple_set:
    simple_spec_init
    | subrange_spec_init
    | ref_spec_init
    | string_spec_init
    | ambiguous_type_decl
    ;

var_decl_init_set:
    var_decl_init_simple_set
    |array_var_decl_init
    | struct_var_decl_init
    | fb_decl_init
    | interface_spec_init
    ;

interface_spec_init:
    variable_list
    | variable_list TOK_ASSIGN interface_value

interface_value:
    symbolic_var 
    | TOK_NULL
//    | fb_instance_name
//    | class_instance_name
    ;

array_var_decl_init:
    variable_list TOK_COLON array_spec_init

struct_var_decl_init:
    variable_list TOK_COLON struct_spec_init

fb_decl_init:
    fb_decl_no_init 
    | fb_decl_no_init TOK_ASSIGN struct_init

fb_decl_no_init:
    variable_list TOK_COLON fb_type_access

fb_type_access:
    instance_path

external_var_decls:
    TOK_VAR_EXTERNAL opt_constant opt_external_decls TOK_END_VAR

opt_external_decls:
    /* empty */
    | external_decls
    ;

external_decls:
    external_decl TOK_SEMICOLON
    | external_decls external_decl

external_decl:
   instance_path TOK_COLON external_member_init 

external_member_init:
    simple_spec
    | array_spec
    | instance_path
    
opt_body:
    /* empty */
    | body

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
      enum_value1
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStEnumValueListNode ) );
          CStEnumValueListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pList->m_vecValues.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | enum_value_list TOK_COMMA enum_value1
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
    | enum_value_list error enum_value1
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

enum_value1:
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
    {
        /* no explicit base type */
        $$ = MAKE_EMPTY();
    }
    | int_type_name
    {
        $$ = $1;
    }
    ;

 /* the default initialization for variables of this enum */
opt_assign_enum_val:
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_ID /* one of the enum value */
    {
        $$ = $1;
    }
    | TOK_ID TOK_PUNC TOK_ID /* enum_type_name#member */
    {
        /* keep the qualified name; enum_type_definition reads it with STR() */
        $$ = MAKE_VALUE( Variant( ID($1) + "#" + ID($3) ),
            LOC_RANGE($1, $3) );
    }
    ;

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

enum_type_decl:
    TOK_ID TOK_COLON enum_spec_init

enum_spec_init:
    TOK_LPAREN identifier_list TOK_RPAREN
    | TOK_LPAREN identifier_list TOK_RPAREN TOK_ASSIGN enum_value

enum_type_access:
    instance_path

enum_value:
    TOK_ID
    | enum_type_access TOK_PUNC TOK_ID

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

    | TOK_ID TOK_COLON struct_decl semicolons   {
        /* Struct: TYPE Motor : STRUCT... */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($1);
        ObjPtr pStructType = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
            ToObjPtrVal( $3 ) : nullptr;
        ObjPtr pTypeDecl = pFactory->CreateTypeDecl(
            strName, pStructType, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pTypeDecl ), LOC_RANGE($1, $4) );
    }
    ;

opt_overlap:
    /* empty */
    | TOK_OVERLAP
    { $$ = $1; }

struct_decl:
      TOK_STRUCT opt_overlap struct_elem_decl_list TOK_END_STRUCT
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strTypeName = "";
          std::vector< CStStructTypeNode::CStructMember > vecMembers;
          ObjPtr pNode;
          if( $3 != nullptr && IsObjPtrVal( $3 ) )
          {
              CStStructTypeNode* pStruct = ToObjPtrVal( $3 );
              if( pStruct != nullptr )
              {
                  strTypeName = pStruct->m_strTypeName;
                  vecMembers = pStruct->m_vecMembers;
              }
          }
          else
          {
              pNode = pFactory->CreateStructTypeNode(
                  strTypeName, vecMembers, LOC_RANGE($1, $4) );
          }
          if( $2 != nullptr )
          {
              CStStructTypeNode* pst = pNode;
              pst->m_bOverlap = true;
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
      }
    ;

struct_elem_decl_list:
      struct_elem_decl semicolons
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::vector< CStStructTypeNode::CStructMember > vecMembers;
          if( $1 != nullptr && IsObjPtrVal( $1 ) )
              vecMembers = MakeStructMembers( ToObjPtrVal( $1 ) );
          ObjPtr pNode = pFactory->CreateStructTypeNode(
              "", vecMembers, LOC($1) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | struct_elem_decl_list struct_elem_decl semicolons
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
              std::vector< CStStructTypeNode::CStructMember > vecNew =
                  MakeStructMembers( ToObjPtrVal( $2 ) );
              vecMembers.insert( vecMembers.end(),
                  vecNew.begin(), vecNew.end() );
          }
          ObjPtr pNode = pFactory->CreateStructTypeNode(
              "", vecMembers, LOC_RANGE($1, $3) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $3) );
      }
    ;

struct_elem_decl:
      var_declaration
      {
          /* var_declaration already builds the CStVarDeclNode; the
             member_list accumulator converts it to CStructMember
             entries, one per declared name */
          $$ = $1;
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
    | TOK_NON_RETAIN
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStVarDeclNode::vqNonRetain, LOC($1) );
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
    | var_list semicolons var_declaration
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
    | struct_init     /* Struct: := (Speed := 10, Run := TRUE); */
    | ref_value
    ;

array_elem_init1:
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
    | TOK_NUMBER TOK_LPAREN array_elem_init1 TOK_RPAREN
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
    | array_init_list TOK_COMMA array_elem_init1
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

struct_elem_init_list:
    struct_elem_init
    | struct_elem_init_list TOK_COMMA struct_elem_init
    ;

struct_elem_init:
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
    | TOK_ID locate_at TOK_COLON type_spec
    {
        /* Located declaration per the spec, e.g.
           'head AT %B0 INT;' in a STRUCT */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($1);
        std::string strAddr = "";
        ObjPtr pDirectAddr = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        if( !pDirectAddr.IsEmpty() )
        {
            /* Keep the wrapper node as a marker for the
               translator, which emits different code for direct
               addresses; the text stays for display */
            CStDirectAddressNode* pDirect = pDirectAddr;
            if( pDirect != nullptr )
                strAddr = pDirect->m_strAddress;
        }
        ObjPtr pType = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        ObjPtr pNode = pFactory->CreateVarDeclNode(
            std::vector<std::string>{strName}, pType, CStVarDeclNode::vcLocal,
            CStVarDeclNode::vqNone, nullptr, strAddr, true,
            pDirectAddr, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | TOK_ID locate_at TOK_COLON type_spec TOK_ASSIGN initial_value
    {
        /* Colon variant, e.g. 'head AT %B0 : INT;', consistent
           with the VAR_CONFIG binding form */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strName = ID($1);
        std::string strAddr = "";
        ObjPtr pDirectAddr = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        if( !pDirectAddr.IsEmpty() )
        {
            /* Keep the wrapper node as a marker for the
               translator, which emits different code for direct
               addresses; the text stays for display */
            CStDirectAddressNode* pDirect = pDirectAddr;
            if( pDirect != nullptr )
                strAddr = pDirect->m_strAddress;
        }
        ObjPtr pType = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        ObjPtr pInit = ( $6 != nullptr && IsObjPtrVal( $6 ) ) ?
            ToObjPtrVal( $6 ) : nullptr;
        ObjPtr pNode = pFactory->CreateVarDeclNode(
            std::vector<std::string>{strName}, pType, CStVarDeclNode::vcLocal,
            CStVarDeclNode::vqNone, pInit, strAddr, true,
            pDirectAddr, LOC_RANGE($1, $6) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }
    ;


direct_variable:
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
        if( strAddr.size() && strAddr.back() == '*' )
        {
            CStDirectAddressNode* pdan = pAddr;
            pdan->m_bPartialAddr = true;
        }
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

numeric_type_name:
    int_type_name
    |real_type_name
    ;

real_type_name:
    TOK_REAL
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
int_type_name:
    signed_int_name
    | unsigned_int_name
    ;

signed_int_name:
    TOK_INT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_DINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btDInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_SINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btSInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_LINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btLInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

unsigned_int_name:
    TOK_UINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_UDINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUDInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_USINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUSInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_ULINT
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btULInt, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

bit_str_type_name:
    bool_type_name
    | multibits_type_name
    ;

multibits_type_name:
    TOK_WORD
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btWord, 0, LOC($1) );
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
    | TOK_LWORD
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btLWord, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

bool_type_name:
     TOK_BOOL
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btBool, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

time_type_name:
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

varied_length_dim_list:
    TOK_MUL
    {
        /* Count of variable-length dimensions: 1 */
        $$ = MAKE_VALUE( ( guint32 )1, LOC($1) );
    }
    | varied_length_dim_list TOK_COMMA TOK_MUL
    {
        /* Accumulate dimension count */
        guint32 iCount = 1;
        if( $1 != nullptr )
            iCount = ( guint32 )$1->first + 1;
        $$ = MAKE_VALUE( iCount, LOC_RANGE($1, $3) );
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
    | TOK_ARRAY TOK_LBRACKET varied_length_dim_list TOK_RBRACKET TOK_OF type_spec
    {
        /* Variable-length array, e.g. ARRAY [*, *] OF INT */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pElementType = nullptr;
        if( $6 != nullptr && IsObjPtrVal( $6 ) )
        {
            ObjPtr pTypeSpec = ToObjPtrVal( $6 );
            CStDataTypeSpecNode* pSpecNode = pTypeSpec;
            if( pSpecNode != nullptr )
                pElementType = pSpecNode->m_pTypeSpec;
        }
        guint32 iDimCount = ( $3 != nullptr ) ? ( guint32 )$3->first : 0;
        std::vector< CStArrayTypeNode::CArrayDim > vecDims;
        for( guint32 i = 0; i < iDimCount; i++ )
        {
            CStArrayTypeNode::CArrayDim dim;
            dim.m_pStart = nullptr;  // variable length marker
            dim.m_pEnd = nullptr;
            vecDims.push_back( dim );
        }
        ObjPtr pNode = pFactory->CreateArrayTypeNode(
            pElementType, vecDims, LOC_RANGE($1, $5) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $5) );
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

string_type_name:
    TOK_STRING_TYPE
    {
        Variant oVar( ( guint32 )CStBasicTypeNode::btString );
        $$ = MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_WSTRING_TYPE
    {
        Variant oVar( ( guint32 )CStBasicTypeNode::btWString );
        $$ = MAKE_VALUE( oVar, LOC($1) );
    }
    | TOK_USTRING_TYPE
    {
        Variant oVar( ( guint32 )CStBasicTypeNode::btUString );
        $$ = MAKE_VALUE( oVar, LOC($1) );
    }
    ;

string_type_decl:
    TOK_ID TOK_COLON string_spec_init

string_spec_init:
    string_spec 
    | string_spec TOK_ASSIGN string_literal

string_spec:
    string_type_name TOK_LPAREN full_expression TOK_RPAREN
    {  /* Specific length: the length is a constant expression, kept
          as parsed; the numeric length is evaluated by the semantic
          phase */
        
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
           ( cpp::CStBasicTypeNode::enumBasicType) (int)NUM($1),
           0, LOC_RANGE($1, $4) );
        CStBasicTypeNode* pType = pNode;
        if( pType != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            pType->m_pStringLength = UnwrapFullExpression(
                ToObjPtrVal( $3 ) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | string_type_name TOK_LBRACKET full_expression TOK_RBRACKET
    {  /* Specific length: constant expression, see above */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
           ( cpp::CStBasicTypeNode::enumBasicType) (int)NUM($1),
           0, LOC_RANGE($1, $4) );
        CStBasicTypeNode* pType = pNode;
        if( pType != nullptr && $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            pType->m_pStringLength = UnwrapFullExpression(
                ToObjPtrVal( $3 ) );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | string_type_name
    {  /* Default length is 80 */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
           ( cpp::CStBasicTypeNode::enumBasicType) (int)NUM($1),
           80, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_CHAR
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btChar, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_WCHAR
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btWChar, 0, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | TOK_UCHAR
    {
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateBasicTypeNode(
            CStBasicTypeNode::btUChar, 0, LOC($1) );
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

elem_type_name:
    numeric_type_name
    | bit_str_type_name
    ;
    
data_type_access:
    elem_type_name
    {
        // Wrap elementry_type result in CStDataTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    | derived_type_access
    {
        // Wrap derived_type in CStTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pType = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateTypeSpecNode( pType, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;


array_var_decl:

data_type_to_use:
    /* implicit enum - anonymous enum type */
     TOK_LPAREN enum_value_list TOK_RPAREN
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
    | int_type_name TOK_LPAREN range TOK_RPAREN
    {
        // TODO: Handle ranged integer types
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC_RANGE($1, $4) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | string_spec
    {
        // Wrap string_type result in CStDataTypeSpecNode
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pTypeSpec = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateDataTypeSpecNode(
            pTypeSpec, LOC($1) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
    }
    ;

type_spec:
    data_type_access
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
    ;

derived_type_access: instance_path
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
    ;

/*statements:
    statement
    | statements semicolons statement
    ;
    */


statement:
    assignment_statement
    { $$ = $1; }
    | if_statement
    { $$ = $1; }
    | for_statement
    { $$ = $1; }
    | while_statement
    { $$ = $1; }
    | repeat_statement
    { $$ = $1; }
    | function_call_statement
    { $$ = $1; }
    | case_statement
    { $$ = $1; }
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
    {
        /* conditional pragmas carry no AST value */
        $$ = MAKE_EMPTY();
    }
    | TOK_VSTART_PRAGMA TOK_ELSIF full_expression TOK_RBRACE
    { $$ = MAKE_EMPTY(); }
    | TOK_VSTART_PRAGMA TOK_ELSE TOK_RBRACE
    { $$ = MAKE_EMPTY(); }
    | TOK_VSTART_PRAGMA TOK_END_IF TOK_RBRACE
    { $$ = MAKE_EMPTY(); }
    | TOK_VSTART_PRAGMA TOK_INFO TOK_STRING TOK_RBRACE {
        // Access string value from $3 which is YYSTYPE (shared_ptr<YYSPAIR>)
        stdstr strMsg = GetVariantVal( $3 );
        YYLTYPE2 oLoc;
        ParserPrint( pCtx->GetCurFileName().c_str(),
            oLoc.first_line, strMsg.c_str(), true );
        $$ = MAKE_EMPTY();
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
        $$ = MAKE_EMPTY();
    }
    | TOK_VSTART_PRAGMA TOK_ATTRIBUTE TOK_STRING opt_attr_values TOK_RBRACE
    { $$ = MAKE_EMPTY(); }
    ;

string_list:
    TOK_STRING
    { $$ = $1; }
    | string_list TOK_COMMA TOK_STRING
    { $$ = $1; }
    ;

opt_attr_values :
    /* empty */
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_ASSIGN string_list
    {
        $$ = $2;
    }
    ;

    
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
    | this_notation l_value_var
      {
          /* This member access via pointer */
          if( $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              CStAstFactory* pFactory = GET_FACTORY(pCtx);
              ObjPtr pThis = pFactory->CreateIdentifierExpr( "this", LOC($1) );
              ObjPtr pMember = ToObjPtrVal( $2 );

              /* Build member access: this.member */
              CStMemberAccessExpr* pMemberExpr = dynamic_cast< CStMemberAccessExpr* >( ( CObjBase* )pMember );
              if( pMemberExpr != nullptr )
              {
                  ObjPtr pAccess = pFactory->CreateMemberAccessExpr(
                      CStMemberAccessExpr::atDot, pThis, pMemberExpr->m_strMember, LOC_RANGE($1, $2) );
                  ObjPtr pLValue = pFactory->CreateLValueNode( pAccess, LOC_RANGE($1, $2) );
                  $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $2) );
              }
              else
              {
                  /* Fallback: wrap the member as is */
                  ObjPtr pLValue = pFactory->CreateLValueNode( pMember, LOC_RANGE($1, $2) );
                  $$ = MAKE_VALUE( Variant( pLValue ), LOC_RANGE($1, $2) );
              }
          }
          else
          {
              $$ = MAKE_VALUE( Variant(), LOC_RANGE($1, $2) );
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
    | direct_variable
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
    { $$ = MAKE_EMPTY(); }
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
      primary_expr
      {
          $$ = $1;
      }
    | primary_expr TOK_POWER primary_expr
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

constant:
    numeric_literal
    | string_literal
    | time_literal
    | bit_str_literal
    | bool_literal

numeric_literal:
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
      ;

string_literal:
     TOK_STRING
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
     | TOK_USTRING
     ;

time_literal:
    TOK_TIME
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
      ;

bool_value: 
    TOK_NUMBER
    { /* must be 0 or 1 */
        $$ = $1;
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
    ;

bool_literal:
    bool_value
    | bool_type_name TOK_PUNC bool_value
     
bit_str_literal:
    TOK_NUMBER
    { /* must be sint/uint */
        $$ = $1;
    }
    | bit_str_type_name TOK_PUNC TOK_NUMBER
    ;


/* 8. PRIMARY (Highest Precedence) */
primary_expr:
    constant
    | variable_access
    | func_call
    | ref_value
    | TOK_LPAREN full_expression TOK_RPAREN
      {
          /* Pass through the parenthesized expression, unwrapped from
             the full-expression boundary node */
          ObjPtr pInner = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
              UnwrapFullExpression( ToObjPtrVal( $2 ) ) : nullptr;
          $$ = MAKE_VALUE( Variant( pInner ), LOC_RANGE($1, $3) );
      }
    ;

variable_access:
    symbolic_var
    // | symbolic_var TOK_DOT multi_part_access
    | symbolic_var TOK_VSEP_MULTI_PART TOK_DOT multi_part_access

multi_part_access:
    TOK_NUMBER
    | TOK_MULTPART_ACCESS

func_call:
      func_access TOK_LPAREN arg_list TOK_RPAREN
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
      ;

func_access:
    derived_func_name
    {
        /* must check the standard functions here*/
        $$ = $1;
    }
    ;

derived_func_name:
    instance_path


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
    {
        $$ = MAKE_EMPTY();
    }
    | semicolons
    {
        $$ = $1;
    }
    ;

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
    {
        $$ = MAKE_EMPTY();
    }
    | TOK_BY full_expression
    { $$ = $2; }
    ;

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
    {
        $$ = $1;
    }
    ;

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
      { $$ = $1; }
    | param_assignments
      { $$ = $1; }
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
      {
          ObjPtr pNode;
          pNode.NewObj( clsid( CStMethodDeclListNode ) );
          CStMethodDeclListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
              pList->m_vecMethods.push_back( ToObjPtrVal( $1 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }
    | method_declaration_list method_declaration
      {
          /* rebuild-and-copy: the previous methods plus the new
             one */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStMethodDeclListNode ) );
          CStMethodDeclListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pPrev = ToObjPtrVal( $1 );
              CStMethodDeclListNode* pPrevList = pPrev;
              if( pPrevList != nullptr )
                  pList->m_vecMethods = pPrevList->m_vecMethods;
          }
          if( pList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
              pList->m_vecMethods.push_back( ToObjPtrVal( $2 ) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
      }
    /* error recovery for method declarations */
    | method_declaration_list error
      {
          pCtx->IncError();
          ParserPrint( basename(pCtx->GetCurFileName().c_str()),
              @2.last_line,
              "invalid method declaration, skipping", true );
          yyerrok;
          /* Keep the methods accumulated so far */
          $$ = $1;
      }
    ;
opt_override:
    /* empty */
    | TOK_OVERRIDE
    ;

method_declaration:
    TOK_METHOD opt_access_spec opt_fb_modifier opt_override TOK_ID TOK_COLON data_type_access TOK_VSEMICOLON
        var_declarations
        block_statements
    TOK_END_METHOD
      {
          CStAstFactory* pFactory = GET_FACTORY(pCtx);
          std::string strName = ID($5);
          CStMethodDecl::enumAccessModifier eAccess =
              ( CStMethodDecl::enumAccessModifier )NUM( $2 );
          ObjPtr pReturnType = ( $7 != nullptr && IsObjPtrVal( $7 ) ) ?
              ToObjPtrVal( $7 ) : nullptr;

          /* Split the accumulated declarations by category */
          std::vector< ObjPtr > vecInput, vecOutput, vecInOut;
          std::vector< ObjPtr > vecLocal, vecTemp, vecOther;
          if( $9 != nullptr && IsObjPtrVal( $9 ) )
              SplitVarDeclList( ToObjPtrVal( $9 ),
                  vecInput, vecOutput, vecInOut,
                  vecLocal, vecTemp, vecOther );

          std::vector< ObjPtr > vecStatements;
          if( $10 != nullptr && IsObjPtrVal( $10 ) )
          {
              /* Extract the body statements from the accumulator */
              ObjPtr pList = ToObjPtrVal( $10 );
              CStStmtListNode* pStmtList = pList;
              if( pStmtList != nullptr )
                  vecStatements = pStmtList->m_vecStatements;
          }

          ObjPtr pNode = pFactory->CreateMethodDecl(
              strName, eAccess, pReturnType,
              vecInput, vecOutput, vecInOut, vecLocal, vecTemp,
              vecStatements, LOC_RANGE($1, $11) );
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $11) );
      }
    ;

opt_access_spec:
    /* empty - defaults to PUBLIC in most ST dialects */
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStMethodDecl::amPublic, YYLTYPE2() );
      }
    | access_spec
    ;

access_spec:
    TOK_PUBLIC
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStMethodDecl::amPublic, LOC($1) );
      }
    | TOK_PROTECTED
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStMethodDecl::amProtected, LOC($1) );
      }
    | TOK_PRIVATE
      {
          $$ = MAKE_VALUE(
              ( guint32 )CStMethodDecl::amPrivate, LOC($1) );
      }
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
        if( $4 != nullptr && IsObjPtrVal( $4 ) )
        {
            /* Extract the accumulated methods */
            ObjPtr pList = ToObjPtrVal( $4 );
            CStMethodDeclListNode* pMethodList = pList;
            if( pMethodList != nullptr )
                vecMethods = pMethodList->m_vecMethods;
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
        std::vector< ObjPtr > vecMethods, vecStatements;
        std::vector< ObjPtr > vecOther;
        if( $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            /* Split the accumulated declarations by category */
            SplitVarDeclList( ToObjPtrVal( $2 ),
                vecInput, vecOutput, vecInOut,
                vecLocal, vecTemp, vecOther );
        }
        if( $3 != nullptr && IsObjPtrVal( $3 ) )
        {
            /* Extract the accumulated methods */
            ObjPtr pList = ToObjPtrVal( $3 );
            CStMethodDeclListNode* pMethodList = pList;
            if( pMethodList != nullptr )
                vecMethods = pMethodList->m_vecMethods;
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
            vecMethods, vecStatements, LOC_RANGE($1, $5) );
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
fb_modifier:
    TOK_ABSTRACT
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
opt_fb_modifier:
    /* empty */
    {
        $$ = MAKE_VALUE(
            ( guint32 )CStFunctionBlockDecl::fbmNone, YYLTYPE2() );
    }
    | fb_modifier 
    { $$ = $1; }
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
        if( $4 != nullptr && IsObjPtrVal( $4 ) )
        {
            ObjPtr pList = ToObjPtrVal( $4 );
            CStCaseBranchListNode* pBranchList = pList;
            if( pBranchList != nullptr )
                vecBranches = pBranchList->m_vecBranches;
        }

        std::vector< ObjPtr > vecElse;
        if( $5 != nullptr && IsObjPtrVal( $5 ) )
        {
            ObjPtr pList = ToObjPtrVal( $5 );
            CStStmtListNode* pStmtList = pList;
            if( pStmtList != nullptr )
                vecElse = pStmtList->m_vecStatements;
        }

        ObjPtr pNode = pFactory->CreateCaseStmt(
            pExpr, vecBranches, vecElse, LOC_RANGE($1, $6) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }
    ;

opt_else_statement:
    /* empty */
    {
        /* no ELSE clause: an empty statement list */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        ObjPtr pNode = pFactory->CreateStmtListNode( YYLTYPE2() );
        $$ = MAKE_VALUE( Variant( pNode ), YYLTYPE2() );
    }
    |TOK_ELSE block_statements
    {
        $$ = $2;
    }
    ;

case_element_list:
    case_element
    { $$ = $1; }
    | case_element_list case_element
    {
        /* Accumulate the case branches */
        ObjPtr pNode;
        pNode.NewObj( clsid( CStCaseBranchListNode ) );
        CStCaseBranchListNode* pBranchList = pNode;
        if( pBranchList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStCaseBranchListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pBranchList->m_vecBranches = pPrevList->m_vecBranches;
        }
        if( pBranchList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
        {
            ObjPtr pNext = ToObjPtrVal( $2 );
            CStCaseBranchListNode* pNextList = pNext;
            if( pNextList != nullptr )
            {
                pBranchList->m_vecBranches.insert(
                    pBranchList->m_vecBranches.end(),
                    pNextList->m_vecBranches.begin(),
                    pNextList->m_vecBranches.end() );
            }
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    ;

semicolons:
    TOK_SEMICOLON
    { $$ = $1; }
    | TOK_SEMICOLON semicolons
    { $$ = $1; }
    ;

cinner_statements_1:
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
    | cinner_statements_1 TOK_SEMICOLON statement
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
    | cinner_statements_1 TOK_SEMICOLON
    {
        /* trailing semicolon - keep the statements so far */
        $$ = $1;
    }
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
        /* keep the recovery statement in the list */
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


cinner_statements: cinner_statements_1
    { $$ = $1; }
    ;

case_element:
    // case_list_selector cinner_statements TOK_SEMICOLON
    // TOK_VCASE_SEP does not come from source, it is used to resolve the
    // shift/reduce conflict
    case_list_selector cinner_statements TOK_VCASE_SEP
    {
        /* One branch with its selector labels and statements */
        ObjPtr pNode;
        pNode.NewObj( clsid( CStCaseBranchListNode ) );
        CStCaseBranchListNode* pBranchList = pNode;
        if( pBranchList != nullptr )
        {
            CStCaseStmt::CCaseBranch oBranch;

            /* Convert the selector labels { start, end } */
            if( $1 != nullptr && IsObjPtrVal( $1 ) )
            {
                ObjPtr pPrev = ToObjPtrVal( $1 );
                CStSubrangeListNode* pRanges = pPrev;
                if( pRanges != nullptr )
                {
                    for( size_t i = 0;
                        i < pRanges->m_vecRanges.size(); i++ )
                    {
                        CStSubrangeNode* pRange = pRanges->m_vecRanges[ i ];
                        if( pRange == nullptr )
                            continue;
                        CStCaseStmt::CSelectRange oSel;
                        oSel.m_pStartValue = pRange->m_pStart;
                        oSel.m_pEndValue = pRange->m_pEnd;
                        oBranch.m_vecSelectors.push_back( oSel );
                    }
                }
            }

            /* Extract the branch statements from the accumulator */
            if( $2 != nullptr && IsObjPtrVal( $2 ) )
            {
                ObjPtr pList = ToObjPtrVal( $2 );
                CStStmtListNode* pStmtList = pList;
                if( pStmtList != nullptr )
                    oBranch.m_vecStatements =
                        pStmtList->m_vecStatements;
            }

            pBranchList->m_vecBranches.push_back( oBranch );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
    }
    ;

case_check_statement:
    assignment_statement
    { $$ = $1; }
    | function_call_statement
    { $$ = $1; }

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
    {
        /* First selector label - create the accumulator */
        ObjPtr pList;
        pList.NewObj( clsid( CStSubrangeListNode ) );
        CStSubrangeListNode* pRangeList = pList;
        if( pRangeList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
            pRangeList->m_vecRanges.push_back( ToObjPtrVal( $1 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $1) );
    }
    | case_list_selector TOK_COMMA case_selector TOK_COLON
    {
        /* Accumulate selector labels */
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
    /* error recovery for case selectors */
    | case_list_selector TOK_COMMA error TOK_COLON
    {
        pCtx->IncError();
        ParserPrint( basename(pCtx->GetCurFileName().c_str()),
            @3.last_line,
            "invalid case selector, skipping", true );
        yyerrok;
        /* keep the labels accumulated so far */
        $$ = $1;
    }
    ;

case_selector:
    case_constant_expression
    {
        CStAstFactory* pFactory = GET_FACTORY( pCtx );
        ObjPtr pValue = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pNode = pFactory->CreateSubrangeNode(
            pValue, nullptr, LOC( $1 ) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC( $1 ) );
    }

    | case_constant_expression TOK_RANGE case_constant_expression
    {
        CStAstFactory* pFactory = GET_FACTORY( pCtx );
        ObjPtr pStart = ( $1 != nullptr && IsObjPtrVal( $1 ) ) ?
            ToObjPtrVal( $1 ) : nullptr;
        ObjPtr pEnd = ( $3 != nullptr && IsObjPtrVal( $3 ) ) ?
            ToObjPtrVal( $3 ) : nullptr;
        ObjPtr pNode = pFactory->CreateSubrangeNode(
            pStart, pEnd, LOC_RANGE( $1, $3 ) );
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE( $1, $3 ) );
    }
    ;

case_constant_expression:
    full_expression
    {
        /* Per the spec the case labels are constant expressions. They
           may reference named constants or enum values and involve
           arithmetic, which requires the variable tables of the
           semantic phase, so the parser keeps the label expressions
           as-is and performs no numeric evaluation. */
        $$ = $1;
    }
    ;

config_init:
    TOK_VAR_CONFIG
        config_inst_init
    TOK_END_VAR
    {
        /* Pass through the config list for the semantic phase;
           each entry is a CStVarConfigDecl with one address config */
        $$ = $2;
    }
    ;

config_inst_init:
    /* empty */
    {
        ObjPtr pList;
        pList.NewObj( clsid( CStVarConfigListNode ) );
        $$ = MAKE_VALUE( Variant( pList ), YYLTYPE2() );
    }
    | config_inst_init instance_specific_init
    {
        /* Accumulate the config entries */
        ObjPtr pList;
        pList.NewObj( clsid( CStVarConfigListNode ) );
        CStVarConfigListNode* pCfgList = pList;
        if( pCfgList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            ObjPtr pPrev = ToObjPtrVal( $1 );
            CStVarConfigListNode* pPrevList = pPrev;
            if( pPrevList != nullptr )
                pCfgList->m_vecConfigs = pPrevList->m_vecConfigs;
        }
        if( pCfgList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
            pCfgList->m_vecConfigs.push_back( ToObjPtrVal( $2 ) );
        $$ = MAKE_VALUE( Variant( pList ), LOC_RANGE($1, $2) );
    }
    ;

locate_at:
    TOK_AT direct_variable
    { $$ = $2; }

instance_specific_init:
    instance_path locate_at TOK_COLON type_spec semicolons
    {
        /* One instance-specific config: path AT address : type */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strPath;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            CStInstancePathNode* pPath = ToObjPtrVal( $1 );
            if( pPath != nullptr )
                strPath = pPath->GetDottedName();
        }
        std::string strAddr;
        ObjPtr pDirectAddr = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        if( !pDirectAddr.IsEmpty() )
        {
            CStDirectAddressNode* pDirect = pDirectAddr;
            if( pDirect != nullptr )
                strAddr = pDirect->m_strAddress;
        }
        ObjPtr pType = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        ObjPtr pNode = pFactory->CreateVarConfigDecl(
            strPath, pType, LOC_RANGE($1, $4) );
        CStVarConfigDecl* pCfg = pNode;
        if( pCfg != nullptr )
        {
            CStVarConfigDecl::CInstanceConfig instCfg;
            instCfg.m_strPath = strAddr;
            instCfg.m_pValue = nullptr;
            pCfg->m_vecConfigs.push_back( instCfg );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $4) );
    }
    | instance_path locate_at TOK_COLON type_spec TOK_ASSIGN initial_value semicolons
    {
        /* One instance-specific config with initial value */
        CStAstFactory* pFactory = GET_FACTORY(pCtx);
        std::string strPath;
        if( $1 != nullptr && IsObjPtrVal( $1 ) )
        {
            CStInstancePathNode* pPath = ToObjPtrVal( $1 );
            if( pPath != nullptr )
                strPath = pPath->GetDottedName();
        }
        std::string strAddr;
        ObjPtr pDirectAddr = ( $2 != nullptr && IsObjPtrVal( $2 ) ) ?
            ToObjPtrVal( $2 ) : nullptr;
        if( !pDirectAddr.IsEmpty() )
        {
            CStDirectAddressNode* pDirect = pDirectAddr;
            if( pDirect != nullptr )
                strAddr = pDirect->m_strAddress;
        }
        ObjPtr pType = ( $4 != nullptr && IsObjPtrVal( $4 ) ) ?
            ToObjPtrVal( $4 ) : nullptr;
        ObjPtr pInit = ( $6 != nullptr && IsObjPtrVal( $6 ) ) ?
            ToObjPtrVal( $6 ) : nullptr;
        ObjPtr pNode = pFactory->CreateVarConfigDecl(
            strPath, pType, LOC_RANGE($1, $6) );
        CStVarConfigDecl* pCfg = pNode;
        if( pCfg != nullptr )
        {
            CStVarConfigDecl::CInstanceConfig instCfg;
            instCfg.m_strPath = strAddr;
            instCfg.m_pValue = pInit;
            pCfg->m_vecConfigs.push_back( instCfg );
        }
        $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $6) );
    }
    ;

global_name_space:
    TOK_DOT 
    | TOK_UNDERSCORE TOK_DOT

identifier_dot_list:
    TOK_ID
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
    %prec TOK_DOT
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

instance_path :
    identifier_dot_list
    | global_name_space identifier_dot_list
    %prec TOK_DOT
    ;

using_directive_list : using_directive
      {
          /* Accumulate the namespace names, e.g. 'A.B' for
             USING A.B; */
          ObjPtr pNode;
          pNode.NewObj( clsid( CStIdentifierListNode ) );
          CStIdentifierListNode* pList = pNode;
          if( pList != nullptr && $1 != nullptr && IsObjPtrVal( $1 ) )
          {
              ObjPtr pUsing = ToObjPtrVal( $1 );
              CStUsingDirective* pDirective = pUsing;
              if( pDirective != nullptr )
                  pList->m_vecIdentifiers.push_back(
                      pDirective->m_strNamespace );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC($1) );
      }

    | using_directive_list using_directive
      {
          /* rebuild-and-copy: the previous namespaces plus the new
             one */
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
          if( pList != nullptr && $2 != nullptr && IsObjPtrVal( $2 ) )
          {
              ObjPtr pUsing = ToObjPtrVal( $2 );
              CStUsingDirective* pDirective = pUsing;
              if( pDirective != nullptr )
                  pList->m_vecIdentifiers.push_back(
                      pDirective->m_strNamespace );
          }
          $$ = MAKE_VALUE( Variant( pNode ), LOC_RANGE($1, $2) );
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

