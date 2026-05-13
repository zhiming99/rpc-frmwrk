/*
 * =====================================================================================
 *
 *       Filename:  stscript.y
 *
 *    Description:  The grammar parser for a subset of Structured Text Language
 *
 *        Version:  1.0
 *        Created:  04/24/2026 11:18:30 PM
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
%define api.value.type {Variant}
%require "3.0"

%{
#include <stdio.h>
#include <stdlib.h>

using namespace rpcf;

std::shared_ptr< CSTParserContext > g_pParserCtx( new CSTParserContext );

%}

%code requires {

#include "parsrctx.h"

extern gint32 EvalConstExpr( CSTParserContext* pCtx );

extern void ParserPrint(
    const char* szFile,
    gint32 iLineNo,
    const char* strMsg );

}

%token TOK_PROGRAM TOK_VAR TOK_END_VAR TOK_IF TOK_THEN TOK_ELSE TOK_ELSIF TOK_END_IF TOK_FOR TOK_TO TOK_DO TOK_END_FOR TOK_WHILE TOK_END_WHILE TOK_REPEAT TOK_UNTIL TOK_END_REPEAT
%token TOK_TON TOK_TON_VALUE TOK_STRING TOK_WSTRING TOK_INT TOK_REAL TOK_LREAL TOK_BOOL TOK_TRUE TOK_FALSE TOK_TIME TOK_TYPED_LITERAL TOK_TYPE TOK_END_TYPE TOK_STRUCT TOK_END_STRUCT
%token TOK_UINT TOK_DINT TOK_UDINT TOK_SINT TOK_USINT TOK_BYTE TOK_WORD TOK_DWORD TOK_ULINT TOK_LINT TOK_LWORD

%token TOK_ID TOK_NUMBER TOK_ASSIGN TOK_SEMICOLON TOK_COLON TOK_COMMA TOK_ARRAY TOK_RANGE TOK_DOT
%token TOK_PLUS TOK_MINUS TOK_MULTIPLY TOK_DIVIDE TOK_MOD TOK_NOT TOK_AND TOK_OR TOK_XOR TOK_DATE TOK_TIME_OF_DAY TOK_DATE_TIME TOK_ABS_ADDR_PERIPHERAL TOK_ABS_ADDR_BIT TOK_ABS_ADDR_BLOCK

%token TOK_EQUAL TOK_POWER TOK_LBRACKET TOK_RBRACKET TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_LE TOK_GT TOK_NEQU TOK_NLE TOK_NGT
%token TOK_CASE_SEP TOK_START_PRAGMA TOK_START_MAIN
               
%token TOK_FUNCTION_BLOCK TOK_FUNCTION TOK_END_FUNCTION_BLOCK TOK_END_FUNCTION TOK_END_PROGRAM TOK_INCLUDE
%token TOK_VAR_INPUT TOK_VAR_OUTPUT TOK_VAR_IN_OUT TOK_VAR_GLOBAL TOK_CONSTANT TOK_PUNC TOK_VAR_TEMP TOK_AT TOK_VAR_EXTERNAL TOK_RETAIN TOK_PERSISTENT TOK_VAR_CONFIG TOK_CARET TOK_POINTER

%token TOK_TIME_TYPE TOK_TIME_OF_DAY_TYPE TOK_DATE_TYPE TOK_STRING_TYPE TOK_WSTRING_TYPE TOK_COMMENT TOK_BY TOK_CASE TOK_END_CASE TOK_OF TOK_ABSTRACT TOK_FINAL TOK_EXTENDS TOK_IMPLEMENTS TOK_SUPER TOK_THIS TOK_PRIVATE TOK_PUBLIC TOK_INTERNAL TOK_PROTECTED TOK_REFERENCE TOK_REF_TO TOK_METHOD TOK_END_METHOD TOK_ATTRIBUTE TOK_INFO TOK_REGION TOK_END_REGION TOK_RPCF_ADDR TOK_OUTPUT_ASSIGN

 /*%glr-parser*/

%start start_point
%parse-param { CSTParserContext *pCtx }

%%

start_point:
    TOK_START_MAIN script_file
    | conditional_pragma

script_file:
    type_definition_block
    | var_declarations
    | statements
    | function
    | var_config_declaration
    | pragma_statement
    ;

function:
    TOK_FUNCTION TOK_ID var_declarations statements TOK_END_FUNCTION TOK_SEMICOLON
    ;

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
    enum_type_head TOK_ASSIGN opt_base_type opt_assign_enum_val TOK_SEMICOLON

type_assignment:
      TOK_ID TOK_COLON type_spec TOK_SEMICOLON {
        /* Alias: TYPE MyInt : INT; */
        add_alias_to_symtab($1, $3);
      }

    | TOK_ID TOK_COLON struct_definition TOK_SEMICOLON   {
        /* Struct: TYPE Motor : STRUCT... */
        add_struct_to_symtab($1, $3);
    }
    | enum_type_definition
    ;

