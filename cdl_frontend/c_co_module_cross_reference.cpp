/*a Copyright
  
  This file 'c_co_module_cross_reference.cpp' copyright Gavin J Stark 2003, 2004
  
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

#include "c_co_bus_or_inst.h"
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

/*a c_co_bus_or_inst
 */
/*f c_co_bus_or_inst::cross_reference
 */
int c_co_bus_or_inst::cross_reference( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types, t_co_scope *variables )
{
     int i;
     c_co_module *com;

     refers_to = NULL;

     if (!instance_of_id) // If for some reason the module being instanced is NULL (in error somehow), protect ourselves by just returning
         return 0;

     for (i=0; modules->co[i].co_module; i++)
     {
          com = modules->co[i].co_module;
          if (com->id->lex_symbol == instance_of_id->lex_symbol)
          {
               refers_to = modules->co[i];
               //printf("Referring module instance %s / %s\n", lex_string_from_terminal( instance_of_id ), lex_string_from_terminal( instance_id ) );
          }
     }
     if (!refers_to)
     {
         cyclicity->set_parse_error((c_cyc_object *)combe->data.bus_or_inst, co_compile_stage_cross_reference, "Failed to find module '%s'", lex_string_from_terminal(combe->data.bus_or_inst->instance_of_id));
         return 0;
     }
     if (port_map)
     {
         return port_map->cross_reference( cyclicity, types, variables );
     }
     return 0;
}

/*a c_co_case_entry
 */
/*f c_co_case_entry::cross_reference_within_type_context
 */
void c_co_case_entry::cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type )
{
     if (expression)
     {
          expression->cross_reference_within_type_context( cyclicity, types, variables, type );
     }
     if (statements)
     {
          statements->cross_reference( cyclicity, types, variables );
     }
     if (next_in_list)
     {
          ((c_co_case_entry *)next_in_list)->cross_reference_signals_and_constants( cyclicity, types, variables, type );
     }
}

/*a c_co_expression
 */
/*f c_co_expression::cross_reference_signals_and_constants_within_type_context
 */
void c_co_expression::cross_reference_signals_and_constants_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value expected_type )
{
     type = type_value_undefined;
     switch (expr_type)
     {
     case expr_type_sized_int:
          type = type_value_integer;
          break;
     case expr_type_symbol:
          user_id_refers_to = cyclicity->cross_reference_symbol_in_scopes( user_id, variables );
          if (!user_id_refers_to.cyc_object)
          {
               user_id_refers_to = cyclicity->cross_reference_symbol_in_scopes( user_id, types );
          }
          if (!user_id_refers_to.cyc_object)
          {
               cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Constant/signal/parameter/state/enum '%s' not declared", lex_string_from_terminal( user_id ));
          }
          else
          {
               switch (user_id_refers_to.cyc_object->co_type)
               {
               case co_type_type_definition:
                    type = user_id_refers_to.co_type_definition->type_value;
                    break;
               case co_type_signal_declaration:
                    if (user_id_refers_to.co_signal_declaration->type_specifier)
                    {
                         type = user_id_refers_to.co_signal_declaration->type_specifier->type_value;
                    }
                    break;
               case co_type_constant_declaration:
                    if (user_id_refers_to.co_constant_declaration->type_specifier)
                    {
                         type = user_id_refers_to.co_constant_declaration->type_specifier->type_value;
                    }
                    break;
               case co_type_enum_identifier:
                    if (user_id_refers_to.co_enum_identifier->expression)
                    {
                         type = user_id_refers_to.co_enum_identifier->expression->type;
                    }
                    break;
               case co_type_fsm_state:
                    //type = user_id_refers_to.co_fsm_state->type_value;
                    fprintf(stderr, "NYI:c_co_expression::cross_reference_signals_and_constants_within_type_context:fsm state type\n");
                    break;
               default:
                    break;
               }
          }
          break;
     case expr_type_structure_element:
          element = -1;
          if (first)
          {
               first->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
               element = cyclicity->type_value_pool->find_structure_element( first->type, user_id );
               if (element<0)
               {
                    fprintf(stderr,"NNE:c_co_expression::cross_reference_signals_and_constants:Check symbol '%s' is in the structure '%d' - got %d\n", lex_string_from_terminal(user_id), first->type, element );
               }
          }
          break;
     case expr_type_unary_function:
     case expr_type_binary_function:
     case expr_type_ternary_function:
          switch (expr_subtype)
          {
          case expr_subtype_binary_not:
               if (first)
               {
                    first->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_bit );
               }
               break;
          default:
               if (first)
               {
                    first->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
               }
               if (second)
               {
                    second->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
               }
               if (third)
               {
                    third->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
               }
               fprintf(stderr, "NYI:c_co_expression::cross_reference:Expression elements not typechecked yet\n");
               break;
          }
          break;
     }
     if (next_in_list)
     {
          // We assume here that all lists of expressions must be of the same type
          // The only issue we could have is when we are building a concatenation of bits - but that is the responsiblitiy of the caller (who should have requested bit_vector)
          ((c_co_expression *)next_in_list)->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, expected_type );
     }
}

