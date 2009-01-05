/*a Copyright
  
  This file 'ef_errors.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_EF_ERRORS
#else
#define __INC_EF_ERRORS

/*a Includes
 */
#include "c_sl_error.h"

/*a Error number enumerations
 */
/*t error_number_ef_*
 */
enum
{
     error_number_ef_stack_not_empty = error_number_base_ef,
     error_number_ef_stack_underflow,
     error_number_ef_uncaught_error,
};

/*t error_id_be_*
 */
enum
{
     error_id_ef_frontend_main,
};

/*a Error messages (default)
 */
/*v default error messages
 */
#define C_EF_ERROR_DEFAULT_MESSAGES \
{ error_number_ef_stack_not_empty,        "%s0 stack not empty at completion of file %f" }, \
{ error_number_ef_stack_underflow,        "%s0 stack underflow at line %l of file %f" }, \
{ error_number_ef_uncaught_error,         "ef frontend execution error at line %l of file %f" }, \
{ -1, NULL }

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

