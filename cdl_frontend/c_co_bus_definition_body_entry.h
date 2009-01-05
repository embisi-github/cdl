/*a Copyright
  
  This file 'c_co_bus_definition_body_entry.h' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
#include "lexical_types.h"
#include "c_cyc_object.h"

/*t bus_definition_body_entry_type
 */
typedef enum
{
     bus_definition_body_entry_type_include,
     bus_definition_body_entry_type_function_call,
     bus_definition_body_entry_type_function_prototype,
} t_bus_definition_body_entry_type;

/*t c_co_bus_definition_body_entry
 */
class c_co_bus_definition_body_entry: public c_cyc_object
{
public:
     c_co_bus_definition_body_entry::c_co_bus_definition_body_entry( t_string *filename ); // cmodel_binding_include
     c_co_bus_definition_body_entry::c_co_bus_definition_body_entry( t_symbol *function_type, t_symbol *function_name, class c_co_token_union *args ); //  cmodel_binding_function_call
     c_co_bus_definition_body_entry::c_co_bus_definition_body_entry( t_symbol *function_type, class c_co_token_union *args ); // cmodel_binding_function_prototype
     c_co_bus_definition_body_entry::~c_co_bus_definition_body_entry();
     c_co_bus_definition_body_entry *c_co_bus_definition_body_entry::chain_tail( c_co_bus_definition_body_entry *entry );

     t_bus_definition_body_entry_type type;
     t_string *string;
     t_symbol *symbol_1;
     t_symbol *symbol_2;
     class c_co_token_union *token_union;

};



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