struct_definition:
      TOK_STRUCT member_list TOK_END_STRUCT          { $$ = $2; }
    ;

member_list:
      member_declaration TOK_SEMICOLON               { $$ = create_member_list($1); }
    | member_list member_declaration TOK_SEMICOLON   { $$ = add_to_member_list($1, $2); }
    ;

member_declaration:
      TOK_ID TOK_COLON type_spec               { $$ = create_member($1, $3); }
    ;

var_declarations:
    /* empty */
    | var_declarations TOK_VAR declaration_list TOK_END_VAR
    ;

declaration_list:
    declaration_list declaration
    | declaration
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
      TOK_VAR var_list TOK_END_VAR             
    | TOK_VAR_INPUT var_list  opt_qualifier TOK_END_VAR       
    | TOK_VAR_OUTPUT var_list opt_qualifier TOK_END_VAR      
    | TOK_VAR_IN_OUT var_list TOK_END_VAR      
    | TOK_VAR_EXTERNAL var_list TOK_END_VAR
    ;

var_list:
      var_declaration TOK_SEMICOLON           { $$ = create_list($1); }
    | var_list var_declaration TOK_SEMICOLON  { $$ = add_to_list($1, $2); }
    ;

initial_value:
      full_expression                    /* Simple: int := 10; */
    | TOK_LBRACKET init_list TOK_RBRACKET             /* Array:int := [1, 2, 3]; */
    | TOK_LPAREN struct_init_list TOK_RPAREN      /* Struct: := (Speed := 10, Run := TRUE); */
    ;

init_list:
      initial_value                    { $$ = create_init_list($1); }
    | init_list TOK_COMMA initial_value      { $$ = add_to_init_list($1, $3); }
    /* ST also supports 'n(value)' for repeating array elements */
    | init_list TOK_COMMA TOK_NUMBER TOK_LPAREN initial_value TOK_RPAREN    { $$ = create_repeated_init($1, $3); }
    ;

struct_init_list:
      TOK_ID TOK_ASSIGN initial_value    { $$ = create_struct_init($1, $3); }
    | struct_init_list TOK_COMMA TOK_ID TOK_ASSIGN initial_value { $$ = add_to_struct_init($1, $3, $5); }
    ;

var_declaration:
      identifier_list TOK_COLON type_spec                  { $$ = create_decl($1, $3, NULL); }
    | identifier_list TOK_COLON type_spec TOK_ASSIGN initial_value  { $$ = create_decl($1, $3, $5); }
    | TOK_ID TOK_AT direct_address 
    | TOK_ID TOK_AT direct_address TOK_ASSIGN initial_value
    ;


direct_address:
    TOK_RPCF_ADDR
    | TOK_ABS_ADDR_BLOCK
    | TOK_ABS_ADDR_BIT
    | TOK_ABS_ADDR_PERIPHERAL
    ;

identifier_list:
      TOK_ID                    { $$ = create_id_list($1); }
    | identifier_list TOK_COMMA TOK_ID { $$ = add_to_id_list($1, $3); }
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
    | range_list TOK_COMMA range  /* Supports multi-dimensional arrays */
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
    
type_spec:
    int_type
    | int_type TOK_LPAREN range TOK_RPAREN
    | other_elementry_type
    | array_type
    | derived_type
    | string_type
    | pointer_type
    | reference_type
    /* implicit enum */
    | TOK_LPAREN enum_value_list TOK_RPAREN
    ;

derived_type: TOK_ID
    ;

statements:
    statements statement
    | statement
    ;
statement:
    assignment_statement TOK_SEMICOLON
    | if_statement TOK_SEMICOLON
    | for_statement TOK_SEMICOLON
    | while_statement TOK_SEMICOLON
    | repeat_statement TOK_SEMICOLON
    | function_call_statement TOK_SEMICOLON
    | case_statement TOK_SEMICOLON
    ;

pragma_statement:
    TOK_LBRACE TOK_REGION TOK_STRING TOK_RBRACE
    | TOK_LBRACE TOK_END_REGION TOK_RBRACE

