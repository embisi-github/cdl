/*a Copyright
  
  This file 'c_co_enum_identifier.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_ENUM_IDENTIFIER
#else
#define __INC_C_CO_ENUM_IDENTIFIER

/*a Includes
 */
#include "lexical_types.h"
#include "c_type_value_pool.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_enum_identifier
 */
class c_co_enum_identifier: public c_cyc_object
{
public:
    c_co_enum_identifier( t_symbol *text, class c_co_expression *expression );
    ~c_co_enum_identifier();
    c_co_enum_identifier *chain_tail( c_co_enum_identifier *entry );

    void cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, class c_co_type_definition *type_def );
    int evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type );

    t_symbol *symbol;
    class c_co_expression *expression;

    t_type_value type_value; // Resolved value of the enumeration identifier, with appropriate bit vector type_value
    class c_co_type_definition *type_def; // Type definition that this enumeration identifier is part of, filled in at final cross referencing time
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



