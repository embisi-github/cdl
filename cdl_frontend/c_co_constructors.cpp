/*a Copyright
  
  This file 'c_co_constructors.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_co_module.h"
#include "c_co_module_body_entry.h"
#include "c_co_module_prototype.h"
#include "c_co_module_prototype_body_entry.h"
#include "c_co_named_expression.h"
#include "c_co_nested_assignment.h"
#include "c_co_port_map.h"
#include "c_co_schematic_symbol.h"
#include "c_co_schematic_symbol_body_entry.h"
#include "c_co_signal_declaration.h"
#include "c_co_sized_int_pair.h"
#include "c_co_statement.h"
#include "c_co_timing_spec.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"

/*a c_co_case_entry
 */
/*f c_co_case_entry::c_co_case_entry( expression, flow_through, statement )
 */
c_co_case_entry::c_co_case_entry( c_co_expression *expression, int flow_through, c_co_statement *statements )
{
    this->expression = expression;
    this->statements = statements;
    this->type = type_value_undefined;
    this->flow_through = flow_through;

    co_init(co_type_case_entry,"case_entry" );
    co_link(co_compile_stage_parse, (class c_cyc_object *)expression, "expression" );
    co_link(co_compile_stage_parse, (class c_cyc_object *)statements, "statements" );
}

/*f c_co_case_entry::~c_co_case_entry
 */
c_co_case_entry::~c_co_case_entry()
{
}

/*a c_co_clock_reset_defn
 */
/*f c_co_clock_reset_defn::c_co_clock_reset_defn( clock_reset_defn_typem, clock_symbol )
 */
c_co_clock_reset_defn::c_co_clock_reset_defn( t_clock_reset_defn_type clock_reset_defn_type, t_symbol *clock_symbol, t_clock_edge clock_edge )
{
    this->clock_reset_defn_type = clock_reset_defn_type;
    this->symbol = clock_symbol;
    this->clock_edge = clock_edge;

    co_init(co_type_clock_reset_defn,"co_type_clock_reset_defn( clock )");
    co_link(co_compile_stage_parse, clock_symbol, "clock_symbol" );

    this->signal_declaration = NULL;
}

/*f c_co_clock_reset_defn::c_co_clock_reset_defn( clock_reset_defn_typem, reset_symbol, reset_active )
 */
c_co_clock_reset_defn::c_co_clock_reset_defn( t_clock_reset_defn_type clock_reset_defn_type, t_symbol *reset_symbol, t_reset_active reset_active )
{
    this->clock_reset_defn_type = clock_reset_defn_type;
    this->symbol = reset_symbol;
    this->reset_active = reset_active;

    co_init(co_type_clock_reset_defn,"co_type_clock_reset_defn( reset )");
    co_link(co_compile_stage_parse, reset_symbol, "reset_symbol" );

    this->signal_declaration = NULL;
}

/*f c_co_clock_reset_defn::~c_co_clock_reset_defn
 */
c_co_clock_reset_defn::~c_co_clock_reset_defn()
{
}

/*a c_co_code_label
 */
/*f c_co_code_label::c_co_code_label( label, documentation, statements )
 */
c_co_code_label::c_co_code_label( t_symbol *label, t_string *documentation, c_co_statement *statements )
{
    this->label = label;
    this->documentation = documentation;
    this->statements = statements;

    co_init(co_type_code_label,"co_type_code_label()" );
    co_link(co_compile_stage_parse, label, "label" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
    co_link(co_compile_stage_parse, statements, "statements" );
}

/*f c_co_code_label::~c_co_code_label
 */
c_co_code_label::~c_co_code_label()
{
}

/*a c_co_constant_declaration
 */
/*f c_co_constant_declaration::c_co_constant_declaration
 */
c_co_constant_declaration::c_co_constant_declaration( t_symbol *symbol, class c_co_type_specifier *type_specifier, class c_co_expression *expression, t_string *documentation )
{
    this->symbol = symbol;
    this->documentation = documentation;
    this->type_specifier = type_specifier;
    this->expression = expression;
    this->overridden = 0;

    co_init(co_type_constant_declaration,"constant_declaration()");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object*)type_specifier, "type_specifier" );
    co_link(co_compile_stage_parse, (c_cyc_object*)expression, "expression" );
    co_link(co_compile_stage_parse, documentation, "documentation" );

    this->type_value = type_value_undefined;
}

/*f c_co_constant_declaration::~c_co_constant_declaration
 */
c_co_constant_declaration::~c_co_constant_declaration()
{
}

/*a c_co_enum_definition
 */
/*f c_co_enum_definition::c_co_enum_definition( symbol, sized_int )
 */
c_co_enum_definition::c_co_enum_definition( class c_co_enum_identifier *enum_identifier, class c_co_expression *first_index, class c_co_expression *second_index )
{
    this->enum_identifier = enum_identifier;
    this->first_index = first_index; // Can be NULL
    this->second_index = second_index; // Can be NULL even if first_index is non-NULL

    co_init(co_type_enum_definition,"enum_definition()");
    co_link(co_compile_stage_parse, (class c_cyc_object *)enum_identifier, "enum_identifier" );
    co_link(co_compile_stage_parse, (class c_cyc_object *)first_index, "first_index" );
    co_link(co_compile_stage_parse, (class c_cyc_object *)second_index, "second_index" );
}

/*f c_co_enum_definition::~c_co_enum_definition
 */
c_co_enum_definition::~c_co_enum_definition()
{
}

/*a c_co_enum_identifier
 */
/*f c_co_enum_identifier::c_co_enum_identifier( symbol, expression )
 */
c_co_enum_identifier::c_co_enum_identifier( t_symbol *symbol, c_co_expression *expression )
{
    this->symbol = symbol;
    this->expression = expression;

    co_init(co_type_enum_identifier,"enum_identifier()");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object *)expression, "expression" );
}

/*f c_co_enum_identifier::~c_co_enum_identifier
 */
c_co_enum_identifier::~c_co_enum_identifier()
{
}

/*a c_co_expression
 */
/*f c_co_expression::c_co_expression( type_value )
 */
