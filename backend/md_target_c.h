/*a Copyright
  
  This file 'md_target_c.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_MD_TARGET_C
#else
#define __INC_MD_TARGET_C

/*a Includes
 */
#include "c_model_descriptor.h"

/*a External functions
 */
extern void target_c_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle, int include_assertions, int include_coverage, int include_stmt_coverage );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

