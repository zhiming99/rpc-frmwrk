/*
 * =====================================================================================
 *
 *       Filename:  st_parser.y
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
%{
#include <stdio.h>
#include <stdlib.h>
%}

%token TOK_PROGRAM TOK_VAR TOK_END_VAR TOK_IF TOK_THEN TOK_ELSE TOK_END_IF CASE OF END_CASE TOK_FOR TOK_TO TOK_DO TOK_END_FOR TOK_WHILE TOK_END_WHILE TOK_REPEAT TOK_UNTIL TOK_END_REPEAT
%token TOK_TON TOK_TON_VALUE TOK_STRING TOK_WSTRING TOK_INT TOK_REAL TOK_LREAL TOK_BOOL TOK_TRUE TOK_FALSE TOK_TIME TOK_TYPED_LITERAL TOK_TYPE TOK_END_TYPE TOK_STRUCT TOK_END_STRUCT
%token TOK_UINT TOK_DINT TOK_UDINT TOK_SINT TOK_USINT TOK_BYTE TOK_WORD TOK_DWORD

%token TOK_ID TOK_NUMBER TOK_ASSIGN TOK_SEMICOLON TOK_COLON TOK_COMMA TOK_ARRAY TOK_RANGE TOK_DOT
%token TOK_PLUS TOK_MINUS TOK_MULTIPLY TOK_DIVIDE TOK_MOD TOK_NOT TOK_AND TOK_OR TOK_XOR TOK_DATE TOK_TIME_OF_DAY TOK_DATE_TIME TOK_ABS_ADDR_PERIPHERAL TOK_ABS_ADDR_BIT TOK_ABS_ADDR_BLOCK

%token TOK_EQUAL TOK_POWER TOK_LBRACKET TOK_RBRACKET TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_LE TOK_GT TOK_NEQU TOK_NLE TOK_NGT
               
%token TOK_FUNCTION_BLOCK TOK_FUNCTION TOK_END_FUNCTION_BLOCK TOK_END_FUNCTION TOK_END_PROGRAM TOK_INCLUDE
%token TOK_VAR_INPUT TOK_VAR_OUTPUT TOK_VAR_IN_OUT TOK_VAR_GLOBAL TOK_CONSTANT TOK_PUNC TOK_VAR_TEMP TOK_AT TOK_VAR_EXTERNAL TOK_RETAIN TOK_PERSISTENT

%token TOK_TIME_TYPE TOK_TIME_OF_DAY_TYPE TOK_DATE_TYPE TOK_STRING_TYPE TOK_WSTRING_TYPE TOK_COMMENT TOK_BY TOK_CASE TOK_END_CASE TOK_OF


%%

source_file:
    include_files
    | pou_list
    | type_definition_block
    | global_var
    ;

pou_list:
    pou_declaration
    | pou_list pou_declaration
    ;

global_var:
    TOK_VAR_GLOBAL var_list TOK_END_VAR 
    | TOK_VAR_GLOBAL TOK_CONSTANT var_list TOK_END_VAR 

pou_declaration:
    program
    | function_block
    | function
    ;

program:
    | TOK_PROGRAM TOK_ID program_unit TOK_END_PROGRAM
    ;

include_files:
    TOK_INCLUDE TOK_STRING TOK_SEMICOLON
    | include_files TOK_INCLUDE TOK_STRING TOK_SEMICOLON

program_unit:
    var_declarations body 

body:
    statements

type_definition_block:
      TOK_TYPE type_assignments TOK_END_TYPE
    ;

type_assignments:
      type_assignment
    | type_assignments type_assignment
    ;

enum_value_list:
    TOK_ID
    | enum_value_list TOK_COMMA TOK_ID

assign_enum_val:
    | TOK_ID
    | TOK_ID TOK_PUNC TOK_ID

enum_type_head:
    TOK_ID TOK_COLON TOK_LPAREN enum_value_list TOK_RPAREN

enum_type_definition:
    enum_type_head TOK_SEMICOLON
    | enum_type_head TOK_ASSIGN assign_enum_val TOK_SEMICOLON

subrange_type_definition:
    TOK_ID TOK_COLON type TOK_LPAREN subrange TOK_RPAREN TOK_SEMICOLON
    |TOK_ID TOK_COLON type TOK_LPAREN subrange TOK_RPAREN TOK_ASSIGN initial_value TOK_SEMICOLON
    ;

subrange:
    signed_integer TOK_RANGE signed_integer
    ;

signed_integer:
    TOK_NUMBER
    | TOK_MINUS TOK_NUMBER
    | TOK_PLUS TOK_NUMBER
    ;

type_assignment:
      TOK_ID TOK_COLON type TOK_SEMICOLON {
        /* Alias: TYPE MyInt : INT; */
        add_alias_to_symtab($1, $3);
      }

    | TOK_ID TOK_COLON struct_definition TOK_SEMICOLON   {
        /* Struct: TYPE Motor : STRUCT... */
        add_struct_to_symtab($1, $3);
    }
    | enum_type_definition
    | subrange_type_definition
    ;

