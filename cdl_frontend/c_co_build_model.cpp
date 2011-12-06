/*a Copyright
  
  This file 'c_co_build_model.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a To do
  When building an expression from an fsm state, the model expression should be adding that fsm state too.
  Add documentation for FSM states
  Add documentation output for statements?
  Add documentation output for code blocks
 */
/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#include "c_co_module_prototype_body_entry.h"
#include "c_co_module.h"
#include "c_co_module_body_entry.h"
#include "c_co_nested_assignment.h"
#include "c_co_named_expression.h"
#include "c_co_port_map.h"
#include "c_co_schematic_symbol.h"
#include "c_co_schematic_symbol_body_entry.h"
#include "c_co_signal_declaration.h"
#include "c_co_sized_int_pair.h"
#include "c_co_statement.h"
#include "c_co_token_union.h"
#include "c_co_timing_spec.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"

#include "c_model_descriptor.h"

/*a Defines
 */
#define DOC_STRING(x) ((x)?lex_string_from_terminal(x):NULL)

/*a Types
 */
/*t t_expr_op
 */
typedef struct t_expr_op
{
    t_expr_subtype expr_subtype;
    t_md_expr_fn fn;
    int num_args;
} t_expr_op;

/*t t_traverse_structure_callback_fn
 */
typedef void (t_traverse_structure_callback_fn)( class c_cyclicity *cyclicity,
                                                 class c_model_descriptor *model,
                                                 struct t_md_module *module,
                                                 void *handle,
                                                 void *model_lvar, // t_md_lvar or t_md_port_lvar
                                                 class c_co_expression *expression,
                                                 struct t_md_lvar *model_expression_lvar,
                                                 struct t_md_lvar *model_lvar_context );

/*a Static variables
 */
/*v expr_ops
 */
static t_expr_op expr_ops[] = 
{
    { expr_subtype_negate, md_expr_fn_neg, 1 },
    { expr_subtype_arithmetic_plus, md_expr_fn_add, 2 },
    { expr_subtype_arithmetic_minus, md_expr_fn_sub, 2 },
    { expr_subtype_arithmetic_multiply, md_expr_fn_mult, 2 },
    { expr_subtype_binary_not, md_expr_fn_not, 1 },
    { expr_subtype_binary_and, md_expr_fn_and, 2 },
    { expr_subtype_binary_or, md_expr_fn_or, 2 },
    { expr_subtype_binary_xor, md_expr_fn_xor, 2 },
    { expr_subtype_logical_not, md_expr_fn_logical_not, 1 },
    { expr_subtype_logical_and, md_expr_fn_logical_and, 2 },
    { expr_subtype_logical_or, md_expr_fn_logical_or, 2 },
    { expr_subtype_compare_eq, md_expr_fn_eq, 2 },
    { expr_subtype_compare_ne, md_expr_fn_neq, 2 },
    { expr_subtype_compare_ge, md_expr_fn_ge, 2 },
    { expr_subtype_compare_gt, md_expr_fn_gt, 2 },
    { expr_subtype_compare_le, md_expr_fn_le, 2 },
    { expr_subtype_compare_lt, md_expr_fn_lt, 2 },
    { expr_subtype_query, md_expr_fn_query, 3 },
    { expr_subtype_none, md_expr_fn_none, -1 }
};

/*v tmp_signal_value
 */
t_md_signal_value tmp_signal_value[4];

/*a Static assist functions
 */
/*f get_sized_value_from_type_value
 */
static int get_sized_value_from_type_value( class c_cyclicity *cyclicity, int tmp, t_type_value type_value )
{
    int size;
    t_type_value_bit_vector_data *bits, *mask;

    if (cyclicity->type_value_pool->bit_vector_values( type_value, &size, &bits, &mask ))
    {
        tmp_signal_value[tmp].width = size;
        tmp_signal_value[tmp].value[0] = bits[0];
        return 1;
    }
    return 0;
}

/*f traverse_structure
  Traverse a structure, seeking terminals; perform a callback on each terminal
  Copy model_expression_lvar to use it
  Copy model_lvar to use it
 */
static void traverse_structure( class c_cyclicity *cyclicity,
                                class c_model_descriptor *model,
                                struct t_md_module *module,
                                void *handle,
                                t_traverse_structure_callback_fn callback,
                                int lvar_is_port,
                                void *model_lvar,
                                t_type_value type_context,
                                class c_co_expression *expression,
                                struct t_md_lvar *model_expression_lvar,
                                struct t_md_lvar *model_lvar_context )
{
    struct t_md_lvar *model_expression_lvar_copy;
    void *model_lvar_copy;

    if (cyclicity->type_value_pool->derefs_to_bit_vector( type_context )) // Termination condition - we hit something we terminals
    {
        callback( cyclicity, model, module, handle, model_lvar, expression, model_expression_lvar, model_lvar_context );
    }
    else if ( cyclicity->type_value_pool->derefs_to_structure( type_context ) ) // assignment is structure_inst <= structure_inst
    {
        t_symbol *element_symbol;
        t_type_value element_type;
        int num_elements, i;
        t_md_statement *last_statement;

        num_elements = cyclicity->type_value_pool->get_structure_element_number( type_context ); // Get number of elements in the structure
        last_statement = NULL;
        if (expression)
        {
            model_expression_lvar = expression->build_lvar( cyclicity, model, module, model_lvar_context );
        }
        for (i=0; i<num_elements; i++) // Run through all elements, and do an assignment; note that for non-terminals (structures or arrays) this means more work!
        {
            element_type = cyclicity->type_value_pool->get_structure_element_type( type_context, i );
            element_symbol = cyclicity->type_value_pool->get_structure_element_name( type_context, i );
            model_expression_lvar_copy = model->lvar_duplicate( module, model_expression_lvar );
            if (lvar_is_port)
            {
                model_lvar_copy = (void *)model->port_lvar_duplicate( module, (t_md_port_lvar *)model_lvar );
                if (model_lvar_copy)
                    model_lvar_copy = (void *)model->port_lvar_reference( module, (t_md_port_lvar *)model_lvar_copy, lex_string_from_terminal(element_symbol) );
            }
            else
            {
                model_lvar_copy = (void *)model->lvar_duplicate( module, (t_md_lvar *)model_lvar );
                if (model_lvar_copy)
                    model_lvar_copy = (void *)model->lvar_reference( module, (t_md_lvar *)model_lvar_copy, lex_string_from_terminal(element_symbol) );
            }
            if (model_expression_lvar_copy)
            {
                model_expression_lvar_copy = model->lvar_reference( module, model_expression_lvar_copy, lex_string_from_terminal(element_symbol) );
            }
            if (model_lvar_copy && model_expression_lvar_copy)
            {
                traverse_structure( cyclicity, model, module, handle, callback, lvar_is_port, model_lvar_copy, element_type, NULL, model_expression_lvar_copy, model_lvar_context );
            }
            if (model_lvar_copy)
            {
                if (lvar_is_port)
                {
                    model->port_lvar_free( module, (t_md_port_lvar *)model_lvar_copy );
                }
                else
                {
                    model->lvar_free( module, (t_md_lvar *)model_lvar_copy );
                }
            }
            if (model_expression_lvar_copy)
            {
                model->lvar_free( module, model_expression_lvar_copy );
            }
        }
        if (expression && model_expression_lvar )
        {
            model->lvar_free( module, model_expression_lvar );
        }
    }
    else if ( cyclicity->type_value_pool->derefs_to_array( type_context ) )
    {
        fprintf(stderr,"traverse_structure::c_co_build_model:Should not get this far - error at cross referencing ****************************************\n");
    }
}

/*a c_co_expression
 */
/*f c_co_expression::build_model
 */
struct t_md_expression *c_co_expression::build_model( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module, t_md_lvar *model_lvar_context )
{
    t_md_expression *model_expression;
    int i;

    model_expression = NULL;

