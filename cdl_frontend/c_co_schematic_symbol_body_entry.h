/*a Copyright
  
  This file 'c_co_schematic_symbol_body_entry.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_SCHEMATIC_SYMBOL_BODY_ENTRY
#else
#define __INC_C_CO_SCHEMATIC_SYMBOL_BODY_ENTRY

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_symbol_object_type
 */
typedef enum
{
     symbol_object_type_port,
     symbol_object_type_line,
     symbol_object_type_fill,
     symbol_object_type_oval,
     symbol_object_type_option,
} t_symbol_object_type;

/*t c_co_schematic_symbol_body_entry
 */
class c_co_schematic_symbol_body_entry: public c_cyc_object
{
public:
     c_co_schematic_symbol_body_entry( t_symbol_object_type type, t_symbol *name, class c_co_sized_int_pair *coords, t_string *documentation );
     c_co_schematic_symbol_body_entry( t_symbol_object_type type, class c_co_sized_int_pair *coords, t_string *documentation );
     c_co_schematic_symbol_body_entry( t_symbol_object_type type, class c_co_sized_int_pair *coord_0, class c_co_sized_int_pair *coord_1, t_string *documentation );
     c_co_schematic_symbol_body_entry( t_symbol_object_type type, t_symbol *name, t_sized_int *sized_int, t_string *documentation );
     ~c_co_schematic_symbol_body_entry();
     c_co_schematic_symbol_body_entry *chain_tail( c_co_schematic_symbol_body_entry *entry );

     t_symbol_object_type type;
     t_string *documentation;
     t_symbol *text_id;
     t_sized_int *sized_int;
     class c_co_sized_int_pair *first_coords;
     class c_co_sized_int_pair *second_coords;

};

/*a Wrapper
 */
#endif



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



