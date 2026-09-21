/*
 * stparser.g4 - ANTLR4 Parser for Structured Text Language (IEC 61131-3)
 * Converted from stparser-full.y
 */

parser grammar stparser;

options { tokenVocab=stlexer; }

// ============================================================
// Parser member functions - semantic predicates
// ============================================================

@parser::members {
  // Semantic predicates for var_decl_init_set disambiguation
  virtual bool IsSimpleType() { return true; }
  virtual bool IsArrayType() { return true; }
  virtual bool IsStructType() { return true; }
  virtual bool IsFBType() { return true; }
  virtual bool IsInterfaceType() { return true; }
  virtual bool IsAmbiguousType() { return true; }

  // Semantic predicates for var_decl_init_simple_set disambiguation
  virtual bool IsSimpleSpec() { return true; }
  virtual bool IsSubrangeSpec() { return true; }
  virtual bool IsRefSpec() { return true; }
  virtual bool IsStringSpec() { return true; }
  virtual bool IsAmbiguousSpec() { return true; }

  virtual bool IsElemTypeName() { return true; }
  virtual bool IsSimpleTypeAccess() { return false; }

  bool m_bPass2 = false;

  inline bool IsPass2() const
  { return m_bPass2; }

  void SetPass2( bool bPass2 = true )
  { m_bPass2 = bPass2; }
}

// ============================================================
// Start Point
// ============================================================

start_point
    : decls
    | conditional_pragma
    | case_selector_check
    ;

// ============================================================
// Declarations
// ============================================================

decls
    : ( decl_set )*
    ;

decl_set
    : namespace_element
    | config_decl
    | access_decl
    | using_directive
    ;

// ============================================================
// Namespace
// ============================================================

namespace_element
    : global_var_decls
    | data_type_decl
    | func_decl
    | prog_decl
    | fb_decl
    | class_decl
    | interface_decl
    | namespace_decl
    ;

namespace_elements
    : namespace_element+
    ;

namespace_name
    : identifier
    ;

namespace_decl
    : TOK_NAMESPACE opt_internal namespace_name ( TOK_DOT namespace_name )*
    using_directive_list? namespace_elements TOK_END_NAMESPACE
    ;

namespace_global_name
    :TOK_UNDERSCORE
    ;

namespace_hierarchy
    : namespace_global_name  ( TOK_DOT namespace_name )*
    | namespace_name ( TOK_DOT namespace_name )*
    ;

// ============================================================
// Configuration
// ============================================================

config_decl
    : TOK_CONFIGURATION identifier
      opt_global_var_decls
      resource_decl
      access_decls_opt
      opt_config_init
      TOK_END_CONFIGURATION
    ;

opt_global_var_decls
    : /* empty */
    | global_var_decls
    ;

resource_decl
    : single_resource_decl
    | resource_decl_list
    ;

resource_decl_list
    : resource_declaration+
    ;

resource_declaration
    : TOK_RESOURCE identifier TOK_ON identifier
      opt_global_var_decls
      single_resource_decl
      TOK_END_RESOURCE
    ;

single_resource_decl
    : resource_body
    ;

resource_body
    : task_configuration_list
    | program_configuration_list
    | access_decls
    | resource_init
    ;

// ============================================================
// Task Configuration
// ============================================================

task_configuration_list
    : task_config+
    ;

task_config
    : TOK_TASK identifier task_init
    ;

task_init
    : TOK_SINGLE opt_single_init
    | TOK_INTERVAL opt_interval_init
    | priority_init
    ;

opt_single_init
    : /* empty */
    | TOK_ASSIGN symbolic_variable
    ;

opt_interval_init
    : /* empty */
    | TOK_ASSIGN time_literal
    ;

priority_init
    : TOK_PRIORITY TOK_ASSIGN TOK_INT
    ;

// ============================================================
// Program Configuration
// ============================================================

program_configuration_list
    : prog_config+
    ;

prog_config
    : TOK_PROGRAM identifier
      opt_prog_conf_elems
      retain?
      identifier
      opt_with_task
      TOK_COLON
      prog_type_access
      TOK_LPAREN
      prog_cnxn
      TOK_RPAREN
      TOK_SEMICOLON
      access_decls_opt
      config_init_opt
    ;

opt_prog_conf_elems
    : /* empty */
    | TOK_LPAREN prog_conf_elements TOK_RPAREN
    ;

prog_conf_elements
    : prog_conf_elem ( TOK_COMMA prog_conf_elem)*
    ;

prog_conf_elem
    : fb_task
    | prog_cnxn
    ;

fb_task: fb_instance_name TOK_WITH TOK_ID
    ;

opt_with_task
    : /* empty */
    | TOK_WITH identifier
    ;

prog_type_access
    : instance_path
    ;

prog_cnxn
    : /* empty */
    | prog_cnxn TOK_COMMA data_sink
    ;