    CO_DEBUG( sl_debug_level_info, "Build model of expression %p expr_type %d type %d value %d", this, expr_type, type, type_value );
    if (cyclicity->type_value_pool->check_resolved(type_value))
    {
        if (get_sized_value_from_type_value( cyclicity, 0, type_value ))
        {
            CO_DEBUG( sl_debug_level_verbose_info, "Get sized value from type value %d gives size %d value %08llx", type_value, tmp_signal_value[0].width, tmp_signal_value[0].value[0] );
            model_expression = model->expression( module, tmp_signal_value[0].width, &tmp_signal_value[0] );
        }
    }
    else
    {
        switch (expr_type)
        {
        case expr_type_sized_int:
        {
            if (get_sized_value_from_type_value( cyclicity, 0, sized_int->type_value ))
            {
                model_expression = model->expression( module, tmp_signal_value[0].width, &tmp_signal_value[0] );
            }
            break;
        }
        case expr_type_lvar:
        {
            if (!lvar)
                return NULL;
            if (lvar->non_signal_declaration.cyc_object)
            {
                switch (lvar->non_signal_declaration.cyc_object->co_type)
                {
                case co_type_constant_declaration:
                {
                    if (get_sized_value_from_type_value( cyclicity, 0, lvar->non_signal_declaration.co_constant_declaration->type_value ))
                    {
                        model_expression = model->expression( module, tmp_signal_value[0].width, &tmp_signal_value[0] );
                        //CO_DEBUG( sl_debug_level_info, "Constant '%s', type_value %d bit value size %d values %08x %p model_expression %p", lex_string_from_terminal(lvar->non_signal_declaration.co_constant_declaration->symbol), lvar->non_signal_declaration.co_constant_declaration->type_value, size, bits[0], bits, model_expression );
                    }
                    break;
                }
                case co_type_enum_identifier:
                {
                    if (get_sized_value_from_type_value( cyclicity, 0, lvar->non_signal_declaration.co_enum_identifier->type_value ))
                    {
                        model_expression = model->expression( module, tmp_signal_value[0].width, &tmp_signal_value[0] );
                        //CO_DEBUG( sl_debug_level_info, "Enum identifier '%s', bit value size %d values %p model_expression %p", lex_string_from_terminal(lvar->non_signal_declaration.co_enum_identifier->symbol), size, bits, model_expression );
                    }
                    break;
                }
                case co_type_fsm_state:
                {
                    if (get_sized_value_from_type_value( cyclicity, 0, lvar->non_signal_declaration.co_fsm_state->type_value ))
                    {
                        model_expression = model->expression( module, tmp_signal_value[0].width, &tmp_signal_value[0] );
                        //CO_DEBUG( sl_debug_level_info, "FSM state '%s', bit value size %d values %p model_expression %p", lex_string_from_terminal(lvar->non_signal_declaration.co_fsm_state->symbol), size, bits, model_expression );
                    }
                    break;
                }
                case co_type_statement:
                {
                    if (lvar->non_signal_declaration.co_statement->statement_type==statement_type_for_statement)
                    {
                        if (get_sized_value_from_type_value( cyclicity, 0, lvar->non_signal_declaration.co_statement->type_data.for_stmt.type_value ))
                        {
                            model_expression = model->expression( module, tmp_signal_value[0].width, &tmp_signal_value[0] );
                            //CO_DEBUG( sl_debug_level_info, "Iterator value '%s', bit value size %d values %p model_expression %p", lex_string_from_terminal(lvar->non_signal_declaration.co_statement->type_data.for_stmt.symbol), size, bits, model_expression );
                        }
                    }
                }
                break;
                default:
                    fprintf(stderr,"c_co_expression::build_model::NYI Lvar %p %d\n", lvar->non_signal_declaration.cyc_object,  lvar->non_signal_declaration.cyc_object->co_type );
                    break;
                }
            }
            else // Signal - lvar->signal_declaration could be null, meaning implied from context, though
            {
                t_md_lvar *model_lvar;
                model_lvar = lvar->build_model( cyclicity, model, module, model_lvar_context );
                if (model_lvar)
                    model_expression = model->expression( module, model_lvar );
                if (!model_expression)
                {
                    fprintf(stderr, "c_co_expression::build_model:Expected non-null expression here\n");
                }
                //CO_DEBUG( sl_debug_level_info, "Model_Expression from lvar %p gives %p", lvar, model_expression );
                break;
            }
            break;
        }
        case expr_type_unary_function:
        case expr_type_binary_function:
        case expr_type_ternary_function:
        {
            t_md_expression *first_expr = NULL;
            t_md_expression *second_expr = NULL;
            t_md_expression *third_expr = NULL;
            int expand_expressions;

            CO_DEBUG( sl_debug_level_info,  "c_co_expression::build_model:function %d:first is %p/%d, second is %p/%d", expr_subtype, first, first?(first->type_value):-1, second, second?(second->type_value):-1 );
            expand_expressions = 0;

            switch (expr_subtype)
            {
            default:
                expand_expressions = 1;
                break;
            }
            if (expand_expressions)
            {
                if (first)
                    first_expr = first->build_model( cyclicity, model, module, model_lvar_context );
                if (second)
                    second_expr = second->build_model( cyclicity, model, module, model_lvar_context );
                if (third)
                    third_expr = third->build_model( cyclicity, model, module, model_lvar_context );
            }

            switch (expr_subtype)
            {
            case expr_subtype_expression:
                model_expression = first->build_model( cyclicity, model, module, model_lvar_context );
                break;
            default:
                for (i=0; expr_ops[i].expr_subtype!=expr_subtype_none; i++)
                {
                    if (expr_subtype==expr_ops[i].expr_subtype)
                    {
                        if (!first_expr)
                            break;
                        if ( (expr_ops[i].num_args>=2) && !second_expr )
                            break;
                        if ( (expr_ops[i].num_args>=3) && !third_expr )
                            break;
                        model_expression = model->expression( module, expr_ops[i].fn, first_expr, second_expr, third_expr );
                        break;
                    }
                }
                if (expr_ops[i].expr_subtype==expr_subtype_none)
                {
                    cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Build model: NYI subtype" );
                    fprintf(stderr,"c_co_expression::build_model:Subtype %d not implemented by backend interface yet********************************************************************************\n", expr_subtype );
                    model_expression = first->build_model( cyclicity, model, module, model_lvar_context );
                }
                break;
            }
               
            break;
        }
        case expr_type_bundle:
            c_co_expression *subexpr;
            model_expression = NULL;
            for (subexpr=first; subexpr; subexpr=(c_co_expression *)(subexpr->next_in_list))
            {
                if (!model_expression)
                {
                    model_expression = subexpr->build_model( cyclicity, model, module, model_lvar_context );
                }
                else
                {
                    model_expression = model->expression_bundle( module, model_expression, subexpr->build_model( cyclicity, model, module, model_lvar_context ) );
                }
            }
            break;
        default:
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Build model: NYI type" );
            fprintf(stderr,"c_co_expression::build_model:Expression type %d not implemented by backend interface yet********************************************************************************\n", expr_type );
            model_expression = first->build_model( cyclicity, model, module, model_lvar_context );
            break;
        }
    }

    switch (implicit_cast)
    {
    case expr_cast_none:
    case expr_cast_bool_to_bit_vector: // No cast requried - single bit to single bit
        break;
    case expr_cast_integer_to_bool:
    case expr_cast_bit_vector_to_bool:
        if (model_expression)
        {
            model_expression = model->expression( module, md_expr_fn_neq, model_expression, model->expression( module, cyclicity->type_value_pool->get_size_of(precast_type), &md_signal_value_zero ), NULL );
        }
        break;
    case expr_cast_integer_to_bit_vector:
        if (model_expression)
        {
            model_expression = model->expression_cast( module, cyclicity->type_value_pool->get_size_of( type ), 1, model_expression );
        }
        break;
    case expr_cast_bool_to_integer:
        if (model_expression)
        {
            model_expression = model->expression_cast( module, 8*sizeof(t_sl_uint64), 0, model_expression );
        }
        break;
    case expr_cast_bit_vector_to_integer:
        if (model_expression)
        {
            model_expression = model->expression_cast( module, 8*sizeof(t_sl_uint64), 0, model_expression );
        }
        break;
    }

    return model_expression;
}

/*f c_co_expression::build_model_given_element
struct t_md_expression *c_co_expression::build_model_given_element( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module, t_md_lvar *model_lvar_context )
{
    t_md_expression *expression;

    expression = NULL;

    if (expr_type==expr_type_lvar)
    {
        if (!lvar)
            return NULL;
        if (lvar->non_signal_declaration.cyc_object)
        {
            return NULL;
        }
        else // Signal - lvar->signal_declaration could be null, meaning implied from context, though
        {
            t_md_lvar *model_lvar;
            model_lvar = lvar->build_model( cyclicity, model, module, model_lvar_context );
            if (model_lvar)
                model_lvar = model->lvar_reference( module, model_lvar, lex_string_from_terminal( symbol ));
            if (model_lvar)
                expression = model->expression( module, model_lvar );
        }
    }

    return expression;
}
 */

/*f c_co_expression::build_lvar
 */
struct t_md_lvar *c_co_expression::build_lvar( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module, t_md_lvar *model_lvar_context )
{
    if (expr_type==expr_type_lvar)
    {
        if (!lvar)
            return NULL;
        if (lvar->non_signal_declaration.cyc_object)
        {
            return NULL;
        }
        else // Signal - lvar->signal_declaration could be null, meaning implied from context, though
        {
            return lvar->build_model( cyclicity, model, module, model_lvar_context );
        }
    }
    else
    {
        fprintf( stderr, "Cannot build lvar from expression type %d\n", expr_type);
    }

    return NULL;
}

/*a c_co_lvar
 */
/*f c_co_lvar::build_model
*/
struct t_md_lvar *c_co_lvar::build_model( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module, t_md_lvar *model_lvar_context )
{
    t_md_lvar *model_lvar;

    model_lvar = NULL;

