/*a Copyright
  
  This file 'c_co_type_specifier.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_TYPE_SPECIFIER
#else
#define __INC_C_CO_TYPE_SPECIFIER

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"
#include "c_type_value_pool.h"

/*a Types
 */
/*t t_system_type
 */
typedef enum
{
    system_type_none,
    system_type_bit,
    system_type_integer,
    system_type_string,
} t_system_type;

/*t c_co_type_specifier
 */
class c_co_type_specifier: public c_cyc_object
{
public:
    c_co_type_specifier( c_cyclicity *cyc, t_symbol *symbol);
    c_co_type_specifier( c_co_type_specifier *type_specifier, class c_co_expression *first, class c_co_expression *last );
    ~c_co_type_specifier();

    int cross_reference( t_co_scope *types ); // Resolve type reference either as system or as user type
    void cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables ); // Resolve cross-references in indices and type references
    void evaluate( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

    t_symbol *symbol;
    class c_co_type_specifier *type_specifier;
    class c_co_expression *first;
    class c_co_expression *last;

    class c_co_type_definition *refers_to; // filled in at cross-referencing - matches symbol to a type_definition (if not bit, integer, string etc)
    t_type_value type_value; // type_value of this type specifier

};

/*a Functions
 */

/*a Wrapper
 */
#endif



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