/*a c_co_fsm_state
 */
/*f c_co_fsm_state::cross_reference
 */
void c_co_fsm_state::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
     c_co_fsm_state *state;
     int has_expression;

     has_expression = 0;
     if (expression)
     {
          expression->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
          has_expression = 1;
          fprintf(stderr, "**** WARNING - expressions given to FSM declarations are NOT checked for being suitably in range or one-hot; please do not specify values for states in FSMs unless absolutely required\nNYI:c_co_fsm_state::cross_reference:Expressions not typechecked yet\n");
     }
     for (state = (c_co_fsm_state *)next_in_list; state; )
     {
          if ((state->expression))
          {
               if (!has_expression)
               {
                    cyclicity->set_parse_error( (c_cyc_object *)state, co_compile_stage_cross_reference, "Either all or no FSM states need to be assigned, and this state is, others are not" );
                    return;
               }
               state->expression->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
          }
          else if (has_expression)
          {
               cyclicity->set_parse_error( (c_cyc_object *)state, co_compile_stage_cross_reference, "Either all or no FSM states need to be assigned, and this state is not" );
               return;
          }
          state = (c_co_fsm_state *)state->next_in_list;
     }
}

/*a c_co_lvar
 */
/*f c_co_lvar::cross_reference
  Fill out the type of the lvar, and the port that it corresponds to
 */
void c_co_lvar::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
     t_co_union symbol_refers_to;

     type = type_value_error;
     signal_declaration = NULL;

     symbol_refers_to = cyclicity->cross_reference_symbol_in_scopes( symbol, variables );
     if (symbol_refers_to.cyc_object->co_type != co_type_signal_declaration)
     {
          fprintf( stderr, "NNE:c_co_lvar::cross_reference:Attempt to assign to a constant or other global '%s'\n", lex_string_from_terminal( symbol ) );
          return;
     }
     if (!symbol_refers_to.cyc_object)
     {
          cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_cross_reference, "Lvar '%s' not declared", lex_string_from_terminal( symbol ));
     }
     signal_declaration = symbol_refers_to.co_signal_declaration;
     if ((signal_declaration) && (signal_declaration->type_specifier))
          type = signal_declaration->type_specifier->type_value;
     if (first)
     {
          first->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_integer );
          fprintf(stderr, "NYI:c_co_lvar::cross_reference_signals_and_constants_within_type_context:indexing changes the type - it must be an array, and it goes to the non-array variant\n");
     }
     if (second)
     {
          second->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_integer );
     }

}

/*a c_co_module
 */
/*f check_valid_clock
     check signal exists and is a clock
     return 0 on success, else reason for failure (1->does not exist, 2->exists but is not a clock)
 */
static int check_valid_clock( t_co_scope *local_signals, int number_of_signals, t_symbol *symbol )
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
static int check_valid_reset( c_cyclicity *cyclicity, t_co_scope *local_signals, int number_of_signals, t_symbol *symbol )
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
     if ( (!local_signals->co[i].co_signal_declaration->type_specifier) ||
          (cyclicity->type_value_pool->find_type(local_signals->co[i].co_signal_declaration->type_specifier) != type_value_bit) )
     {
          return 3;
     }
     return 0;
}

/*f c_co_module::cross_reference_build_signal_declaration_array
 first, count ports and locals, then generate array
 second, add signal_declarations from the ports to the array checking for duplicates
 third, add signal_declarations from the locals to the array checking for duplicates
  a duplicate where the first is an output port and the second is a clock is legal if they match - merge them
 while doing this, keep a watch out for default clocks and resets, and add them in - they must exist, in the input port list
  no default clock or reset for a clocked signal which has no clock or reset is an error
 */