// ============================================================
// Data Source/Sink
// ============================================================

data_sink
    : symbolic_variable TOK_ASSIGN expression
    | symbolic_variable TOK_ASSIGN data_source
    | symbolic_variable
    ;

data_source
    : symbolic_variable
    | tokid_dot symbolic_variable
    | TOK_ID TOK_DOT symbolic_variable
    ;

tokid_dot
    : TOK_ID TOK_DOT
    ;

opt_tokid_dot
    : /* empty */
    | tokid_dot
    ;

opt_dot_tokid
    : /* empty */
    | dot_tokid
    ;

dot_tokid
    : TOK_ID TOK_DOT
    ;

// ============================================================
// Variable Access
// ============================================================

global_var_access
    : opt_tokid_dot identifier opt_dot_tokid
    ;

symbolic_variable
    : ( this_notation | instance_path TOK_DOT )? var_access_or_multi_elem
    ;

var_access_or_multi_elem
    : var_access multi_elem_chain?
    ;

var_access
    : ref_deref
    | variable_name
    ;

variable_name
    : identifier
    ;

multi_elem_chain
    : ( subscript_list | struct_variable )+
    ;

struct_variable
    : TOK_DOT struct_elem_select
    ;

struct_elem_select
    : var_access
    ;

subscript_list
    : TOK_LBRACKET expr_list TOK_RBRACKET
    ;

expr_list
    : full_expression ( TOK_COMMA full_expression )*
    ;

this_notation
    : TOK_THIS TOK_DOT
    ;

// ============================================================
// Instance Path
// ============================================================

instance_path
    : namespace_hierarchy
    ;

global_name_space
    : TOK_DOT
    | TOK_UNDERSCORE TOK_DOT
    ;

// ============================================================
// Expression
// ============================================================

full_expression
    : xor_expr (TOK_OR xor_expr)*
    ;

xor_expr
    : and_expr ( TOK_XOR and_expr )*
    ;

and_expr
    : compare_expr (TOK_AND compare_expr)*
    ;

compare_expr
    : equ_expr ( (TOK_EQUAL|TOK_NEQU) equ_expr)*
    ;

equ_expr
    : add_expr ( ( TOK_LT | TOK_LE | TOK_GT | TOK_NLE ) add_expr )*
    ;

add_expr
    : term ( ( TOK_ADD | TOK_MINUS) term )*
    ;

term
    : power_expr (( TOK_MUL | TOK_DIV | TOK_MOD ) power_expr )*
    ;

power_expr
    : unary_expr (TOK_POWER unary_expr)*
    ;

unary_expr
    : ( TOK_NOT | TOK_MINUS | TOK_CARET )? primary_expr
    ;

primary_expr
    : constant
    | variable_access
    | enum_value
    | namedval_value
    | func_call
    | ref_value
    | TOK_LPAREN full_expression TOK_RPAREN
    ;

variable_access
    : symbolic_variable multibit_part_access?
    ;

func_call
    : func_access TOK_LPAREN param_assign_list? TOK_RPAREN
    ;

derived_func_name
    : identifier
    ;

// ============================================================
// Constants
// ============================================================

constant
    : numeric_literal
    | string_literal
    | time_literal
    | bit_str_literal
    | bool_literal
    ;

numeric_literal
    : TOK_INT
    | TOK_REAL
    ;

time_literal
    : TOK_TIME
    | TOK_LTIME
    | TOK_DATE
    | TOK_TIME_OF_DAY
    | TOK_DATE_TIME
    ;

string_literal
    : TOK_STRING
    | TOK_WSTRING
    | TOK_USTRING
    ;

bool_value
    : TOK_INT
    | TOK_TRUE
    | TOK_FALSE
    ;

bool_literal
    : bool_value
    | bool_type_name TOK_PUNC bool_value
    ;

bit_str_literal
    : TOK_INT
    | bit_str_type_name TOK_PUNC TOK_INT
    ;

bool_type_name
    : TOK_BOOL
    ;

// ============================================================
// Statements
// ============================================================

statement
    : assignment_statement
    | assignment_attempt
    | selection_statement
    | iteration_statement
    | subprog_ctrl_stmt
    ;

stmt_list
    : ( statement semicolons )* 
    ;

assignment_statement
    // extended the iec 61131-3 spec. the origin
    // syntax is 'expression TOK_ASSIGN expression'
    : expression (TOK_ASSIGN expression )*
    ;

assignment_attempt
    : expression TOK_QASSIGN expression 
    ;

subprog_ctrl_stmt:
    func_call
    | invocation
    | TOK_SUPER TOK_LPAREN TOK_RPAREN
    | TOK_RETURN
    ;

method_name:
    TOK_ID
    ;

fb_inst_name_or_class_inst_name:
    fb_instance_name
    | class_instance_name
    ;