struct_definition:
      TOK_STRUCT member_list TOK_END_STRUCT          { $$ = $2; }
    ;

member_list:
      member_declaration TOK_SEMICOLON               { $$ = create_member_list($1); }
    | member_list member_declaration TOK_SEMICOLON   { $$ = add_to_member_list($1, $2); }
    ;

member_declaration:
      TOK_ID TOK_COLON type               { $$ = create_member($1, $3); }
    ;

var_declarations:
    /* empty */
    | var_declarations TOK_VAR declaration_list TOK_END_VAR
    | var_declarations include_files
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
      full_expression                    /* Simple: := 10; */
    | TOK_LBRACKET init_list TOK_RBRACKET             /* Array: := [1, 2, 3]; */
    | TOK_LPAREN struct_init_list TOK_LPAREN      /* Struct: := (Speed := 10, Run := TRUE); */
    ;

init_list:
      initial_value                    { $$ = create_init_list($1); }
    | init_list TOK_COMMA initial_value      { $$ = add_to_init_list($1, $3); }
    /* ST also supports 'n(value)' for repeating array elements */
    | TOK_NUMBER '(' initial_value ')'    { $$ = create_repeated_init($1, $3); }
    ;

struct_init_list:
      TOK_ID TOK_ASSIGN initial_value    { $$ = create_struct_init($1, $3); }
    | struct_init_list TOK_COMMA TOK_ID TOK_ASSIGN initial_value { $$ = add_to_struct_init($1, $3, $5); }
    ;

var_declaration:
      identifier_list TOK_COLON type                  { $$ = create_decl($1, $3, NULL); }
    | identifier_list TOK_COLON type TOK_ASSIGN initial_value  { $$ = create_decl($1, $3, $5); }
    | TOK_ID TOK_AT direct_address TOK_ASSIGN initial_value
    ;

direct_address:
    TOK_ABS_ADDR_BLOCK
    | TOK_ABS_ADDR_BIT
    | TOK_ABS_ADDR_PERIPHERAL
    ;

identifier_list:
      TOK_ID                    { $$ = create_id_list($1); }
    | identifier_list TOK_COMMA TOK_ID { $$ = add_to_id_list($1, $3); }
    ;

elementry_type:
    TOK_INT
    | TOK_REAL
    | TOK_LREAL
    | TOK_BOOL
    | TOK_WORD
    | TOK_UINT
    | TOK_DINT
    | TOK_UDINT
    | TOK_SINT
    | TOK_USINT
    | TOK_BYTE
    | TOK_DWORD
    | TOK_TIME_TYPE
    | TOK_TIME_OF_DAY_TYPE
    | TOK_DATE_TYPE

array_type:
    TOK_ARRAY TOK_LBRACKET range_list TOK_RBRACKET OF type
    ;

range_list:
      range
    | range_list TOK_COMMA range  /* Supports multi-dimensional arrays */
    ;

range:
    TOK_ID TOK_RANGE TOK_ID    /* e.g., 1..10 */
    ;

