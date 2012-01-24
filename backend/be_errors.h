/*a Copyright
  
  This file 'be_errors.h' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_BE_ERRORS
#else
#define __INC_BE_ERRORS

/*a Includes
 */
#include "c_sl_error.h"

/*a Error number enumerations
 */
/*t error_number_be_*
 */
enum
{
     error_number_be_undeclared_clock_or_reset = error_number_base_be,
     error_number_be_undeclared_clock,
     error_number_be_unresolved_reference,
     error_number_be_duplicate_name,
     error_number_be_unknown_module_name,
     error_number_be_signal_in_prototype,
     error_number_be_signal_use_not_in_prototype,
     error_number_be_unresolved_clock_reference,
     error_number_be_unresolved_input_reference,
     error_number_be_unresolved_output_reference,
     error_number_be_signal_driven_by_module_not_a_net,
     error_number_be_unresolved_clock_port_reference,
     error_number_be_unresolved_clock_portref_reference,
     error_number_be_unresolved_input_port_reference,
     error_number_be_unresolved_output_port_reference,
     error_number_be_port_cannot_be_array,
     error_number_be_assignment_in_many_code_blocks,
     error_number_be_incorrect_usage_assignment,
     error_number_be_incorrect_usage_evalution,
     error_number_be_mixing_of_assert_and_cover,
     error_number_be_likely_transparent_latch,
     error_number_be_used_while_not_live,
     error_number_be_never_assigned,
     error_number_be_signal_width,
     error_number_be_expression_width,
};

/*t error_id_be_*
 */
enum
{
     error_id_be_c_model_descriptor_code_block_add_statement = error_id_base_be,
     error_id_be_c_model_descriptor_code_block_create,
     error_id_be_c_model_descriptor_module_instance_create,
     error_id_be_c_model_descriptor_module_instance_add_clock,
     error_id_be_c_model_descriptor_message_create,
     error_id_be_c_model_descriptor_module_instance_analyze,
     error_id_be_c_model_descriptor_expression,
     error_id_be_c_model_descriptor_expression_create,
     error_id_be_c_model_descriptor_module_create,
     error_id_be_c_model_descriptor_port_lvar_resolve,
     error_id_be_c_model_descriptor_signal_add_clock,
     error_id_be_c_model_descriptor_signal_add_combinatorial,
     error_id_be_c_model_descriptor_signal_add_net,
     error_id_be_c_model_descriptor_signal_add_input,
     error_id_be_c_model_descriptor_signal_add_output,
     error_id_be_c_model_descriptor_signal_create,
     error_id_be_c_model_descriptor_state_create,
     error_id_be_c_model_descriptor_statement_create,
     error_id_be_c_model_descriptor_switch_item_create,
     error_id_be_c_model_descriptor_statement_analyze,
     error_id_be_backend_message,
};

/*a Error messages (default)
 */
/*v default error messages
 */
#define C_BE_ERROR_DEFAULT_MESSAGES \
{ error_number_be_undeclared_clock_or_reset,          "Undeclared clock '%s1' or reset '%s2' for state '%s0'" }, \
{ error_number_be_undeclared_clock,                   "Undeclared clock '%s0'" }, \
{ error_number_be_unresolved_reference,               "Unresolved reference to '%s0'" }, \
{ error_number_be_unresolved_clock_reference,         "'%s0' not known as a clock in module prototype '%s1'" }, \
{ error_number_be_unresolved_input_reference,         "'%s0' not known as an input in module prototype '%s1'" }, \
{ error_number_be_unresolved_output_reference,        "'%s0' not known as an output in module prototype '%s1'" }, \
{ error_number_be_duplicate_name,                     "Duplicate name '%s0'" }, \
{ error_number_be_signal_driven_by_module_not_a_net,  "Signal '%s0' driven by the module instance '%s1' of '%s2' must be a net, but is not" }, \
{ error_number_be_unresolved_clock_port_reference,    "Clock port '%s0' on module instance '%s1' of type '%s2' could not be found" }, \
{ error_number_be_unresolved_clock_portref_reference, "Clock '%s0' for module instance '%s1' of type '%s2' could not be found" }, \
{ error_number_be_unresolved_input_port_reference,    "Input port '%s0' on module instance '%s1' of type '%s2' could not be found" }, \
{ error_number_be_unresolved_output_port_reference,   "Output port '%s0' on module instance '%s1' of type '%s2' could not be found" }, \
{ error_number_be_unknown_module_name,                "Unknown module instance name '%s0'" }, \
{ error_number_be_port_cannot_be_array,               "Input/output ports '%s0' cannot be arrays of bit vectors as Verilog does not support them" }, \
{ error_number_be_assignment_in_many_code_blocks,     "Assignment to signal '%s0' in more than one code block '%s1', '%s2'" }, \
{ error_number_be_incorrect_usage_assignment,         "Assignment to signal '%s0' in an assert/cover/RTL scope when it is declared differently" }, \
{ error_number_be_incorrect_usage_evalution,          "Use of signal '%s0' in an expression within RTL scope when it is declared as assert or cover" }, \
{ error_number_be_mixing_of_assert_and_cover,         "Mixing of assert and cover in code block '%s0' - cover() is not permitted within assert() statements, and vice versa" }, \
{ error_number_be_likely_transparent_latch,           "Expected '%s0' to be live at all points, but it is not - probably transparent latch" }, \
{ error_number_be_used_while_not_live,                "Signal '%s0' is used while it is not live in block '%s1%' - code needs to be reordered" }, \
{ error_number_be_never_assigned,                     "Signal or state '%s0' is never assigned" }, \
{ error_number_be_signal_width,                       "Signal or state element '%s0' wider than supported (64 bits in C backend)" }, \
{ error_number_be_expression_width,                   "Expression in code block '%s0' is wider than supported (64 bits in C backend)" }, \
{ -1, NULL }

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