invocation_header
    : fb_instance_name
    | method_name
    | TOK_THIS
    | ( TOK_THIS TOK_DOT )? ((fb_inst_name_or_class_inst_name TOK_DOT)+) method_name
    ;

invocation:
    invocation_header TOK_LPAREN param_assign_list? TOK_RPAREN
    ;
    

selection_statement
    : if_statement
    | case_statement
    ;

if_statement
    : TOK_IF expression TOK_THEN stmt_list
      elseif_statement*
      else_branch?
      TOK_END_IF
    ;

elseif_statement
    : TOK_ELSIF expression TOK_THEN stmt_list
    ;

else_branch
    : TOK_ELSE stmt_list
    ;

case_statement
    : TOK_CASE expression TOK_OF
      case_selection+
      else_statement?
      TOK_END_CASE
    ;

case_selection
    : case_list TOK_COLON stmt_list
    ;

case_list
    : case_list_elem (TOK_COMMA case_list_elem)*
    ;

case_list_elem
    : subrange
    | expression
    ;

iteration_statement
    : for_statement
    | while_statement
    | repeat_statement
    | TOK_EXIT
    | TOK_CONTINUE
    ;

for_statement
    : TOK_FOR control_variable TOK_ASSIGN for_list TOK_DO stmt_list TOK_END_FOR
    ;

control_variable
    : identifier
    ;

for_list
    : expression TOK_TO expression (TOK_BY expression)?
    ;

while_statement
    : TOK_WHILE expression TOK_DO stmt_list
      TOK_END_WHILE
    ;

repeat_statement
    : TOK_REPEAT stmt_list
      TOK_UNTIL expression
      TOK_END_REPEAT
    ;

expression
    : full_expression
    ;

ref_assign:
    ref_name TOK_ASSIGN ( ref_name | ref_value | ref_deref )
    ;

// ============================================================
// Function Block Declaration
// ============================================================

fb_decl
    : TOK_FUNCTION_BLOCK opt_internal fb_modifier? derived_fb_name
      using_directive_list?
      extends_clause?
      implements_clause?
      fb_var_decls*
      method_or_property_list*
      opt_fb_body
      TOK_END_FUNCTION_BLOCK
    ;

opt_fb_body
    : /* empty */ 
    |fb_body
    ;

fb_body
    : stmt_list
//  | SFC
//  | ladder_diagram
//  | fb_diagram
//  | other_languages
    ;

fb_decl_init
    : fb_decl_no_init (TOK_ASSIGN struct_init)?
    ;

fb_decl_no_init
    : variable_list TOK_COLON fb_type_access
    ;

fb_modifier
    : TOK_ABSTRACT
    | TOK_FINAL
    ;

derived_fb_name
    : TOK_ID
    ;

fb_name
    : derived_fb_name
//  | std_fb_name
    ;


using_directive_list
    : using_directive+
    ;

using_directive
    : TOK_USING instance_path (TOK_COMMA instance_path)* semicolons
    ;

extends_clause
    :TOK_EXTENDS instance_path
    ;

implements_clause
    :TOK_IMPLEMENTS interface_name_list
    ;

interface_name_list
    : instance_path (TOK_COMMA instance_path)*
    ;

fb_var_decls
    : fb_io_var_decls
    | func_var_decls
    | temp_var_decls
    | other_var_decls
    ;

fb_io_var_decls
    : fb_input_decls
    | fb_output_decls
    | in_out_decls
    ;

fb_input_decls
    : TOK_VAR_INPUT retain? fb_input_decl_list* TOK_END_VAR
    ;

fb_input_decl_list
    : ( fb_input_decl semicolons )+
    ;

fb_input_decl
    : var_decl_init
//  | edge_decl
    ;

fb_output_decls
    : TOK_VAR_OUTPUT retain? fb_output_decl_list* TOK_END_VAR
    ;

fb_output_decl_list
    : (fb_output_decl semicolons)+
    ;

fb_output_decl
    : var_decl_init
    ;

func_var_decls
    : external_var_decls
    | var_decls
    ;

external_decls
    : (external_decl semicolons)+
    ;

external_decl
    : instance_path TOK_COLON external_member_init
    ;

var_decls
    : TOK_VAR opt_constant access_spec? opt_var_decls_init TOK_END_VAR
    ;

var_decl
    : identifier_list TOK_COLON var_member_init
    ;

var_decl_init
    : {IsSimpleType()}? variable_list TOK_COLON var_decl_init_set
    | {IsArrayType()}? array_var_decl_init
    | {IsStructType()}? struct_var_decl_init
    | {IsFBType()}? fb_decl_init
    | {IsInterfaceType()}? interface_spec_init
    | variable_list TOK_COLON ambiguous_var_decl_init
    ;

