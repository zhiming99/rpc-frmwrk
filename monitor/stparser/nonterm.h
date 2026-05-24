/*
 * =====================================================================================
 *
 *       Filename:  nonterm.h
 *
 *    Description:  declarations of non-terminal symbols. 
 *
 *        Version:  1.0
 *        Created:  05/07/2026 05:27:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include "rpc.h"

namespace rpcf
{
typedef enum {
  nt_invalid = -10,
  nt_start_point = 146,              /* start_point  */
  nt_source_file = 147,              /* source_file  */
  nt_namespace_name = 148,           /* namespace_name  */
  nt_namespace_elements = 149,       /* namespace_elements  */
  nt_include_directive = 150,        /* include_directive  */
  nt_namespace_element = 151,        /* namespace_element  */
  nt_namespace_declaration = 152,    /* namespace_declaration  */
  nt_global_var = 153,               /* global_var  */
  nt_pou_declaration = 154,          /* pou_declaration  */
  nt_program = 155,                  /* program  */
  nt_program_unit = 156,             /* program_unit  */
  nt_body = 157,                     /* body  */
  nt_type_definition_block = 158,    /* type_definition_block  */
  nt_type_assignments = 159,         /* type_assignments  */
  nt_enum_value_list = 160,          /* enum_value_list  */
  nt_enum_value = 161,               /* enum_value  */
  nt_opt_base_type = 162,            /* opt_base_type  */
  nt_opt_assign_enum_val = 163,      /* opt_assign_enum_val  */
  nt_enum_type_head = 164,           /* enum_type_head  */
  nt_enum_type_definition = 165,     /* enum_type_definition  */
  nt_type_assignment = 166,          /* type_assignment  */
  nt_struct_definition = 167,        /* struct_definition  */
  nt_member_list = 168,              /* member_list  */
  nt_member_declaration = 169,       /* member_declaration  */
  nt_var_declarations = 170,         /* var_declarations  */
  nt_declaration_list = 171,         /* declaration_list  */
  nt_opt_qualifier = 172,            /* opt_qualifier  */
  nt_declaration = 173,              /* declaration  */
  nt_var_list = 174,                 /* var_list  */
  nt_initial_value = 175,            /* initial_value  */
  nt_init_list = 176,                /* init_list  */
  nt_struct_init_list = 177,         /* struct_init_list  */
  nt_var_declaration = 178,          /* var_declaration  */
  nt_direct_address = 179,           /* direct_address  */
  nt_identifier_list = 180,          /* identifier_list  */
  nt_int_type = 181,                 /* int_type  */
  nt_time_type = 182,                /* time_type  */
  nt_array_type = 183,               /* array_type  */
  nt_range_list = 184,               /* range_list  */
  nt_range = 185,                    /* range  */
  nt_string_type = 186,              /* string_type  */
  nt_pointer_type = 187,             /* pointer_type  */
  nt_reference_type = 188,           /* reference_type  */
  nt_other_elementry_type = 189,     /* other_elementry_type  */
  nt_type_spec = 190,                /* type_spec  */
  nt_derived_type = 191,             /* derived_type  */
  nt_statements = 192,               /* statements  */
  nt_statement = 193,                /* statement  */
  nt_pragma_statement = 194,         /* pragma_statement  */
  nt_conditional_pragma = 195,       /* conditional_pragma  */
  nt_string_list = 196,              /* string_list  */
  nt_opt_attr_values = 197,          /* opt_attr_values  */
  nt_assignment_statement = 198,     /* assignment_statement  */
  nt_function_call_statement = 199,  /* function_call_statement  */
  nt_l_value = 200,                  /* l_value  */
  nt_pointer = 201,                  /* pointer  */
  nt_l_value_var = 202,              /* l_value_var  */
  nt_full_expression = 203,          /* full_expression  */
  nt_and_expression = 204,           /* and_expression  */
  nt_comp_op = 205,                  /* comp_op  */
  nt_comparison_expression = 206,    /* comparison_expression  */
  nt_arithmetic_expr = 207,          /* arithmetic_expr  */
  nt_term = 208,                     /* term  */
  nt_unary_expr = 209,               /* unary_expr  */
  nt_power_expr = 210,               /* power_expr  */
  nt_factor = 211,                   /* factor  */
  nt_if_statement = 212,             /* if_statement  */
  nt_opt_by_step = 213,              /* opt_by_step  */
  nt_for_statement = 214,            /* for_statement  */
  nt_while_statement = 215,          /* while_statement  */
  nt_repeat_statement = 216,         /* repeat_statement  */
  nt_positional_args = 217,          /* positional_args  */
  nt_arg_list = 218,                 /* arg_list  */
  nt_param_assignments = 219,        /* param_assignments  */
  nt_param_assignment = 220,         /* param_assignment  */
  nt_method_declaration_list = 221,  /* method_declaration_list  */
  nt_method_declaration = 222,       /* method_declaration  */
  nt_opt_access_modifier = 223,      /* opt_access_modifier  */
  nt_function_block = 224,           /* function_block  */
  nt_function_block_header = 225,    /* function_block_header  */
  nt_opt_fb_modifier = 226,          /* opt_fb_modifier  */
  nt_opt_extends_clause = 227,       /* opt_extends_clause  */
  nt_opt_implements_clause = 228,    /* opt_implements_clause  */
  nt_interface_list = 229,           /* interface_list  */
  nt_function = 230,                 /* function  */
  nt_case_statement = 231,           /* case_statement  */
  nt_opt_else_statement = 232,       /* opt_else_statement  */
  nt_case_element_list = 233,        /* case_element_list  */
  nt_cinner_statements = 234,        /* cinner_statements  */
  nt_case_body = 235,                /* case_body  */
  nt_case_element = 236,             /* case_element  */
  nt_case_list_selector = 237,       /* case_list_selector  */
  nt_case_selector = 238,            /* case_selector  */
  nt_case_constant_expression = 239, /* case_constant_expression  */
  nt_var_config_declaration = 240,   /* var_config_declaration  */
  nt_instance_specific_init_list = 241, /* instance_specific_init_list  */
  nt_instance_specific_init = 242,   /* instance_specific_init  */
  nt_instance_path = 243,            /* instance_path  */
  nt_using_directive_list = 244,     /* using_directive_list  */
  nt_using_directive = 245           /* using_directive  */

} EnumNonTerm;

}
