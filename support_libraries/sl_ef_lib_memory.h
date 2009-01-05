/*a Copyright
  
  This file 'sl_ef_lib_memory.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_EF_LIB_MEMORY
#else
#define __INC_SL_EF_LIB_MEMORY

/*a Includes
 */
#include "sl_general.h"

/*a Defines
 */

/*a Types
 */
/*t t_sl_ef_lib_memory
 */
typedef struct t_sl_ef_lib_memory
{
     t_sl_uint64 *contents;
     int bit_size;
     int max_size;
} t_sl_ef_lib_memory;

/*a External functions
 */
/*b Exec file fns
 */
extern void sl_ef_lib_memory_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data_ptr );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