ambiguous_var_decl_init
    : array_type_access (TOK_ASSIGN array_init)?
    | struct_type_access ( TOK_ASSIGN struct_init )?
    | fb_type_access ( TOK_ASSIGN struct_init )?
    | interface_type_access ( TOK_ASSIGN interface_value )?
    | ambiguous_spec_decl
    ;

identifier_list
    : identifier (TOK_COMMA identifier)*
    ;

identifier
    : TOK_ID
    ;

temp_var_decls
    : TOK_VAR_TEMP temp_var_decl_list? TOK_END_VAR
    ;

temp_var_decl_list
    : (temp_var_decl semicolons)+
    ;

temp_var_decl
    : var_decl
    | ref_var_decl
    | interface_var_decl
    ;

loc_var_decl
    : variable_name locate_at direct_variable TOK_COLON loc_var_init
    ;

loc_var_init
    : simple_spec_init
    | array_spec_init
    | struct_spec_init
    | string_spec_init
    ;

ref_var_decl
    : variable_list TOK_COLON ref_spec
    ;

interface_type_access
    : instance_path
    ;

interface_var_decl
    : variable_list TOK_COLON interface_type_access
    ;

other_var_decls
    : retain_var_decls
    | no_retain_var_decls
    ;

retain_var_decls
    : TOK_VAR TOK_RETAIN access_spec? var_decls TOK_END_VAR
    ;

no_retain_var_decls
    : TOK_VAR TOK_NON_RETAIN access_spec? var_decls TOK_END_VAR
    ;

retain
    : TOK_RETAIN
    | TOK_NON_RETAIN
    ;

access_spec
    : TOK_PRIVATE
    | TOK_PUBLIC
    | TOK_PROTECTED
    | TOK_INTERNAL
    ;

method_or_property_list
    : method_or_property+
    ;

method_or_property
    : method_decl
    | property_decl
    ;

opt_data_type_access
    : /* empty */
    | TOK_COLON data_type_access
    ;

opt_io_var_decls
    : /* empty */
    | io_var_decls
    ;

io_var_decls
    : input_decls
    | output_decls
    | in_out_decls
    ;

input_decls
    : TOK_VAR_INPUT retain? input_decl_list? TOK_END_VAR
    ;

input_decl_list
    : ( input_decl semicolons )+
    ;

input_decl
    : var_decl_init
    | array_conform_decl
    ;

output_decls
    : TOK_VAR_OUTPUT retain? output_decl_list? TOK_END_VAR
    ;

output_decl_list
    : ( output_decl semicolons )+
    ;

output_decl
    : input_decl
    ;

setter_or_getter
    : TOK_PROPERTY_SET
    | TOK_PROPERTY_GET
    ;

opt_override
    : /* empty */
    | TOK_OVERRIDE
    ;

property_var_decl
    : func_var_decls
    | temp_var_decls
    ;

// ============================================================
// Function Declaration
// ============================================================

func_decl
    : TOK_FUNCTION opt_internal derived_func_name
      opt_data_type_access
      using_directive_list?
      func_var_decls_set*
      func_body
      TOK_END_FUNCTION
    ;

opt_internal
    : /* empty */
    | TOK_INTERNAL
    ;

func_var_decls_set
    : io_var_decls
    | func_var_decls
    | temp_var_decls
    ;

func_body
    : stmt_list
//  | ladder_diagram
//  | fb_diagram
//  | other_languages
    ;

// ============================================================
// Type Declaration
// ============================================================

data_type_decl
    : TOK_TYPE opt_override using_directive_list? type_decl TOK_SEMICOLON TOK_END_TYPE
    ;

type_decl
    : simple_type_decl
    | subrange_type_decl
    | enum_type_decl
    | namedval_spec_init
    | array_type_decl
    | struct_type_decl
    | string_type_decl
    | ref_type_decl
    | ambiguous_type_decl
    ;

simple_type_decl
    : simple_type_name TOK_COLON simple_spec_init
    ;

simple_spec_init
    : simple_spec ( TOK_ASSIGN expression )?
    ;

simple_spec
    : elem_type_name
    | simple_type_access
    ;

date_type_name
    : TOK_TIME_TYPE
    | TOK_LTIME_TYPE
    ;

time_type_name
    : TOK_DATE_TYPE
    | TOK_LDATE_TYPE
    ;

elem_type_name
    : numeric_type_name
    | bit_str_type_name
    | string_simple_type
    | date_type_name
    | time_type_name
    ;

string_simple_type
    : TOK_STRING_TYPE
    | TOK_WSTRING_TYPE 
    | TOK_USTRING_TYPE
    | TOK_CHAR_TYPE
    | TOK_WCHAR_TYPE
    | TOK_UCHAR_TYPE
    ;

simple_type_name
    : identifier 
    ;

simple_type_access
    : instance_path
    ;

subrange_type_decl
    : subrange_type_name TOK_COLON subrange_spec_init
    ;

