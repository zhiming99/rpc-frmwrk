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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <rpc.h>
#include "stlexer.h"
#include "parsrctx.h"

using namespace rpcf;

extern std::shared_ptr< rpcf::CSTParserContext > g_pParserCtx;

%}

%code requires {

#include <rpc.h>
#include "parsrctx.h"
using namespace rpcf;

extern void ParserPrint(
    const char* szFile,
    gint32 iLineNo,
    const char* strMsg,
    bool bErr = false );

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
    | conditional_pragma
    | TOK_EOF
    | case_selector_check
    ;

source_file:
    /* empty */
    | namespace_elements
    ;

namespace_name : TOK_ID 
    ;

namespace_elements : namespace_element
      {  }

    | namespace_elements namespace_element
      {  }
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
          /* AST logic: Wrap nested components in a Namespace AST Node */
          
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
    ;

enum_value_list:
    enum_value
    | enum_value_list TOK_COMMA enum_value

enum_value:
    TOK_ID
    | TOK_ID TOK_ASSIGN full_expression

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

enum_type_definition:
    enum_type_head TOK_ASSIGN opt_base_type opt_assign_enum_val semicolons

type_assignment:
      TOK_ID TOK_COLON type_spec semicolons {
        /* Alias: TYPE MyInt : INT; */
        // add_alias_to_symtab($1, $3);
      }

    | TOK_ID TOK_COLON struct_definition semicolons   {
        /* Struct: TYPE Motor : STRUCT... */
        // add_struct_to_symtab($1, $3);
    }
    | enum_type_definition
    ;

struct_definition:
      TOK_STRUCT member_list TOK_END_STRUCT          { $$ = $2; }
    ;

member_list:
      member_declaration semicolons               {  }
    | member_list member_declaration semicolons   {  }
    ;

member_declaration:
      TOK_ID TOK_COLON type_spec               {  }
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
    ;

initial_value:
      full_expression                    /* Simple: int := 10; */
    | TOK_LBRACKET init_list TOK_RBRACKET             /* Array:int := [1, 2, 3]; */
    | TOK_LPAREN struct_init_list TOK_RPAREN      /* Struct: := (Speed := 10, Run := TRUE); */
    ;

init_list:
      initial_value                    {  }
    | init_list TOK_COMMA initial_value      {  }
    /* ST also supports 'n(value)' for repeating array elements */
    | init_list TOK_COMMA TOK_NUMBER TOK_LPAREN initial_value TOK_RPAREN    {  }
    ;

struct_init_list:
      TOK_ID TOK_ASSIGN initial_value    {  }
    | struct_init_list TOK_COMMA TOK_ID TOK_ASSIGN initial_value {  }
    ;

var_declaration:
      identifier_list TOK_COLON type_spec                  {  }
    | identifier_list TOK_COLON type_spec TOK_ASSIGN initial_value  {  }
    | TOK_ID TOK_AT direct_address 
    | TOK_ID TOK_AT direct_address TOK_ASSIGN initial_value
    ;


direct_address:
    TOK_RPCF_ADDR
    | TOK_ABS_ADDR_PERIPHERAL
    | TOK_ABS_ADDR_PERIPHERAL TOK_LBRACKET full_expression TOK_RBRACKET
  

identifier_list:
      TOK_ID                    {  }
    | identifier_list TOK_COMMA TOK_ID {  }
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
    | TOK_LBRACE TOK_END_REGION TOK_RBRACE

conditional_pragma:
    TOK_VSTART_PRAGMA TOK_IF full_expression TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_ELSIF full_expression TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_ELSE TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_END_IF TOK_RBRACE
    | TOK_VSTART_PRAGMA TOK_INFO TOK_STRING TOK_RBRACE {
        stdstr strMsg = $3.get()->first;
        YYLTYPE2& curloc = $2.get()->second;
        strMsg.insert( 0, "info, " );
        ParserPrint( pCtx->GetCurFileName().c_str(),
            curloc.first_line, strMsg.c_str() );
    }
    | TOK_VSTART_PRAGMA TOK_INCLUDE TOK_STRING TOK_RBRACE {

        yyscan_t yyscanner = pCtx->GetScanner();
        YYLTYPE2& curloc = $2.get()->second;

        std::string& strFile = $3.get()->first;
        stdstr strCurFile = basename(
            pCtx->GetFileName(
                curloc.fidx).c_str() );
        if( strFile.empty() )
        {
            if( strCurFile.size() )
                ParserPrint( 
                    strCurFile.c_str(),
                    curloc.first_line,
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
                curloc.first_line,
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
                curloc.first_line,
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
      l_value_ext TOK_ASSIGN full_expression {
          // Wasm logic to store the result of full_expression into l_value
          // emit_assignment($1, $3);
      }
    ;

/* Rule for Standalone Calls: Used for functions/methods that return void or whose return is ignored */
function_call_statement:
      l_value TOK_LPAREN arg_list TOK_RPAREN {
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
    | full_expression TOK_OR and_expression
    | full_expression TOK_XOR and_expression
    ;

and_expression:
    comparison_expression
    | and_expression TOK_AND comparison_expression
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
    | arithmetic_expr comp_op  arithmetic_expr {  }
    ;

/* 4. ADDITIVE (+, -) */
arithmetic_expr:
      term
    | arithmetic_expr TOK_ADD term {  }
    | arithmetic_expr TOK_VSUB term {  }
    ;

/* 5. MULTIPLICATIVE (*, /, MOD) */
term:
      unary_expr

    | term TOK_MUL   unary_expr {  }
    | term TOK_DIV   unary_expr {  }
    | term TOK_MOD unary_expr {  }
    ;

/* 6. UNARY (NOT, -) */
unary_expr:
      power_expr

    | TOK_MINUS power_expr {  }
    | TOK_NOT power_expr {  }
    ;

/* 7. POWER (**) */
power_expr:
      factor
    | factor TOK_POWER factor {  }
    ;

/* 8. PRIMARY (Highest Precedence) */
factor:
      TOK_NUMBER
    | TOK_STRING
    | TOK_WSTRING
    | TOK_TIME      /* T#5s */
    | TOK_LTIME     /* LT#05s */
    | TOK_DATE      /* D#2024-01-01 */
    | TOK_DATE_TIME      /* DT#2024-01-01 */
    | TOK_TIME_OF_DAY    /* TOD#2024-01-01 */
    | TOK_TRUE
    | TOK_FALSE
    /* function call or an array element*/
    | l_value TOK_LPAREN arg_list TOK_RPAREN {

        }
    | l_value_ext
    | TOK_LPAREN full_expression TOK_RPAREN
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
    ;

while_statement:
    TOK_WHILE full_expression TOK_DO block_statements TOK_END_WHILE
    ;

repeat_statement:
    TOK_REPEAT block_statements TOK_UNTIL full_expression TOK_END_REPEAT
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

param_assignment:
    TOK_ID TOK_ASSIGN full_expression  /* Formal Input: IN := True */
    | TOK_ID TOK_OUTPUT_ASSIGN l_value_ext           /* Formal Output: Q => MyLamp */
    ;
    ;

method_declaration_list:
    method_declaration
    | method_declaration_list method_declaration
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
    | function_block_header var_declaration method_declaration_list block_statements TOK_END_FUNCTION_BLOCK 

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
    ;

case_statement:
    TOK_CASE full_expression TOK_OF
        case_element_list
        opt_else_statement
    TOK_END_CASE
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
    /* error check */
    | cinner_statements_1 error
    { 
        pCtx->IncError();
        stdstr strCurFile = basename(
            pCtx->GetCurFileName().c_str() );
        ParserPrint( strCurFile.c_str(),
            @2.last_line,
            "';' is expected");
    }
    ;


opt_semicolon:
    /* empty */
    | TOK_SEMICOLON

cinner_statements: cinner_statements_1  opt_semicolon

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
            YYSTYPE casep_lval( new YYSPAIR() );

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
        printf(
            "error from lookahead check, %s\n",
            yymsgp );
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
        strMsg = "Parser stopped at '";
        strMsg += pVal->second.text + "'";
        ParserPrint(
            strCurFile.c_str(),
            pVal->second.first_line,
            strMsg.c_str() );
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

