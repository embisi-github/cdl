/*a Copyright
  
  This file 'c_co_declspec.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_DECLSPEC
#else
#define __INC_C_CO_DECLSPEC

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_declspec_type
 */
typedef enum
{
    declspec_type_async_read,
} t_declspec_type;

/*t c_co_declspec
 */
class c_co_declspec: public c_cyc_object
{
public:
    c_co_declspec( t_declspec_type declspec_type, t_symbol *clock_symbol );
    ~c_co_declspec();
    c_co_declspec *chain_tail( c_co_declspec *entry );
    int has_declspec_type( t_declspec_type declspec_type );

    t_symbol *symbol; // Type of the declspec
    t_declspec_type declspec_type; // Enumeration

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



