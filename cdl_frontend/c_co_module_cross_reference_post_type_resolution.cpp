/*a Copyright
  
  This file 'c_co_module_cross_reference_post_type_resolution.cpp' copyright Gavin J Stark 2003, 2004
  
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

#include "c_co_instantiation.h"
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
#include "c_co_named_expression.h"
#include "c_co_nested_assignment.h"
#include "c_co_port_map.h"
#include "c_co_schematic_symbol.h"
#include "c_co_schematic_symbol_body_entry.h"
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
void c_co_case_entry::cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type )
{

    this->type = type;

    if (expression)
    {
        c_co_expression *expr;
        for (expr = expression; expr; expr=(c_co_expression *)(expr->next_in_list))
        {
            expr->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type );
        }
    }
    if (statements)
    {
        statements->cross_reference_post_type_resolution( cyclicity, types, variables );
    }
    if (next_in_list)
    {
        ((c_co_case_entry *)next_in_list)->cross_reference_within_type_context( cyclicity, types, variables, type );
    }
}

/*a c_co_clock_reset_defn
 */
/*f c_co_clock_reset_defn::cross_reference_post_type_resolution
 */
void c_co_clock_reset_defn::cross_reference_post_type_resolution( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    t_co_union cou;
    SL_DEBUG( sl_debug_level_verbose_info, "Cross reference clock/reset signal %s", lex_string_from_terminal(symbol));
    cou = cyclicity->cross_reference_symbol_in_scopes( symbol, variables );
    if (cou.cyc_object && (cou.cyc_object->co_type==co_type_signal_declaration))
    {
        signal_declaration = cou.co_signal_declaration;
    }
    else
    {
        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Failed to find clock/reset signal '%s'", lex_string_from_terminal( symbol ));
    }
}

/*a c_co_instantiation
 */
/*f c_co_instantiation::cross_reference_post_type_resolution
 */
int c_co_instantiation::cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    int i;
    c_co_module *com;
    c_co_module_prototype *comp;
    t_co_scope *port_scope;

    refers_to.cyc_object = NULL;

    if (!instance_of_id) // If for some reason the module being instanced is NULL (in error somehow), protect ourselves by just returning
        return 0;

    for (i=0; cyclicity->modules->co[i].co_module; i++)
    {
        if (cyclicity->modules->co[i].cyc_object->co_type==co_type_module)
        {
            com = cyclicity->modules->co[i].co_module;
            if (com->symbol->lex_symbol == instance_of_id->lex_symbol)
            {
                refers_to = cyclicity->modules->co[i];
                port_scope = refers_to.co_module->port_scope;
                //printf("Referring module instance %s / %s\n", lex_string_from_terminal( instance_of_id ), lex_string_from_terminal( instance_id ) );
            }
        }
        if ((cyclicity->modules->co[i].cyc_object->co_type==co_type_module_prototype) && (!refers_to.cyc_object))
        {
            comp = cyclicity->modules->co[i].co_module_prototype;
            if (comp->symbol->lex_symbol == instance_of_id->lex_symbol)
            {
                refers_to = cyclicity->modules->co[i];
                port_scope = refers_to.co_module_prototype->port_scope;
                //printf("Referring module instance %s / %s\n", lex_string_from_terminal( instance_of_id ), lex_string_from_terminal( instance_id ) );
            }
        }
    }
    if (!refers_to.cyc_object)
    {
        cyclicity->set_parse_error((c_cyc_object *)this, co_compile_stage_cross_reference, "Failed to find module '%s'", lex_string_from_terminal(instance_of_id));
        return 0;
    }
    if (port_map)
    {
        port_map->cross_reference_post_type_resolution( cyclicity, types, variables, port_scope );
    }
    if (index)
    {
        index->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
    }
    return 0;
}

/*a c_co_lvar
 */