c_co_expression::c_co_expression( t_sized_int *sized_int )
{
    this->expr_type = expr_type_sized_int;
    this->sized_int = sized_int;
    this->first = NULL;
    this->second = NULL;
    this->third = NULL;

    co_init(co_type_expression,"expression( type_value )");
    co_link(co_compile_stage_parse, sized_int, "type_value" );

    this->type_value = type_value_undefined;
    this->type = type_value_undefined;
    this->implicit_cast = expr_cast_none;
}

/*f c_co_expression::c_co_expression( lvar )
 */
c_co_expression::c_co_expression( c_co_lvar *lvar )
{
    this->expr_type = expr_type_lvar;
    this->sized_int = NULL;
    this->lvar = lvar;
    this->first = NULL;
    this->second = NULL;
    this->third = NULL;

    co_init(co_type_expression,"expression( lvar ) ");
    co_link(co_compile_stage_parse, lvar, "lvar" );

    this->type_value = type_value_undefined;
    this->type = type_value_undefined;
    this->implicit_cast = expr_cast_none;
}

/*f c_co_expression::c_co_expression( expr_subtype, expression, expression, expression )
 */
c_co_expression::c_co_expression( t_expr_subtype expr_subtype, c_co_expression *left, c_co_expression *mid, c_co_expression *right )
{
    this->expr_type = expr_type_ternary_function;
    this->sized_int = NULL;
    this->expr_subtype = expr_subtype;
    this->first = left;
    this->second = mid;
    this->third = right;

    co_init(co_type_expression,"expression( ternary )");
    co_link(co_compile_stage_parse, (c_cyc_object*)left, "left" );
    co_link(co_compile_stage_parse, (c_cyc_object*)mid, "mid" );
    co_link(co_compile_stage_parse, (c_cyc_object*)right, "right" );

    this->type_value = type_value_undefined;
    this->type = type_value_undefined;
    this->implicit_cast = expr_cast_none;
}

/*f c_co_expression::c_co_expression( expr_subtype, expression, expression )
 */
c_co_expression::c_co_expression( t_expr_subtype expr_subtype, c_co_expression *left, c_co_expression *right )
{
    this->expr_type = expr_type_binary_function;
    this->sized_int = NULL;
    this->expr_subtype = expr_subtype;
    this->first = left;
    this->second = right;
    this->third = NULL;

    co_init(co_type_expression,"expression( binary )");
    co_link(co_compile_stage_parse, (c_cyc_object*)left, "left" );
    co_link(co_compile_stage_parse, (c_cyc_object*)right, "right" );

    this->type_value = type_value_undefined;
    this->type = type_value_undefined;
    this->implicit_cast = expr_cast_none;
}

/*f c_co_expression::c_co_expression( expr_subtype, expression )
 */
c_co_expression::c_co_expression( t_expr_subtype expr_subtype, c_co_expression *arg )
{
    this->expr_type = expr_type_unary_function;
    this->sized_int = NULL;
    this->expr_subtype = expr_subtype;
    this->type_value = type_value_undefined;
    this->first = arg;
    this->second = NULL;
    this->third = NULL;

    co_init(co_type_expression,"expression( unary )");
    co_link(co_compile_stage_parse, (c_cyc_object*)arg, "left" );

    this->type_value = type_value_undefined;
    this->type = type_value_undefined;
    this->implicit_cast = expr_cast_none;
}

/*f c_co_expression::c_co_expression( expr_type, expr_subtype, expression )
 */
c_co_expression::c_co_expression( t_expr_type expr_type, t_expr_subtype expr_subtype, c_co_expression *arg )
{
    this->expr_type = expr_type;
    this->sized_int = NULL;
    this->expr_subtype = expr_subtype;
    this->type_value = type_value_undefined;
    this->first = arg;
    this->second = NULL;
    this->third = NULL;

    co_init(co_type_expression,"expression( specified )");
    co_link(co_compile_stage_parse, (c_cyc_object*)arg, "left" );

    this->type_value = type_value_undefined;
    this->type = type_value_undefined;
    this->implicit_cast = expr_cast_none;
}

/*f c_co_expression::~c_co_expression
 */
c_co_expression::~c_co_expression()
{
}

/*a c_co_fsm_state
 */
/*f c_co_fsm_state::c_co_fsm_state( symbol, token_union, token_union, documentation )
 */
c_co_fsm_state::c_co_fsm_state( t_symbol *name, class c_co_expression *expression, class c_co_token_union *preceding_states, class c_co_token_union *succeeding_states, t_string *optional_documentation )
{
    this->symbol = name;
    this->fsm_style = fsm_style_plain;
    this->expression = expression;
    this->preceding_states = preceding_states;
    this->succeeding_states = succeeding_states;
    this->documentation = optional_documentation;

    co_init(co_type_fsm_state,"fsm_state()");
    co_link(co_compile_stage_parse, name, "name" );
    co_link(co_compile_stage_parse, (c_cyc_object*)expression, "expression" );
    co_link(co_compile_stage_parse, (c_cyc_object*)preceding_states, "preceding_states" );
    co_link(co_compile_stage_parse, (c_cyc_object*)succeeding_states, "succeeding_states" );
    co_link(co_compile_stage_parse, optional_documentation, "documentation" );

    this->type_value = type_value_undefined;
}

/*f c_co_fsm_state::~c_co_fsm_state
 */
c_co_fsm_state::~c_co_fsm_state()
{
}

/*f c_co_fsm_state::set_style
 */
void c_co_fsm_state::set_style( t_fsm_style fsm_style )
{
    this->fsm_style = fsm_style;
}

/*a c_co_instantiation
 */
/*f c_co_instantiation::c_co_instantiation( symbol )
 */
c_co_instantiation::c_co_instantiation( t_symbol *instance_of_id, t_symbol *instance_id, class c_co_port_map *port_map, c_co_expression *index )
{
    this->port_map = port_map;
    this->instance_of_id = instance_of_id;
    this->instance_id = instance_id;
    this->index = index;

    co_init(co_type_instantiation,"instantiation(module instantiation)");
    co_link(co_compile_stage_parse, instance_of_id, "instance_of_id" );
    co_link(co_compile_stage_parse, instance_id, "instance_id" );
    co_link(co_compile_stage_parse, (class c_cyc_object *)port_map, "port_map" );
    co_link(co_compile_stage_parse, (class c_cyc_object *)index, "index" );
}

/*f c_co_instantiation::~c_co_instantiation
 */
