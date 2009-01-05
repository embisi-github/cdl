/*a Copyright
  
  This file 'sl_fifo.cpp' copyright Embisi 2003, 2004
  
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_general.h"
#include "sl_fifo.h"
#include "sl_exec_file.h"

/*a Defines
 */

/*a Types
 */
/*t t_sl_fifo_chain
 */
typedef struct t_sl_fifo_chain
{
    t_sl_fifo_chain *next_in_list;
    char *name;
    t_sl_fifo *fifo;
} t_sl_fifo_chain;

/*a Fifo functions
 */
/*f sl_fifo_init
 */
extern void sl_fifo_init( t_sl_fifo *fifo, int byte_size, int size, int nearly_empty_watermark, int nearly_full_watermark )
{
    int entry_int_size;
    if (!fifo) return;

    entry_int_size = (byte_size+3)/4;
    fifo->size = size;
    fifo->entry_int_size = entry_int_size;
    fifo->nearly_empty_watermark = nearly_empty_watermark;
    fifo->nearly_full_watermark = nearly_full_watermark;
    sl_fifo_reset( fifo );
}

/*f sl_fifo_create
  Return NULL for failure - malloc failed
  Return t_sl_fifo * for success
 */
extern t_sl_fifo *sl_fifo_create( int byte_size, int size, int nearly_empty_watermark, int nearly_full_watermark, int include_data )
{
    t_sl_fifo *fifo;
    int entry_int_size;

    entry_int_size = (byte_size+3)/4;
    fifo = (t_sl_fifo *)malloc(sizeof(t_sl_fifo) + (include_data?sizeof(unsigned int)*size*entry_int_size:0));
    if (!fifo) return NULL;
    fifo->size = size;
    fifo->entry_int_size = entry_int_size;
    fifo->nearly_empty_watermark = nearly_empty_watermark;
    fifo->nearly_full_watermark = nearly_full_watermark;
    fifo->contents = include_data?((t_sl_uint64 *)&(fifo[1])):NULL; // If it includes data, the data is at the end
    sl_fifo_reset( fifo );
    return fifo;
}

/*f sl_fifo_free
 */
extern void sl_fifo_free( t_sl_fifo *fifo )
{
    if (fifo) free(fifo);
}

/*f sl_fifo_update_flags
  Update the pointers and flags so they are within bounds and correct
  If the pointers are equal then the full_hint tells whether the FIFO is full or empty
  If adding to the FIFO, full_hint should b 1; if removing, it should be 0; if no adjustments were made, use the full flag
 */
static void sl_fifo_update_flags( t_sl_fifo *fifo, int full_hint )
{
    int committed_entries;  // Number of entries in the FIFO as seen by write side
    int uncommitted_entries; // Number of entries in the FIFO as seen by read side
    fifo->read_ptr = fifo->read_ptr%fifo->size;
    fifo->committed_read_ptr = fifo->committed_read_ptr%fifo->size;
    fifo->write_ptr = fifo->write_ptr%fifo->size;
    committed_entries = (fifo->write_ptr-fifo->committed_read_ptr+fifo->size)%fifo->size; // This is the larger of the two - if zero, then use full_hint
    uncommitted_entries = (fifo->write_ptr-fifo->read_ptr+fifo->size)%fifo->size; // This is fewer or same as committed
    if (committed_entries==0) // ptrs equal - full if we were full or if we added, which should be in the hint
    {
        fifo->full = full_hint;
        fifo->nearly_full = full_hint;
    }
    else
    {
        fifo->full = 0;
        fifo->nearly_full = (committed_entries>=fifo->size-fifo->nearly_full_watermark);
    }
    if (uncommitted_entries==0) // we do not permit be empty while full, as that requires uncommitted reads of max size, which is beyond the scope here.
    {
        fifo->empty = !fifo->full; // If full we cannot be empty; if not full then we must be empty
        fifo->nearly_empty = !fifo->full;
    }
    else
    {
        fifo->empty = 0;
        fifo->nearly_empty = (uncommitted_entries<=fifo->nearly_empty_watermark);
    }
}

/*f sl_fifo_reset
 */
extern void sl_fifo_reset( t_sl_fifo *fifo )
{
    if (!fifo) return;
    fifo->read_ptr = 0;
    fifo->committed_read_ptr = 0;
    fifo->write_ptr = 0;
    fifo->overflowed = 0;
    fifo->underflowed = 0;
    sl_fifo_update_flags( fifo, 0 );
}