void c_co_module::cross_reference_build_signal_declaration_array( class c_cyclicity *cyclicity, t_co_scope *global_constants )
{
     int i, j, max_signals, number_of_signals;

     c_co_signal_declaration *cosd;
     c_co_module_body_entry *combe;
     c_co_clock_reset_defn *default_clock, *default_reset;

     /*b First count the signals
      */
     number_of_signals = 0;
     for (cosd = ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list)
     {
          number_of_signals++;
     }
     for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
     {
          if (combe->type == module_body_entry_type_signal_declaration)
          {
               number_of_signals++;
          }
     }

     /*b Allocate arrays
      */
     max_signals = number_of_signals;
     local_signals = create_scope( max_signals );
     local_signals->next_scope = global_constants;

     /*b Add ports to the array from the signals, checking for duplicates
      */
     number_of_signals = 0;
     for (cosd = ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list)
     {
          i = find_symbol_in_scope( local_signals->co, cosd->signal_id );
          if (i>=0)
          {
               cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Duplicate port name '%s' in module '%s'", lex_string_from_terminal( cosd->signal_id ), lex_string_from_terminal( this->id ));
          }
          else
          {
               local_signals->co[number_of_signals].co_signal_declaration = cosd;
               number_of_signals++;
          }
     }

     /*b Add ports to the array from the locals, checking for duplicates, default clocks and default resets
      */
     default_clock = NULL;
     default_reset = NULL;
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
                    i = find_symbol_in_scope( local_signals->co, cosd->signal_id );
                    j = number_of_signals;
                    if (i>=0)
                    {
                         // check the first is an output port and this is a clocked, in which case just continue
                         if ( (local_signals->co[i].co_signal_declaration->signal_declaration_type != signal_declaration_type_comb_output) ||
                              (cosd->signal_declaration_type != signal_declaration_type_clocked_local) ||
                              (!cyclicity->type_value_pool->compare_types( local_signals->co[i].co_signal_declaration->type_specifier,
                                                                     cosd->type_specifier )) )
                         {
                              j = -1;
                              cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Bad redefinition with signal '%s' of port '%s'", lex_string_from_terminal( cosd->signal_id ), lex_string_from_terminal( local_signals->co[i].co_signal_declaration->signal_id ));
                         }
                         else
                         {
                              j = i;
                              local_signals->co[j].co_signal_declaration->signal_declaration_type = signal_declaration_type_clocked_output;
                              local_signals->co[j].co_signal_declaration->data.clocked.clock_spec = cosd->data.clocked.clock_spec;
                              local_signals->co[j].co_signal_declaration->data.clocked.reset_spec = cosd->data.clocked.reset_spec;
                              local_signals->co[j].co_signal_declaration->data.clocked.reset_value = cosd->data.clocked.reset_value;
                              local_signals->co[j].co_signal_declaration->co_link(co_compile_stage_cross_reference, (c_cyc_object *)cosd->data.clocked.clock_spec, "clock" );
                              local_signals->co[j].co_signal_declaration->co_link(co_compile_stage_cross_reference, (c_cyc_object *)cosd->data.clocked.reset_spec, "reset" );
                              local_signals->co[j].co_signal_declaration->co_link(co_compile_stage_cross_reference, (c_cyc_object *)cosd->data.clocked.reset_value, "reset_value" );
                         }
                    }
                    else
                    {
                         local_signals->co[j].co_signal_declaration = cosd;
                    }
                    if (j>=0)
                    {
                         // patch a clocked signal which does not have a reset or clock - error if neither
                         if (cosd->signal_declaration_type == signal_declaration_type_clocked_local)
                         {
                              if (!local_signals->co[j].co_signal_declaration->data.clocked.clock_spec)
                              {
                                   local_signals->co[j].co_signal_declaration->data.clocked.clock_spec = default_clock;
                              }
                              if (!local_signals->co[j].co_signal_declaration->data.clocked.reset_spec)
                              {
                                   local_signals->co[j].co_signal_declaration->data.clocked.reset_spec = default_reset;
                              }
                              if ( (!local_signals->co[j].co_signal_declaration->data.clocked.clock_spec) ||
                                   (!local_signals->co[j].co_signal_declaration->data.clocked.reset_spec) )
                              {
                                   cyclicity->set_parse_error( (c_cyc_object *)cosd, co_compile_stage_cross_reference, "Clocked signal '%s' does not have both clock and reset (default or otherwise)", lex_string_from_terminal( cosd->signal_id ) );
                              }
                         }
                         if (j==number_of_signals)
                              number_of_signals++;
                    }
               }
               break;
          /*b Default clock
           */
          case module_body_entry_type_default_clock_defn:
               i = check_valid_clock( local_signals, number_of_signals, combe->data.default_clock_defn->symbol );
               if (i==1)
               {
                    cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_clock_defn, co_compile_stage_cross_reference, "Use of a non-port '%s' as a clock in module '%s'", lex_string_from_terminal( combe->data.default_clock_defn->symbol ), lex_string_from_terminal( this->id ));
               }
               else if (i==2)
               {
                    cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_clock_defn, co_compile_stage_cross_reference, "Use of a non-clock '%s' as a clock in module '%s'", lex_string_from_terminal( combe->data.default_clock_defn->symbol ), lex_string_from_terminal( this->id ));
               }
               else
               {
                    default_clock = combe->data.default_clock_defn;
               }
               break;
          /*b Default reset
           */
          case module_body_entry_type_default_reset_defn:
               i = check_valid_reset( cyclicity, local_signals, number_of_signals, combe->data.default_reset_defn->symbol );
               if (i==1)
               {
                    cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-port '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->id ));
               }
               else if (i==2)
               {
                    cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-input '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->id ));
               }
               else if (i==3)
               {
                    cyclicity->set_parse_error( (c_cyc_object *)combe->data.default_reset_defn, co_compile_stage_cross_reference, "Use of a non-single bit input '%s' as a reset in module '%s'", lex_string_from_terminal( combe->data.default_reset_defn->symbol ), lex_string_from_terminal( this->id ));
               }
               else
               {
                    default_reset = combe->data.default_reset_defn;
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

/*f c_co_module::cross_reference
 cross reference types of ports
 cross reference module body contents
 cross reference types of signal_declarations in the body
 cross reference expressions with local and global scope
 cross reference lvars with local and global scope
 cross reference module/bus instantiation port_maps with local and global scope
 */
void c_co_module::cross_reference( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types, t_co_scope *variables )
{
     c_co_module_body_entry *combe;
     c_co_signal_declaration *cosd;

     if (ports)
     {
          ports->cross_reference( cyclicity, types, variables );
     }
     for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
     {
          switch (combe->type)
          {
          case module_body_entry_type_signal_declaration:
               cosd = combe->data.signal_declaration;
               if (cosd)
               {
                    cosd->cross_reference( cyclicity, types, variables );
               }
               break;
          case module_body_entry_type_instantiation:
               if (combe->data.bus_or_inst)
               {
                    combe->data.bus_or_inst->cross_reference_port_map( cyclicity, modules, types, variables );
               }
               break;
          case module_body_entry_type_statement:
               // bind for the statement
               if (combe->data.statement)
               {
                    combe->data.statement->cross_reference( cyclicity, types, variables );
               }
               break;
          case module_body_entry_type_code_label: // no cross referencing required
          case module_body_entry_type_default_clock_defn: // handled in building the local signal array
          case module_body_entry_type_default_reset_defn: // handled in building the local signal array
          case module_body_entry_type_schematic_symbol:
               // Done...
               break;
          }
     }
}

/*a c_co_nested_assignment
 */
/*f c_co_nested_assignment::cross_reference_within_type_context
  The nested assignment should consist of a set of elements (of the specified type) and expressions
  We need to check the element is of the specified type, and check the expression matches the type of that element
  Note that the whole assignment may be nested again.
 */
void c_co_nested_assignment::cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type )
{
     int element;
     t_type_value element_type;

     if (symbol)
     {
         element = cyclicity->type_value_pool->find_structure_element( type, symbol );
         if (element<0)
         {
             fprintf(stderr,"NNE:c_co_nested_assignment::cross_reference_within_type_context:Check symbol '%s' is in the structure '%d' - got %d\n", lex_string_from_terminal(na->symbol), type, element );
         }
     }
     if (nested_assignment)
     {
         nested_assignment->cross_reference_within_type_context( cyclicity, types, variables, type );
     }
     if (expression)
     {
         element_type = type_value_undefined;
         if (element>=0)
         {
             element_type = cyclicity->type_value_pool->get_structure_element_type( type, na->element );
         }
         expression->cross_reference_within_type_context( cyclicity, types, variables, element_type );
     }
     if (next_in_list)
         ((c_co_nested_assignment *)na->next_in_list)->cross_reference_within_type_context( cyclicity, types, variables, type );
}

