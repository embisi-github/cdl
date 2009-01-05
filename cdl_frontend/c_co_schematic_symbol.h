/*a Copyright
  
  This file 'c_co_schematic_symbol.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_SCHEMATIC_SYMBOL
#else
#define __INC_C_CO_SCHEMATIC_SYMBOL

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_schematic_symbol
 */
class c_co_schematic_symbol: public c_cyc_object
{
public:
	 c_co_schematic_symbol( t_string *variant, class c_co_schematic_symbol_body_entry *body );
	 ~c_co_schematic_symbol();

	 t_string *variant;
	 class c_co_schematic_symbol_body_entry *body;

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



