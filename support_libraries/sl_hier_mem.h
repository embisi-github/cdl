/*a Copyright
  
  This file 'sl_hier_mem.h' copyright Gavin J Stark 2003, 2004, 2007
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_HIER_MEM
#else
#define __INC_SL_HIER_MEM

/*a Includes
 */
#include "sl_general.h"
#include "sl_mif.h"
#include "c_sl_error.h"

/*a Defines
 */
//d This define specifies the maximum number of levels of hierarchy
//d permissible in a memory. 8 is way overkill.
#define SL_HIER_MAX_LEVEL (8)

/*a Types
 */
/*t t_sl_hier_mem_unwritten_data -- enumeration of how uninitilialized data should be read
//d An enumeration of how an unwritten data word should be read back
//d when it is accessed. Currently no warnings are returned for such
//d accesses, but this should perhaps be added. The available types are
//d currently sl_hier_mem_unwritten_data_zero and
//d sl_hier_mem_unwritten_data_ones, which return all 0's or all
//d 1's for uninitialized data when it is read.
 */
typedef enum
{
    sl_hier_mem_unwritten_data_zero,
    sl_hier_mem_unwritten_data_ones
} t_sl_hier_mem_unwritten_data;

/*t t_sl_hier_mem_ptr -- internal type, see main code for use
 */
typedef union t_sl_hier_mem_ptr *t_sl_hier_mem_submem_ptr;

/*t t_sl_hier_mem_data_ptr -- basic type of pointer to data in the memory
//d A pointer to the data in the memory. A value of this type is
//d returned, for example, when you request the location of a particular
//d address in the hierarchical memory. It is defined as a pointer to
//d unsigned char, as the memory is assumed to be (at the smallest) an
//d array of bytes; the actual size of each address location is specified
//d at creation, though.
 */
typedef unsigned char *t_sl_hier_mem_data_ptr;

/*t t_sl_hier_mem_range -- a range in the memory return
//d When an iter is set up over a hierarchical memory instance this
//d structure is returned. It provides a start address and length of a
//d block of memory, and a pointer to the first entry in that memory.
 */
typedef struct t_sl_hier_mem_range
{
    t_sl_uint64 start_address;
    int length; // Number of addresses
    t_sl_hier_mem_data_ptr data; // Pointer to data
} t_sl_hier_mem_range;

/*t t_sl_hier_mem_ptr -- internal union of possible pointers
 */
typedef union t_sl_hier_mem_ptr
{
    t_sl_hier_mem_submem_ptr submem;
    t_sl_hier_mem_data_ptr   data; // NULL or points to mallocked area of size bytes_per_memory*page_size
} t_sl_hier_mem_ptr;

/*t t_sl_hier_mem -- The actual external type
//d  Single mallocked block, allocated using sl_hier_mem_create call
//d  Each level in the memory points to a t_sl_hier_submem_ptr array,
//d  unless page_shift[level+1] is 0, in which case it is t_sl_hier_mem_data_ptr
//d  However, if page_shift[0] is 0 then the top level has only one pointer in array[0]
//d  which is a t_sl_hier_mem_data_ptr - i.e. there is no hierarchy
//d  Always use sl_hier_mem_free to free this structure
 */
typedef struct t_sl_hier_mem
{
    t_sl_uint64 total_size;
    int bytes_per_memory; // 1, 2, 4, or 4n bytes
    int num_levels;
    int page_size[SL_HIER_MAX_LEVEL];  //d Number of address in each level (mask has size bits set, shift[n-1]-shift[n] = size[n])
    int page_shift[SL_HIER_MAX_LEVEL]; //d address shift right that each level's masked address required (bottom level has 0)
    t_sl_uint64 page_mask[SL_HIER_MAX_LEVEL]; //d address mask that each level is for, post-shift (bottom level if shift is 0)
    t_sl_hier_mem_unwritten_data unwritten_data_method;

    int bit_start; // Used internally for MIF file reading

    t_sl_hier_mem_ptr array[1];
} t_sl_hier_mem;

/*a Hierarchical memory functions
 */
/*f sl_hier_mem_create
//d Create a hierarchical memory of a given total size, with a hierarchy
//d layout specified. Each memory location will consume bytes_per_memory
//d bytes, and any access to unwrittend data will return data specified by
//d the unwritten_data_method.
//d
//d The log2_page_sizes is a null-terminated array of integers specifying
//d how the total size should be broken up in to hierarchical page
//d tables. The last non-null entry specifies how large the actual memory
//d blocks should be (bytes_per_memory<<log2_page_size) - these are the
//d leaves, as it were, of the page table tree. The entry before
//d that specifies how many leaves should be gathered together at the next
//d higher level, and so on back to the top of the tree. There should be
//d enough levels/bits in each level to accomodate the total_size
//d given. If there are too many, then the leading entries in the
//d log2_page_sizes will be reduced or ignored as necessary, so that the
//d leaf size is always that specified last in the array. A good array,
//d then, for any memory from 20 bits up in address size, could be
//d {14,10,10,10,10,10,0}; the entries are consumed from the right, so
//d leaves will have 1024 (2^10) memory addresses, and there will be 1024
//d of these lumped together, an 1024 of those above up to the required amount.
//d 
//d If the total size is less than the size of the leaf block, then no
//d hierarchy will be imposed, and a simple memory block will be used. So,
//d there is very little penalty for using the hierarchical memories in
//d pretty much every reasonable sized simulation memory, say from 16kB upwards.
//d 
//d The function returns NULL on failure or a pointer to the memory on
//d success. The memory must be freed with sl_hier_mem_free.
 */
