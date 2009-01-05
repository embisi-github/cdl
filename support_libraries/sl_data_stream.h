/*a Copyright
  
  This file 'sl_data_stream.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_DATA_STREAM
#else
#define __INC_SL_DATA_STREAM

/*a Includes
 */

/*a Defines
 */

/*a Types
 */
typedef struct t_sl_data_stream;

/*a Functions
 */
extern void sl_data_stream_start_packet( t_sl_data_stream *ds );
extern void sl_data_stream_next_data_word( t_sl_data_stream *ds );
extern void sl_data_stream_next_data_byte( t_sl_data_stream *ds );
extern int sl_data_stream_args( t_sl_data_stream *ds, unsigned int **args );
extern t_sl_data_stream *sl_data_stream_create( char *option_string );
extern int sl_data_stream_packet_length( t_sl_data_stream *ds );
extern unsigned int sl_data_stream_packet_header( t_sl_data_stream *ds );
extern unsigned int sl_data_stream_packet_data_word( t_sl_data_stream *ds );
extern unsigned int sl_data_stream_packet_data_byte( t_sl_data_stream *ds );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

