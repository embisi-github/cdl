/*a Copyright
  
  This file 'c_co_module_cross_reference_pre_type_resolution.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "c_type_value_pool.h"
#include "c_lexical_analyzer.h"
#include "c_cyclicity.h"
#include "cyclicity_grammar.h"
#include "sl_debug.h"

#include "c_co_case_entry.h"
#include "c_co_clock_reset_defn.h"
#include "c_co_code_label.h"
#include "c_co_constant_declaration.h"
#include "c_co_enum_definition.h"
#include "c_co_enum_identifier.h"
#include "c_co_expression.h"
#include "c_co_fsm_state.h"
#include "c_co_lvar.h"
#include "c_co_module_prototype.h"
#include "c_co_module.h"
#include "c_co_module_body_entry.h"
#include "c_co_nested_assignment.h"
#include "c_co_named_expression.h"
#include "c_co_signal_declaration.h"
#include "c_co_sized_int_pair.h"
#include "c_co_statement.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"

/*a c_co_case_entry
 */
/*f c_co_case_entry::cross_reference_within_type_context
 */
void c_co_case_entry::cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, class c_co_clock_reset_defn *default_clock, class c_co_clock_reset_defn *default_reset )
{
    if (statements)
    {
        statements->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
    }
    if (next_in_list)
    {
        ((c_co_case_entry *)next_in_list)->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
    }
}

/*a c_co_module_prototype
 */
/*f c_co_module_prototype::cross_reference_pre_type_resolution
  cross reference types of ports
*/
void c_co_module_prototype::cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types, t_co_scope *global_constants )
{
    int i, number_of_ports;
    c_co_signal_declaration *cosd;

    SL_DEBUG( sl_debug_level_verbose_info, "Module prototype '%s'", lex_string_from_terminal( symbol ));

    /*b First count the ports
     */
    number_of_ports = 0;
    for (cosd = ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list)
    {
        number_of_ports++;
    }

    /*b Allocate arrays
     */
    port_scope = create_scope( number_of_ports );
    port_scope->next_scope = global_constants;

    /*b Add ports to the array from the ports, checking for duplicates
     */
    number_of_ports = 0;
    for (cosd = ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list)
    {
        i = find_symbol_in_scope( port_scope->co, cosd->symbol );
        if (i>=0)
        {
            cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Duplicate port name '%s' in module prototype '%s'", lex_string_from_terminal( cosd->symbol ), lex_string_from_terminal( this->symbol ));
        }
        else
        {
            SL_DEBUG( sl_debug_level_verbose_info, "Added port '%s' to local ports (number %d)",  lex_string_from_terminal( cosd->symbol ), number_of_ports );
            port_scope->co[number_of_ports].co_signal_declaration = cosd;
            number_of_ports++;
        }
    }

    if (ports)
    {
        ports->cross_reference_pre_type_resolution( cyclicity, types, port_scope );
    }
}

/*a c_co_module
 */
/*f check_valid_clock
  check signal exists and is a clock
  return 0 on success, else reason for failure (1->does not exist, 2->exists but is not a clock)
*/
static int check_valid_clock( t_co_scope *local_signals, t_symbol *symbol )
{
    int i;
    i = find_symbol_in_scope( local_signals->co, symbol );
    if (i<0)
    {
        return 1;
    }
    else if (local_signals->co[i].co_signal_declaration->signal_declaration_type != signal_declaration_type_clock)
    {
        return 2;
    }
    return 0;
}

/*f check_valid_reset
  check signal exists and is an input
  return 0 on success, else reason for failure (1->does not exist, 2->exists but is not an input, 3 if it is not a single bit)
*/
static int check_valid_reset( c_cyclicity *cyclicity, t_co_scope *local_signals, t_symbol *symbol )
{
    int i;
    i = find_symbol_in_scope( local_signals->co, symbol );
    if (i<0)
    {
        return 1;
    }
    if (local_signals->co[i].co_signal_declaration->signal_declaration_type != signal_declaration_type_input)
    {
        return 2;
    }
    return 0;
}