    if (lvar) // If this is a subelement of another lvar, build that first
    {
        model_lvar = lvar->build_model( cyclicity, model, module, model_lvar_context );
        CO_DEBUG( sl_debug_level_info, "subelement/index of %p", model_lvar);
        if (model_lvar)
        {
            if (first)
            {
                if (cyclicity->type_value_pool->derefs_to_bit_vector(lvar->type))
                {
                    CO_DEBUG( sl_debug_level_info, "indexed bit vector", 0 );
                    if (length)
                    {
                        t_md_expression *model_expression;
                        model_expression = first->build_model( cyclicity, model, module, model_lvar_context );
                        if (model_expression)
                        {
                            model_lvar = model->lvar_bit_range_select( module, model_lvar, model_expression, cyclicity->type_value_pool->integer_value(length->type_value) );
                        }
                    }
                    else if (cyclicity->type_value_pool->check_resolved( first->type_value ))
                    {
                        model_lvar = model->lvar_bit_select( module, model_lvar, cyclicity->type_value_pool->integer_value(first->type_value) );
                    }
                    else
                    {
                        t_md_expression *model_expression;
                        model_expression = first->build_model( cyclicity, model, module, model_lvar_context );
                        if (model_expression)
                        {
                            model_lvar = model->lvar_bit_select( module, model_lvar, model_expression );
                        }
                    }
                }
                else
                {
                    CO_DEBUG( sl_debug_level_info, "indexed array", 0 );
                    if (cyclicity->type_value_pool->check_resolved( first->type_value ))
                    {
                        model_lvar = model->lvar_index( module, model_lvar, cyclicity->type_value_pool->integer_value(first->type_value) );
                    }
                    else
                    {
                        t_md_expression *model_expression;
                        model_expression = first->build_model( cyclicity, model, module, NULL );
                        if (model_expression)
                        {
                            model_lvar = model->lvar_index( module, model_lvar, model_expression );
                        }
                    }
                    CO_DEBUG( sl_debug_level_info, "indexed array lvar %p", model_lvar );
                }
            }
            if (symbol)
            {
                model_lvar = model->lvar_reference( module, model_lvar, lex_string_from_terminal( symbol ) );
                CO_DEBUG( sl_debug_level_info, "element '%s' lvar %p", lex_string_from_terminal( symbol ), model_lvar );
            }
        }
    }
    else if (signal_declaration) // else we are the toplevel - if a signal, use that
    {
        model_lvar = model->lvar_reference( module, NULL, lex_string_from_terminal( symbol ) );
        CO_DEBUG( sl_debug_level_info, "Lvar from %s is %p", lex_string_from_terminal( symbol ), model_lvar);
    }
    else // else we are subelement of a toplevel derived from context, so duplicate the context and reference in that
    {
        model_lvar = model->lvar_duplicate( module, model_lvar_context );
        if (model_lvar)
        {
            model_lvar = model->lvar_reference( module, model_lvar, lex_string_from_terminal( symbol ) );
        }
        CO_DEBUG( sl_debug_level_info, "Lvar from subelement %s is %p", lex_string_from_terminal( symbol ), model_lvar);
    }
    return model_lvar;
}

/*f c_co_lvar::build_model_port
*/
struct t_md_port_lvar *c_co_lvar::build_model_port( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module )
{
    t_md_port_lvar *model_port_lvar;

    model_port_lvar = NULL;

    if (lvar) // If this is a subelement of another lvar, build that first
    {
        model_port_lvar = lvar->build_model_port( cyclicity, model, module );
        if (model_port_lvar)
        {
            if (symbol)
            {
                model_port_lvar = model->port_lvar_reference( module, model_port_lvar, lex_string_from_terminal( symbol ) );
            }
        }
    }
    else // else this is a reference to a name of a port on the module instance
    {
        if (symbol)
        {
            model_port_lvar = model->port_lvar_reference( module, NULL, lex_string_from_terminal( symbol ) );
        }
    }
    //CO_DEBUG( sl_debug_level_info, "Lvar from %s is %p", lex_string_from_terminal( symbol ), model_lvar);
    return model_port_lvar;
}

/*a c_co_module_prototype
 */
/*f c_co_module_prototype::build_model
*/
void c_co_module_prototype::build_model( c_cyclicity *cyclicity, c_model_descriptor *model )
{
    c_co_module_prototype_body_entry *compbe;
    c_co_signal_declaration *cosd;
    t_md_type_definition_handle model_type;

    md_module_prototype = model->module_create_prototype( lex_string_from_terminal( symbol ), 1, (void *)this, (void *)this, 0 );
    if (!md_module_prototype)
        return;

    CO_DEBUG( sl_debug_level_info, "Build model of module prototype %s %p", lex_string_from_terminal( symbol ), md_module_prototype );

    for (cosd=ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list )
    {
        const char *signal_name;

        signal_name = lex_string_from_terminal(cosd->symbol);

        model_type.type = md_type_definition_handle_type_none;
        if (cosd->type_specifier)
        {
            model_type = cyclicity->type_value_pool->get_model_type( model, cosd->type_specifier->type_value );
        }

        switch (cosd->signal_declaration_type)
        {
        case signal_declaration_type_clock:
            model->signal_add_clock( md_module_prototype, signal_name, 1, (void *)this, (void *)cosd, 0 );
            CO_DEBUG( sl_debug_level_info, "Added clock %s to prototype %s", signal_name, lex_string_from_terminal( symbol ) );
            break;
        case signal_declaration_type_input:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_input( md_module_prototype, signal_name, 1, (void *)this, (void *)cosd, 0, model_type );
                CO_DEBUG( sl_debug_level_info, "Added input %s to prototype %s", signal_name, lex_string_from_terminal( symbol ) );
            }
            break;
        case signal_declaration_type_comb_output:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_output( md_module_prototype, signal_name, 1, (void *)this, (void *)cosd, 0, model_type );
                CO_DEBUG( sl_debug_level_info, "Added output %s to prototype %s", signal_name, lex_string_from_terminal( symbol ) );
            }
            break;
        default:
            CO_DEBUG( sl_debug_level_info, "c_co_module_prototype::build_model:%s: Type %d", signal_name, cosd->signal_declaration_type );
            break;
        }
    }

    /*b Now build the module prototype body
     */
    for (compbe = body; compbe; compbe=(c_co_module_prototype_body_entry *)compbe->next_in_list)
    {
        switch (compbe->type)
        {
            /*b Timing declaration
             */
        case module_prototype_body_entry_type_timing_spec:
        {
            c_co_timing_spec *cots;
            c_co_token_union *cotu;
            cots = compbe->data.timing_spec;
            switch (cots->type)
            {
            case timing_spec_type_to_clock:
                for (cotu=cots->signals; cotu; cotu=(c_co_token_union *)cotu->next_in_list)
                {
                    model->signal_input_used_on_clock( md_module_prototype, lex_string_from_terminal(cotu->token_union_data.symbol), lex_string_from_terminal(cots->clock_symbol), cots->clock_edge == timing_spec_clock_edge_falling );
                    CO_DEBUG( sl_debug_level_info, "To clock %s input %s negedge %d", lex_string_from_terminal(cots->clock_symbol) , lex_string_from_terminal(cotu->token_union_data.symbol), cots->clock_edge == timing_spec_clock_edge_falling);
                }
                break;
            case timing_spec_type_from_clock:
                for (cotu=cots->signals; cotu; cotu=(c_co_token_union *)cotu->next_in_list)
                {
                    model->signal_output_generated_from_clock( md_module_prototype, lex_string_from_terminal(cotu->token_union_data.symbol), lex_string_from_terminal(cots->clock_symbol), cots->clock_edge == timing_spec_clock_edge_falling );
                    CO_DEBUG( sl_debug_level_info, "From clock %s output %s negedge %d", lex_string_from_terminal(cots->clock_symbol) , lex_string_from_terminal(cotu->token_union_data.symbol), cots->clock_edge == timing_spec_clock_edge_falling);
                }
                break;
            case timing_spec_type_comb_inputs:
                for (cotu=cots->signals; cotu; cotu=(c_co_token_union *)cotu->next_in_list)
                {
                    model->signal_input_used_combinatorially( md_module_prototype, lex_string_from_terminal(cotu->token_union_data.symbol) );
                    CO_DEBUG( sl_debug_level_info, "Comb input %s", lex_string_from_terminal(cotu->token_union_data.symbol));
                }
                break;
            case timing_spec_type_comb_outputs:
                for (cotu=cots->signals; cotu; cotu=(c_co_token_union *)cotu->next_in_list)
                {
                    model->signal_output_derived_combinatorially( md_module_prototype, lex_string_from_terminal(cotu->token_union_data.symbol) );
                    CO_DEBUG( sl_debug_level_info, "Comb output %s", lex_string_from_terminal(cotu->token_union_data.symbol));
                }
                break;
            }
            break;
        }
        }
    }
}

/*a c_co_module
 */
