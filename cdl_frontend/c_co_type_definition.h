/*a Copyright
  
  This file 'c_co_type_definition.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_TYPE_DEFINITION
#else
#define __INC_C_CO_TYPE_DEFINITION

/*a Includes
 */
#include "lexical_types.h"
#include "c_type_value_pool.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_type_definition
 */
class c_co_type_definition: public c_cyc_object
{
public:
    c_co_type_definition( t_symbol *symbol, class c_co_type_specifier *type_specifier );
    c_co_type_definition( t_symbol *symbol, class c_co_enum_definition *enum_definition );
    c_co_type_definition( t_symbol *symbol, class c_co_fsm_state *fsm_state );
    c_co_type_definition( t_symbol *symbol, class c_co_type_struct *type_struct );
    ~c_co_type_definition();

    void cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

    t_symbol *symbol;
    class c_co_type_specifier *type_specifier;
    class c_co_enum_definition *enum_definition;
    class c_co_fsm_state *fsm_state;
    class c_co_type_struct *type_struct;

    t_type_value type_value; // filled in at evaluation time, when a type for this is added to the type pool
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