c_co_instantiation::~c_co_instantiation()
{
}

/*a c_co_lvar
 */
/*f c_co_lvar::c_co_lvar( symbol )
 */
c_co_lvar::c_co_lvar( t_symbol *symbol )
{
    this->symbol = symbol;
    this->lvar = NULL;
    this->first = NULL;
    this->length = NULL;

    this->signal_declaration = NULL;
    this->non_signal_declaration.cyc_object = NULL;
    this->type = type_value_undefined;

    co_init(co_type_lvar,"lvar( symbol )" );
    co_link(co_compile_stage_parse, symbol, "symbol" );
}

/*f c_co_lvar::c_co_lvar( lvar, expression, expression )
 */
c_co_lvar::c_co_lvar( c_co_lvar *lvar, c_co_expression *first, c_co_expression *length )
{
    this->symbol = NULL;
    this->lvar = lvar;
    this->first = first;
    this->length = length;

    this->signal_declaration = NULL;
    this->non_signal_declaration.cyc_object = NULL;
    this->type = type_value_undefined;

    co_init(co_type_lvar,"lvar( lvar, expression, expression )" );
    co_link(co_compile_stage_parse, (c_cyc_object*)lvar, "lvar" );
    co_link(co_compile_stage_parse, (c_cyc_object*)first, "first" );
    co_link(co_compile_stage_parse, (c_cyc_object*)length, "length" );
}

/*f c_co_lvar::c_co_lvar( lvar, symbol )
 */
c_co_lvar::c_co_lvar( c_co_lvar *lvar, t_symbol *symbol )
{
    this->symbol = symbol;
    this->lvar = lvar;
    this->first = NULL;
    this->length = NULL;

    this->signal_declaration = NULL;
    this->non_signal_declaration.cyc_object = NULL;
    this->type = type_value_undefined;

    co_init(co_type_lvar,"lvar( lvar, symbol )" );
    co_link(co_compile_stage_parse, (c_cyc_object*)lvar, "lvar" );
    co_link(co_compile_stage_parse, symbol, "symbol" );
}

/*f c_co_lvar::~c_co_lvar
 */
c_co_lvar::~c_co_lvar()
{
}

/*a c_co_module_body_entry
 */
c_co_module_body_entry::c_co_module_body_entry( int clock_not_reset, class c_co_clock_reset_defn *clock_reset_defn )
{
    if (clock_not_reset)
    {
        this->type = module_body_entry_type_default_clock_defn;
        this->data.default_clock_defn = clock_reset_defn;
        co_init(co_type_module_body_entry,"module_body_entry( clock )" );
    }
    else
    {
        this->type = module_body_entry_type_default_reset_defn;
        this->data.default_reset_defn = clock_reset_defn;
        co_init(co_type_module_body_entry,"module_body_entry( reset )" );
    }

    co_link(co_compile_stage_parse, (c_cyc_object*)clock_reset_defn, "clock_reset_defn" );
}
c_co_module_body_entry::c_co_module_body_entry( class c_co_signal_declaration *signal_declaration )
{
    this->type = module_body_entry_type_signal_declaration;
    this->data.signal_declaration = signal_declaration;

    co_init(co_type_module_body_entry,"module_body_entry( signal_declaration )" );
    co_link(co_compile_stage_parse, (c_cyc_object*)signal_declaration, "signal_declaration" );
}
c_co_module_body_entry::c_co_module_body_entry( class c_co_code_label *code_label )
{
    this->type = module_body_entry_type_code_label;
    this->data.code_label = code_label;

    co_init(co_type_module_body_entry,"module_body_entry( code_label )" );
    co_link(co_compile_stage_parse, (c_cyc_object*)code_label, "code_label" );
}
c_co_module_body_entry::c_co_module_body_entry( class c_co_schematic_symbol *schematic_symbol )
{
    this->type = module_body_entry_type_schematic_symbol;
    this->data.schematic_symbol = schematic_symbol;

    co_init(co_type_module_body_entry,"module_body_entry( schematic_symbol )" );
    co_link(co_compile_stage_parse, (c_cyc_object*)schematic_symbol, "schematic_symbol" );
}

/*f c_co_module_body_entry::~c_co_module_body_entry
 */
c_co_module_body_entry::~c_co_module_body_entry()
{
}

/*a c_co_module
 */
/*f c_co_module::c_co_module( symbol )
 */
c_co_module::c_co_module( t_symbol *id, class c_co_signal_declaration *ports, class c_co_module_body_entry *body, t_string *documentation )
{
    this->symbol = id;
    this->ports = ports;
    this->body = body;
    this->documentation = documentation;

    this->local_signals = NULL;

    this->md_module = NULL;

    co_init(co_type_module,"module()" );
    co_link(co_compile_stage_parse, id, "name" );
    co_link(co_compile_stage_parse, (c_cyc_object*)ports, "ports" );
    co_link(co_compile_stage_parse, (c_cyc_object*)body, "body" );
    co_link(co_compile_stage_parse, documentation, "documentation" );

}

/*f c_co_module::~c_co_module
 */
c_co_module::~c_co_module()
{
    if (!local_signals)
    {
        free(local_signals);
        local_signals = NULL;
    }
}

/*a c_co_module_prototype_body_entry
 */
c_co_module_prototype_body_entry::c_co_module_prototype_body_entry( class c_co_timing_spec *timing_spec )
{
    this->type = module_prototype_body_entry_type_timing_spec;
    this->data.timing_spec = timing_spec;

    co_init(co_type_module_prototype_body_entry,"module_prototype_body_entry( timing_spec )" );
    co_link(co_compile_stage_parse, (c_cyc_object*)timing_spec, "timing_spec" );
}

/*f c_co_module_prototype_body_entry::~c_co_module_prototype_body_entry
 */
c_co_module_prototype_body_entry::~c_co_module_prototype_body_entry()
{
}

/*a c_co_module_prototype
 */
/*f c_co_module_prototype::c_co_module_prototype( symbol )
 */