/*f c_co_lvar::cross_reference_post_type_resolution
  Fill out the type of the lvar, and the port that it corresponds to
  If it is a structure element then find the type of the structure (and its port/local), and dig in to that.
  Similarly for an array.
  If it is a terminal, then find it in the scope
*/
void c_co_lvar::cross_reference_post_type_resolution_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type_context )
{
    t_co_union symbol_refers_to;

    type = type_value_error;
    signal_declaration = NULL;
    non_signal_declaration.cyc_object = NULL;

    if (lvar) // Part of a structure or an element of an array, etc; not a terminal. Evaluate the parent which contains us, then do the indexing/subelement
    {
        lvar->cross_reference_post_type_resolution_within_type_context( cyclicity, types, variables, type_context );
        signal_declaration = lvar->signal_declaration;
        non_signal_declaration = lvar->non_signal_declaration;
        if (non_signal_declaration.cyc_object)
        {
            int file_number, first_line, last_line, char_offset;
            cyclicity->translate_lex_file_posn( &non_signal_declaration.cyc_object->start_posn, &file_number, &first_line, &last_line, &char_offset );
            fprintf(stderr,"c_co_lvar::cross_reference_post_type_resolution_within_type_context: Bit selection of a non-signal (%p) is not supported %s:%d:(char %d)\n", non_signal_declaration.cyc_object, cyclicity->get_filename(file_number), 1+first_line, char_offset );
            exit(4);
        }
        if (symbol)
        {
            element = cyclicity->type_value_pool->find_structure_element( lvar->type, symbol );
            if (element>=0)
            {
                type = cyclicity->type_value_pool->get_structure_element_type( lvar->type, element );
            }
        }
        if (first)
        {
            // We need to put a guess of the type in, so type referencing with context for nested assignments will work.
            // The size of the array, if an array, is not relevant; if no length is given, though, the type is the array element type
            // The final type determination is done in 'check_types'
            if (length)
            {
                type = lvar->type;
            }
            else if (cyclicity->type_value_pool->derefs_to_bit_vector(lvar->type))
            {
                type = type_value_bit_array_size_1;
            }
            else
            {
                type = cyclicity->type_value_pool->array_type(lvar->type);
            }
            first->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
        }
        if (length)
        {
            length->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
        }
    }
    else
    {
        if (symbol)
        {
            element = -1;
            symbol_refers_to = cyclicity->cross_reference_symbol_in_scopes( symbol, variables );
            if (!symbol_refers_to.cyc_object)
            {
                if (type_context!=type_value_undefined)
                {
                    SL_DEBUG( sl_debug_level_verbose_info, "Looking for symbol '%s' in type context %d as structure element - should also do it as enum or FSM?", lex_string_from_terminal( symbol ), type_context );
                    element = cyclicity->type_value_pool->find_structure_element( type_context, symbol );
                    if (element>=0)
                    {
                        type = cyclicity->type_value_pool->get_structure_element_type( type_context, element );
                    }
                }
                if (element<0)
                {
                    cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Lvar '%s' not declared", lex_string_from_terminal( symbol ));
                }
            }
            else
            {
                SL_DEBUG( sl_debug_level_verbose_info, "Xrefing symbol '%s' in scope got %p (co type %d)", lex_string_from_terminal( symbol ), symbol_refers_to.cyc_object, symbol_refers_to.cyc_object->co_type );
                if (symbol_refers_to.cyc_object->co_type == co_type_signal_declaration)
                {
                    signal_declaration = symbol_refers_to.co_signal_declaration;
                    if (signal_declaration->type_specifier)
                    {
                        type = signal_declaration->type_specifier->type_value;
                    }
                }
                else
                {
                    switch (symbol_refers_to.cyc_object->co_type)
                    {
                    case co_type_constant_declaration:
                        non_signal_declaration = symbol_refers_to;
                        if (non_signal_declaration.co_constant_declaration)
                        {
                            type = cyclicity->type_value_pool->get_type( non_signal_declaration.co_constant_declaration->type_value );
                            SL_DEBUG( sl_debug_level_verbose_info, "Constant declaration '%s' type %d", lex_string_from_terminal( symbol ), type );
                        }
                        break;
                    case co_type_enum_identifier:
                        non_signal_declaration = symbol_refers_to;
                        if ( (non_signal_declaration.co_enum_identifier) &&
                             (non_signal_declaration.co_enum_identifier->type_def) )
                        {
                            type = non_signal_declaration.co_enum_identifier->type_def->type_value;
                        }
                        break;
                    case co_type_fsm_state:
                        non_signal_declaration = symbol_refers_to;
                        if ( (non_signal_declaration.co_fsm_state) &&
                             (non_signal_declaration.co_fsm_state->type_def) )
                        {
                            type = non_signal_declaration.co_fsm_state->type_def->type_value;
                        }
                        break;
                    case co_type_statement:
                        non_signal_declaration = symbol_refers_to;
                        if (symbol_refers_to.co_statement->statement_type==statement_type_for_statement)
                        {
                            type = type_value_integer;
                        }
                        else
                        {
                            printf("c_co_lvar::cross_reference_post_type_resolution:Unhandled lvar type that derives from a statement\n");
                        }
                        break;
                    default:
                        printf("c_co_lvar::cross_reference_post_type_resolution:Unhandled lvar type\n");
                        break;
                    }
                }
            }
        }
    }
}

