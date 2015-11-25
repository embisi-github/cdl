/*a Copyright
  
  This file 'c_co_module_check_types.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_co_module.h"
#include "c_co_module_body_entry.h"
#include "c_co_module_prototype.h"
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
/*f c_co_case_entry::check_types
 */
void c_co_case_entry::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{

    if (expression)
    {
        expression->type_check_within_type_context( cyclicity, types, variables, type );
    }
    if (statements)
    {
        statements->check_types( cyclicity, types, variables );
    }
    if (next_in_list)
    {
        ((c_co_case_entry *)next_in_list)->check_types( cyclicity, types, variables );
    }
}

/*a c_co_instantiation
 */
/*f c_co_instantiation::check_types
 */
void c_co_instantiation::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    t_co_scope *port_scope;

    port_scope = NULL;
    if (refers_to.cyc_object->co_type==co_type_module)
        port_scope = refers_to.co_module->port_scope;
    if (refers_to.cyc_object->co_type==co_type_module_prototype)
        port_scope = refers_to.co_module_prototype->port_scope;

    if (port_map)
    {
        port_map->check_types( cyclicity, types, variables, port_scope );
    }
    if (index)
    {
        index->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
    }
}

/*a c_co_lvar
 */
/*f c_co_lvar::check_types
  Check the types in an lvar.
  Generally this just involves creating an error message for invalid lvars, and type checking any index expressions
*/
void c_co_lvar::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (lvar)
    {
        lvar->check_types( cyclicity, types, variables );
        if ((symbol) && (element<0))
        {
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_check_types, "Element '%s' not part of structure in lvar", lex_string_from_terminal( symbol ));
        }
        if (first)
        {
            if (cyclicity->type_value_pool->derefs_to_bit_vector(lvar->type))
            {
                type = type_value_bit_array_size_1;
            }
            else if (cyclicity->type_value_pool->derefs_to_array(lvar->type))
            {
                type = cyclicity->type_value_pool->array_type(lvar->type);
            }
            else
            {
                cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_check_types, "Not an array to be indexed" );
            }
            first->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
            first->evaluate_potential_constant( cyclicity, types, variables, 0 );
            if (length)
            {
                if ( (length->type_check_within_type_context( cyclicity, types, variables, type_value_integer )) &&
                     (length->evaluate_constant( cyclicity, types, variables, 0 )) )
                {
                    int vector_length;
                    vector_length = cyclicity->type_value_pool->integer_value( length->type_value );
                    type = cyclicity->type_value_pool->add_array( type, vector_length );
                    if (cyclicity->type_value_pool->check_resolved(first->type_value))
                    {
                        int start_index;
                        char buffer[256];
                        start_index = cyclicity->type_value_pool->integer_value( first->type_value );
                        if ((start_index<0) || (start_index+vector_length>cyclicity->type_value_pool->array_size(lvar->type)) )
                        {
                            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Range [%d;%d] out of range for array or vector %s", vector_length, start_index, cyclicity->type_value_pool->display(buffer,sizeof(buffer),lvar->type) );
                        }
                    }
                }
                else
                { 
                    cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Length field for subscripting array/vector not static" );
                    type = type_value_error;
                }
            }
            else
            {
                if (cyclicity->type_value_pool->check_resolved(first->type_value))
                {
                    char buffer[256];
                    int first_value = cyclicity->type_value_pool->integer_value( first->type_value );
                    if ((first_value<0) || (first_value>=cyclicity->type_value_pool->array_size(lvar->type)) )
                    {
                        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Index %d out of range for array or vector type %s", first_value, cyclicity->type_value_pool->display(buffer,sizeof(buffer),lvar->type) );
                    }
                }
            }
        }
    }

}

/*a c_co_module
 */
