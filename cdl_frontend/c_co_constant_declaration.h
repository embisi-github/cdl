/*a Copyright
  
  This file 'c_co_constant_declaration.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_CONSTANT_DECLARATION
#else
#define __INC_C_CO_CONSTANT_DECLARATION

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"
#include "c_type_value_pool.h"

/*a Types
 */
/*t c_co_constant_declaration
 */
class c_co_constant_declaration: public c_cyc_object
{
public:
    c_co_constant_declaration( t_symbol *symbol, class c_co_type_specifier *type_specifier, class c_co_expression *expression, t_string *documentation );
    ~c_co_constant_declaration();

    void cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void override( t_type_value value );

    t_symbol *symbol; // Symbol with the name of the constant
    t_string *documentation; // Documentation for the constant
    class c_co_type_specifier *type_specifier; // Type of the constant
    class c_co_expression *expression; // Expression with the value of the constant, which should be constant at build time at the global level
    t_type_value type_value; // Value of the expression, evaluated after cross referencing and type checking
    int overridden;

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



