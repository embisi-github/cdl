/*a Copyright
  
  This file 'c_co_token_union.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_TOKEN_UNION
#else
#define __INC_C_CO_TOKEN_UNION

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_token_union_type
 */
typedef enum
{
	 token_union_type_none,
	 token_union_type_string,
	 token_union_type_sized_int,
	 token_union_type_symbol,
} t_token_union_type;

/*t c_co_token_union
 */
class c_co_token_union: public c_cyc_object
{
public:
	 c_co_token_union( t_symbol *symbol );
	 c_co_token_union( t_sized_int *sized_int );
	 c_co_token_union( t_string *string );
	 ~c_co_token_union();
	 c_co_token_union *chain_tail( c_co_token_union *entry );

	 t_token_union_type token_union_type;
	 union
	 {
		  t_symbol *symbol;
		  t_string *string;
		  t_sized_int *sized_int;
	 } token_union_data;
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



