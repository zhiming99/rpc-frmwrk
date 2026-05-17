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
  nt_start_point = 138,              /* start_point  */
  nt_script_file = 139,              /* script_file  */
  nt_function = 140,                 /* function  */
  nt_type_definition_block = 141,    /* type_definition_block  */
  nt_type_assignments = 142,         /* type_assignments  */
  nt_enum_value_list = 143,          /* enum_value_list  */
  nt_enum_value = 144,               /* enum_value  */
  nt_opt_base_type = 145,            /* opt_base_type  */
  nt_opt_assign_enum_val = 146,      /* opt_assign_enum_val  */
  nt_enum_type_head = 147,           /* enum_type_head  */
  nt_enum_type_definition = 148,     /* enum_type_definition  */
  nt_type_assignment = 149,          /* type_assignment  */
  nt_struct_definition = 150,        /* struct_definition  */
  nt_member_list = 151,              /* member_list  */
  nt_member_declaration = 152,       /* member_declaration  */
  nt_var_declarations = 153,         /* var_declarations  */
  nt_declaration_list = 154,         /* declaration_list  */
  nt_opt_qualifier = 155,            /* opt_qualifier  */
  nt_declaration = 156,              /* declaration  */
  nt_var_list = 157,                 /* var_list  */
  nt_initial_value = 158,            /* initial_value  */
  nt_init_list = 159,                /* init_list  */
  nt_struct_init_list = 160,         /* struct_init_list  */
  nt_var_declaration = 161,          /* var_declaration  */
  nt_direct_address = 162,           /* direct_address  */
  nt_identifier_list = 163,          /* identifier_list  */
  nt_int_type = 164,                 /* int_type  */
  nt_time_type = 165,                /* time_type  */
  nt_array_type = 166,               /* array_type  */
  nt_range_list = 167,               /* range_list  */
  nt_range = 168,                    /* range  */
  nt_string_type = 169,              /* string_type  */
  nt_pointer_type = 170,             /* pointer_type  */
  nt_reference_type = 171,           /* reference_type  */
  nt_other_elementry_type = 172,     /* other_elementry_type  */
  nt_type_spec = 173,                /* type_spec  */
  nt_derived_type = 174,             /* derived_type  */
  nt_statements = 175,               /* statements  */
  nt_statement = 176,                /* statement  */
  nt_pragma_statement = 177,         /* pragma_statement  */
  nt_conditional_pragma = 178,       /* conditional_pragma  */
  nt_assignment_statement = 179,     /* assignment_statement  */
  nt_function_call_statement = 180,  /* function_call_statement  */
  nt_l_value = 181,                  /* l_value  */
  nt_pointer = 182,                  /* pointer  */
  nt_l_value_var = 183,              /* l_value_var  */
  nt_full_expression = 184,          /* full_expression  */
  nt_and_expression = 185,           /* and_expression  */
  nt_comparison_expression = 186,    /* comparison_expression  */
  nt_arithmetic_expr = 187,          /* arithmetic_expr  */
  nt_term = 188,                     /* term  */
  nt_unary_expr = 189,               /* unary_expr  */
  nt_power_expr = 190,               /* power_expr  */
  nt_factor = 191,                   /* factor  */
  nt_if_statement = 192,             /* if_statement  */
  nt_opt_by_step = 193,              /* opt_by_step  */
  nt_for_statement = 194,            /* for_statement  */
  nt_while_statement = 195,          /* while_statement  */
  nt_repeat_statement = 196,         /* repeat_statement  */
  nt_positional_args = 197,          /* positional_args  */
  nt_arg_list = 198,                 /* arg_list  */
  nt_param_assignments = 199,        /* param_assignments  */
  nt_param_assignment = 200,         /* param_assignment  */
  nt_case_statement = 201,           /* case_statement  */
  nt_opt_else_statement = 202,       /* opt_else_statement  */
  nt_case_element_list = 203,        /* case_element_list  */
  nt_cinner_statements = 204,        /* cinner_statements  */
  nt_case_body = 205,                /* case_body  */
  nt_case_element = 206,             /* case_element  */
  nt_case_list_selector = 207,       /* case_list_selector  */
  nt_case_selector = 208,            /* case_selector  */
  nt_case_constant_expression = 209, /* case_constant_expression  */
  nt_var_config_declaration = 210,   /* var_config_declaration  */
  nt_instance_specific_init_list = 211, /* instance_specific_init_list  */
  nt_instance_specific_init = 212,   /* instance_specific_init  */
  nt_instance_path = 213,            /* instance_path  */

} EnumNonTerm;

}