subrange_spec
    : int_type_name TOK_LPAREN subrange TOK_RPAREN
    | subrange_type_access
    ;

subrange_type_access
    : instance_path
    ;

subrange_spec_init
    : subrange_spec (TOK_ASSIGN TOK_INT)?
    ;

subrange_type_name
    : identifier
    ;

int_type_name
    : signed_int_name
    | unsigned_int_name
    ;

subrange
    : expression TOK_RANGE expression
    ;

enum_type_name
    : identifier
    ;

enum_type_decl
    : enum_type_name TOK_COLON enum_spec_init
    ;

enum_spec_init
    : TOK_LPAREN ( identifier ( TOK_COMMA identifier )* ) TOK_RPAREN
    ( TOK_ASSIGN enum_value )?
    ;

enum_value
    : ( enum_type_access TOK_PUNC )? identifier
    ;

enum_type_access
    : instance_path
    ;

namedval_type_name
    : identifier
    ;

namedval_type_decl
    : namedval_type_name TOK_COLON namedval_type namedval_spec_init
    ;

namedval_type
    :int_type_name
    | multibits_type_name
    ;

namedval_spec_init
    : TOK_LPAREN namedval_spec_list TOK_RPAREN ( TOK_ASSIGN expression )?
    ;

namedval_spec_list
    : namedval_spec ( TOK_COMMA namedval_spec )*
    ;

namedval_spec
    : identifier TOK_ASSIGN expression
    ;

namedval_value
    : ( namedval_type_access TOK_PUNC )? identifier
    ;

namedval_type_access
    : instance_path
    ;
    
array_type_decl
    : array_type_name TOK_COLON array_spec_init
    ;

array_spec_init
    : array_spec (TOK_ASSIGN array_init)?
    ;

array_spec
    : TOK_ARRAY TOK_LBRACKET range_list TOK_RBRACKET TOK_OF data_type_access
    | array_type_access
    ;

array_type_access
    : instance_path
    ;

array_var_decl_init
    : variable_list TOK_COLON array_spec_init
    ;

range_list
    : subrange ( TOK_COMMA subrange )*
    ;

array_init
    : TOK_LBRACKET array_elem_init_list TOK_RBRACKET
    ;

array_elem_init_list
    : array_elem_init (TOK_COMMA array_elem_init)*
    ;

array_elem_init
    : array_elem_init_value
    | TOK_INT TOK_LPAREN array_elem_init_list TOK_RPAREN
    ;

array_elem_init_value
    : expression 
    | enum_value
    | struct_init
    | array_init
    ;

struct_type_decl
    : struct_type_name TOK_COLON struct_decl
    ;

struct_type_decl_init
    : struct_spec
    ;

struct_spec_init
    : struct_type_access (TOK_ASSIGN struct_init)?
    ;

struct_init
    : TOK_LPAREN struct_elem_init_list TOK_RPAREN
    ;

struct_elem_init_list
    : struct_elem_init (TOK_COMMA struct_elem_init)*
    ;
struct_elem_init_value
    : array_elem_init_value
    ;

struct_elem_init
    : struct_elem_name TOK_ASSIGN struct_elem_init_value
    | struct_elem_name TOK_ASSIGN ref_value
    ;

string_type_decl
    : string_type_name TOK_COLON string_spec_init
    ;

string_spec_init
    : string_spec (TOK_ASSIGN string_literal)?
    ;

string_type_name
    : identifier
    ;

string_type_access
    : instance_path
    ;

ref_type_name
    : identifier
    ;

ref_type_decl
    : ref_type_name TOK_COLON ref_spec_init TOK_SEMICOLON
    ;

ref_spec_init
    : ref_spec (TOK_ASSIGN ref_value)?
    ;

ref_spec
    : TOK_REF_TO+ data_type_access
    ;

ref_type_access:
    instance_path
    ;

ref_value
    : ref_addr
    | TOK_NULL
    ;

ref_addr
    : TOK_REF TOK_LPAREN
    ( symbolic_variable | fb_instance_name | class_instance_name )
    TOK_RPAREN
    ;

ref_deref
    : ref_name caret_list
    ;

ref_name
    : TOK_ID
    ;

caret_list
    : TOK_CARET (TOK_CARET)*
    ;

ambiguous_type_decl
    : variable_list TOK_COLON instance_path (TOK_ASSIGN spec_type_accesses)?
    ;

spec_type_accesses
    : simple_type_access
    | struct_spec_init
    ;

// ============================================================
// Class Declaration
// ============================================================

class_decl
    : TOK_CLASS class_modifier identifier
      extends_clause?
      implements_clause?
      class_var_decls_list?
      class_method_or_property_list?
      TOK_END_CLASS
    ;

class_modifier
    : /* empty */
    | TOK_ABSTRACT
    | TOK_FINAL
    ;