/*f c_co_module::index_local_signals
  first, count ports and locals, then generate array
  second, add signal_declarations from the ports to the array checking for duplicates
  third, add signal_declarations from the locals to the array checking for duplicates
  a duplicate where the first is an output port and the second is a clock is legal if they match - merge them
  while doing this, keep a watch out for default clocks and resets, and add them in - they must exist, in the input port list
  no default clock or reset for a clocked signal which has no clock or reset is an error
*/
void c_co_module::index_local_signals( class c_cyclicity *cyclicity, t_co_scope *global_constants )
{
    int i, j, max_signals, number_of_signals, number_of_ports;

    c_co_signal_declaration *cosd;
    c_co_module_body_entry *combe;

    SL_DEBUG( sl_debug_level_verbose_info, "Module '%s'", lex_string_from_terminal( symbol ));

    /*b First count the signals
     */
    number_of_signals = 0;
    for (cosd = ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list)
    {
        number_of_signals++;
    }
    number_of_ports = number_of_signals;
    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        if (combe->type == module_body_entry_type_signal_declaration)
        {
            number_of_signals++;
        }
    }

    /*b Allocate arrays
     */
    port_scope = create_scope( number_of_ports );
    max_signals = number_of_signals;
    local_signals = create_scope( max_signals );
    local_signals->next_scope = global_constants;

    /*b Add ports to the array from the signals, checking for duplicates
     */
    number_of_signals = 0;
    number_of_ports = 0;
    for (cosd = ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list)
    {
        i = find_symbol_in_scope( local_signals->co, cosd->symbol );
        if (i>=0)
        {
            cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Duplicate port name '%s' in module '%s'", lex_string_from_terminal( cosd->symbol ), lex_string_from_terminal( this->symbol ));
        }
        else
        {
            SL_DEBUG( sl_debug_level_verbose_info, "Added port '%s' to local signals (number %d)",  lex_string_from_terminal( cosd->symbol ), number_of_signals );
            local_signals->co[number_of_signals].co_signal_declaration = cosd;
            number_of_signals++;
            port_scope->co[number_of_ports].co_signal_declaration = cosd;
            number_of_ports++;
        }
    }

    /*b Add ports to the array from the locals, checking for duplicates (clock specs for output ports)
     */
    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
            /*b Signal declaration
             */
        case module_body_entry_type_signal_declaration:
            cosd = combe->data.signal_declaration;
            if ( cosd )
            {
                i = find_symbol_in_scope( local_signals->co, cosd->symbol ); // Try to find the signal id in the ports as well
                j = number_of_signals;
                if (i>=0)
                {
                    // check the first is an output port and this is a clocked or net, in which case just continue
                    if (local_signals->co[i].co_signal_declaration->signal_declaration_type != signal_declaration_type_comb_output)
                    {
                        j = -1;
                        cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Bad redefinition with signal '%s' of port '%s'", lex_string_from_terminal( cosd->symbol ), lex_string_from_terminal( local_signals->co[i].co_signal_declaration->symbol ));
                    }
                    else
                    {
                        if (cosd->signal_declaration_type == signal_declaration_type_net_local)
                        {
                            // Found an output sibling (i) for the local signal declaration (cosd, j)
                            // Replace the local signal declaration cosd with the net output
                            combe->data.signal_declaration = local_signals->co[i].co_signal_declaration;
                            // Chain the local declaration from the net output for security reasons, and the other way round too
                            local_signals->co[i].co_signal_declaration->local_clocked_signal = cosd;
                            cosd->output_port = local_signals->co[i].co_signal_declaration;
                            // Now change hands
                            cosd = local_signals->co[i].co_signal_declaration;
                            j = i;
                            // Set the port to be clocked
                            cosd->signal_declaration_type = signal_declaration_type_net_output;
                            // For freeing purposes, link in the local clocked signal to the port
                            cosd->co_link( co_compile_stage_cross_reference, (c_cyc_object *)cosd->local_clocked_signal, "local clocked signal declaration" );
                        }                           
                        else if (cosd->signal_declaration_type == signal_declaration_type_clocked_local)
                        {
                            // Found a clocked output sibling (i) for the local signal declaration (cosd, j)
                            // Replace the local signal declaration cosd with the clocked output
                            combe->data.signal_declaration = local_signals->co[i].co_signal_declaration;
                            // Chain the local declaration from the clocked output for security reasons, and the other way round too
                            local_signals->co[i].co_signal_declaration->local_clocked_signal = cosd;
                            cosd->output_port = local_signals->co[i].co_signal_declaration;
                            // Now change hands
                            cosd = local_signals->co[i].co_signal_declaration;
                            j = i;
                            // Set the port to be clocked
                            cosd->signal_declaration_type = signal_declaration_type_clocked_output;
                            // Now copy the data down from the local clocked signal to the port - clock_spec, reset_spec, reset_value (they remain linked for freeing purposes to the local clocked signal declaration)
                            cosd->data.clocked.clock_spec = cosd->local_clocked_signal->data.clocked.clock_spec;
                            cosd->data.clocked.reset_spec = cosd->local_clocked_signal->data.clocked.reset_spec;
                            cosd->data.clocked.reset_value = cosd->local_clocked_signal->data.clocked.reset_value;
                            cosd->data.clocked.usage_type = cosd->local_clocked_signal->data.clocked.usage_type;
                            // For freeing purposes, link in the local clocked signal to the port
                            cosd->co_link( co_compile_stage_cross_reference, (c_cyc_object *)cosd->local_clocked_signal, "local clocked signal declaration" );
                        }
                        else
                        {
                            j = -1;
                            cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Bad redefinition with signal '%s' of port '%s'", lex_string_from_terminal( cosd->symbol ), lex_string_from_terminal( local_signals->co[i].co_signal_declaration->symbol ));
                        }
                    }
                }
                else
                {
                    local_signals->co[j].co_signal_declaration = cosd;
                }
                if (j>=0)
                {
                    SL_DEBUG( sl_debug_level_verbose_info, "Added local signal '%s' to local signals (or possibly marked output as clocked) (number %d) (reset %p clock %p)",  lex_string_from_terminal( cosd->symbol ), j, local_signals->co[j].co_signal_declaration->data.clocked.reset_spec, local_signals->co[j].co_signal_declaration->data.clocked.clock_spec );
                    if (j==number_of_signals)
                        number_of_signals++;
                }
            }
            break;
            /*b Else
             */
        default:
            break;
        }
    }

    /*b Close out list
     */
    local_signals->co[number_of_signals].cyc_object = NULL;
}

