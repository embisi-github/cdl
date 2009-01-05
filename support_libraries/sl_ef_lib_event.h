/*a Copyright
  
  This file 'sl_ef_lib_fifo.h' copyright Gavin J Stark 2007, 2008
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_EF_LIB_EVENT
#else
#define __INC_SL_EF_LIB_EVENT

/*a Includes
 */
#include "sl_general.h"

/*a Defines
 */

/*a Types
 */
typedef struct t_sl_ef_lib_event *t_sl_ef_lib_event_ptr;

/*a External functions
 */
/*b Exec file fns
 */
extern void sl_ef_lib_event_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data_ptr );

/*b Event fns
 */
extern t_sl_ef_lib_event_ptr sl_ef_lib_event_create( char *name );
extern void sl_ef_lib_event_free( t_sl_ef_lib_event_ptr event );
extern int sl_ef_lib_event_reset( t_sl_ef_lib_event_ptr event );
extern int sl_ef_lib_event_fire( t_sl_ef_lib_event_ptr event );
extern int sl_ef_lib_event_fired( t_sl_ef_lib_event_ptr event );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