conditional_pragma:
    TOK_START_PRAGMA TOK_IF full_expression TOK_RBRACE
    | TOK_START_PRAGMA TOK_ELSIF full_expression TOK_RBRACE
    | TOK_START_PRAGMA TOK_ELSE TOK_RBRACE
    | TOK_START_PRAGMA TOK_END_IF TOK_RBRACE
    | TOK_START_PRAGMA TOK_INFO TOK_STRING TOK_RBRACE {
        std::string& strMsg = $3;
        ParserPrint( 
            curloc->file->name,
            yyget_lineno( yyscanner ),
            "Info: %s ", strMsg.c_str() );
    }
    | TOK_START_PRAGMA TOK_INCLUDE TOK_STRING TOK_RBRACE {

        yyscan_t yyscanner = pCtx->yyscanner;
        YYLTYPE *curloc = yyget_lloc( yyscanner );

        std::string& strFile = $3;
        if( strFile.empty() )
        {
            ParserPrint( 
                curloc->file->name,
                yyget_lineno( yyscanner ),
                "Error, expecting file name" );
            pCtx->IncSemError();
            YYERROR;
        }

        if( strFile[ 0 ] != '/' )
        {
            //relative path
            char* pPath = realpath(
                strFile.c_str(), nullptr );

            if( pPath == nullptr )
            {
                stdstr& strTop =
                    pCtx->m_vecInclStack.back()->m_strPath;
                strFile = GetDirName( strTop ) +
                    "/" + strFile;
            }
            else
            {
                strFile = pPath;
                free( pPath );
            }
        }
        FILE* pIncl = TryOpenFile( strFile.c_str() );
        if ( !pIncl )
        {
            ParserPrint(
                curloc->file->name,
                yyget_lineno( yyscanner ),
                strerror( errno ) );
            pCtx->IncSemError();
            YYERROR;
        }

        FILECTX2* pfc = new FILECTX2();
        pfc->m_strPath = strFile;
        pfc->m_fp = pIncl;
        pfc->m_oLocation = yyget_lloc( pCtx->yyscanner );
        pCtx->m_vecInclStack.push_back(
            std::unique_ptr< FILECTX2 >( pfc ) );
        yypush_buffer_state(
            yy_create_buffer( pIncl, YY_BUF_SIZE ), yyscanner );
        yyset_lineno( 1, yyscanner );
        yyset_column( 1, yyscanner );
    }
    | TOK_LBRACE TOK_ATTRIBUTE TOK_STRING TOK_RBRACE

    ;

/* Rule for Assignments: Only allows memory locations on the LHS */
assignment_statement:
      l_value TOK_ASSIGN full_expression {
          // Wasm logic to store the result of full_expression into l_value
          // emit_assignment($1, $3);
      }
    ;

/* Rule for Standalone Calls: Used for functions/methods that return void or whose return is ignored */
function_call_statement:
      instance_path TOK_LPAREN arg_list TOK_RPAREN {
          // Wasm logic to call the function
          // If it's a DCS Utility, we push the g_pUtils handle first
          printf("%s",$1);
      }
    ;

/* L-Value rule: Strictly limited to writable memory locations */
l_value:
    l_value_var
    | TOK_SUPER pointer l_value_var
    | TOK_THIS pointer l_value_var
    /* %Q and %M can be l_value */
    | direct_address
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
    /* bit access */
    | instance_path TOK_DOT TOK_NUMBER {  }
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
comparison_expression:
      arithmetic_expr
    | arithmetic_expr TOK_EQUAL  arithmetic_expr { printf($1.type == TYPE_INT ? "i32.eq\n" : "f32.eq\n"); $$.type = TYPE_BOOL; }
    | arithmetic_expr TOK_NEQU arithmetic_expr { printf($1.type == TYPE_INT ? "i32.ne\n" : "f32.ne\n"); $$.type = TYPE_BOOL; }

    | arithmetic_expr TOK_LE   arithmetic_expr 
    | arithmetic_expr TOK_GT   arithmetic_expr
    | arithmetic_expr TOK_NLE  arithmetic_expr
    | arithmetic_expr TOK_NGT  arithmetic_expr
    ;

/* 4. ADDITIVE (+, -) */
arithmetic_expr:
      term
    | arithmetic_expr TOK_PLUS term { printf($1.type == TYPE_INT ? "i32.add\n" : "f32.add\n"); }
    | arithmetic_expr TOK_MINUS term { printf($1.type == TYPE_INT ? "i32.sub\n" : "f32.sub\n"); }
    ;

/* 5. MULTIPLICATIVE (*, /, MOD) */
term:
      unary_expr

    | term TOK_MULTIPLY   unary_expr { printf($1.type == TYPE_INT ? "i32.mul\n" : "f32.mul\n"); }
    | term TOK_DIVIDE   unary_expr { printf($1.type == TYPE_INT ? "i32.div_s\n" : "f32.div\n"); }
    | term TOK_MOD unary_expr { printf("i32.rem_s\n"); }
    ;

/* 6. UNARY (NOT, -) */
unary_expr:
      power_expr

    | TOK_MINUS   power_expr { printf($2.type == TYPE_INT ? "i32.const 0\ni32.sub\n" : "f32.neg\n"); }
    | TOK_NOT power_expr { printf("i32.eqz\n"); }
    ;