/*a c_co_module
 */
/*f c_co_module::cross_reference_post_type_resolution
  cross reference types of ports
  cross reference module body contents
  cross reference types of signal_declarations in the body
  cross reference expressions with local and global scope
  cross reference lvars with local and global scope
  cross reference module/bus instantiation port_maps with local and global scope
*/
void c_co_module::cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types )
{
    c_co_module_body_entry *combe;

    SL_DEBUG( sl_debug_level_verbose_info, "Module '%s'", lex_string_from_terminal( symbol ));

    if (ports)
    {
        ports->cross_reference_post_type_resolution( cyclicity, types, local_signals );
    }
    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
        case module_body_entry_type_signal_declaration:
            if (combe->data.signal_declaration)
            {
                combe->data.signal_declaration->cross_reference_post_type_resolution( cyclicity, types, local_signals );
            }
            break;
        case module_body_entry_type_code_label: // cross reference statements in the module body
            if (combe->data.code_label &&
                combe->data.code_label->statements)
            {
                combe->data.code_label->statements->cross_reference_post_type_resolution( cyclicity, types, local_signals );
            }
            break;
        case module_body_entry_type_default_clock_defn: // handled in building the local signal array
        case module_body_entry_type_default_reset_defn: // handled in building the local signal array
        case module_body_entry_type_schematic_symbol:
            // Done...
            break;
        }
    }
}

/*a c_co_named_expression
 */
/*f c_co_named_expression::cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
 */
void c_co_named_expression::cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (expression)
    {
        expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
    }
}

/*a c_co_nested_assignment
 */
/*f c_co_nested_assignment::cross_reference_within_type_context
  The nested assignment should consist of a set of elements (of the specified type) and expressions
  We need to find the element is part of a structure of the specified type (if the element name is given), and cross reference the expression within the context of that type
  Note that the whole assignment may be nested again.
*/
void c_co_nested_assignment::cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value parent_type , t_type_value type )
{
    t_type_value element_type;

    type_context = type;
    element = -1;

    if (symbol)
    {
        element = cyclicity->type_value_pool->find_structure_element( type, symbol );
    }
    if (nested_assignment)
    {
        if (symbol) // If this is a structure element, get its type and cross reference the nested assignment within that type context
        {
            element_type = type_value_undefined;
            if (element>=0)
            {
                element_type = cyclicity->type_value_pool->get_structure_element_type( type_context, element );
            }
            nested_assignment->cross_reference_within_type_context( cyclicity, types, variables, type, element_type );
        }
        else if (cyclicity->type_value_pool->is_array(type_context)) // Not a structure element, must be an array element, so use the non-array version of the given type
        {
            element_type = cyclicity->type_value_pool->array_type(type_context);
            nested_assignment->cross_reference_within_type_context( cyclicity, types, variables, type_context, element_type );
        }
        else 
        {
            printf("c_co_nested_assignment::cross_reference_within_type_context:PROBABLY SHOUD NOT BE HERE\n");
        }
    }
    if (expression) // If there is an expression then it is just that, and within the type given in the call
    {
        expression->cross_reference_within_type_context( cyclicity, types, variables, parent_type, type_context );
    }
    if (next_in_list)
        ((c_co_nested_assignment *)next_in_list)->cross_reference_within_type_context( cyclicity, types, variables, parent_type, type_context );
}