/*f c_co_module::build_model
  first create instances of all the state in the type_value_pool
  then build the backend structure from the internal structure
*/
void c_co_module::build_model( c_cyclicity *cyclicity, c_model_descriptor *model )
{
    int i;
    t_md_lvar *model_lvar;
    c_co_module_body_entry *combe;
    t_md_type_definition_handle model_type;
    t_md_usage_type model_usage_type[5];
    model_usage_type[signal_usage_type_rtl]    = md_usage_type_rtl;
    model_usage_type[signal_usage_type_assert] = md_usage_type_assert;
    model_usage_type[signal_usage_type_cover]  = md_usage_type_cover;

    /*b Create md_module
     */
    md_module = model->module_create( lex_string_from_terminal( symbol ), 1, (void *)this, (void *)this, 0, documentation?lex_string_from_terminal(documentation):NULL );
    if (!md_module)
        return;

    CO_DEBUG( sl_debug_level_info, "Build model of module %s %p", lex_string_from_terminal( symbol ), md_module );

    /*b Create clocks first (so state can depend on them - especially for gated clocks which will be later than ports)
     */
    for (i=0; local_signals->co[i].cyc_object; i++)
    {
        c_co_signal_declaration *cosd;
        const char *signal_name;

        cosd = local_signals->co[i].co_signal_declaration;
        signal_name = lex_string_from_terminal(cosd->symbol);

        if (cosd->signal_declaration_type == signal_declaration_type_clock)
        {
            CO_DEBUG( sl_debug_level_info, "Build model - local signal clock %d : %p / %d . %s", i, cosd, cosd->signal_declaration_type, signal_name );

            model->signal_add_clock( md_module, signal_name, 1, (void *)this, (void *)cosd, 0 );
            if (cosd->documentation)
                model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
        }
    }

    /*b Create local non-clock signals
     */
    for (i=0; local_signals->co[i].cyc_object; i++)
    {
        c_co_signal_declaration *cosd;
        const char *signal_name;
        int array, array_size;

        array=0;
        array_size=1;
        cosd = local_signals->co[i].co_signal_declaration;
        signal_name = lex_string_from_terminal(cosd->symbol);

        model_type.type = md_type_definition_handle_type_none;
        if (cosd->type_specifier)
        {
            model_type = cyclicity->type_value_pool->get_model_type( model, cosd->type_specifier->type_value );
        }

        CO_DEBUG( sl_debug_level_info, "Build model - local signal %d : %p / %d . %s (will skip clocks)", i, cosd, cosd->signal_declaration_type, signal_name );

        switch (cosd->signal_declaration_type)
        {
        case signal_declaration_type_input:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_input( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, model_type );
                if (cosd->documentation)
                    model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
            }
            break;
        case signal_declaration_type_comb_output:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_output( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, model_type );
                model->signal_add_combinatorial( md_module, signal_name, 1, (void *)this, (void *)cosd,  0, model_type, model_usage_type[signal_usage_type_rtl] );
                if (cosd->documentation)
                    model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
            }
            break;
        case signal_declaration_type_net_output:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_output( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, model_type );
                model->signal_add_net( md_module, signal_name, 1, (void *)this, (void *)cosd,  0, model_type );
                if (cosd->documentation)
                    model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
            }
            break;
        case signal_declaration_type_clocked_output:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_output( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, model_type );
                if (cosd->documentation)
                {
                    model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
                }
                CO_DEBUG( sl_debug_level_info, "Build model - clocked output state add for %s ", signal_name );
                model->state_add( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, 0, model_type, model_usage_type[cosd->data.clocked.usage_type],
                                  lex_string_from_terminal(cosd->data.clocked.clock_spec->symbol), (cosd->data.clocked.clock_spec->clock_edge == clock_edge_falling),
                                  lex_string_from_terminal(cosd->data.clocked.reset_spec->symbol), (cosd->data.clocked.reset_spec->reset_active == reset_active_high) );
                CO_DEBUG( sl_debug_level_info, "Build model - clocked output state done for %s ", signal_name );
                if (!cosd->documentation && cosd->local_clocked_signal && cosd->local_clocked_signal->documentation)
                {
                    model->state_document( md_module, signal_name, lex_string_from_terminal(cosd->local_clocked_signal->documentation) );
                }
                model_lvar = model->lvar_reference( md_module, NULL, signal_name );
                if (model_lvar)
                {
                    cosd->data.clocked.reset_value->build_model_reset( cyclicity, model, md_module, model_lvar, model_lvar );
                }
            }
            break;
        case signal_declaration_type_comb_local:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_combinatorial( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, model_type, model_usage_type[cosd->data.comb.usage_type] );
                if (cosd->documentation)
                    model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
            }
            break;
        case signal_declaration_type_net_local:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                model->signal_add_net( md_module, signal_name, 1, (void *)this, (void *)cosd,  0, model_type );
                if (cosd->documentation)
                    model->signal_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
            }
            break;
        case signal_declaration_type_clocked_local:
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                if (model->state_add( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, 0, model_type, model_usage_type[cosd->data.clocked.usage_type],
                                      lex_string_from_terminal(cosd->data.clocked.clock_spec->symbol), (cosd->data.clocked.clock_spec->clock_edge == clock_edge_falling),
                                      lex_string_from_terminal(cosd->data.clocked.reset_spec->symbol), (cosd->data.clocked.reset_spec->reset_active == reset_active_high) ))
                {
                    model_lvar = model->lvar_reference( md_module, NULL, signal_name );
                    if (model_lvar)
                    {
                        cosd->data.clocked.reset_value->build_model_reset( cyclicity, model, md_module, model_lvar, model_lvar );
                    }
                    if (cosd->documentation)
                        model->state_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
                }
            }
            break;
        default:
//               CO_DEBUG( sl_debug_level_info, "%s: Type %p refers to %p value %d symbol %p ts %p first %p second %p", signal_name, cots, cots->refers_to, (int)cots->type_value, cots->symbol, cots->type_specifier, cots->first, cots->last );
            break;
        }
    }

    /*b Add clock gates and reset signals (which must now be declared)
     */
    for (i=0; local_signals->co[i].cyc_object; i++)
    {
        c_co_signal_declaration *cosd;
        const char *signal_name;
        int array, array_size;

        array=0;
        array_size=1;
        cosd = local_signals->co[i].co_signal_declaration;
        signal_name = lex_string_from_terminal(cosd->symbol);

        model_type.type = md_type_definition_handle_type_none;
        if (cosd->type_specifier)
        {
            model_type = cyclicity->type_value_pool->get_model_type( model, cosd->type_specifier->type_value );
        }

        CO_DEBUG( sl_debug_level_info, "Build model - local signal %d : %p / %d . %s (will skip clocks)", i, cosd, cosd->signal_declaration_type, signal_name );

        switch (cosd->signal_declaration_type)
        {
        case signal_declaration_type_clock:
            if (cosd->data.clock.gate) // Gated clock
            {
                CO_DEBUG( sl_debug_level_info, "Build model - local signal clock gate %d : %p / %d . %s", i, cosd, cosd->signal_declaration_type, signal_name );
                model->signal_gate_clock( md_module,
                                         signal_name,
                                         lex_string_from_terminal(cosd->data.clock.clock_to_gate->symbol),
                                         lex_string_from_terminal(cosd->data.clock.gate->symbol),
                                         (cosd->data.clock.gate->reset_active == reset_active_high),
                                         (void *)this, (void *)cosd, 0 );
            }
            break;
            
        case signal_declaration_type_clocked_output:
            /* NEED TO DO RESETS HERE
               model->state_declare_reset( md_module, signal_name, (void *)this, (void *)cosd, 0, 0, model_type, model_usage_type[cosd->data.clocked.usage_type],
                                  lex_string_from_terminal(cosd->data.clocked.clock_spec->symbol), (cosd->data.clocked.clock_spec->clock_edge == clock_edge_falling),
                                  lex_string_from_terminal(cosd->data.clocked.reset_spec->symbol), (cosd->data.clocked.reset_spec->reset_active == reset_active_high) );
                CO_DEBUG( sl_debug_level_info, "Build model - clocked output state done for %s ", signal_name );
                model_lvar = model->lvar_reference( md_module, NULL, signal_name );
                if (model_lvar)
                {
                    cosd->data.clocked.reset_value->build_model_reset( cyclicity, model, md_module, model_lvar, model_lvar );
                }
            }
            */
            break;
        case signal_declaration_type_clocked_local:
            /* NEED TO DO RESETS HERE
            if (MD_TYPE_DEFINITION_HANDLE_VALID( model_type ))
            {
                if (model->state_add( md_module, signal_name, 1, (void *)this, (void *)cosd, 0, 0, model_type, model_usage_type[cosd->data.clocked.usage_type],
                                      lex_string_from_terminal(cosd->data.clocked.clock_spec->symbol), (cosd->data.clocked.clock_spec->clock_edge == clock_edge_falling),
                                      lex_string_from_terminal(cosd->data.clocked.reset_spec->symbol), (cosd->data.clocked.reset_spec->reset_active == reset_active_high) ))
                {
                    model_lvar = model->lvar_reference( md_module, NULL, signal_name );
                    if (model_lvar)
                    {
                        cosd->data.clocked.reset_value->build_model_reset( cyclicity, model, md_module, model_lvar, model_lvar );
                    }
                }
                //if (cosd->documentation)
                //    model->state_document( md_module, signal_name, lex_string_from_terminal(cosd->documentation) );
            }
            */
            break;
        default:
            break;
        }
    }

    /*b Now build the module body
     */
    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
            /*b Signal declaration
             */
        case module_body_entry_type_signal_declaration:
        {
            break;
        }

        /*b module_body_entry_type_code_label
         */
        case module_body_entry_type_code_label:
        {
            t_md_statement *md_statement;
            const char *code_label;

            code_label = lex_string_from_terminal( combe->data.code_label->label );
            CO_DEBUG( sl_debug_level_info, "Output code label '%s'", code_label );
            model->code_block( md_module, code_label, 1, (void *)this, (void *)combe->data.code_label, 0, combe->data.code_label->documentation?lex_string_from_terminal(combe->data.code_label->documentation):NULL );
            md_statement = NULL;
            if (combe->data.code_label->statements)
            {
                md_statement = combe->data.code_label->statements->build_model( cyclicity, model, md_module );
            }
            if (md_statement)
            {
                CO_DEBUG( sl_debug_level_info, "Adding statements %p to code label %s", md_statement, code_label );
                model->code_block_add_statement( md_module, code_label, md_statement );
            }
            break;
        }
        /*b Else
         */
        default:
            break;
        }
    }

}