/*a c_co_port_map
 */
/*f c_co_port_map::cross_reference
 */
int c_co_port_map::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (next_in_list)
    {
        return ((c_co_port_map *)next_in_list)->cross_reference( cyclicity, types, variables );
    }
    return 1;
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::cross_reference
 */
void c_co_signal_declaration::cross_reference( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (signal_declaration_type == signal_declaration_type_bus)
    {
        printf("NYI:c_co_signal_declaration::cross_reference_types:Need to cross reference the bus\n");
    }
    if (type_specifier)
    {
        type_specifier->cross_reference( cyclicity, types, variables );
    }

     /*b If this is a clocked signal, cross reference symbols within the reset value or values
      */
     if ( (signal_declaration_type == signal_declaration_type_clocked_output) ||
          (signal_declaration_type == signal_declaration_type_clocked_local) )
     {
          /*b If there is a list of reset values (nested assignment), then cross reference that, noting the type of this signal
           */
          if ((data.clocked.reset_value) && (type_specifier))
          {
               data.clocked.reset_value->cross_reference( cyclicity, types, variables, type_specifier->type_value );
          }
     }
     if (next_in_list)
     {
         ((c_co_signal_declaration *)next_in_list)->cross_reference( cyclicity, types, variables );
     }
}

/*a c_co_statement
 */
