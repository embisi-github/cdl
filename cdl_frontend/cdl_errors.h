/*a Copyright
  
  This file 'cdl_errors.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_CDL_ERRORS
#else
#define __INC_CDL_ERRORS

/*a Includes
 */
#include "c_sl_error.h"

/*a Error number enumerations
 */
/*t error_number_cdl_*
 */
enum
{
     error_number_cdl_stack_not_empty = error_number_base_cdl,
     error_number_cdl_stack_underflow,
};

/*t error_id_be_*
 */
enum
{
     error_id_cdl_frontend_main,
};

/*a Error messages (default)
 */
/*v default error messages
 */
#define C_CDL_ERROR_DEFLAULT_MESSAGES \
{ error_number_cdl_stack_not_empty,        "%s0 stack not empty at completion of file %f" }, \
{ error_number_cdl_stack_underflow,        "%s0 stack underflow at line %l of file %f" }, \
{ -1, NULL }

/*a Wrapper
 */
#endif

/*a Import
 */
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Constant/signal/parameter/state/enum '%s' not declared", lex_string_from_terminal( user_id ));
cyclicity->set_parse_error( (c_cyc_object *)state, co_compile_stage_cross_reference, "Either all or no FSM states need to be assigned, and this state is, others are not" );
cyclicity->set_parse_error( (c_cyc_object *)state, co_compile_stage_cross_reference, "Either all or no FSM states need to be assigned, and this state is not" );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Lvar '%s' not declared", lex_string_from_terminal( symbol ));
cyclicity->set_parse_error((c_cyc_object *)combe->data.bus_or_inst, co_compile_stage_cross_reference, "Failed to find module '%s'", lex_string_from_terminal(combe->data.bus_or_inst->instance_of_id));
cyclicity->set_parse_error((c_cyc_object *)combe->data.bus_or_inst, co_compile_stage_cross_reference, "Failed to find bus '%s'", lex_string_from_terminal(combe->data.bus_or_inst->bus_id));
cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Duplicate port name '%s' in module '%s'", lex_string_from_terminal( cosd->signal_id ), lex_string_from_terminal( this->id ));
cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Bad redefinition with signal '%s' of port '%s'", lex_string_from_terminal( cosd->signal_id ), lex_string_from_terminal( local_signals->co[i].co_signal_declaration->signal_id ));
cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Clocked signal '%s' does not have both clock and reset (default or otherwise)", lex_string_from_terminal( cosd->signal_id ) );
cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_clock_defn, co_compile_stage_cross_reference, "Use of a non-port '%s' as a clock in module '%s'", lex_string_from_terminal( combe->data.default_clock_defn->symbol ), lex_string_from_terminal( this->id ));
cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_clock_defn, co_compile_stage_cross_reference, "Use of a non-clock '%s' as a clock in module '%s'", lex_string_from_terminal( combe->data.default_clock_defn->symbol ), lex_string_from_terminal( this->id ));
cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-port '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->id ));
cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-input '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->id ));
cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-single bit input '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->id ));
cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Failed to resolve type '%s'", lex_string_from_terminal( symbol ) );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Indices to enum is not an integer" );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Indices to enum is not an integer" );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Size of enum must be greater than 0 and less than 32" );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Size of enum must be greater than 0 and less than 32" );
cyclicity->set_parse_error( expression, co_compile_stage_evaluate_constants, "Use of unresolved value in evaluation\n"  );
cyclicity->set_parse_error( (c_cyc_object *)expression, co_compile_stage_evaluate_constants, "No 'x's allowed in binary function operands" );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Unimplemented subtype %d of binary function", expr_subtype );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Bit bundle too long, or tried to concatenate an integer in a bundle" );
cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Probable bug (or not-yet implemented feature) - expected to have resolved expression, but had not" );
cyclicity->set_parse_error( this, co_compile_stage_evaluate_constants, "Failed to resolve type '%s' during evaluation", lex_string_from_terminal( symbol ) );
cyclicity->set_parse_error( this, co_compile_stage_evaluate_constants, "Array indices must be integers" );
cyclicity->set_parse_error( this, co_compile_stage_evaluate_constants, "Array indices must be integers" );
c_cyclicity::set_parse_error
c_cyclicity::set_parse_error( t_lex_file_posn lex_file_posn, t_co_compile_stage co_compile_stage, const char *format, va_list ap)
set_parse_error( lexical_analyzer->get_current_location(), co_compile_stage, format, ap );
set_parse_error( cot, co_compile_stage_cross_reference, "Duplicate global-level symbol '%s' ", lex_string_from_terminal( cot->data.constant_declaration->symbol));
set_parse_error( coei, co_compile_stage_cross_reference, "Duplicate global-level symbol '%s' ", lex_string_from_terminal( coei->symbol));
set_parse_error( cofs, co_compile_stage_cross_reference, "Duplicate global-level symbol '%s' ", lex_string_from_terminal( cofs->name));
set_parse_error( NULL, co_compile_stage_evaluate_constants, "Bad sized integer value in overriding constant '%s'", string );
set_parse_error( NULL, co_compile_stage_evaluate_constants, "Could not match symbol to a global constant in overriding constant '%s'", string );
cyclicity->set_parse_error( co_compile_stage_tokenize, "Failed to include file '%s'", buffer );
cyclicity->set_parse_error( file->terminal_entries+file->number_terminal_entries, co_compile_stage_tokenize, "/* inside comment" );
cyclicity->set_parse_error( file->terminal_entries+file->number_terminal_entries, co_compile_stage_tokenize, "Error reading sized int - too large, too big for its size, bad character, ..." );
cyc->set_parse_error( co_compile_stage_parse, "Parse error '%s'", s );


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