/*a c_co_port_map
 */
/*f c_co_port_map::cross_reference_post_type_resolution
 */
int c_co_port_map::cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_co_scope *port_scope )
{
    // port_lvar is the port; lvar is a signal driven by an output port_lvar, expression is a value for an input port_lvar.
    if (port_lvar)
    {
        port_lvar->cross_reference_post_type_resolution_within_type_context( cyclicity, types, port_scope, type_value_undefined );
    }
    if (lvar) // Output, drive this lvar with port_lvar - it has better be an output port_lvar, hadn't it
    {
        lvar->cross_reference_post_type_resolution_within_type_context( cyclicity, types, variables, port_lvar?port_lvar->type:type_value_undefined );
    }
    if (expression) // Input or parameter; drive the port_lvar with this expression (should be a nested assignment?)
    {
        expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, port_lvar?port_lvar->type:type_value_undefined );
    }
    if (next_in_list)
    {
        return ((c_co_port_map *)next_in_list)->cross_reference_post_type_resolution( cyclicity, types, variables, port_scope );
    }
    return 1;
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::cross_reference_post_type_resolution
 */
void c_co_signal_declaration::cross_reference_post_type_resolution( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    /*b If this is a clocked signal, cross reference symbols within the reset value or values and clock/reset specs
     */
    if ( (signal_declaration_type == signal_declaration_type_clocked_output) ||
         (signal_declaration_type == signal_declaration_type_clocked_local) )
    {
        SL_DEBUG( sl_debug_level_verbose_info, "Cross reference clocked signal %s (reset %p clock %p)", lex_string_from_terminal(symbol), data.clocked.reset_spec, data.clocked.clock_spec);
        if (data.clocked.reset_spec)
            data.clocked.reset_spec->cross_reference_post_type_resolution( cyclicity, types, variables );

        if (data.clocked.clock_spec)
            data.clocked.clock_spec->cross_reference_post_type_resolution( cyclicity, types, variables );

        /*b If there is a list of reset values (nested assignment), then cross reference that, noting the type of this signal
         */
        if (data.clocked.reset_value)
        {
            if (type_specifier)
            {
                SL_DEBUG( sl_debug_level_verbose_info, "Cross reference clocked local reset value nested assignment for this signal %p type_value %d", this, type_specifier->type_value );
                data.clocked.reset_value->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_specifier->type_value );
            }
        }
    }
    /*b If this is a gated clock signal, cross reference symbols within the clock/reset specs
     */
    if ((signal_declaration_type == signal_declaration_type_clock) && (data.clock.gate))
    {
        SL_DEBUG( sl_debug_level_verbose_info, "Cross reference gated clock %s (enable %p clock %p)", lex_string_from_terminal(symbol), data.clock.gate, data.clock.clock_to_gate );
        data.clock.gate->cross_reference_post_type_resolution( cyclicity, types, variables );

        if (data.clock.clock_to_gate)
        {
            data.clock.clock_to_gate->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
    }
}

/*a c_co_statement
 */