c_co_module_prototype::c_co_module_prototype( t_symbol *id, class c_co_signal_declaration *ports, class c_co_module_prototype_body_entry *body, t_string *documentation )
{
    this->symbol = id;
    this->ports = ports;
    this->body = body;
    this->documentation = documentation;

    co_init(co_type_module_prototype,"module_prototype()" );
    co_link(co_compile_stage_parse, id, "name" );
    co_link(co_compile_stage_parse, (c_cyc_object*)ports, "ports" );
    co_link(co_compile_stage_parse, (c_cyc_object*)body, "body" );
    co_link(co_compile_stage_parse, documentation, "documentation" );

}

/*f c_co_module_prototype::~c_co_module_prototype
 */
c_co_module_prototype::~c_co_module_prototype()
{
}

/*a c_co_named_expression
 */
/*f c_co_named_expression::c_co_named_expression
 */
c_co_named_expression::c_co_named_expression( t_string *name, class c_co_expression *expression )
{
    this->name = name;
    this->expression = expression;

    co_init(co_type_named_expression, "named_expressions");
    co_link(co_compile_stage_parse, name, "Name" );
    co_link(co_compile_stage_parse, expression, "Value" );
}

/*a c_co_nested_assignment
 */
/*f c_co_nested_assignment::c_co_nested_assignment( nested_assignment ) ... nesting a list of assignments
 */
c_co_nested_assignment::c_co_nested_assignment( class c_co_nested_assignment *nested_assignment )
{
    this->nested_assignment = nested_assignment;
    this->symbol = NULL;
    this->expression = NULL;
    this->element = -1;
    this->wildcard = 0;

    co_init(co_type_nested_assignment,"nested_assignment( nested )");
    co_link(co_compile_stage_parse, (c_cyc_object *)nested_assignment, "nested_assignment" );

}

/*f c_co_nested_assignment::c_co_nested_assignment( int, expression ) ... value (or * = value)
 */
c_co_nested_assignment::c_co_nested_assignment( int wildcard, class c_co_expression *expression )
{
    this->nested_assignment = NULL;
    this->symbol = NULL;
    this->expression = expression;
    this->element = -1;
    this->wildcard = wildcard;

    co_init(co_type_nested_assignment,"nested_assignment( value )");
    co_link(co_compile_stage_parse, (c_cyc_object *)expression, "expression" );

}

/*f c_co_nested_assignment::c_co_nested_assignment( symbol, nested_assignment ) ... assignment
 */
c_co_nested_assignment::c_co_nested_assignment( t_symbol *symbol, class c_co_nested_assignment *nested_assignment )
{
    this->nested_assignment = nested_assignment;
    this->symbol = symbol;
    this->expression = NULL;
    this->element = -1;
    this->wildcard = 0;

    co_init(co_type_nested_assignment,"nested_assignment( assignment )");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object *)nested_assignment, "nested_assignment" );

}

/*f c_co_nested_assignment::~c_co_nested_assignment
 */
c_co_nested_assignment::~c_co_nested_assignment()
{
}

/*a c_co_port_map
 */
/*f c_co_port_map::c_co_port_map( lvar, lvar )
 */
c_co_port_map::c_co_port_map( class c_co_lvar *port_lvar, class c_co_lvar *lvar )
{
    this->type = port_map_type_output;
    this->port_id = NULL;
    this->port_lvar = port_lvar;
    this->clock_id = NULL;
    this->lvar = lvar;
    this->expression = NULL;

    co_init(co_type_port_map,"port_map()" );
    co_link(co_compile_stage_parse, port_lvar, "port_lvar" );
    co_link(co_compile_stage_parse, lvar, "lvar" );
}


/*f c_co_port_map::c_co_port_map( lvar, expression )
 */
c_co_port_map::c_co_port_map( class c_co_lvar *port_lvar, class c_co_expression *expression )
{
    this->type = port_map_type_input;
    this->port_id = NULL;
    this->port_lvar = port_lvar;
    this->clock_id = NULL;
    this->lvar = NULL;
    this->expression = expression;

    co_init(co_type_port_map,"port_map()" );
    co_link(co_compile_stage_parse, port_lvar, "port_lvar" );
    co_link(co_compile_stage_parse, expression, "expression" );
}

/*f c_co_port_map::c_co_port_map( symbol, expression )
 */
c_co_port_map::c_co_port_map( t_symbol *port_id, class c_co_expression *expression )
{
    this->type = port_map_type_input;
    this->port_id = port_id;
    this->port_lvar = NULL;
    this->clock_id = NULL;
    this->lvar = NULL;
    this->expression = expression;

    co_init(co_type_port_map,"port_map()" );
    co_link(co_compile_stage_parse, port_id, "port_id" );
    co_link(co_compile_stage_parse, expression, "expression" );
}

/*f c_co_port_map::c_co_port_map( symbol, symbol )
 */
c_co_port_map::c_co_port_map( t_symbol *port_id, t_symbol *clock_id )
{
    this->type = port_map_type_clock;
    this->port_id = port_id;
    this->port_lvar = NULL;
    this->clock_id = clock_id;
    this->lvar = NULL;
    this->expression = NULL;

    co_init(co_type_port_map,"port_map()" );
    co_link(co_compile_stage_parse, port_id, "port_id" );
    co_link(co_compile_stage_parse, clock_id, "clock_id" );
}

/*f c_co_port_map::~c_co_port_map
 */
c_co_port_map::~c_co_port_map()
{
}

/*a c_co_schematic_symbol_body_entry
 */