/*f sl_fifo_add_entry 
  Returns -1 if the addition was not possible (FIFO already full)
  Returns write_ptr for success
  Adds the data to the FIFO
*/
extern int sl_fifo_add_entry( t_sl_fifo *fifo, void *data_ptr )
{
    int ptr;
    if (!fifo) return -1;
    if (fifo->full)
    {
        fifo->overflowed=1;
        return -1;
    }
    ptr = fifo->write_ptr;
    if (fifo->contents && data_ptr)
    {
        memcpy( ((char *)(fifo->contents))+fifo->entry_int_size*sizeof(int)*ptr, data_ptr, fifo->entry_int_size*sizeof(int) );
    }
    fifo->write_ptr++;
    sl_fifo_update_flags( fifo, 1 );
    return ptr;
}
  
/*f sl_fifo_remove_entry
  Returns -1 if the removal was not possible (FIFO already empty)
  Returns read_ptr for success
  Sets data
 */
extern int sl_fifo_remove_entry( t_sl_fifo *fifo, void *data_buffer )
{
    int ptr;
    if (!fifo) return 0;
    if (fifo->empty)
    {
        fifo->underflowed=1;
        return -1;
    }
    ptr = fifo->read_ptr;
    if (data_buffer && fifo->contents)
    {
        memcpy( data_buffer, ((char *)(fifo->contents))+fifo->entry_int_size*sizeof(int)*ptr, fifo->entry_int_size*sizeof(int) );
    }
    fifo->read_ptr++;
    sl_fifo_update_flags( fifo, fifo->full );
    return ptr;
}

/*f sl_fifo_commit_reads
 */
extern void sl_fifo_commit_reads( t_sl_fifo *fifo )
{
    if (!fifo) return;
    if (fifo->committed_read_ptr != fifo->read_ptr)
    {
        fifo->committed_read_ptr = fifo->read_ptr;
        sl_fifo_update_flags( fifo, 0 );
    }
}

/*f sl_fifo_revert_reads
 */
extern void sl_fifo_revert_reads( t_sl_fifo *fifo )
{
    if (!fifo) return;
    fifo->read_ptr = fifo->committed_read_ptr;
    sl_fifo_update_flags( fifo, fifo->full );
}

/*f sl_fifo_flags
  Sets flags if the ptrs are not NULL
 */
extern int sl_fifo_flags( t_sl_fifo *fifo, int *empty, int *full, int *nearly_empty, int *nearly_full, int *underflowed, int *overflowed )
{
    if (!fifo) return -1;
    if (empty) (*empty)=fifo->empty;
    if (full) (*full)=fifo->full;
    if (nearly_empty) (*nearly_empty)=fifo->nearly_empty;
    if (nearly_full) (*nearly_full)=fifo->nearly_full;
    if (underflowed) (*underflowed)=fifo->underflowed;
    if (overflowed) (*overflowed)=fifo->overflowed;

    return ( (fifo->empty<<0) |
             (fifo->full<<1) |
             (fifo->nearly_empty<<2) |
             (fifo->nearly_full<<3) |
             (fifo->underflowed<<4) |
             (fifo->overflowed<<5) );
}

/*f sl_fifo_is_empty
  Return 1 if empty, 0 if not
 */
extern int sl_fifo_is_empty( t_sl_fifo *fifo )
{
    if (!fifo) return -1;
    return fifo->empty;
}

/*f sl_fifo_commit_reads
 */
extern int sl_fifo_count( t_sl_fifo *fifo, int entries )
{
    int count;
    if (!fifo) return 0;
    count = (fifo->write_ptr - fifo->committed_read_ptr + fifo->size) % fifo->size; // write-read = num entries
    if (fifo->full) count=fifo->size;
    if (entries) // Count entries
    {
        return count;
    }
    // Return spaces = size-entries
    return fifo->size-count;
}

/*f sl_fifo_ptrs
 */
extern void sl_fifo_ptrs( t_sl_fifo *fifo, int *read_ptr, int *committed_read_ptr, int *write_ptr )
{
    if (!fifo) return;
    if (read_ptr) (*read_ptr)=fifo->read_ptr;
    if (committed_read_ptr) (*committed_read_ptr)=fifo->committed_read_ptr;
    if (write_ptr) (*write_ptr)=fifo->write_ptr;
}