/*f cross_reference_post_type_resolution
  Cross reference a statement and any following statements in its list
  For an assignment, this means:
  Cross reference the lvar, and get its type
  Cross reference the expression using that type
  Check the assignment is approriately clocked
*/
void c_co_statement::cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    switch (statement_type)
    {
    case statement_type_assignment:
    {
        c_co_lvar *lvar;
        lvar = type_data.assign_stmt.lvar;
        if (lvar)
        {
            lvar->cross_reference_post_type_resolution_within_type_context( cyclicity, types, variables, type_value_undefined );
            if (lvar->signal_declaration)
            {
                switch (lvar->signal_declaration->signal_declaration_type)
                {
                case signal_declaration_type_clocked_local:
                case signal_declaration_type_clocked_output:
                    if (!type_data.assign_stmt.clocked)
                        cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Unclocked assignment to a clocked variable '%s'", lex_string_from_terminal( lvar->symbol ) );
                    break;
                case signal_declaration_type_comb_output:
                case signal_declaration_type_comb_local:
                    if (type_data.assign_stmt.clocked)
                        cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Clocked assignment to an combinatorial variable '%s'", lex_string_from_terminal( lvar->symbol ) );
                    break;
                case signal_declaration_type_net_local:
                case signal_declaration_type_net_output:
                case signal_declaration_type_input:
                case signal_declaration_type_clock:
                case signal_declaration_type_parameter:
                    cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Assignment to a non-port '%s' (may need to unref if lvar is a struct or array)", lex_string_from_terminal( lvar->symbol ) );
                    break;
                }
                if (type_data.assign_stmt.nested_assignment)
                {
                    SL_DEBUG( sl_debug_level_verbose_info, "Cross reference assignment statement nested assignment for type_value %d", lvar->type );
                    type_data.assign_stmt.nested_assignment->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, lvar->type );
                }
            }
        }
        break;
    }
    case statement_type_if_statement:
        if (type_data.if_stmt.expression)
        {
            type_data.if_stmt.expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_bool );
        }
        if (type_data.if_stmt.if_true)
        {
            type_data.if_stmt.if_true->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        if (type_data.if_stmt.if_false)
        {
            type_data.if_stmt.if_false->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        if (type_data.if_stmt.elsif)
        {
            type_data.if_stmt.elsif->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        break;
    case statement_type_for_statement:
        // #iterations and first_value are not within the scope of the iterator; last_value and next_value are, and so is the statement
        if (type_data.for_stmt.iterations)
        {
            type_data.for_stmt.iterations->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
        }
        if (type_data.for_stmt.first_value)
        {
            type_data.for_stmt.first_value->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
        }
        if (type_data.for_stmt.last_value)
        {
            type_data.for_stmt.last_value->cross_reference_within_type_context( cyclicity, types, type_data.for_stmt.scope, type_value_undefined, type_value_bool );
        }
        if (type_data.for_stmt.next_value)
        {
            type_data.for_stmt.next_value->cross_reference_within_type_context( cyclicity, types, type_data.for_stmt.scope, type_value_undefined, type_value_integer );
        }
        if (type_data.for_stmt.statement)
        {
            type_data.for_stmt.statement->cross_reference_post_type_resolution( cyclicity, types, type_data.for_stmt.scope );
        }
        break;
    case statement_type_full_switch_statement:
    case statement_type_partial_switch_statement:
    case statement_type_priority_statement:
        if (type_data.switch_stmt.expression)
        {
            type_data.switch_stmt.expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_undefined );
            if (type_data.switch_stmt.cases)
            {
                type_data.switch_stmt.cases->cross_reference_within_type_context( cyclicity, types, variables, type_data.switch_stmt.expression->type );
            }
        }
        break;
    case statement_type_block:
        if (type_data.block)
        {
            type_data.block->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        break;
    case statement_type_print:
    case statement_type_assert:
    case statement_type_cover:
        if (type_data.print_assert_stmt.assertion)
        {
            if (type_data.print_assert_stmt.value_list)
            {
                type_data.print_assert_stmt.assertion->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_undefined );
                c_co_expression *expr;
                for (expr = type_data.print_assert_stmt.value_list; expr; expr=(c_co_expression *)(expr->next_in_list))
                {
                    expr->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_undefined );
                }
            }
            else
            {
                type_data.print_assert_stmt.assertion->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_bool );
            }
        }
        if (type_data.print_assert_stmt.text_args)
        {
            c_co_expression *expr;
            for (expr = type_data.print_assert_stmt.text_args; expr; expr=(c_co_expression *)(expr->next_in_list))
            {
                expr->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
            }
        }
        if (type_data.print_assert_stmt.statement)
        {
            type_data.print_assert_stmt.statement->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        break;
    case statement_type_log:
    {
        class c_co_named_expression *arg;
        for (arg = type_data.log_stmt.values; arg; arg=(c_co_named_expression *)(arg->next_in_list))
        {
            arg->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        break;
    }
    case statement_type_instantiation:
        if (type_data.instantiation)
        {
            type_data.instantiation->cross_reference_post_type_resolution( cyclicity, types, variables );
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_statement *)next_in_list)->cross_reference_post_type_resolution( cyclicity, types, variables );
    }
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