class_var_decls_list
    : class_var_decls+
    ;

class_var_decls
    : func_var_decls
    | other_var_decls
    ;

class_method_or_property_list
    : class_method_or_property+
    ;

class_method_or_property
    : method_decl
    | property_decl
    ;

// ============================================================
// Interface Declaration
// ============================================================

interface_decl
    : TOK_INTERFACE opt_internal identifier
      using_directive_list?
      interface_extends_clause?
      method_or_property_prototypes?
      TOK_END_INTERFACE
    ;

interface_extends_clause
    : TOK_EXTENDS interface_name_list
    ;

method_or_property_prototypes
    : method_or_property_prototype+
    ;

method_or_property_prototype
    : method_prototype
    | property_prototype
    ;

method_prototype
    : TOK_METHOD identifier opt_data_type_access io_var_decls* TOK_END_METHOD
    ;

property_prototype
    : setter_or_getter identifier TOK_COLON data_type_access TOK_END_PROPERTY
    ;

// ============================================================
// Data Type Access
// ============================================================

data_type_access
    : elem_type_name
    | derived_type_access
    ;

derived_type_access
/*    : single_elem_type_access
    | array_type_access
    | struct_type_access
    | string_type_access
    | fb_type_access
    | class_type_access
    | ref_type_access
    | interface_type_access
*/
    :instance_path
    ;

single_elem_type_access
    : simple_type_access
    | subrange_type_access
    | enum_type_access
    ;

array_type_name
    : identifier
    ;

struct_type_name
    : identifier
    ;

class_type_access
    : instance_path
    ;

// ============================================================
// Access Declarations
// ============================================================

access_decls_opt
    : /* empty */
    | access_decls
    ;

access_decls
    : TOK_VAR_ACCESS access_decl_list TOK_END_VAR
    ;

access_decl_list
    : access_decl TOK_SEMICOLON
    | access_decl_list access_decl TOK_SEMICOLON
    ;

access_decl
    : identifier TOK_COLON access_path
    ;

access_path
    : identifier
    | access_path TOK_DOT identifier
    ;

access_direction
    : TOK_READ_ONLY
    | TOK_READ_WRITE
    ;

// ============================================================
// Initialization
// ============================================================

opt_config_init
    : /* empty */
    | config_inst_init
    ;

config_inst_init
    : /* empty */
    | config_inst_init instance_specific_init
    ;

resource_init
    : /* to be defined */
    ;

config_init_opt
    : /* empty */
    | config_inst_init
    ;

initial_value
    : constant
    | array_init
    | struct_init
    | ref_value
    ;

// ============================================================
// Pragma
// ============================================================

pragma_statement
    : /* to be defined */
    ;

conditional_pragma
    : TOK_IF expression TOK_RBRACE
    ;

case_selector_check
    : expression
    ;

// ============================================================
// Additional Rules
// ============================================================

variable_list
    : identifier_list
    ;

struct_type_access
    : instance_path
    ;

locate_at
    : TOK_AT direct_variable
    ;

direct_variable
    : TOK_ABS_ADDR_PERIPHERAL
    | TOK_RPCF_ADDR
    ;

fb_type_access
    : instance_path
    ;

loc_var_spec_init
    : simple_spec_init
    | array_spec_init
    | struct_spec_init
    | string_spec_init
    ;

opt_constant
    : /* empty */
    | TOK_CONSTANT
    ;

loc_partly_var
    : identifier TOK_AT TOK_PART_ADDR TOK_COLON var_spec semicolons
    ;

var_spec
    : simple_spec
    | array_spec
    | struct_type_access
    | string_spec
    ;

// Virtual tokens for various purposes
prog_output_access
    : tokid_dot symbolic_variable
    ;

prog_config_list
    : prog_config+
    ;

// Array conformant declaration
array_conform_decl
    : variable_list TOK_COLON array_conformand
    ;

array_conformand
    : TOK_ARRAY TOK_LBRACKET varied_length_dim_list TOK_RBRACKET TOK_OF data_type_access
    ;

varied_length_dim_list
    : TOK_MUL ( TOK_COMMA TOK_MUL )*
    ;

// Array var declaration
array_var_decl
    : identifier_list TOK_COLON array_spec_init
    ;

// Bit string type name
bit_str_type_name
    : bool_type_name
    | multibits_type_name
    ;

// Class instance name
class_instance_name
    : instance_path (caret_list)*
    ;

// External member init
external_member_init
    : simple_spec
    | array_spec
    | instance_path
    ;

// External var decls (for functions)
external_var_decls
    : TOK_VAR_EXTERNAL opt_constant external_decls? TOK_END_VAR
    ;

// FB instance or class instance
fb_inst_or_class_inst
    : instance_path
    ;