/*a c_co_nested_assignment
 */
/*t t_build_model_structure_assignment_callback_handle
 */
typedef struct
{
    struct t_md_statement *statement;
    int clocked;
    class c_co_nested_assignment *cona;
    const char *documentation;
} t_build_model_structure_assignment_callback_handle;

/*f build_model_structure_assignment_callback
 */
static void build_model_structure_assignment_callback( class c_cyclicity *cyclicity,
                                                       class c_model_descriptor *model,
                                                       struct t_md_module *module,
                                                       void *handle,
                                                       void *model_lvar_in,
                                                       class c_co_expression *expression,
                                                       struct t_md_lvar *model_expression_lvar,
                                                       struct t_md_lvar *model_lvar_context )
{
    struct t_md_expression *model_expression;
    struct t_md_lvar *model_expression_lvar_copy;
    struct t_md_lvar *model_lvar;
    struct t_md_lvar *model_lvar_copy;
    struct t_md_statement *statement;

    t_build_model_structure_assignment_callback_handle *bmsach;
    bmsach = (t_build_model_structure_assignment_callback_handle *)handle;
    model_lvar = (t_md_lvar *)model_lvar_in;

    statement = NULL;
    if (expression) // If at top level of an assignment, we have not got a (sub-elemented) model lvar from the expression
    {
        model_expression = expression->build_model( cyclicity, model, module, model_lvar_context ); // model_lvar_context is the context on the 'lhs' of the assignment
    }
    else // we are nested in an implied assignment with the current (sub-elemented) model lvar valid; we should copy it and use it
    {
        model_expression_lvar_copy = model->lvar_duplicate( module, model_expression_lvar );
        model_expression = model->expression( module, model_expression_lvar_copy );
    }
    model_lvar_copy = model->lvar_duplicate( module, model_lvar );
    if (model_lvar_copy && model_expression)
    {
        if (bmsach->clocked)
        {
            statement = model->statement_create_state_assignment( module, (void *)module, bmsach->cona, 0, model_lvar_copy, model_expression, bmsach->documentation );
        }
        else
        {
            statement = model->statement_create_combinatorial_assignment( module, (void *)module, bmsach->cona, 0, model_lvar_copy, model_expression, bmsach->documentation );
        }
    }
    SL_DEBUG( sl_debug_level_info, "Built assignment statement %p with lvar %p expression %p", statement, model_lvar_copy, model_expression );
    if (bmsach->statement)
    {
        statement = model->statement_add_to_chain( bmsach->statement, statement );
    }
    bmsach->statement = statement;
}

/*f build_model_structure_assignment
  Copy model_expression_lvar to use it
  Copy model_lvar to use it
static struct t_md_statement *build_model_structure_assignment( class c_cyclicity *cyclicity,
                                                                class c_model_descriptor *model,
                                                                struct t_md_module *module,
                                                                void *handle,
                                                                struct t_md_lvar *model_lvar,
                                                                t_type_value type_context,
                                                                class c_co_expression *expression,
                                                                struct t_md_lvar *model_expression_lvar,
                                                                int clocked,
                                                                struct t_md_lvar *model_lvar_context )
{
    struct t_md_expression *model_expression;
    struct t_md_lvar *model_expression_lvar_copy;
    struct t_md_lvar *model_lvar_copy;
    struct t_md_statement *statement;

    if (cyclicity->type_value_pool->derefs_to_bit_vector( type_context ))
    {
        if (expression) // If at top level of an assignment, we have not got a (sub-elemented) model lvar from the expression
        {
            model_expression = expression->build_model( cyclicity, model, module, model_lvar_context ); // model_lvar_context is the context on the 'lhs' of the assignment
        }
        else // we are nested in an implied assignment with the current (sub-elemented) model lvar valid; we should copy it and use it
        {
            model_expression_lvar_copy = model->lvar_duplicate( module, model_expression_lvar );
            model_expression = model->expression( module, model_expression_lvar_copy );
        }
        model_lvar_copy = model->lvar_duplicate( module, model_lvar );
        if (model_lvar_copy && model_expression)
        {
            if (clocked)
            {
                statement = model->statement_create_state_assignment( module, (void *)module, handle, 0, model_lvar_copy, model_expression, NULL );
            }
            else
            {
                statement = model->statement_create_combinatorial_assignment( module, (void *)module, handle, 0, model_lvar_copy, model_expression, NULL );
            }
        }
        CO_DEBUG( sl_debug_level_info, "Built assignment statement %p with lvar %p expression %p", statement, model_lvar_copy, model_expression );
    }
    else if ( cyclicity->type_value_pool->derefs_to_structure( type_context ) ) // assignment is structure_inst <= structure_inst
    {
        t_symbol *element_symbol;
        t_type_value element_type;
        int num_elements, i;
        t_md_statement *last_statement;

        num_elements = cyclicity->type_value_pool->get_structure_element_number( type_context ); // Get number of elements in the structure
        last_statement = NULL;
        if (expression)
        {
            model_expression_lvar = expression->build_lvar( cyclicity, model, module, model_lvar_context );
        }
        for (i=0; i<num_elements; i++) // Run through all elements, and do an assignment; note that for non-terminals (structures or arrays) this means more work!
        {
            element_type = cyclicity->type_value_pool->get_structure_element_type( type_context, i );
            element_symbol = cyclicity->type_value_pool->get_structure_element_name( type_context, i );
            model_expression_lvar_copy = model->lvar_duplicate( module, model_expression_lvar );
            model_lvar_copy = model->lvar_duplicate( module, model_lvar );
            if (model_lvar_copy)
            {
                model_lvar_copy = model->lvar_reference( module, model_lvar_copy, lex_string_from_terminal(element_symbol) );
            }
            if (model_expression_lvar_copy)
            {
                model_expression_lvar_copy = model->lvar_reference( module, model_expression_lvar_copy, lex_string_from_terminal(element_symbol) );
            }
            if (model_lvar_copy && model_expression_lvar_copy)
            {
                statement = build_model_structure_assignment( cyclicity, model, module, handle, model_lvar_copy, element_type, NULL, model_expression_lvar_copy, clocked, model_lvar_context );
                if (last_statement)
                {
                    statement = model->statement_add_to_chain( last_statement, statement );
                }
                last_statement = statement;
            }
            if (model_lvar_copy)
            {
                model->lvar_free( module, model_lvar_copy );
            }
            if (model_expression_lvar_copy)
            {
                model->lvar_free( module, model_expression_lvar_copy );
            }
        }
        if (expression && model_expression_lvar )
        {
            model->lvar_free( module, model_expression_lvar );
        }
    }
    else if ( cyclicity->type_value_pool->derefs_to_array( type_context ) )
    {
        fprintf(stderr,"c_co_statement::build_model:Should not get this far - error at cross referencing ****************************************\n");
    }
    return statement;
}
*/

/*f c_co_nested_assignment::build_model_from_statement
 */
struct t_md_statement *c_co_nested_assignment::build_model_from_statement( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar, int clocked, struct t_md_lvar *model_lvar_context, const char *documentation )
{
    t_md_statement *statement;
    t_md_lvar *model_lvar_copy;
    c_co_nested_assignment *cona;

    statement = NULL;
    if (expression)
    {
        t_build_model_structure_assignment_callback_handle handle;
        handle.statement = NULL;
        handle.clocked = clocked;
        handle.cona = this;
        handle.documentation = documentation;
        traverse_structure( cyclicity, model, module, (void *) &handle, build_model_structure_assignment_callback, 0, (void *)model_lvar, this->type_context, this->expression, NULL, model_lvar_context );
        statement = handle.statement;
    }
    else
    {
        t_md_statement *last_statement;
        last_statement = NULL;
        for (cona=this; cona; cona=(c_co_nested_assignment *)cona->next_in_list)
        {
            if (cona->element>=0)
            {
                model_lvar_copy = model->lvar_duplicate( module, model_lvar );
                if (model_lvar_copy)
                {
                    model_lvar_copy = model->lvar_reference( module, model_lvar_copy, lex_string_from_terminal( cona->symbol ) );
                }
                if (model_lvar_copy)
                {
                    statement = cona->nested_assignment->build_model_from_statement( cyclicity, model, module, model_lvar_copy, clocked, model_lvar_context, documentation );
                }
                if (model_lvar_copy)
                {
                    model->lvar_free( module, model_lvar_copy );
                }
            }
            if (last_statement)
            {
                statement = model->statement_add_to_chain( last_statement, statement );
            }
            last_statement = statement;
        }
    }
    return statement;
}

/*f c_co_nested_assignment::build_model_reset_wildcarded
  The model_lvar_context is the toplevel lvar for the signal
  model_lvar is the parent for which this entry (and any following ones) are part of
 */
