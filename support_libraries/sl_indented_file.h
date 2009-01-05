/*a Copyright
  
  This file 'sl_indented_file.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_INDENTED_FILE
#else
#define __INC_SL_INDENTED_FILE

/*a Includes
 */
#include "c_sl_error.h"
#include "sl_cons.h"

/*a Defines
 */

/*a Types
 */
/*t t_sl_indented_file_cmd
 */
typedef struct t_sl_indented_file_cmd
{
     const char *cmd;
     const char *args;
     const char *syntax;
     int takes_items;
} t_sl_indented_file_cmd;

/*a External functions
 */
extern t_sl_error_level sl_allocate_and_read_indented_file( c_sl_error *error, const char *filename, const char *user, t_sl_indented_file_cmd *file_cmds, t_sl_cons_list *cl );

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