fb_inst_or_class_inst_list
    : fb_inst_or_class_inst
    | fb_inst_or_class_inst_list fb_inst_or_class_inst
    ;

// Function access
func_name
    : std_func_name 
    | derived_func_name
    ;

std_func_name
    : TOK_SIN
    | TOK_ABS | TOK_SQRT | TOK_LN | TOK_EXP | TOK_LOG
    | TOK_SIN | TOK_COS
    ;

func_access
    : derived_func_name
    ;

// Global var decl
global_var_decl
    : global_var_spec TOK_COLON global_var_type_set
    ;

global_var_name
    : identifier
    ;

global_var_spec
    : global_var_name ( TOK_SEMICOLON global_var_name )*
    | global_var_name locate_at
    ;

global_var_type_set
    : loc_var_spec_init
    | fb_type_access
    ;

// Config init
config_init
    : TOK_VAR_CONFIG config_inst_init TOK_END_VAR
    ;

// More missing rules

// Class method/property decl list
class_method_or_property_decl_list
    : class_method_or_property_decl
    | class_method_or_property_decl_list class_method_or_property_decl
    ;

class_method_or_property_decl
    : method_decl
    | property_decl
    ;

// Method decl
method_decl
    : TOK_METHOD identifier opt_data_type_access method_var_decls* func_body TOK_END_METHOD
    ;

// Method decl list
method_decl_list
    : method_decl+
    ;

// Method var decls
method_var_decls
    : io_var_decls
    | func_var_decls
    | temp_var_decls
    ;

// Global var decls
global_var_decls
    : TOK_VAR_GLOBAL opt_internal retain_or_constant? global_var_decl_list? TOK_END_VAR
    ;

retain_or_constant
    : TOK_RETAIN
    | TOK_CONSTANT
    ;

global_var_decl_list
    : ( global_var_decl TOK_SEMICOLON )+
    ;

// Identifier dot list
identifier_dot_list
    : identifier
    | identifier_dot_list TOK_DOT identifier
    ;

// In/out decls
in_out_decls
    : TOK_VAR_IN_OUT in_out_var_decl_list? TOK_END_VAR
    ;

in_out_var_decl_list
    : (in_out_var_decl TOK_SEMICOLON)+
    ;

in_out_var_decl
    : var_decl
    | fb_decl_no_init
    | array_conform_decl
    ;

// Interface spec init
interface_spec_init
    : variable_list (TOK_ASSIGN interface_value)?
    ;

// Interface value
interface_value
    : symbolic_variable
    | fb_instance_name
    | class_instance_name
    | TOK_NULL
    ;

// Multibits type name
multibits_type_name
    : TOK_BYTE
    | TOK_WORD
    | TOK_DWORD
    | TOK_LWORD
    ;

// Multi-part access
multibit_part_access 
    : TOK_DOT TOK_INT
    | TOK_DOT TOK_MULTPART_ACCESS
    ;

// Instance specific init
instance_specific_init
    : resource_name TOK_DOT prog_name TOK_DOT instance_tail
    ;

// Resource name
resource_name
    : identifier
    ;

// Program name
prog_name
    : identifier
    ;

// Instance tail
instance_tail
    : opt_fb_inst_or_class_inst_list instance_tail1 
    ;

// Instance tail 1
instance_tail1
    : identifier locate_at TOK_COLON loc_var_spec_init
    | instance_tail2 TOK_ASSIGN struct_init
    ;

// Instance tail 2
instance_tail2
    : fb_instance_name TOK_COLON fb_type_access
    | class_instance_name TOK_COLON class_type_access
    ;

// FB instance name
fb_instance_name
    : instance_path (caret_list)*
    ;

// Numeric type name
numeric_type_name
    : int_type_name
    | real_type_name
    ;

// Optional by step
opt_by_step
    : /* empty */
    | TOK_BY expression
    ;

// Optional else statement
else_statement
    : TOK_ELSE stmt_list
    ;


// Optional FB instance list
opt_fb_inst_or_class_inst_list
    : /* empty */
    | fb_inst_or_class_inst_list
    ;

// Optional FB modifier
opt_fb_modifier
    : /* empty */
    | TOK_ABSTRACT
    | TOK_FINAL
    ;

// Optional in-out var decl list
opt_in_out_var_decl_list
    : /* empty */
    | in_out_var_decl_list
    ;

// Optional locate at
opt_locate_at
    : /* empty */
    | locate_at
    ;

// Optional method var decls
opt_method_var_decls
    : /* empty */
    | method_var_decls
    ;

// Optional overlap
opt_overlap
    : /* empty */
    | TOK_OVERLAP
    ;


// Prog var decls set list
prog_var_decls_set_list
    : (prog_var_decls_set opt_semicolons )+
    ;

// More missing

// Optional semicolons
opt_semicolons
    : /* empty */
    | semicolons
    ;