/*f cross_reference
  Cross reference a statement and any following statements in its list
  For an assignment, this means:
    Cross reference the lvar, and get its type
    Cross reference the expression using that type
    Check the assignment is approriately clocked
 */
void c_co_statement::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
     switch (statement_type)
     {
     case statement_type_assignment:
     {
          c_co_lvar *lvar;
          lvar = type_data.assign_stmt.lvar;
          if (lvar)
          {
               lvar->cross_reference( cyclicity, types, variables );
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
                    case signal_declaration_type_bus:
                    case signal_declaration_type_input:
                    case signal_declaration_type_clock:
                    case signal_declaration_type_parameter:
                         cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Assignment to a non-port '%s'", lex_string_from_terminal( lvar->symbol ) );
                         break;
                    }
                    if (cyclicity->type_value_pool->derefs_to_bit_vector( lvar->type ))
                    {
                         if (type_data.assign_stmt.nested_assignment)
                         {
                              type_data.assign_stmt.nested_assignment->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_data.assign_stmt.lvar->type );
                         }
                    }
                    else if ( cyclicity->type_value_pool->is_array( lvar->type ) )
                    {
                         cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Assignment to an array not supported '%s'", lex_string_from_terminal( lvar->symbol ) );
                    }
                    else
                    {
                         if (type_data.assign_stmt.nested_assignment)
                         {
                              type_data.assign_stmt.nested_assignment->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_data.assign_stmt.lvar->type );
                         }
                    }
               }
          }
          break;
     }
     case statement_type_if_statement:
          if (type_data.if_stmt.expression)
          {
               type_data.if_stmt.expression->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_bool );
          }
          if (type_data.if_stmt.if_true)
          {
               type_data.if_stmt.if_true->cross_reference( cyclicity, types, variables );
          }
          if (type_data.if_stmt.if_false)
          {
               type_data.if_stmt.if_false->cross_reference( cyclicity, types, variables );
          }
          if (type_data.if_stmt.elsif)
          {
               type_data.if_stmt.elsif->cross_reference( cyclicity, types, variables );
          }
          break;
     case statement_type_switch_statement:
          if (type_data.switch_stmt.expression)
          {
               type_data.switch_stmt.expression->cross_reference_signals_and_constants_within_type_context( cyclicity, types, variables, type_value_undefined );
               if (type_data.switch_stmt.cases)
               {
                    type_data.switch_stmt.cases->cross_reference_signals_and_constants( cyclicity, types, variables, type_data.switch_stmt.expression->type );
               }
          }
          break;
     case statement_type_block:
          if (type_data.block)
               type_data.block->cross_reference( cyclicity, types, variables );
          break;
     }
     if (next_in_list)
          ((c_co_statement *)next_in_list)->cross_reference( cyclicity, types, variables );
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


