/*a Copyright
  
  This file 'c_co_code_label.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_CODE_LABEL
#else
#define __INC_C_CO_CODE_LABEL

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_code_label
 */
class c_co_code_label: public c_cyc_object
{
public:
	 c_co_code_label( t_symbol *symbol, t_string *documentation, class c_co_statement *statements );
	 ~c_co_code_label();

	 struct t_symbol *label;
	 struct t_string *documentation;
     class c_co_statement *statements; // List of statements that make up the code for this code_label

	 class c_co__module *module;				// filled in at binding stage
	 class c_co_code_label *next_in_module;	// filled in at binding stage

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



