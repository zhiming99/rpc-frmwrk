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
  nt_start_point = 151,              /* start_point  */
  nt_source_file = 152,              /* source_file  */
  nt_namespace_name = 153,           /* namespace_name  */
  nt_namespace_elements = 154,       /* namespace_elements  */
  nt_namespace_element = 155,        /* namespace_element  */
  nt_namespace_declaration = 156,    /* namespace_declaration  */
  nt_global_var = 157,               /* global_var  */
  nt_pou_declaration = 158,          /* pou_declaration  */
  nt_program = 159,                  /* program  */
  nt_program_unit = 160,             /* program_unit  */
  nt_body = 161,                     /* body  */
  nt_type_definition_block = 162,    /* type_definition_block  */
  nt_type_assignments = 163,         /* type_assignments  */
  nt_enum_value_list = 164,          /* enum_value_list  */
  nt_enum_value = 165,               /* enum_value  */
  nt_opt_base_type = 166,            /* opt_base_type  */
  nt_opt_assign_enum_val = 167,      /* opt_assign_enum_val  */
  nt_enum_type_head = 168,           /* enum_type_head  */
  nt_enum_type_definition = 169,     /* enum_type_definition  */
  nt_type_assignment = 170,          /* type_assignment  */
  nt_struct_definition = 171,        /* struct_definition  */
  nt_member_list = 172,              /* member_list  */
  nt_member_declaration = 173,       /* member_declaration  */
  nt_var_declarations = 174,         /* var_declarations  */
  nt_declaration_list = 175,         /* declaration_list  */
  nt_opt_qualifier = 176,            /* opt_qualifier  */
  nt_declaration = 177,              /* declaration  */
  nt_var_list = 178,                 /* var_list  */
  nt_initial_value = 179,            /* initial_value  */
  nt_init_list = 180,                /* init_list  */
  nt_struct_init_list = 181,         /* struct_init_list  */
  nt_var_declaration = 182,          /* var_declaration  */
  nt_direct_address = 183,           /* direct_address  */
  nt_identifier_list = 184,          /* identifier_list  */
  nt_int_type = 185,                 /* int_type  */
  nt_time_type = 186,                /* time_type  */
  nt_array_type = 187,               /* array_type  */
  nt_range_list = 188,               /* range_list  */
  nt_range = 189,                    /* range  */
  nt_string_type = 190,              /* string_type  */
  nt_pointer_type = 191,             /* pointer_type  */
  nt_reference_type = 192,           /* reference_type  */
  nt_other_elementry_type = 193,     /* other_elementry_type  */
  nt_data_type_spec = 194,           /* data_type_spec  */
  nt_type_spec = 195,                /* type_spec  */
  nt_derived_type = 196,             /* derived_type  */
  nt_statements = 197,               /* statements  */
  nt_statement = 198,                /* statement  */
  nt_pragma_statement = 199,         /* pragma_statement  */
  nt_conditional_pragma = 200,       /* conditional_pragma  */
  nt_string_list = 201,              /* string_list  */
  nt_opt_attr_values = 202,          /* opt_attr_values  */
  nt_assignment_statement = 203,     /* assignment_statement  */
  nt_function_call_statement = 204,  /* function_call_statement  */
  nt_l_value = 205,                  /* l_value  */
  nt_l_value_ext = 206,              /* l_value_ext  */
  nt_pointer = 207,                  /* pointer  */
  nt_l_value_var = 208,              /* l_value_var  */
  nt_full_expression = 209,          /* full_expression  */
  nt_and_expression = 210,           /* and_expression  */
  nt_comp_op = 211,                  /* comp_op  */
  nt_comparison_expression = 212,    /* comparison_expression  */
  nt_arithmetic_expr = 213,          /* arithmetic_expr  */
  nt_term = 214,                     /* term  */
  nt_unary_expr = 215,               /* unary_expr  */
  nt_power_expr = 216,               /* power_expr  */
  nt_factor = 217,                   /* factor  */
  nt_if_statement = 218,             /* if_statement  */
  nt_opt_by_step = 219,              /* opt_by_step  */
  nt_for_statement = 220,            /* for_statement  */
  nt_while_statement = 221,          /* while_statement  */
  nt_repeat_statement = 222,         /* repeat_statement  */
  nt_positional_args = 223,          /* positional_args  */
  nt_arg_list = 224,                 /* arg_list  */
  nt_param_assignments = 225,        /* param_assignments  */
  nt_param_assignment = 226,         /* param_assignment  */
  nt_method_declaration_list = 227,  /* method_declaration_list  */
  nt_opt_global_namespace = 228,     /* opt_global_namespace  */
  nt_method_declaration = 229,       /* method_declaration  */
  nt_opt_access_modifier = 230,      /* opt_access_modifier  */
  nt_function_block = 231,           /* function_block  */
  nt_function_block_header = 232,    /* function_block_header  */
  nt_opt_fb_modifier = 233,          /* opt_fb_modifier  */
  nt_opt_extends_clause = 234,       /* opt_extends_clause  */
  nt_opt_implements_clause = 235,    /* opt_implements_clause  */
  nt_interface_list = 236,           /* interface_list  */
  nt_function = 237,                 /* function  */
  nt_case_statement = 238,           /* case_statement  */
  nt_opt_else_statement = 239,       /* opt_else_statement  */
  nt_case_element_list = 240,        /* case_element_list  */
  nt_cinner_statements = 241,        /* cinner_statements  */
  nt_case_body = 242,                /* case_body  */
  nt_case_element = 243,             /* case_element  */
  nt_case_list_selector = 244,       /* case_list_selector  */
  nt_case_selector = 245,            /* case_selector  */
  nt_case_constant_expression = 246, /* case_constant_expression  */
  nt_var_config_declaration = 247,   /* var_config_declaration  */
  nt_instance_specific_init_list = 248, /* instance_specific_init_list  */
  nt_instance_specific_init = 249,   /* instance_specific_init  */
  nt_instance_path = 250,            /* instance_path  */
  nt_using_directive_list = 251,     /* using_directive_list  */
  nt_using_directive = 252           /* using_directive  */

} EnumNonTerm;

}