string_type:
    TOK_STRING_TYPE TOK_LPAREN TOK_NUMBER TOK_RPAREN    { $$ = create_string_node($3); }  /* Specific length */
    | TOK_STRING_TYPE                    { $$ = create_string_node(80); }  /* Default length is 80 */
    | TOK_WSTRING_TYPE TOK_LPAREN TOK_NUMBER TOK_RPAREN   { $$ = create_wstring_node($3); }
    | TOK_WSTRING_TYPE                   { $$ = create_wstring_node(80); } /* Wide string (UTF-16) */
    ;

type:
    elementry_type
    | array_type
    | derived_type
    | string_type
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

/* Rule for Assignments: Only allows memory locations on the LHS */
assignment_statement:
      l_value TOK_ASSIGN full_expression {
          // Wasm logic to store the result of full_expression into l_value
          // emit_assignment($1, $3);
      }
    ;

/* Rule for Standalone Calls: Used for functions/methods that return void or whose return is ignored */
function_call_statement:
      TOK_ID TOK_LPAREN arg_list TOK_RPAREN {
          // Wasm logic to call the function
          // If it's a DCS Utility, we push the g_pUtils handle first
          printf("%s",$1);
      }
    ;

/* L-Value rule: Strictly limited to writable memory locations */
l_value:
    l_value_var
    /* %Q and %M can be l_value */
    | direct_address
    ;

l_value_var:
      TOK_ID { /* simple variable */ }

    | l_value TOK_LBRACKET full_expression TOK_RBRACKET { /* array element */ }
    | l_value TOK_DOT TOK_ID { /* struct field */ }
    | l_value TOK_DOT TOK_NUMBER { /* bit access */ }
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

    | TOK_TIME    /* T#5s */
    | TOK_DATE      /* D#2024-01-01 */
    | TOK_DATE_TIME      /* DT#2024-01-01 */
    | TOK_TIME_OF_DAY    /* TOD#2024-01-01 */
    | TOK_TRUE
    | TOK_FALSE
    /* function call */
    | TOK_ID TOK_LPAREN arg_list TOK_RPAREN {
          /* 
             1. Identify if it's a standard function or DCS utility.
             2. For utilities, push the g_pUtils handle first.
             3. Generate the Wasm 'call' instruction.
          */
          printf("function call $%s\n", $1);
        }
    | l_value
    | '(' full_expression ')'
    ;

if_statement:
    TOK_IF full_expression TOK_THEN statements TOK_ELSE statements TOK_END_IF
    ;

opt_by_step:
    /* empty */
    | TOK_BY full_expression

for_statement:
    TOK_FOR TOK_ID TOK_ASSIGN full_expression TOK_TO full_expression opt_by_step TOK_DO statements TOK_END_FOR
    ;

while_statement:
    TOK_WHILE full_expression TOK_DO statements TOK_END_WHILE
    ;

repeat_statement:
    TOK_REPEAT statements TOK_UNTIL full_expression TOK_END_REPEAT
    ;

arg_list:
      param_assignment
    | arg_list ',' param_assignment
    ;

param_assignment:
      full_expression             /* Positional: MyFunc(10) */
    | TOK_ID TOK_ASSIGN full_expression  /* Formal Input: IN := True */
    | TOK_ID TOK_NLE l_value           /* Formal Output: Q => MyLamp */
    ;

function_block:
    TOK_FUNCTION_BLOCK TOK_ID var_declarations statements TOK_END_FUNCTION_BLOCK TOK_SEMICOLON
    ;

function:
    TOK_FUNCTION TOK_ID var_declarations statements TOK_END_FUNCTION TOK_SEMICOLON
    ;

case_statement:
    TOK_CASE full_expression TOK_OF
        case_element_list
        opt_else_statement
    TOK_END_CASE TOK_SEMICOLON
    ;

opt_else_statement:
    /* empty */
    | TOK_ELSE statements
    ;

case_element_list:
    case_element

    | case_element_list case_element
    ;

case_element:
    case_list_selector TOK_COLON statements
    ;

case_list_selector:
    case_selector
    | case_list_selector TOK_COMMA case_selector
    ;

case_selector:
    case_constant_expression

    | case_constant_expression TOK_RANGE case_constant_expression
    ;

case_constant_expression:
    full_expression { /* evaluate the full_expression to get a constant value */ }
    ;

%%

int main() {
    yyparse();
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