/* 7. POWER (**) */
power_expr:
      factor
    | factor TOK_POWER factor { printf("call $math_pow\n"); }
    ;

/* 8. PRIMARY (Highest Precedence) */
factor:
      TOK_NUMBER
    | TOK_STRING
    | TOK_WSTRING

    | TOK_TIME      /* T#5s */
    | TOK_DATE      /* D#2024-01-01 */
    | TOK_DATE_TIME      /* DT#2024-01-01 */
    | TOK_TIME_OF_DAY    /* TOD#2024-01-01 */
    | TOK_TRUE
    | TOK_FALSE
    /* function call */
    | instance_path TOK_LPAREN arg_list TOK_RPAREN {
          /* 
             1. Identify if it's a standard function or DCS utility.
             2. For utilities, push the g_pUtils handle first.
             3. Generate the Wasm 'call' instruction.
          */
          printf("function call $%s\n", $1);
        }
    | l_value
    | TOK_LPAREN full_expression TOK_RPAREN
    ;

if_statement:
    TOK_IF full_expression TOK_THEN statements TOK_ELSE statements TOK_END_IF
    ;

opt_by_step:
    /* empty */
    | TOK_BY full_expression

for_statement:
    TOK_FOR instance_path TOK_ASSIGN full_expression TOK_TO full_expression opt_by_step TOK_DO statements TOK_END_FOR
    ;

while_statement:
    TOK_WHILE full_expression TOK_DO statements TOK_END_WHILE
    ;

repeat_statement:
    TOK_REPEAT statements TOK_UNTIL full_expression TOK_END_REPEAT
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
    | TOK_ID TOK_OUTPUT_ASSIGN l_value           /* Formal Output: Q => MyLamp */
    ;

case_statement:
    TOK_CASE full_expression TOK_OF
        case_element_list
        opt_else_statement
    TOK_END_CASE TOK_SEMICOLON
    ;

opt_else_statement:
    /* empty */
    |TOK_ELSE statements
    ;

case_element_list:
    case_element
    | case_element_list case_element 
    ;

cinner_statements:
    case_body
    | case_body TOK_SEMICOLON cinner_statements
    ;

case_body:
    assignment_statement 
    | if_statement 
    | for_statement 
    | while_statement 
    | repeat_statement
    | function_call_statement
    | case_statement
    ;

case_element:
    case_list_selector cinner_statements TOK_SEMICOLON
      /* TOK_CASE_SEP does not come from source, it is used to resolve the shift/reduce conflict */
    | case_list_selector cinner_statements TOK_CASE_SEP
    ;

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
    instance_path TOK_AT direct_address TOK_COLON type_spec TOK_SEMICOLON
    instance_path TOK_AT direct_address TOK_COLON type_spec TOK_ASSIGN initial_value TOK_SEMICOLON
    ;

instance_path:
    TOK_ID
    | instance_path TOK_DOT TOK_ID  /* e.g., MainProg.Motor1.SensorIn */
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}

void ParserPrint(
    const char* szFile,
    gint32 iLineNo,
    const char* strMsg )
{
    char szBuf[ 512 ];
    sprintf( szBuf, "%s(%d): %s",
        szFile, iLineNo, strMsg );
    fprintf( stderr, szBuf );
}

FILECTX2::FILECTX2()
{
    m_oVal.Clear(); 
    m_oLocation.first_line =
    m_oLocation.first_column =  
    m_oLocation.last_line =
    m_oLocation.last_column = 1;
}

FILECTX2::FILECTX2( const std::string& strPath )
    : FILECTX2()
{
    m_fp = fopen( strPath.c_str(), "r");
    if( m_fp != nullptr )
    {
        m_strPath = strPath;
    }
    else
    {
        std::string strMsg = "error cannot open file '";
        strMsg += strPath + "'";
        throw std::invalid_argument( strMsg );
    }
}

FILECTX2::~FILECTX2()
{
    if( m_fp != nullptr )
    {
        fclose( m_fp );
        m_fp = nullptr;
    }
}

FILECTX2::FILECTX2( const FILECTX2& rhs )
{
    m_fp = rhs.m_fp;
    m_oVal = rhs.m_oVal;
    m_strPath = rhs.m_strPath;
    m_strLastVal = rhs.m_strLastVal;
    memcpy( &m_oLocation,
        &rhs.m_oLocation,
        sizeof( m_oLocation ) );
}

FILE* CSTParserContext::TryOpenFile(
    const std::string& strFile )
{
    FILE* fp = fopen( strFile.c_str(), "r");
    if ( fp != nullptr )
        return fp;

    for( auto elem : m_vecInclPaths )
    {
        std::string strPath =
            elem + "/" + strFile;
        fp = fopen( strPath.c_str(), "r" );
        if( fp != nullptr )
            break;
    }
    return fp;
}