c_co_schematic_symbol_body_entry::c_co_schematic_symbol_body_entry( t_symbol_object_type type, t_symbol *name, class c_co_sized_int_pair *coords, t_string *documentation )
{
    this->type = type;
    this->text_id = name;
    this->first_coords = coords;
    this->second_coords = NULL;
    this->documentation = documentation;
    this->sized_int = NULL;

    co_init(co_type_schematic_symbol_body_entry,"schematic_symbol_body_entry()" );
    co_link(co_compile_stage_parse, name, "name" );
    co_link(co_compile_stage_parse, (c_cyc_object *)coords, "coords" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

c_co_schematic_symbol_body_entry::c_co_schematic_symbol_body_entry( t_symbol_object_type type, class c_co_sized_int_pair *coords, t_string *documentation )
{
    this->type = type;
    this->text_id = NULL;
    this->first_coords = coords;
    this->second_coords = NULL;
    this->documentation = documentation;
    this->sized_int = NULL;

    co_init(co_type_schematic_symbol_body_entry,"schematic_symbol_body_entry()" );
    co_link(co_compile_stage_parse, (c_cyc_object *)coords, "coords" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

c_co_schematic_symbol_body_entry::c_co_schematic_symbol_body_entry( t_symbol_object_type type, class c_co_sized_int_pair *coord_0, class c_co_sized_int_pair *coord_1, t_string *documentation )
{
    this->type = type;
    this->text_id = NULL;
    this->first_coords = coord_0;
    this->second_coords = coord_1;
    this->documentation = documentation;
    this->sized_int = NULL;

    co_init(co_type_schematic_symbol_body_entry,"schematic_symbol_body_entry()" );
    co_link(co_compile_stage_parse, (c_cyc_object *)coord_0, "coord_0" );
    co_link(co_compile_stage_parse, (c_cyc_object *)coord_1, "coord_1" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

c_co_schematic_symbol_body_entry::c_co_schematic_symbol_body_entry( t_symbol_object_type type, t_symbol *name, t_sized_int *sized_int, t_string *documentation )
{
    this->type = type;
    this->text_id = name;
    this->first_coords = NULL;
    this->second_coords = NULL;
    this->documentation = documentation;
    this->sized_int = sized_int;

    co_init(co_type_schematic_symbol_body_entry,"schematic_symbol_body_entry()" );
    co_link(co_compile_stage_parse, name, "name" );
    co_link(co_compile_stage_parse, sized_int, "sized_int" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

/*f c_co_schematic_symbol_body_entry::~c_co_schematic_symbol_body_entry
 */
c_co_schematic_symbol_body_entry::~c_co_schematic_symbol_body_entry()
{
}

/*a c_co_schematic_symbol
 */
c_co_schematic_symbol::c_co_schematic_symbol( t_string *variant, class c_co_schematic_symbol_body_entry *body )
{
    this->variant = variant;
    this->body = body;

    co_init(co_type_schematic_symbol,"schematic_symbol()" );
    co_link(co_compile_stage_parse, variant, "variant" );
    co_link(co_compile_stage_parse, (c_cyc_object *)body, "body" );
}

/*f c_co_schematic_symbol::~c_co_schematic_symbol
 */
c_co_schematic_symbol::~c_co_schematic_symbol()
{
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::c_co_signal_declaration( symbol, type, clock, reset, nested_assignment ) ... clocked nested
 */
c_co_signal_declaration::c_co_signal_declaration( t_symbol *id,
                                                  class c_co_type_specifier *type,
                                                  t_signal_usage_type usage_type,
                                                  class c_co_clock_reset_defn *clock,
                                                  class c_co_clock_reset_defn *reset,
                                                  class c_co_nested_assignment *reset_value,
                                                  t_string *documentation ) // local clocked signal with nested reset
{
    this->symbol = id;
    this->type_specifier = type;
    this->signal_declaration_type = signal_declaration_type_clocked_local;
    this->documentation = documentation;
    this->data.clocked.usage_type = usage_type;
    this->data.clocked.clock_spec = clock;
    this->data.clocked.reset_spec = reset;
    this->data.clocked.reset_value = reset_value;
    this->local_clocked_signal = NULL;
    this->output_port = NULL;

    co_init(co_type_signal_declaration,"signal_declaration( clocked local, nested reset )");
    co_link(co_compile_stage_parse, id, "id" );
    co_link(co_compile_stage_parse, (c_cyc_object *)type, "type" );
    co_link(co_compile_stage_parse, (c_cyc_object *)clock, "clock" );
    co_link(co_compile_stage_parse, (c_cyc_object *)reset, "reset" );
    co_link(co_compile_stage_parse, (c_cyc_object *)reset_value, "reset_value" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

/*f c_co_signal_declaration::c_co_signal_declaration( symbol, type, value ) ... combinatorial
 */
c_co_signal_declaration::c_co_signal_declaration( t_symbol *id,
                                                  class c_co_type_specifier *type,
                                                  t_signal_usage_type usage_type,
                                                  t_string *documentation )
{
    this->symbol = id;
    this->type_specifier = type;
    this->signal_declaration_type = signal_declaration_type_comb_local;
    this->documentation = documentation;
    this->data.comb.usage_type = usage_type;
    this->local_clocked_signal = NULL;
    this->output_port = NULL;

    co_init(co_type_signal_declaration,"signal_declaration( comb local )");
    co_link(co_compile_stage_parse, id, "id" );
    co_link(co_compile_stage_parse, (c_cyc_object *)type, "type" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

/*f c_co_signal_declaration::c_co_signal_declaration( signal_declaration_type, symbol, type ) ... parameter/net
 */
c_co_signal_declaration::c_co_signal_declaration( t_signal_declaration_type sd_type,
                                                  t_symbol *id,
                                                  class c_co_type_specifier *type,
                                                  t_string *documentation )
{
    this->symbol = id;
    this->type_specifier = type;
    this->signal_declaration_type = sd_type;
    this->documentation = documentation;
    this->local_clocked_signal = NULL;
    this->output_port = NULL;

    co_init(co_type_signal_declaration,"signal_declaration( port parameter )");
    co_link(co_compile_stage_parse, id, "id" );
    co_link(co_compile_stage_parse, (c_cyc_object *)type, "type" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

/*f c_co_signal_declaration::c_co_signal_declaration( symbol, dirn type ) ... input/output port
 */
c_co_signal_declaration::c_co_signal_declaration( t_symbol *id, t_symbol *dirn, class c_co_type_specifier *type, t_string *documentation ) // input/output port
{
    this->symbol = id;
    this->type_specifier = type;
    if (get_symbol_type(dirn->lex_symbol)==TOKEN_TEXT_OUTPUT)
    {
        this->signal_declaration_type = signal_declaration_type_comb_output;
        co_init(co_type_signal_declaration,"signal_declaration( output port )");
    }
    else
    {
        this->signal_declaration_type = signal_declaration_type_input;
        co_init(co_type_signal_declaration,"signal_declaration( input port )");
    }
    this->documentation = documentation;
    this->local_clocked_signal = NULL;
    this->output_port = NULL;

    co_link(co_compile_stage_parse, id, "id" );
    co_link(co_compile_stage_parse, (c_cyc_object *)type, "type" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

/*f c_co_signal_declaration::c_co_signal_declaration( symbol ) ... clock input
 */
c_co_signal_declaration::c_co_signal_declaration( t_symbol *id, t_string *documentation ) // Clock input
{
    this->symbol = id;
    this->type_specifier = NULL;
    this->signal_declaration_type = signal_declaration_type_clock;
    this->documentation = documentation;
    this->local_clocked_signal = NULL;
    this->output_port = NULL;

    this->data.clock.clock_to_gate = NULL;
    this->data.clock.gate          = NULL;

    co_init(co_type_signal_declaration,"signal_declaration( clock port )");
    co_link(co_compile_stage_parse, id, "id" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
}

/*f c_co_signal_declaration::c_co_signal_declaration( symbol, clock, 'reset' ) ... gated clock
 */
c_co_signal_declaration::    c_co_signal_declaration( t_symbol *id, c_co_clock_reset_defn *clock, c_co_clock_reset_defn *clock_enable, t_string *documentation ) // gated clock
{
    this->symbol = id;
    this->type_specifier = NULL;
    this->signal_declaration_type = signal_declaration_type_clock;
    this->documentation = documentation;
    this->local_clocked_signal = NULL;
    this->output_port = NULL;

    this->data.clock.clock_to_gate = clock;
    this->data.clock.gate          = clock_enable;

    co_init(co_type_signal_declaration,"signal_declaration( gated clock )");
    co_link(co_compile_stage_parse, id, "id" );
    co_link(co_compile_stage_parse, documentation, "documentation" );
    co_link(co_compile_stage_parse, (c_cyc_object *)clock, "clock" );
    co_link(co_compile_stage_parse, (c_cyc_object *)clock_enable, "clock_enable" );
}

/*f c_co_signal_declaration::~c_co_signal_declaration
 */
c_co_signal_declaration::~c_co_signal_declaration()
{
}

/*a c_co_sized_int_pair
 */
/*f c_co_sized_int_pair::c_co_sized_int_pair
 */
c_co_sized_int_pair::c_co_sized_int_pair( t_sized_int *left, t_sized_int *right )
{
    left = left;
    right = right;

    co_init(co_type_sized_int_pair,"sized_int_pait()");
    co_link(co_compile_stage_parse, left, "left" );
    co_link(co_compile_stage_parse, right, "right" );
}

/*f c_co_sized_int_pair::~c_co_sized_int_pair
 */
c_co_sized_int_pair::~c_co_sized_int_pair()
{
}

/*a c_co_statement
 */
/*f c_co_statement::c_co_statement( lvar, nested_assignment, clocked_or_wired_or )
 */
c_co_statement::c_co_statement( class c_co_lvar *lvar, class c_co_nested_assignment *nested_assignment, int clocked_or_wired_or, t_string *documentation )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type_assignment;
    this->type_data.assign_stmt.lvar = lvar;
    this->type_data.assign_stmt.clocked  = (clocked_or_wired_or==1);
    this->type_data.assign_stmt.wired_or = (clocked_or_wired_or==2);
    this->type_data.assign_stmt.nested_assignment = nested_assignment;
    this->type_data.assign_stmt.documentation = documentation;

    if (clocked_or_wired_or==1)
        co_init(co_type_statement, "statement( clocked assignment )");
    else if (clocked_or_wired_or==2)
        co_init(co_type_statement, "statement( comb wired-or assignment )");
    else
        co_init(co_type_statement, "statement( comb assignment )");
    co_link(co_compile_stage_parse, (c_cyc_object*)lvar, "lvar" );
    co_link(co_compile_stage_parse, (c_cyc_object*)nested_assignment, "nested_assignment" );
}

/*f c_co_statement::c_co_statement( clock, reset, statement_type, assertion expression, expression match list, string format, expression list args, code block
 */
c_co_statement::c_co_statement( class c_co_clock_reset_defn *clock_spec, class c_co_clock_reset_defn *reset_spec, t_statement_type type, c_co_expression *assertion, c_co_expression *value_list, t_string *text, c_co_expression *text_args, c_co_statement *code )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = type;
    this->type_data.print_assert_stmt.assertion = assertion;
    this->type_data.print_assert_stmt.value_list = value_list;
    this->type_data.print_assert_stmt.clock_spec = clock_spec;
    this->type_data.print_assert_stmt.reset_spec = reset_spec;
    this->type_data.print_assert_stmt.text = text;
    this->type_data.print_assert_stmt.text_args = text_args;
    this->type_data.print_assert_stmt.statement = code;

    co_init(co_type_statement, "statement( print )");
    co_link(co_compile_stage_parse, assertion, "Assertion" );
    co_link(co_compile_stage_parse, clock_spec, "Clock spec" );
    co_link(co_compile_stage_parse, reset_spec, "Reset spec" );
    co_link(co_compile_stage_parse, text, "Text" );
}

/*f c_co_statement::c_co_statement( class c_co_clock_reset_defn *clock_spec, class c_co_clock_reset_defn *reset_spec, t_string *log_message, class c_co_named_expression *log_values); // log message and arguments
 */
c_co_statement::c_co_statement( class c_co_clock_reset_defn *clock_spec, class c_co_clock_reset_defn *reset_spec, t_string *log_message, class c_co_named_expression *log_values)
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type_log;
    this->type_data.log_stmt.message = log_message;
    this->type_data.log_stmt.values = log_values;
    this->type_data.log_stmt.clock_spec = clock_spec;
    this->type_data.log_stmt.reset_spec = reset_spec;

    co_init(co_type_statement, "statement( log )");
    co_link(co_compile_stage_parse, clock_spec, "Clock spec" );
    co_link(co_compile_stage_parse, reset_spec, "Reset spec" );
    co_link(co_compile_stage_parse, log_message, "Log message" );
//    co_link(co_compile_stage_parse, log_values, "Log values" );
}

/*f c_co_statement::c_co_statement( statement_list )
 */
c_co_statement::c_co_statement( c_co_statement *statement_list )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type_block;
    this->type_data.block = statement_list;

    co_init(co_type_statement, "statement( statement block )");
    co_link(co_compile_stage_parse, (c_cyc_object*)statement_list, "statement_list" );
}


/*f c_co_statement::c_co_statement( statement_type, expression, cases )
 */
c_co_statement::c_co_statement( t_statement_type statement_type, class c_co_expression *expression, class c_co_case_entry *cases, t_string *expr_documentation )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type;
    this->type_data.switch_stmt.expression = expression;
    this->type_data.switch_stmt.cases = cases;
    this->type_data.switch_stmt.expr_documentation = expr_documentation;

    co_init(co_type_statement, "statement( switch statement )");
    co_link(co_compile_stage_parse, (c_cyc_object*)expression, "expression" );
    co_link(co_compile_stage_parse, (c_cyc_object*)cases, "cases" );
}

/*f c_co_statement::c_co_statement( expression, if_true, if_false, elsif )
 */
c_co_statement::c_co_statement( class c_co_expression *expression, c_co_statement *if_true, c_co_statement *if_false, c_co_statement *elsif, t_string *expr_documentation )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type_if_statement;
    this->type_data.if_stmt.expression = expression;
    this->type_data.if_stmt.if_true = if_true;
    this->type_data.if_stmt.if_false = if_false;
    this->type_data.if_stmt.elsif = elsif;
    this->type_data.if_stmt.expr_documentation = expr_documentation;

    co_init(co_type_statement, "statement( if statement )");
    co_link(co_compile_stage_parse, (c_cyc_object*)expression, "expression" );
    co_link(co_compile_stage_parse, (c_cyc_object*)if_true, "if_true" );
    co_link(co_compile_stage_parse, (c_cyc_object*)if_false, "if_false" );
    co_link(co_compile_stage_parse, (c_cyc_object*)elsif, "elsif" );
}

/*f c_co_statement::c_co_statement( symbol, expression, statement, first_value, last_value, next_value )
 */
c_co_statement::c_co_statement( t_symbol *symbol, class c_co_expression *iterations, class c_co_statement *statement, class c_co_expression *first_value, class c_co_expression *last_value, class c_co_expression *next_value )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type_for_statement;
    this->type_data.for_stmt.symbol = symbol;
    this->type_data.for_stmt.iterations = iterations;
    this->type_data.for_stmt.statement = statement;
    this->type_data.for_stmt.first_value = first_value;
    this->type_data.for_stmt.last_value = last_value;
    this->type_data.for_stmt.next_value = next_value;
    this->type_data.for_stmt.scope = NULL;
    this->type_data.for_stmt.type_value = type_value_undefined;

    co_init(co_type_statement, "statement( for statement )");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object*)iterations, "iterations" );
    co_link(co_compile_stage_parse, (c_cyc_object*)statement, "statement" );
    co_link(co_compile_stage_parse, (c_cyc_object*)first_value, "first_value" );
    co_link(co_compile_stage_parse, (c_cyc_object*)last_value, "last_value" );
    co_link(co_compile_stage_parse, (c_cyc_object*)next_value, "next_value" );
}

/*f c_co_statement::c_co_statement( instantiation )
 */
c_co_statement::c_co_statement( c_co_instantiation *instantiation )
{
    this->parent = NULL;
    this->toplevel = NULL;
    this->code_label = NULL;
    this->statement_type = statement_type_instantiation;
    this->type_data.instantiation = instantiation;

    co_init(co_type_statement, "statement( instantiation )");
    co_link(co_compile_stage_parse, (c_cyc_object*)instantiation, "instantiation" );
}

/*f c_co_statement::~c_co_statement
 */
c_co_statement::~c_co_statement()
{
}

/*a c_co_timing_spec
 */
/*f c_co_timing_spec::c_co_timing_spec( type, clock edge, symbol, dependents )
 */
c_co_timing_spec::c_co_timing_spec( t_timing_spec_type type, t_timing_spec_clock_edge clock_edge, t_symbol *clock_symbol, c_co_token_union *signals )
{
    this->clock_symbol = clock_symbol;
    this->signals = signals;
    this->type = type;
    this->clock_edge = clock_edge;

    co_init(co_type_timing_spec, "timing_spec( type, clock edge, symbol, signals )");
    co_link(co_compile_stage_parse, clock_symbol, "clock_symbol" );
    co_link(co_compile_stage_parse, signals, "signals" );
}

/*f c_co_timing_spec::c_co_timing_spec( type, signals )
 */
c_co_timing_spec::c_co_timing_spec( t_timing_spec_type type, c_co_token_union *signals )
{
    this->clock_symbol = NULL;
    this->signals = signals;
    this->type = type;

    co_init(co_type_timing_spec, "timing_spec( type, signals )");
    co_link(co_compile_stage_parse, signals, "signals" );
}

/*f c_co_timing_spec::~c_co_timing_spec
 */
c_co_timing_spec::~c_co_timing_spec()
{
}

/*a c_co_token_union
 */
/*f c_co_token_union::c_co_token_union( t_symbol )
 */
c_co_token_union::c_co_token_union( t_symbol *symbol )
{
    token_union_type = token_union_type_symbol;
    token_union_data.symbol = symbol;

    co_init(co_type_token_union, "token_union(symbol)");
    co_link(co_compile_stage_parse, symbol, "symbol" );
}

/*f c_co_token_union::c_co_token_union( t_sized_int )
 */
c_co_token_union::c_co_token_union( t_sized_int *sized_int )
{
    token_union_type = token_union_type_sized_int;
    token_union_data.sized_int = sized_int;

    co_init(co_type_token_union,"token_union(sized_int)");
    co_link(co_compile_stage_parse, sized_int, "sized_int" );
}

/*f c_co_token_union::c_co_token_union( t_string )
 */
c_co_token_union::c_co_token_union( t_string *string )
{
    token_union_type = token_union_type_string;
    token_union_data.string = string;

    co_init(co_type_token_union,"token_union(string)");
    co_link(co_compile_stage_parse, string, "string" );
}

/*f c_co_token_union::~c_co_token_union
 */
c_co_token_union::~c_co_token_union()
{
}

/*a c_co_toplevel
 */
/*f c_co_toplevel::c_co_toplevel( type_definition )
 */
c_co_toplevel::c_co_toplevel( c_co_type_definition *type_definition )
{
    if (type_definition)
    {
        this->data.type_definition = type_definition;
        this->toplevel_type = toplevel_type_type_definition;

        co_init(co_type_toplevel, "toplevel(type definition)");
        co_link(co_compile_stage_parse, (c_cyc_object *)type_definition, "type_definition" );
    }
}

/*f c_co_toplevel::c_co_toplevel( module )
 */
c_co_toplevel::c_co_toplevel( c_co_module *module )
{
    if (module)
    {
        this->data.module = module;
        this->toplevel_type = toplevel_type_module;

        co_init(co_type_toplevel, "toplevel( module )");
        co_link(co_compile_stage_parse, (c_cyc_object *)module, "module" );
    }
}

/*f c_co_toplevel::c_co_toplevel( module_prototype )
 */
c_co_toplevel::c_co_toplevel( c_co_module_prototype *module_prototype )
{
    if (module_prototype)
    {
        this->data.module_prototype = module_prototype;
        this->toplevel_type = toplevel_type_module_prototype;

        co_init(co_type_toplevel, "toplevel( module_prototype )");
        co_link(co_compile_stage_parse, (c_cyc_object *)module_prototype, "module_prototype" );
    }
}

/*f c_co_toplevel::c_co_toplevel( constant_declaration )
 */
c_co_toplevel::c_co_toplevel( c_co_constant_declaration *constant_declaration )
{
    if (constant_declaration)
    {
        this->data.constant_declaration = constant_declaration;
        this->toplevel_type = toplevel_type_constant_declaration;

        co_init(co_type_toplevel, "toplevel( constant_declaration )");
        co_link(co_compile_stage_parse, (c_cyc_object *)constant_declaration, "constant_declaration" );
    }
}

/*f c_co_toplevel::~c_co_toplevel
 */
c_co_toplevel::~c_co_toplevel()
{
}

/*a c_co_type_definition
 */
/*f c_co_type_definition::c_co_type_definition( symbol, type_specifier )
 */
c_co_type_definition::c_co_type_definition( t_symbol *symbol, class c_co_type_specifier *type_specifier )
{
    this->symbol = symbol;
    this->type_specifier = type_specifier;
    this->enum_definition = NULL;
    this->fsm_state = NULL;
    this->type_struct = NULL;

    co_init(co_type_type_definition,"type_definition( type )");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object*)type_specifier, "type_specifier" );

    this->type_value = type_value_error;
}

/*f c_co_type_definition::c_co_type_definition( symbol, enum_definition )
 */
c_co_type_definition::c_co_type_definition( t_symbol *symbol, class c_co_enum_definition *enum_definition )
{
    this->symbol = symbol;
    this->type_specifier = NULL;
    this->enum_definition = enum_definition;
    this->fsm_state = NULL;
    this->type_struct = NULL;

    co_init(co_type_type_definition,"type_definition( enum )");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object*)enum_definition, "enum_definition" );

    this->type_value = type_value_error;
}

/*f c_co_type_definition::c_co_type_definition( symbol, fsm_state )
 */
c_co_type_definition::c_co_type_definition( t_symbol *symbol, class c_co_fsm_state *fsm_state )
{
    this->symbol = symbol;
    this->type_specifier = NULL;
    this->enum_definition = NULL;
    this->fsm_state = fsm_state;
    this->type_struct = NULL;

    co_init(co_type_type_definition,"type_definition( fsm )");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object*)fsm_state, "fsm_state" );

    this->type_value = type_value_error;
}

/*f c_co_type_definition::c_co_type_definition( symbol, type_struct )
 */
c_co_type_definition::c_co_type_definition( t_symbol *symbol, class c_co_type_struct *type_struct )
{
    this->symbol = symbol;
    this->type_specifier = NULL;
    this->enum_definition = NULL;
    this->fsm_state = NULL;
    this->type_struct = type_struct;

    co_init(co_type_type_definition,"type_definition( struct )");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object*)type_struct, "type_struct" );

    this->type_value = type_value_error;
}

/*f c_co_type_definition::~c_co_type_definition
 */
c_co_type_definition::~c_co_type_definition()
{
}

/*a c_co_type_specifier
 */
/*f c_co_type_specifier::c_co_type_specifier ( symbol )
  If remapping types, then we have to find 'symbol' and replace it with another
 */
c_co_type_specifier::c_co_type_specifier( c_cyclicity *cyc, t_symbol *symbol)
{
    cyc->remap_type_specifier_symbol( symbol );

    this->symbol = symbol;
    this->type_specifier = NULL;
    this->first = NULL;
    this->last = NULL;

    co_init(co_type_type_specifier,"type_specifier( symbol )");
    co_link(co_compile_stage_parse, symbol, "symbol" );

    this->refers_to = NULL;
    this->type_value = type_value_error;
}

/*f c_co_type_specifier::c_co_type_specifier( type, first, last )
 */
c_co_type_specifier::c_co_type_specifier( c_co_type_specifier *type_specifier, class c_co_expression *first, class c_co_expression *last )
{
    this->symbol = NULL;
    this->type_specifier = type_specifier;
    this->first = first;
    this->last = last;

    co_init(co_type_type_specifier,"type_specifier( array )");
    co_link(co_compile_stage_parse, (c_cyc_object*)type_specifier, "type specifier" );
    co_link(co_compile_stage_parse, (c_cyc_object*)first, "first" );
    co_link(co_compile_stage_parse, (c_cyc_object*)last, "last" );

    this->refers_to = NULL;
    this->type_value = type_value_error;
}

/*f c_co_type_specifier::~c_co_type_specifier
 */
c_co_type_specifier::~c_co_type_specifier()
{
}

/*a c_co_type_struct
 */
/*f c_co_type_struct::c_co_type_struct ( symbol )
  If remapping types, then the type_specifier is remapped automatically
 */
c_co_type_struct::c_co_type_struct( t_symbol *symbol, c_co_type_specifier *type_specifier, t_string *documentation )
{
    this->symbol = symbol;
    this->type_specifier = type_specifier;
    this->documentation = documentation;

    co_init(co_type_type_struct,"type_struct");
    co_link(co_compile_stage_parse, symbol, "symbol" );
    co_link(co_compile_stage_parse, (c_cyc_object *)type_specifier, "type_specifier" );
    co_link(co_compile_stage_parse, documentation, "documentation" );

}

/*f c_co_type_struct::~c_co_type_struct
 */
c_co_type_struct::~c_co_type_struct()
{
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