// Optional temp var member list
opt_temp_var_member_list
    : /* empty */
    | temp_var_member_list
    ;

// Optional var decls init set
opt_var_decls_init
    : /* empty */
    | var_decls_init_set semicolons (var_decls_init_set semicolons)*
    ;

// Param assignment
param_assign
    : (variable_name TOK_ASSIGN)? expression
    | ref_assign
    | TOK_NOT? variable_name TOK_OUTPUT_ASSIGN symbolic_variable
    ;

// Param assignments
param_assign_list
    : param_assign ( TOK_COMMA param_assign )*
    ;

// Pointer
pointer
    : TOK_REF_TO data_type_access
    ;

// Prog access decl
prog_access_decl
    : identifier TOK_COLON symbolic_variable multibit_part_access? TOK_COLON
    data_type_access access_direction?
    ;

// Prog access decls
prog_access_decls
    : TOK_VAR_ACCESS (prog_access_decl semicolons)* TOK_END_VAR
    ;

// Prog decl
prog_decl
    : TOK_PROGRAM opt_internal identifier prog_var_decls_set_list? fb_body TOK_END_PROGRAM
    ;

// Prog var decls set
prog_var_decls_set
    : fb_io_var_decls
    | func_var_decls
    | global_var_decls
    | temp_var_decls
    | other_var_decls
    | prog_access_decls
    ;


// Property decl
property_decl
    : setter_or_getter access_spec? opt_fb_modifier opt_override identifier TOK_COLON data_type_access
      property_var_decl* func_body TOK_END_PROPERTY
    ;

// Real type name
real_type_name
    : TOK_REAL_TYPE
    | TOK_LREAL_TYPE
    ;

// Final batch of missing rules

// Semicolons
semicolons
    : TOK_SEMICOLON
    | semicolons TOK_SEMICOLON
    ;

// Signed int name
signed_int_name
    : TOK_INT_TYPE | TOK_DINT | TOK_LINT | TOK_SINT
    ;

// String list
string_list
    : string_literal
    | string_list TOK_COMMA string_literal
    ;

// String spec
string_spec
    : ( TOK_STRING_TYPE | TOK_WSTRING_TYPE | TOK_USTRING_TYPE )
        ( TOK_LBRACKET TOK_INT TOK_RBRACKET )?
    | TOK_CHAR_TYPE
    | TOK_WCHAR_TYPE
    | TOK_UCHAR_TYPE
    ;

// Struct decl
struct_decl
    : TOK_STRUCT TOK_OVERLAP? struct_elem_decl+ TOK_END_STRUCT
    ;

// Struct elem decl
struct_elem_decl
    : struct_elem_name ( locate_at multibit_part_access )? TOK_COLON struct_elem_init_set
    | var_decl_init
    ;

struct_elem_name
    : identifier
    ;

struct_elem_init_set
    : simple_spec_init
    | subrange_spec_init
    | ref_spec_init
    | string_spec_init
    | array_spec_init
    | struct_spec_init
    ;

// Struct spec
struct_spec
    : struct_decl
    | struct_spec_init
    ;

// Struct var decl
struct_var_decl
    : variable_list TOK_COLON struct_type_access
    ;

// Struct var decl init
struct_var_decl_init
    : variable_list TOK_COLON struct_spec_init
    ;

// Task config list
task_config_list
    : task_config+
    ;

// Temp var member
temp_var_member
    : var_decl
    | ref_var_decl
    | interface_var_decl
    ;

// Temp var member list
temp_var_member_list
    : temp_var_member semicolons
    | temp_var_member_list temp_var_member semicolons
    ;

// Unsigned int name
unsigned_int_name
    : TOK_UINT | TOK_UDINT | TOK_ULINT | TOK_USINT
    ;

// Var decl init set
// Uses semantic predicates to disambiguate based on symbol table lookup
var_decl_init_set
    : var_decl_init_simple_set
    ;

// Ambiguous type reference - unresolved until symbol table is complete
ambiguous_type_ref
    : variable_list TOK_COLON instance_path
    ;

// Var decl init simple set
// Uses semantic predicates to further disambiguate simple type declarations
var_decl_init_simple_set
    : {IsSimpleSpec()}? simple_spec_init
    | {IsSubrangeSpec()}? subrange_spec_init
    | {IsRefSpec()}? ref_spec_init
    | {IsStringSpec()}? string_spec_init
    | ambiguous_spec_decl
    ;

ambiguous_spec_decl
    : simple_type_access ( TOK_ASSIGN expression )?
    | subrange_type_access ( TOK_ASSIGN int_type_name )?
    ;

// Var decls init set
var_decls_init_set
    : var_decl_init
    | loc_var_decl
    | loc_partly_var
    ;

// Var member init
var_member_init
    : simple_spec_init
    | string_spec
    | array_var_decl
    | struct_var_decl
    | struct_decl
    ;