void c_co_nested_assignment::build_model_reset_wildcarded( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar, struct t_md_lvar *model_lvar_context, t_type_value nested_type_context )
{
    int i, size;
    t_md_lvar *model_lvar_copy;
    t_md_expression *model_expression;

    CO_DEBUG( sl_debug_level_info, "Build reset wildcarded with lvar %p expression (ought to be non-NULL) %p symbol (ought to be NULL) %p element (ought to be NULL) %d na %p nested_type_context %d", model_lvar, expression, symbol, element, nested_assignment, nested_type_context );
    if (!expression)
        return;

    if (cyclicity->type_value_pool->derefs_to_bit_vector( nested_type_context ))
    {
        model_expression = expression->build_model( cyclicity, model, module, model_lvar_context );
        model_expression = model->expression_cast( module, cyclicity->type_value_pool->get_size_of(nested_type_context), 0 /* not signed */, model_expression );
        model_lvar_copy = model->lvar_duplicate( module, model_lvar );
        if (model_lvar_copy && model_expression)
        {
            model->state_reset( module, model_lvar_copy, model_expression );
        }
        CO_DEBUG( sl_debug_level_info, "Built reset with model_lvar %p model_expression %p", model_lvar_copy, model_expression );
    }
    else if ( cyclicity->type_value_pool->derefs_to_structure( nested_type_context ) )
    {
        int num_elements;
        t_symbol *element_symbol;

        num_elements = cyclicity->type_value_pool->get_structure_element_number( nested_type_context );
        for (i=0; i<num_elements; i++)
        {
            element_symbol = cyclicity->type_value_pool->get_structure_element_name( nested_type_context, i );
            model_lvar_copy = model->lvar_duplicate( module, model_lvar );
            if (model_lvar_copy)
            {
                model_lvar_copy = model->lvar_reference( module, model_lvar_copy, lex_string_from_terminal(element_symbol) );
            }
            if (model_lvar_copy)
            {
                build_model_reset_wildcarded( cyclicity, model, module, model_lvar_copy, model_lvar_context, cyclicity->type_value_pool->get_structure_element_type(nested_type_context, i));
            }
            CO_DEBUG( sl_debug_level_info, "Built full type structure reset", 0 );
        }
    }
    else if (cyclicity->type_value_pool->derefs_to_array( nested_type_context ))
    {
        CO_DEBUG( sl_debug_level_info, "Building reset array model_lvar %p", model_lvar_copy );
        // We have an array. The model_lvar, of course, is never an array of structures - the arrayness is pushed to the bottom
        size = cyclicity->type_value_pool->array_size( nested_type_context );
        for (i=0; i<size; i++)
        {
            model_lvar_copy = model->lvar_duplicate( module, model_lvar );
            if (model_lvar_copy)
            {
                model_lvar_copy = model->lvar_index( module, model_lvar_copy, i );
            }
            if (model_lvar_copy)
            {
                build_model_reset_wildcarded( cyclicity, model, module, model_lvar_copy, model_lvar_context, cyclicity->type_value_pool->array_type(nested_type_context) );
            }
            if (model_lvar_copy)
            {
                model->lvar_free( module, model_lvar_copy );
            }
            CO_DEBUG( sl_debug_level_info, "Built reset array index %d model_lvar %p", i, model_lvar_copy );
        }
    }
    else // What else could it be?
    {
        fprintf(stderr,"c_co_statement::build_model_reset_wildcarded:Should not get this far - unknown type ****************************************\n");
        exit(4);
    }
}

/*f c_co_nested_assignment::build_model_reset
  The model_lvar_context is the toplevel lvar for the signal; if the signal is an array, that will be obvious there.
 */
void c_co_nested_assignment::build_model_reset( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar, struct t_md_lvar *model_lvar_context )
{
    int i, size;
    t_md_lvar *model_lvar_copy;
    t_md_expression *model_expression;
    c_co_nested_assignment *cona;

    CO_DEBUG( sl_debug_level_info, "Build reset with lvar %p expression %p symbol %p element %d na %p type_context %d", model_lvar, expression, symbol, element, nested_assignment, type_context );
    if (expression)
    {
        if (wildcard)
        {
            build_model_reset_wildcarded( cyclicity, model, module, model_lvar, model_lvar_context, type_context );
            CO_DEBUG( sl_debug_level_info, "Built reset with model_lvar %p model_expression %p", model_lvar_copy, model_expression );
        }
        else if (cyclicity->type_value_pool->derefs_to_bit_vector( type_context ))
        {
            model_expression = expression->build_model( cyclicity, model, module, model_lvar_context );
            model_lvar_copy = model->lvar_duplicate( module, model_lvar );
            if (model_lvar_copy && model_expression)
            {
                model->state_reset( module, model_lvar_copy, model_expression );
             }
            CO_DEBUG( sl_debug_level_info, "Built reset with model_lvar %p model_expression %p", model_lvar_copy, model_expression );
        }
        else
        {
            fprintf(stderr,"c_co_statement::build_model:Should not get this far - reset expression expected a leaf lvar but got a structure or array - error at cross referencing ****************************************\n");
            exit(4);
        }
    }
    else if (cyclicity->type_value_pool->derefs_to_array( type_context ))
    {
        CO_DEBUG( sl_debug_level_info, "Building reset array model_lvar %p", model_lvar_copy );
        // We have an array. The model_lvar, of course, is never an array of structures - the arrayness is pushed to the bottom
        size = cyclicity->type_value_pool->array_size( type_context );
        for (i=0; i<size; i++)
        {
            model_lvar_copy = model->lvar_duplicate( module, model_lvar );
            if (model_lvar_copy)
            {
                model_lvar_copy = model->lvar_index( module, model_lvar_copy, i );
            }
            if (model_lvar_copy)
            {
                this->nested_assignment->build_model_reset( cyclicity, model, module, model_lvar_copy, model_lvar_context );
            }
            if (model_lvar_copy)
            {
                model->lvar_free( module, model_lvar_copy );
            }
            CO_DEBUG( sl_debug_level_info, "Built reset array index %d model_lvar %p", i, model_lvar_copy );
        }
    }
    else if ( cyclicity->type_value_pool->derefs_to_structure( type_context ) )
    {
        if (element>=0)
        {
            model_lvar_copy = model->lvar_duplicate( module, model_lvar );
            if (model_lvar_copy)
            {
                model_lvar_copy = model->lvar_reference( module, model_lvar_copy, lex_string_from_terminal( symbol ) );
            }
            if (model_lvar_copy)
            {
                nested_assignment->build_model_reset( cyclicity, model, module, model_lvar_copy, model_lvar_context );
            }
            if (model_lvar_copy)
            {
                model->lvar_free( module, model_lvar_copy );
            }
        }
    }
    if (next_in_list)
    {
        ((c_co_nested_assignment *)next_in_list)->build_model_reset( cyclicity, model, module, model_lvar, model_lvar_context );
    }
}

/*a c_co_case_entry
 */
/*f c_co_case_entry::build_model
 */
struct t_md_switch_item *c_co_case_entry::build_model( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module )
{
    t_md_switch_item *model_switem, *following_switems;
    t_md_statement *model_statements;
    t_md_expression *model_expression;

    model_statements = NULL;
    model_switem = NULL;
    if (statements)
    {
        model_statements = statements->build_model( cyclicity, model, module );
    }
    else
    {
        c_co_case_entry *coce;
        coce = (c_co_case_entry *)next_in_list;
        //fprintf(stderr,"Looking for case entry with statements at %p\n",coce);
        while ( (coce) &&
                (!coce->statements) )
        {
            coce = (c_co_case_entry *)(coce->next_in_list);
            //fprintf(stderr,"Looking for case entry with statements at %p\n",coce);
        }
        //fprintf(stderr,"Found case entry with statements at %p\n",coce);
        if (coce)
        {
            model_statements = coce->statements->build_model( cyclicity, model, module );
        }
        else // Case statement with no statements in it - 
        {                                                 
            model_statements = NULL;
        }
    }
    if (!expression) // default
    {
        model_switem = model->switch_item_create( module, (void *)module, (void *)this, 0, model_statements );  // default
    }
    else
    {
        t_md_switch_item *new_switem;
        model_switem = NULL;
        c_co_expression *expr;
        for (expr=expression; expr; expr = (c_co_expression *)(expr->next_in_list) )
        {
            //printf("Case statement expression %p\n",expr);
            new_switem = NULL;
            model_expression = NULL;
            if (cyclicity->type_value_pool->check_resolved(expr->type_value))
            {
                if (get_sized_value_from_type_value( cyclicity, 0, expr->type_value))
                {
                    //printf("constant %d\n",tmp_signal_value[0].value[0]);
                    new_switem = model->switch_item_create( module, (void *)module, (void *)this, 0, &tmp_signal_value[0], model_statements ); // static expression
                }
            }
            else
            {
                model_expression = expr->build_model( cyclicity, model, module, NULL );
                //printf("dynamic %p\n",model_expression);
                if (model_expression)
                {
                    new_switem = model->switch_item_create( module, (void *)module, (void *)this, 0, model_expression, model_statements ); // dynamic expression
                }
            }
            if (model_switem)
            {
                model_switem = model->switch_item_chain( model_switem, new_switem );
            }
            else
            {
                model_switem = new_switem;
            }
        }             
    }
//        model->switch_item_create( module, (void *)module, (void *)this, 0, signal_value, signal_value, statement ); // static expression with mask
    CO_DEBUG( sl_debug_level_info, "created switem %p", model_switem );

    if (next_in_list)
    {
        following_switems = ((c_co_case_entry *)next_in_list)->build_model( cyclicity, model, module );
        if (following_switems && model_switem)
        {
            model_switem = model->switch_item_chain( model_switem, following_switems );
        }
    }
    return model_switem;
}

