/*a Copyright
  
  This file 'model_list.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_MODEL_LIST
#else
#define __INC_MODEL_LIST

/*a Types
 */
/*t t_python_init_fn
 */
typedef void (*t_python_init_fn)(void);

/*a Global variables
 */
/*v model_init_fns
 */
extern t_python_init_fn model_init_fns[];

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

