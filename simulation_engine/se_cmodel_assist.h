/*a Copyright
  
  This file 'se_cmodel_assist.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_SE_CMODEL_ASSIST
#else
#define __INC_SE_CMODEL_ASSIST

/*a Includes
 */
#include "sl_general.h"
/*a Defines
 */
#define se_cmodel_assist_stmt_coverage_register(se,se_handle,data) se->coverage_stmt_register(se_handle,(void *)(&data),sizeof(data))
#define se_cmodel_assist_stmt_coverage_statement_reached(coverage,number) {coverage.map[number]=(coverage.map[number]==255)?255:coverage.map[number]+1;}
#define se_cmodel_assist_code_coverage_register(se,se_handle,data) se->coverage_code_register(se_handle,(void *)(&data),sizeof(data))
#define se_cmodel_assist_code_coverage_case_already_reached(coverage,c,b) (!((coverage.bitmap[c]>>(b))&1))
#define se_cmodel_assist_code_coverage_case_now_reached(coverage,c,b) {coverage.bitmap[c]|=(1<<(b));}

/*a Function declarations
 */
extern void se_cmodel_assist_assign_to_bit( t_sl_uint64 *vector, int size, int bit, unsigned int value);
extern void se_cmodel_assist_assign_to_bit_range( t_sl_uint64 *vector, int size, int bit, int length, t_sl_uint64 value);
extern char *se_cmodel_assist_vsnprintf( char *buffer, int buffer_size, const char *format, int num_args, ... );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