/*a c_co_port_map
 */
/*t t_build_model_port_map_input_callback_handle
 */
typedef struct
{
    const char *module_name;
} t_build_model_port_map_input_callback_handle;

/*f build_model_port_map_input_callback
 */
static void build_model_port_map_input_callback( class c_cyclicity *cyclicity,
                                                 class c_model_descriptor *model,
                                                 struct t_md_module *module,
                                                 void *handle,
                                                 void *model_port_lvar_in,
                                                 class c_co_expression *expression,
                                                 struct t_md_lvar *model_expression_lvar,
                                                 struct t_md_lvar *model_lvar_context )
{
    struct t_md_expression *model_expression;
    struct t_md_lvar *model_expression_lvar_copy;
    struct t_md_port_lvar *model_port_lvar_copy;
    struct t_md_port_lvar *model_port_lvar;

    t_build_model_port_map_input_callback_handle *bmpmich;
    bmpmich = (t_build_model_port_map_input_callback_handle *)handle;
    model_port_lvar = (t_md_port_lvar *)model_port_lvar_in;

    if (expression) // If at top level of an assignment, we have not got a (sub-elemented) model lvar from the expression
    {
        model_expression = expression->build_model( cyclicity, model, module, model_lvar_context ); // model_lvar_context is the context on the 'lhs' of the assignment
    }
    else // we are nested in an implied assignment with the current (sub-elemented) model lvar valid; we should copy it and use it
    {
        model_expression_lvar_copy = model->lvar_duplicate( module, model_expression_lvar );
        model_expression = model->expression( module, model_expression_lvar_copy );
    }
    model_port_lvar_copy = model->port_lvar_duplicate( module, model_port_lvar );
    if (model_port_lvar_copy && model_expression)
    {
        model->module_instance_add_input( module, bmpmich->module_name, model_port_lvar_copy, model_expression );
    }
    SL_DEBUG( sl_debug_level_info, "Added assignment with port lvar %p expression %p", model_port_lvar_copy, model_expression );
}

/*t t_build_model_port_map_output_callback_handle
 */
typedef struct
{
    const char *module_name;
} t_build_model_port_map_output_callback_handle;

/*f build_model_port_map_output_callback
  The model_expression_lvar (the net to drive) and model_port_lvar_in (the port on the module instance) are the same base type (bit vector of length 'n')
  But the lvar can be a bit_vector with an index rather than a bit range, if it were part of a structure which has not been pushed down e.g. fred[0] where fred[0] is a structure with elements bit a and bit b
  Fortunately the backend can cope with this
 */
static void build_model_port_map_output_callback( class c_cyclicity *cyclicity,
                                                 class c_model_descriptor *model,
                                                 struct t_md_module *module,
                                                 void *handle,
                                                 void *model_port_lvar_in,
                                                 class c_co_expression *expression,
                                                 struct t_md_lvar *model_expression_lvar,
                                                 struct t_md_lvar *model_lvar_context )
{
    struct t_md_lvar *model_expression_lvar_copy;
    struct t_md_port_lvar *model_port_lvar_copy;
    struct t_md_port_lvar *model_port_lvar;

    t_build_model_port_map_output_callback_handle *bmpmoch;
    bmpmoch = (t_build_model_port_map_output_callback_handle *)handle;
    model_port_lvar = (t_md_port_lvar *)model_port_lvar_in;

    model_expression_lvar_copy = model->lvar_duplicate( module, model_expression_lvar );
    model_port_lvar_copy = model->port_lvar_duplicate( module, model_port_lvar );
    if (model_port_lvar_copy && model_expression_lvar_copy)
    {
        model->module_instance_add_output( module, bmpmoch->module_name, model_port_lvar_copy, model_expression_lvar_copy );
    }
    SL_DEBUG( sl_debug_level_info, "Added assignment with port lvar %p lvar %p", model_port_lvar_copy, model_expression_lvar_copy );
}

/*f c_co_port_map::build_model
  Interacts with the backend to build a model of a clock, input or output assignment in a module instantiation (and any chained afterwards)
 */
void c_co_port_map::build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, const char *module_name )
{
    t_md_port_lvar *model_port_lvar;
    t_md_lvar *model_lvar;

    // Build port mapping using port_lvar, expressions and lvars
    CO_DEBUG( sl_debug_level_info, "Building port map %p for module %s", this, module_name );
    switch (type)
    {
    case port_map_type_clock:
        model->module_instance_add_clock( module, module_name, lex_string_from_terminal( port_id ), lex_string_from_terminal( clock_id ) );
        break;
    case port_map_type_input:
        if (expression && port_lvar)
        {
            model_port_lvar = port_lvar->build_model_port( cyclicity, model, module );
            if (model_port_lvar)
            {
                t_build_model_port_map_input_callback_handle handle;
                handle.module_name = module_name;
                traverse_structure( cyclicity, model, module, (void *) &handle, build_model_port_map_input_callback, 1, (void *)model_port_lvar, port_lvar->type, this->expression, NULL, NULL );
            }
        }
        break;
    case port_map_type_output:
        if (lvar && port_lvar)
        {
            model_port_lvar = port_lvar->build_model_port( cyclicity, model, module );
            model_lvar = lvar->build_model( cyclicity, model, module, NULL );
            if (model_port_lvar && model_lvar)
            {
                t_build_model_port_map_output_callback_handle handle;
                handle.module_name = module_name;
                traverse_structure( cyclicity, model, module, (void *) &handle, build_model_port_map_output_callback, 1, (void *)model_port_lvar, port_lvar->type, NULL, model_lvar, NULL );
            }
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_port_map *)next_in_list)->build_model( cyclicity, model, module, module_name );
    }
}

/*a c_co_statement
 */
/*f c_co_statement::build_model
 */
struct t_md_statement *c_co_statement::build_model( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module )
{
    t_md_statement *statement; // Just this statement
    t_md_statement *following_statements; // All succeeding statements

