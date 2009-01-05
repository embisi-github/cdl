/*a Copyright
  
  This file 'c_co_chain.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_co_cmodel_eq.h"
#include "c_co_code_label.h"
#include "c_co_constant_declaration.h"
#include "c_co_enum_definition.h"
#include "c_co_enum_identifier.h"
#include "c_co_expression.h"
#include "c_co_fsm_state.h"
#include "c_co_lvar.h"
#include "c_co_module_body_entry.h"
#include "c_co_module_prototype_body_entry.h"
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

/*a Defines
 */
#define CHAIN_TAIL(class_name) \
class_name *class_name::chain_tail( class_name *entry ) \
{ \
    if (!this) return NULL; \
    if (!this->last_in_list) return NULL; \
    this->last_in_list->next_in_list = (class c_cyc_object *)entry; \
    this->last_in_list = (class c_cyc_object *)entry;                   \
    return this;                                                            \
}

/*a chain_tails
 */
CHAIN_TAIL( c_co_case_entry );
CHAIN_TAIL( c_co_enum_identifier );
CHAIN_TAIL( c_co_expression );
CHAIN_TAIL( c_co_fsm_state );
CHAIN_TAIL( c_co_module_prototype_body_entry );
CHAIN_TAIL( c_co_module_body_entry );
CHAIN_TAIL( c_co_nested_assignment );
CHAIN_TAIL( c_co_named_expression );
CHAIN_TAIL( c_co_port_map );
CHAIN_TAIL( c_co_schematic_symbol_body_entry );
CHAIN_TAIL( c_co_signal_declaration );
CHAIN_TAIL( c_co_sized_int_pair );
CHAIN_TAIL( c_co_statement );
CHAIN_TAIL( c_co_token_union );
CHAIN_TAIL( c_co_toplevel );
CHAIN_TAIL( c_co_type_struct );


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