/*f c_co_module::cross_reference_pre_type_resolution
  cross reference types of ports
  cross reference module body contents
  cross reference types of signal_declarations in the body
  cross reference expressions with local and global scope
  cross reference lvars with local and global scope
  cross reference module/bus instantiation port_maps with local and global scope
*/
void c_co_module::cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types )
{
    c_co_module_body_entry *combe;
    c_co_signal_declaration *cosd;
    c_co_clock_reset_defn *default_clock, *default_reset;
    int i;

    SL_DEBUG( sl_debug_level_verbose_info, "Module '%s'", lex_string_from_terminal( symbol ));
    if (ports)
    {
        ports->cross_reference_pre_type_resolution( cyclicity, types, local_signals );
    }
    default_clock = NULL;
    default_reset = NULL;
    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
        case module_body_entry_type_signal_declaration:
            cosd = combe->data.signal_declaration;
            if (cosd)
            {
                cosd->cross_reference_pre_type_resolution( cyclicity, types, local_signals );
                // patch a clocked signal which does not have a reset or clock - error if neither
                if ( (cosd->signal_declaration_type == signal_declaration_type_clocked_local) ||
                     (cosd->signal_declaration_type == signal_declaration_type_clocked_output) )
                {
                    if (!cosd->data.clocked.clock_spec)
                    {
                        cosd->data.clocked.clock_spec = default_clock;
                    }
                    if (!cosd->data.clocked.reset_spec)
                    {
                        cosd->data.clocked.reset_spec = default_reset;
                    }
                    if ( (!cosd->data.clocked.clock_spec) ||
                         (!cosd->data.clocked.reset_spec) )
                    {
                        cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Clocked signal '%s' does not have both clock and reset (default or otherwise)", lex_string_from_terminal( cosd->symbol ) );
                    }
                }
                if ( (cosd->signal_declaration_type == signal_declaration_type_clock) && (cosd->data.clock.gate) ) // If a gated clock, fill in default_clock if necessary
                {
                    if (!cosd->data.clock.clock_to_gate)
                    {
                        cosd->data.clock.clock_to_gate = default_clock;
                    }
                    if (!cosd->data.clock.clock_to_gate)
                    {
                        cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Gated clock '%s' does not have both clock and enable (default or otherwise)", lex_string_from_terminal( cosd->symbol ) );
                    }
                }
            }
            break;
        case module_body_entry_type_default_clock_defn:
            i = check_valid_clock( local_signals, combe->data.default_clock_defn->symbol );
            if (i==1)
            {
                cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_clock_defn, co_compile_stage_cross_reference, "Use of a non-port '%s' as a clock in module '%s'", lex_string_from_terminal( combe->data.default_clock_defn->symbol ), lex_string_from_terminal( this->symbol ));
            }
            else if (i==2)
            {
                cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_clock_defn, co_compile_stage_cross_reference, "Use of a non-clock '%s' as a clock in module '%s'", lex_string_from_terminal( combe->data.default_clock_defn->symbol ), lex_string_from_terminal( this->symbol ));
            }
            else
            {
                default_clock = combe->data.default_clock_defn;
            }
            break;
            /*b Default reset
             */
        case module_body_entry_type_default_reset_defn:
            i = check_valid_reset( cyclicity, local_signals, combe->data.default_reset_defn->symbol );
            if (i==1)
            {
                cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-port '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->symbol ));
            }
            else if (i==2)
            {
                cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-input '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->symbol ));
            }
            else
            {
                default_reset = combe->data.default_reset_defn;
            }
            break;
        case module_body_entry_type_code_label: // cross reference statements in the module body
            if (combe->data.code_label &&
                combe->data.code_label->statements)
            {
                combe->data.code_label->statements->cross_reference_pre_type_resolution( cyclicity, types, local_signals, default_clock, default_reset );
            }
            break;
        case module_body_entry_type_schematic_symbol:
            // Done...
            break;
        }
    }
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::cross_reference_pre_type_resolution
 */