    /*b Build the current statement
     */
    statement = NULL;
    switch (statement_type)
    {
    /*b Assignment statement
     */
    case statement_type_assignment:
    {
        c_co_lvar *lvar;
        t_md_lvar *model_lvar;

        lvar = type_data.assign_stmt.lvar;
        model_lvar = lvar->build_model( cyclicity, model, module, NULL );
        if (!model_lvar)
        {
            fprintf(stderr,"Failed to build model_lvar for assignment %p\n",this);
        }
        statement = type_data.assign_stmt.nested_assignment->build_model_from_statement( cyclicity, model, module, model_lvar, type_data.assign_stmt.clocked, model_lvar, DOC_STRING(type_data.assign_stmt.documentation) );
        model->lvar_free( module, model_lvar );
        break;
    }
    /*b If statement
     */
    case statement_type_if_statement:
    {
        t_md_statement *if_true, *if_false;
        t_md_expression *expression;
        expression = type_data.if_stmt.expression->build_model( cyclicity, model, module, NULL );
        if_true = NULL;
        if_false = NULL;
        if (type_data.if_stmt.if_true)
        {
            if_true = type_data.if_stmt.if_true->build_model( cyclicity, model, module );
        }
        if (type_data.if_stmt.if_false)
        {
            if_false = type_data.if_stmt.if_false->build_model( cyclicity, model, module );
        }
        if (type_data.if_stmt.elsif)
        {
            // We can set if_false, as the backend does not have the distinction
            if_false = type_data.if_stmt.elsif->build_model( cyclicity, model, module );
        }
        CO_DEBUG( sl_debug_level_info, "Output if statement expr %p if_true %p if_false %p", expression, if_true, if_false );
        if (expression && if_true)
        {
            statement = model->statement_create_if_else( module, (void *)module, (void *)this, 0, expression, if_true, if_false, DOC_STRING(type_data.if_stmt.expr_documentation) );
        }
        break;
    }
    /*b For statement
     */
    case statement_type_for_statement:
    {
        // At this point the first_value and iterations should have been evaluated properly
        if ( type_data.for_stmt.first_value && 
             type_data.for_stmt.last_value &&
             type_data.for_stmt.next_value )
        {
            int i, max;
            t_md_statement *last_statement;
            last_statement = NULL;
            if ( cyclicity->type_value_pool->check_resolved(type_data.for_stmt.first_value->type_value) &&
                 cyclicity->type_value_pool->check_resolved(type_data.for_stmt.iterations->type_value) )
            {
                CO_DEBUG( sl_debug_level_info, "Read maximum iterations for %d", lex_string_from_terminal( type_data.for_stmt.symbol ) );
                max = cyclicity->type_value_pool->integer_value(type_data.for_stmt.iterations->type_value);
                type_data.for_stmt.type_value = type_data.for_stmt.first_value->type_value;
                i = 0;
                for (; i<max+1; i++)
                {
                    CO_DEBUG( sl_debug_level_info, "Evaluate last iteration for %s", lex_string_from_terminal( type_data.for_stmt.symbol ) );
                    if (type_data.for_stmt.last_value->evaluate_constant( cyclicity, cyclicity->type_definitions, type_data.for_stmt.scope, 1))
                    {
                        if (!cyclicity->type_value_pool->logical_value(type_data.for_stmt.last_value->type_value))
                        {
                            break;
                        }
                    }
                    else
                    {
                        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Next value fails to evaluate in for statement" );
                    }
                    CO_DEBUG( sl_debug_level_info,
                              "For %s value %d iteration %d", lex_string_from_terminal( type_data.for_stmt.symbol ),
                              cyclicity->type_value_pool->integer_value(type_data.for_stmt.type_value),
                              i );
                    if (i==max)
                    {
                        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "For statement reached maximum iteration count without completion" );
                        break;
                    }
                    if (type_data.for_stmt.statement)
                    {
                        type_data.for_stmt.statement->evaluate_constant_expressions( cyclicity, cyclicity->type_definitions, type_data.for_stmt.scope, 1 );
                        statement = type_data.for_stmt.statement->build_model( cyclicity, model, module );
                        if (last_statement)
                        {
                            statement = model->statement_add_to_chain( last_statement, statement );
                        }
                        last_statement = statement;
                    }
                    if (!type_data.for_stmt.next_value->evaluate_constant( cyclicity, cyclicity->type_definitions, type_data.for_stmt.scope, 1))
                    {
                        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Next value fails to evaluate in for statement" );
                        break;
                    }
                    type_data.for_stmt.type_value = type_data.for_stmt.next_value->type_value;
               }
            }
        }
        else
        {
            int i, max;
            t_md_statement *last_statement;
            last_statement = NULL;
            max = cyclicity->type_value_pool->integer_value(type_data.for_stmt.iterations->type_value);
            for (i=0; i<max; i++)
            {
                CO_DEBUG( sl_debug_level_info, "For %s value %d", lex_string_from_terminal( type_data.for_stmt.symbol ), i );
                type_data.for_stmt.type_value = cyclicity->type_value_pool->new_type_value_integer( i );
                if (type_data.for_stmt.statement)
                {
                    type_data.for_stmt.statement->evaluate_constant_expressions( cyclicity, cyclicity->type_definitions, type_data.for_stmt.scope, 1 );
                    statement = type_data.for_stmt.statement->build_model( cyclicity, model, module );
                    if (last_statement)
                    {
                        statement = model->statement_add_to_chain( last_statement, statement );
                    }
                    last_statement = statement;
                }
            }
        }
        break;
    }
    /*b Switch statement
     */
    case statement_type_full_switch_statement:
    case statement_type_partial_switch_statement:
    case statement_type_priority_statement:
    {
        t_md_switch_item *md_switch_items;
        t_md_expression *md_expression;
        int parallel, full;

        md_expression = type_data.switch_stmt.expression->build_model( cyclicity, model, module, NULL );
        md_switch_items = type_data.switch_stmt.cases->build_model( cyclicity, model, module );

        if (md_switch_items && md_expression)
        {
            if (statement_type==statement_type_full_switch_statement)
            {
                parallel = 1;
                full = 1;
            }
            if (statement_type==statement_type_partial_switch_statement)
            {
                parallel = 1;
                full = 0;
            }
            if (statement_type==statement_type_priority_statement)
            {
                parallel = 0;
                full = 0;
            }
            CO_DEBUG( sl_debug_level_info, "Output switch statement expression %p cases %p", md_expression, md_switch_items );
            statement = model->statement_create_static_switch( module, (void *)module, (void *)this, 0, parallel, full, md_expression, md_switch_items, DOC_STRING(type_data.switch_stmt.expr_documentation) );
        }
        break;
    }
    /*b Block of statements
     */
    case statement_type_block:
    {
        if (type_data.block)
        {
            statement = type_data.block->build_model( cyclicity, model, module );
            CO_DEBUG( sl_debug_level_info, "c_co_statement::build_model:statement type block - will chain incorrectly", 0);
        }
        break;
    }
    /*b Print/assert/cover statement
     */
    case statement_type_print:
    case statement_type_assert:
    case statement_type_cover:
        {
            t_md_message *md_message;
            t_md_statement *md_substatement;
            t_md_expression *md_expression;
            t_md_expression *md_expr_values;

            md_substatement = NULL;
            md_message = NULL;
            md_expression = NULL;
            md_expr_values = NULL;
            if (type_data.print_assert_stmt.text)
            {
                md_message = model->message_create( module, (void *)module, (void *)this, 0, lex_string_from_terminal(type_data.print_assert_stmt.text), 1 );
            }
            if (md_message && type_data.print_assert_stmt.text_args)
            {
                c_co_expression *expr;
                t_md_expression *md_expression;
                for (expr = type_data.print_assert_stmt.text_args; expr; expr=(c_co_expression *)(expr->next_in_list))
                {
                    md_expression = expr->build_model( cyclicity, model, module, NULL );
                    if (md_expression)
                    {
                        md_message = model->message_add_argument( md_message, md_expression );
                    }
                }
            }
            if (type_data.print_assert_stmt.statement)
            {
                md_substatement = type_data.print_assert_stmt.statement->build_model( cyclicity, model, module );
            }
            if (type_data.print_assert_stmt.assertion) // If there is an expression it is an assertion (for now)
            {
                md_expression = type_data.print_assert_stmt.assertion->build_model( cyclicity, model, module, NULL );
                if (type_data.print_assert_stmt.value_list)
                {
                    c_co_expression *expr;
                    t_md_expression *md_expr_value;
                    for (expr = type_data.print_assert_stmt.value_list; expr; expr=(c_co_expression *)(expr->next_in_list))
                    {
                        md_expr_value = expr->build_model( cyclicity, model, module, NULL );
                        md_expr_values = model->expression_chain( md_expr_values, md_expr_value );
                    }
                }
            }
            statement = model->statement_create_assert_cover( module,
                                                              (void *)module,
                                                              (void *)this,
                                                              0,
                                                              type_data.print_assert_stmt.clock_spec ? lex_string_from_terminal( type_data.print_assert_stmt.clock_spec->symbol ):NULL, 
                                                              type_data.print_assert_stmt.clock_spec ? (type_data.print_assert_stmt.clock_spec->clock_edge==clock_edge_falling):0,
                                                              (statement_type==statement_type_assert)?md_usage_type_assert:(statement_type==statement_type_cover)?md_usage_type_cover:md_usage_type_rtl,
                                                              md_expression,
                                                              md_expr_values,
                                                              md_message,
                                                              md_substatement );
        }
        break;
    /*b Log statement
     */
    case statement_type_log:
        {
            class c_co_named_expression *arg;
            t_md_labelled_expression *md_arg_list;
            md_arg_list = NULL;
            for (arg = type_data.log_stmt.values; arg; arg=(c_co_named_expression *)(arg->next_in_list))
            {
                t_md_expression *md_expr;
                t_md_labelled_expression *md_arg;
                if (arg->expression)
                {
                    md_expr = arg->expression->build_model( cyclicity, model, module, NULL );
                    md_arg = model->labelled_expression_create( module, 
                                                                (void *)module,
                                                                (void *)this,
                                                                0,
                                                                lex_string_from_terminal(arg->name),
                                                                1,
                                                                md_expr );
                    model->labelled_expression_append( &md_arg_list, md_arg );
                }
            }
            statement = model->statement_create_log( module,
                                                     (void *)module,
                                                     (void *)this,
                                                     0,
                                                     type_data.log_stmt.clock_spec ? lex_string_from_terminal( type_data.log_stmt.clock_spec->symbol ):NULL, 
                                                     type_data.log_stmt.clock_spec ? (type_data.log_stmt.clock_spec->clock_edge==clock_edge_falling):0,
                                                     lex_string_from_terminal(type_data.log_stmt.message),
                                                     md_arg_list );
        }
        break;
    /*b statement_type_instantiation
     */
        case statement_type_instantiation:
        {
            const char *module_name;
            char *indexed_name;
            indexed_name = NULL;
            module_name = lex_string_from_terminal(type_data.instantiation->instance_id);
            // What of index?
            if (type_data.instantiation->index)
            {
                CO_DEBUG( sl_debug_level_info,
                          "Instance name %s index %d",
                          module_name,
                          cyclicity->type_value_pool->integer_value(type_data.instantiation->index->type_value) );
                if ( (cyclicity->type_value_pool->check_resolved(type_data.instantiation->index->type_value)) &&
                     (get_sized_value_from_type_value( cyclicity, 0, type_data.instantiation->index->type_value )) )
                {
                    indexed_name = (char *)malloc(strlen(module_name)+20);
                    sprintf( indexed_name, "%s___%lld", module_name, tmp_signal_value[0].value[0] );
                    module_name = indexed_name;
                }
                else
                {
                    cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Build model: index not constant for instance" );
                }
            }
            // Build instance
            if (model->module_instance( module, lex_string_from_terminal(type_data.instantiation->instance_of_id), 1, module_name, 1, (void *)module, (void *)this, 0 ))
            {
                if (type_data.instantiation->port_map)
                {
                    type_data.instantiation->port_map->build_model( cyclicity, model, module, module_name );
                }
            }
            if (indexed_name)
            {
                free(indexed_name);
            }
            break;
        }
    /*b All done
     */
    }

    /*b Build following statements
     */
    following_statements = NULL;
    if (next_in_list)
    {
        following_statements = ((c_co_statement *)next_in_list)->build_model( cyclicity, model, module );
    }

    /*b Add statements to model
     */
    CO_DEBUG( sl_debug_level_info, "Statement %p, following_statements %p", statement, following_statements );
    if (statement)
    {
        if (following_statements)
        {
            return model->statement_add_to_chain( statement, following_statements );
        }
        return statement;
    }
    return following_statements;
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