/*f c_co_module::check_types
  check types of ports and signal declarations match (for clocked signals)
  check local signals to see all resets to them are single bit
  check types of port maps in instantiations
  check types of statement contents
*/
void c_co_module::check_types( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types )
{
    c_co_module_body_entry *combe;
    int i;

    SL_DEBUG( sl_debug_level_verbose_info, "Module '%s'", lex_string_from_terminal( symbol ));

    if (ports)
    {
        ports->check_types( cyclicity, types, local_signals );
    }

    for (i=0; local_signals->co[i].cyc_object; i++)
    {
        c_co_signal_declaration *cosd;
        cosd = local_signals->co[i].co_signal_declaration;
        if ( ((cosd->signal_declaration_type==signal_declaration_type_clocked_local) || (cosd->signal_declaration_type==signal_declaration_type_clocked_output)))
        {
            SL_DEBUG( sl_debug_level_verbose_info, "clocked signal has reset spec %p", cosd->data.clocked.reset_spec );
            if (cosd->data.clocked.reset_spec)
            {
                SL_DEBUG( sl_debug_level_verbose_info, "clocked signal has signal_decl %p type_spec %p", cosd->data.clocked.reset_spec->signal_declaration, cosd->data.clocked.reset_spec->signal_declaration->type_specifier);
            }
        }
        if ( ((cosd->signal_declaration_type==signal_declaration_type_clocked_local) || (cosd->signal_declaration_type==signal_declaration_type_clocked_output)) &&
             (cosd->data.clocked.reset_spec) &&
             (cosd->data.clocked.reset_spec->signal_declaration) &&
             (cosd->data.clocked.reset_spec->signal_declaration->type_specifier) )
        {
            if (!cyclicity->type_value_pool->is_bit( cosd->data.clocked.reset_spec->signal_declaration->type_specifier->type_value ))
            {
                cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-single bit input '%s' as a reset for signal '%s' in module '%s'", lex_string_from_terminal( cosd->data.clocked.reset_spec->symbol ), lex_string_from_terminal( cosd->symbol ), lex_string_from_terminal( this->symbol ));
            }
        }
    }

    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
        case module_body_entry_type_signal_declaration:
            if (combe->data.signal_declaration)
            {
                combe->data.signal_declaration->check_types( cyclicity, types, local_signals );
            }
            break;
        case module_body_entry_type_code_label: // cross reference statements in the module body
            if (combe->data.code_label &&
                combe->data.code_label->statements)
            {
                combe->data.code_label->statements->check_types( cyclicity, types, local_signals );
            }
            break;
        case module_body_entry_type_default_clock_defn:  // no type checking required
        case module_body_entry_type_default_reset_defn:  // no type checking required
        case module_body_entry_type_schematic_symbol:    // no type checking required
            // Done...
            break;
        }
    }
}

/*a c_co_named_expression
 */
/*f c_co_named_expression::check_types
  Need to check the types of an expression match the type context
  Need to check that the element number is legal if a symbol is specified (i.e. if this is supposed to be a structure element)
*/
void c_co_named_expression::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    SL_DEBUG( sl_debug_level_verbose_info, "Check types this %p (type context %d)", this, type_value_integer );
    if (expression)
    {
        expression->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
    }
    SL_DEBUG( sl_debug_level_verbose_info, "Done" );
}

/*a c_co_nested_assignment
 */
/*f c_co_nested_assignment::check_types
  Need to check the types of an expression match the type context
  Need to check that the element number is legal if a symbol is specified (i.e. if this is supposed to be a structure element)
*/
void c_co_nested_assignment::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    SL_DEBUG( sl_debug_level_verbose_info, "Check types this %p (type context %d)", this, type_context );
    if (symbol)
    {
        if (element<0)
        {
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_check_types, "Element '%s' not part of structure in assignment", lex_string_from_terminal( symbol ));
        }
    }
    if (nested_assignment)
    {
        nested_assignment->check_types( cyclicity, types, variables );
    }
    if (expression)
    {
        if (wildcard)
        {
            // If a wildcard (i.e. all leaves) then the context is type_value_integer
            expression->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
        }
        else
        {
            expression->type_check_within_type_context( cyclicity, types, variables, type_context );
        }
    }
    if (next_in_list)
        ((c_co_nested_assignment *)next_in_list)->check_types( cyclicity, types, variables );
    SL_DEBUG( sl_debug_level_verbose_info, "Done" );
}

