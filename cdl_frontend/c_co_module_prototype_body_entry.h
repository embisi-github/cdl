/*a Copyright
  
  This file 'c_co_module_prototype_body_entry.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_MODULE_PROTOTYPE_BODY_ENTRY
#else
#define __INC_C_CO_MODULE_PROTOTYPE_BODY_ENTRY

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_module_prototype_body_entry_type
 */
typedef enum
{
    module_prototype_body_entry_type_timing_spec,
} t_module_prototype_body_entry_type;

/*t c_co_module_prototype_body_entry
 */
class c_co_module_prototype_body_entry: public c_cyc_object
{
public:
    c_co_module_prototype_body_entry( class c_co_timing_spec *timing_spec );
    ~c_co_module_prototype_body_entry();
    c_co_module_prototype_body_entry *chain_tail( c_co_module_prototype_body_entry *entry );

    t_module_prototype_body_entry_type type;
    union
    {
        class c_co_timing_spec *timing_spec;
    } data;
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