void c_co_signal_declaration::cross_reference_pre_type_resolution( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    /*b Pre-type resolution just cross reference symbols within the type specifier so it may be resolved correctly
     */
    if (type_specifier)
    {
        type_specifier->cross_reference( cyclicity, types, variables );
    }
    if (local_clocked_signal && local_clocked_signal->type_specifier)
    {
        local_clocked_signal->type_specifier->cross_reference( cyclicity, types, variables );
    }

    /*b Resolve the next in the chain if required - required for the list of ports, not required for individual signal declarations
     */
    if (next_in_list)
    {
        ((c_co_signal_declaration *)next_in_list)->cross_reference_pre_type_resolution( cyclicity, types, variables );
    }
}

/*a c_co_statement
 */
/*f cross_reference_pre_type_resolution
  Cross reference a statement and any following statements in its list
  This is basically to utilize 'default_clock' and 'default_reset'
*/
void c_co_statement::cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, c_co_clock_reset_defn *default_clock, c_co_clock_reset_defn *default_reset )
{
    switch (statement_type)
    {
    case statement_type_assignment:
        break;
    case statement_type_if_statement:
        if (type_data.if_stmt.if_true)
        {
            type_data.if_stmt.if_true->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
        }
        if (type_data.if_stmt.if_false)
        {
            type_data.if_stmt.if_false->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
        }
        if (type_data.if_stmt.elsif)
        {
            type_data.if_stmt.elsif->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
        }
        break;
    case statement_type_for_statement:
        type_data.for_stmt.scope = create_scope( 1 );
        type_data.for_stmt.scope->next_scope = variables;
        type_data.for_stmt.scope->co[0].co_statement = this;
        type_data.for_stmt.scope->co[1].cyc_object = NULL;
        if (type_data.for_stmt.statement)
        {
            type_data.for_stmt.statement->cross_reference_pre_type_resolution( cyclicity, types, type_data.for_stmt.scope, default_clock, default_reset );
        }
        break;
    case statement_type_full_switch_statement:
    case statement_type_partial_switch_statement:
    case statement_type_priority_statement:
        if (type_data.switch_stmt.expression)
        {
            if (type_data.switch_stmt.cases)
            {
                type_data.switch_stmt.cases->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
            }
        }
        break;
    case statement_type_block:
        if (type_data.block)
        {
            type_data.block->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
        }
        break;
    case statement_type_print:
    case statement_type_assert:
    case statement_type_cover:
        if (type_data.print_assert_stmt.statement)
        {
            if (type_data.print_assert_stmt.assertion && statement_type==statement_type_cover)
            {
                cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Cover statement is not permitted with expression" );
            }
            type_data.print_assert_stmt.statement->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
        }
        else if (type_data.print_assert_stmt.text) // If there is no statement it should be a message, which requires a clock edge
        {
            if (!type_data.print_assert_stmt.clock_spec)
            {
                type_data.print_assert_stmt.clock_spec = default_clock;
            }
            if (!type_data.print_assert_stmt.reset_spec)
            {
                type_data.print_assert_stmt.reset_spec = default_reset;
            }
            if ( (!type_data.print_assert_stmt.clock_spec) ||
                 (!type_data.print_assert_stmt.reset_spec) )
            {
                cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Print/assert statement does not have both clock and reset (default or otherwise)" );
            }
        }
        break;
    case statement_type_log:
        if (!type_data.log_stmt.clock_spec)
        {
            type_data.log_stmt.clock_spec = default_clock;
        }
        if (!type_data.log_stmt.reset_spec)
        {
            type_data.log_stmt.reset_spec = default_reset;
        }
        if ( (!type_data.log_stmt.clock_spec) ||
             (!type_data.log_stmt.reset_spec) )
        {
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Log statement does not have both clock and reset (default or otherwise)" );
        }
        break;
    case statement_type_instantiation: // no cross referencing required until types are resolved
        break;
    }
    if (next_in_list)
    {
        ((c_co_statement *)next_in_list)->cross_reference_pre_type_resolution( cyclicity, types, variables, default_clock, default_reset );
    }
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