/*a c_co_port_map
 */
/*f c_co_port_map::check_types
 */
void c_co_port_map::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_co_scope *port_scope )
{
    if (!port_lvar)
    {
        if (type==port_map_type_clock) return;
        cyclicity->set_parse_error( this, co_compile_stage_check_types, "Result of previous error - port_lvar is NULL)");
        return;
    }
    port_lvar->check_types( cyclicity, types, port_scope );
    switch (type)
    {
    case port_map_type_clock:
        break;
    case port_map_type_input:
        if (expression)
        {
            expression->type_check_within_type_context( cyclicity, types, variables, port_lvar?port_lvar->type:type_value_undefined );
            SL_DEBUG( sl_debug_level_info, "Input port expression in type context %p/%d", port_lvar,port_lvar?port_lvar->type:type_value_undefined );
        }
        if ( !expression || (port_lvar->type==type_value_undefined) || (port_lvar->type==type_value_error))
        {
            char buffer[256], buffer2[256];
            cyclicity->set_parse_error( this, co_compile_stage_check_types, "Type mismatch in module instantiation input (port has type %s, expression driving it is %s)",
                                        cyclicity->type_value_pool->display(buffer,sizeof(buffer),port_lvar->type),
                                        cyclicity->type_value_pool->display(buffer2,sizeof(buffer2),expression?expression->type:type_value_undefined) );
        }
        break;
    case port_map_type_output:
        if (lvar)
        {
            lvar->check_types( cyclicity, types, variables );
            if (port_lvar)
            {
                if (cyclicity->type_value_pool->derefs_to(lvar->type) != cyclicity->type_value_pool->derefs_to(port_lvar->type))
                {
                    char buffer[256], buffer2[256];
                    cyclicity->set_parse_error( this, co_compile_stage_check_types, "Type mismatch in module instantiation output (port has %s, net assigned to it is %s)",
                                    cyclicity->type_value_pool->display(buffer,sizeof(buffer),port_lvar->type),
                                    cyclicity->type_value_pool->display(buffer2,sizeof(buffer2),lvar->type) );

                }
            }
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_port_map *)next_in_list)->check_types( cyclicity, types, variables, port_scope );
    }
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::check_types
 */
void c_co_signal_declaration::check_types( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    SL_DEBUG( sl_debug_level_verbose_info, "Check types of signal '%s' (%p) in module", lex_string_from_terminal(symbol), this );

    /*b If this is a clocked port then check the port and the clocked declaration have the same type
     */
    if ( (local_clocked_signal) &&
         (local_clocked_signal->type_specifier->type_value != type_specifier->type_value ) )
    {
        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_check_types, "Type mismatch between clocked signal definition signal '%s' and the module port '%s'", lex_string_from_terminal( symbol ), lex_string_from_terminal( local_clocked_signal->symbol ));
    }

    /*b If this is a clocked signal, check types of reset values
     */
    if ( (signal_declaration_type == signal_declaration_type_clocked_output) ||
         (signal_declaration_type == signal_declaration_type_clocked_local) )
    {
        /*b If there is a list of reset values (nested assignment), then cross reference that, noting the type of this signal
         */
        if ((data.clocked.reset_value) && (type_specifier))
        {
            data.clocked.reset_value->check_types( cyclicity, types, variables );
        }
    }
}

/*a c_co_statement
 */
