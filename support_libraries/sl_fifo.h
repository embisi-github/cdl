/*a Copyright
  
  This file 'sl_fifo.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_FIFO
#else
#define __INC_SL_FIFO

/*a Includes
 */

/*a Defines
 */

/*a Types
 */
/*t t_sl_fifo
 */
typedef struct t_sl_fifo
{
    int entry_int_size; // Number of ints that each entry consists of
    int size; // Number of items in the FIFO max
    int nearly_empty_watermark; // Number of entries there should be less than or equal to for nearly empty
    int nearly_full_watermark; // Number of spaces there should be less than or equal to for nearly full

    int read_ptr; // 0 to size-1, indicating the next entry to be read
    int committed_read_ptr; // 0 to size-1, committed on commits only
    int write_ptr; // 0 to size-1, indicating the next entry to be written
    int empty; // 1 if the FIFO is empty, 0 if not
    int full; // 1 if the FIFO is full, 0 if not
    int overflowed; // 1 if the FIFO has been written to when full
    int underflowed; // 1 if the FIFO has been read when empty
    int nearly_empty;
    int nearly_full;

    t_sl_uint64 *contents; // Contents, if any

} t_sl_fifo;

/*a Fifo functions
 */
extern void       sl_fifo_init( t_sl_fifo *fifo, int byte_size, int size, int nearly_empty_watermark, int nearly_full_watermark );
extern t_sl_fifo *sl_fifo_create( int byte_size, int size, int nearly_empty_watermark, int nearly_full_watermark, int include_data );
extern void       sl_fifo_free( t_sl_fifo *fifo );
extern void       sl_fifo_reset( t_sl_fifo *fifo );
extern int        sl_fifo_add_entry( t_sl_fifo *fifo, void *data_ptr );
extern int        sl_fifo_remove_entry( t_sl_fifo *fifo, void *data_buffer );
extern void       sl_fifo_commit_reads( t_sl_fifo *fifo );
extern void       sl_fifo_revert_reads( t_sl_fifo *fifo );
extern int        sl_fifo_flags( t_sl_fifo *fifo, int *empty, int *full, int *nearly_empty, int *nearly_full, int *underflowed, int *overflowed );
extern int        sl_fifo_is_empty( t_sl_fifo *fifo );
extern int        sl_fifo_count( t_sl_fifo *fifo, int entries ); // Count entires or spaces - entries==1 for entries
extern void       sl_fifo_ptrs( t_sl_fifo *fifo, int *read_ptr, int *committed_read_ptr, int *write_ptr );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/