extern t_sl_hier_mem *sl_hier_mem_create( t_sl_uint64 total_size,
                                          int *log2_page_sizes,
                                          t_sl_hier_mem_unwritten_data unwritten_data_method,
                                          int bytes_per_memory );

/*f sl_hier_mem_free
//d This call frees all the memory used by the hierarchical memory,
//d including the complete hierarchy.
 */
extern void sl_hier_mem_free( t_sl_hier_mem *shm );

/*f sl_hier_mem_read_mif
//d This file reads a MIF file in to the memory given
//d The MIF file reader uses a c_sl_error to handle errors, with a corresponding 'user' string
//d A MIF file can specify an arbitrary address space, and only those addresses in the MIF file
//d starting at address_offset will be loaded in to the t_sl_hier_mem. Basically, the address
//d in the t_sl_hier_mem is MIF address - address_offset
//d Data specified in the MIF file that is outside the address range of the t_sl_hier_mem will be
//d ignored silently if error_on_out_of_range is 0, and it will generate a serious error if
//d error_on_out_of_range is 1.
//d Additionally, only a particular bit range of each MIF file data can be written to the t_sl_hier_mem
//d This is so that a t_sl_hier_mem can contain, for example, just bytes 2 and 3 of a DRAM memory
//d and the MIF file to initialize the memory does not need to be split up.
//d For this, bit_start would be 16, and the t_sl_hier_mem would be 2 bytes wide (at creation).
//d Finally, a callback is provided for each word written to the t_sl_hier_mem from the MIF file
//d This callback has probably got some vital purpose, but it is not known what uses it as yet.
 */
extern t_sl_error_level sl_hier_mem_read_mif( t_sl_hier_mem *shm,
                                              c_sl_error *error,
                                              const char *filename,
                                              const char *user,
                                              t_sl_uint64 address_offset,
                                              int error_on_out_of_range,
                                              int bit_start,
                                              t_sl_mif_file_callback_fn callback,
                                              void *handle );
/*f sl_hier_mem_find
//d This call returns a pointer to where a particular memory location in
//d the hierarchical memory is stored. It returns NULL if the location is
//d outside the address range of the memory, or if the location has not
//d been allocated. However, if the 'alloc' arg is set to 1 then the
//d location will be allocated if possible and a pointer to this newly
//d allocated space returned.
*/
extern t_sl_hier_mem_data_ptr sl_hier_mem_find( t_sl_hier_mem *shm, t_sl_uint64 address, int alloc );

/*f sl_hier_mem_read
//d This call returns the value of a location in a hierarchical memory,
//d for anything hierarchical memoryp to 32-bits wide. If the location
//d accessed is outside the range of the memory then 0 is returned. If the
//d address has not yet been allocated and alloc is 0 then 0 is currently
//d returned; if alloc is 1 then the address will be allocated, and the
//d correct uninitialized value returned.
 */
extern unsigned int sl_hier_mem_read( t_sl_hier_mem *shm, t_sl_uint64 address, int alloc );

/*f sl_hier_mem_write
//d This call writes to a particular memory location in the hierarchical
//d memory. It will not write the location if it is outside the initially
//d specified memory size. It works for any number of bytes per memory
//d location. It will allocate the memory location if it has not already
//d been allocated.
 */
extern void sl_hier_mem_write( t_sl_hier_mem *shm, t_sl_uint64 address, t_sl_hier_mem_data_ptr data );

/*f sl_hier_mem_iter
//d This call iterates through a hierarchical memory giving the allocated
//d ranges of the memory complete with where their data contents are.
//d 
//d If on entry range.length is <=0 then the search will start at address
//d 0, otherwise it will start at range.address+1. This permits a full
//d enumeration of the allocated spaces of the memory, with the initial
//d call having range.length=-1, and subsequent calls requiring no change
//d to range from a previous call. The complete contents of range are
//d filled on return: start_address, length, and pointer to data. The
//d length will currently always be the same, as it is the size of the
//d leaf block of the hierarchy.
//d 
//d The call returns 0 if no more blocks have been allocated after the
//d specified range (e.g. it returns 0 if no blocks are allocated at all
//d and range.length was -1 on entry); it returns 1 if a range was filled
//d out with a block after the specified input range.address.
*/
extern int sl_hier_mem_iter( t_sl_hier_mem *shm, t_sl_hier_mem_range *range );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/