/*f check_types
  Check types within a statement and any following statements in its list
  For an assignment, this means checking the types within the lvar, and checking the nested assignment has the same type
*/
void c_co_statement::check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    switch (statement_type)
    {
    case statement_type_assignment:
    {
        if (type_data.assign_stmt.lvar)
        {
            type_data.assign_stmt.lvar->check_types( cyclicity, types, variables );
        }
        if (type_data.assign_stmt.nested_assignment)
        {
            type_data.assign_stmt.nested_assignment->type_context = type_data.assign_stmt.lvar->type; // Assign, as checking the type of the lvar will change the type of subscripted arrays
            type_data.assign_stmt.nested_assignment->check_types( cyclicity, types, variables );
        }
        break;
    }
    case statement_type_if_statement:
        if (type_data.if_stmt.expression)
        {
            type_data.if_stmt.expression->type_check_within_type_context( cyclicity, types, variables, type_value_bool );
        }
        if (type_data.if_stmt.if_true)
        {
            type_data.if_stmt.if_true->check_types( cyclicity, types, variables );
        }
        if (type_data.if_stmt.if_false)
        {
            type_data.if_stmt.if_false->check_types( cyclicity, types, variables );
        }
        if (type_data.if_stmt.elsif)
        {
            type_data.if_stmt.elsif->check_types( cyclicity, types, variables );
        }
        break;
    case statement_type_for_statement:
        // #iterations and first_value MUST be constant; last_value and next_value may be, and so may the statement
        if (type_data.for_stmt.iterations)
        {
            type_data.for_stmt.iterations->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
        }
        if (type_data.for_stmt.first_value)
        {
            type_data.for_stmt.first_value->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
        }
        if (type_data.for_stmt.last_value)
        {
            type_data.for_stmt.last_value->type_check_within_type_context( cyclicity, types, type_data.for_stmt.scope, type_value_bool );
        }
        if (type_data.for_stmt.next_value)
        {
            type_data.for_stmt.next_value->type_check_within_type_context( cyclicity, types, type_data.for_stmt.scope, type_value_integer );
        }
        if (type_data.for_stmt.statement)
        {
            type_data.for_stmt.statement->check_types( cyclicity, types, type_data.for_stmt.scope );
        }
        break;
    case statement_type_full_switch_statement:
    case statement_type_partial_switch_statement:
    case statement_type_priority_statement:
        if (type_data.switch_stmt.expression)
        {
            type_data.switch_stmt.expression->type_check_within_type_context( cyclicity, types, variables, type_data.switch_stmt.expression->type );
            if (type_data.switch_stmt.cases)
            {
                type_data.switch_stmt.cases->check_types( cyclicity, types, variables );
            }
        }
        break;
    case statement_type_block:
        if (type_data.block)
        {
            type_data.block->check_types( cyclicity, types, variables );
        }
        break;
    case statement_type_print:
    case statement_type_assert:
    case statement_type_cover:
        if (type_data.print_assert_stmt.assertion)
        {
            if (type_data.print_assert_stmt.value_list)
            {
                type_data.print_assert_stmt.assertion->type_check_within_type_context( cyclicity, types, variables, type_value_undefined );
                c_co_expression *expr;
                for (expr = type_data.print_assert_stmt.value_list; expr; expr=(c_co_expression *)(expr->next_in_list))
                {
                    expr->type_check_within_type_context( cyclicity, types, variables, type_data.print_assert_stmt.assertion->type );
                }
            }
            else
            {
                type_data.print_assert_stmt.assertion->type_check_within_type_context( cyclicity, types, variables, type_value_bool );
            }
        }
        if (type_data.print_assert_stmt.text_args)
        {
            c_co_expression *expr;
            for (expr = type_data.print_assert_stmt.text_args; expr; expr=(c_co_expression *)(expr->next_in_list))
            {
                expr->type_check_within_type_context( cyclicity, types, variables, type_value_integer );
            }
        }
        if (type_data.print_assert_stmt.statement)
        {
            type_data.print_assert_stmt.statement->check_types( cyclicity, types, variables );
        }
        break;
    case statement_type_log:
    {
        class c_co_named_expression *arg;
        for (arg = type_data.log_stmt.values; arg; arg=(c_co_named_expression *)(arg->next_in_list))
        {
            arg->check_types( cyclicity, types, variables );
        }
        break;
    }
    case statement_type_instantiation:
        if (type_data.instantiation)
        {
            type_data.instantiation->check_types( cyclicity, types, variables );
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_statement *)next_in_list)->check_types( cyclicity, types, variables );
    }
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


