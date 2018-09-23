/*a Copyright
  
  This file 'sl_hier_mem.cpp' copyright Gavin J Stark, 2007
  
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_general.h"
#include "sl_mif.h"
#include "sl_hier_mem.h"

/*a Defines
 */

/*a Types
 */

/*a Support functions - the bulk of the hard work :-)
 */
/*f free_level
 */
static void free_level( t_sl_hier_mem *shm, t_sl_hier_mem_submem_ptr ptr, int level )
{
    int i;

    if (!ptr)
        return;

    if (shm->page_shift[level+1]==0)
    {
        for (i=0; i<shm->page_size[level]; i++)
        {
            if (ptr[i].data)
            {
                //printf("Free data %p\n",ptr[i].data);
                free(ptr[i].data);
            }
        }
    }
    else
    {
        for (i=0; i<shm->page_size[level]; i++)
        {
            if (ptr[i].submem)
            {
                free_level( shm, ptr[i].submem, level+1 );
                //printf("Free sub %p\n",ptr[i].submem);
                free(ptr[i].submem);
            }
        }
    }
}

/*f init_memory - initialize a memory range
 */
static void init_memory( t_sl_hier_mem *shm, t_sl_hier_mem_data_ptr data, t_sl_uint64 base_address, int length )
{
    switch (shm->unwritten_data_method)
    {
    case sl_hier_mem_unwritten_data_ones:
    {
        memset( data, 0xff, length*shm->bytes_per_memory );
        return;
    }
    case sl_hier_mem_unwritten_data_lfsr_init_one:
    case sl_hier_mem_unwritten_data_lfsr_init_address:
    {
        t_sl_uint64 lfsr;
        int i;
        lfsr = 1;
        if (shm->unwritten_data_method==sl_hier_mem_unwritten_data_lfsr_init_address)
        {
            lfsr = base_address;
        }
        for (i=0; i<length*shm->bytes_per_memory; i++)
        {
            data[i] = (lfsr&0xff);
            if (((lfsr>>39)^(lfsr>>35))&1) { lfsr = (lfsr << 1) | 1; } else { lfsr = lfsr<<1; }
            if (lfsr==0) {lfsr=1;}
        }
        return;
    }
    default:
        break;
    }
    memset( data, 0, length*shm->bytes_per_memory );
    return;
}

/*f alloc_page - Allocate a page of memory or submem ptrs; level is the level of this data, and we want to allocate the data and return a ptr to it
  Effectively we are allocating the next layer up's ptr (level-1).data or (level-1).submem
 */
static t_sl_hier_mem_ptr alloc_page( t_sl_hier_mem *shm, t_sl_uint64 address, int level )
{
    t_sl_hier_mem_ptr ptr;
    int i;
    int page;

    //printf("alloc_page %p:%lld:%d:%d\n", shm, address, level, shm->page_shift[level] );
    if (shm->page_shift[level]==0) // This layer is data, so a pointer to it is a t_sl_hier_mem_data_ptr
    {
        ptr.data = (t_sl_hier_mem_data_ptr)malloc(shm->page_size[level]*shm->bytes_per_memory);
        init_memory( shm, ptr.data, address&~shm->page_mask[level], shm->page_size[level] );
    }
    else
    {
        ptr.submem = (t_sl_hier_mem_submem_ptr)malloc(shm->page_size[level]*sizeof(t_sl_hier_mem_submem_ptr));
        for (i=0; i<shm->page_size[level]; i++)
        {
            ptr.submem[i].data = NULL; // Clear the next level page table down
        }
        page = (address>>shm->page_shift[level])&shm->page_mask[level]; // Get page
        ptr.submem[page] = alloc_page( shm, address, level+1 );
    }
    //printf("alloced %p\n", ptr.data);
    return ptr;
}

/*f find_with_alloc
  Return pointer to data from an address, allocating if required
 */
static t_sl_hier_mem_data_ptr find_with_alloc( t_sl_hier_mem *shm, t_sl_hier_mem_ptr *submem, t_sl_uint64 address, int level, int alloc )
{
    int page;

    //printf("find_with_alloc %p:%p:%llx:%d:%d\n", shm, submem, address, level, alloc );
    page = (address>>shm->page_shift[level])&shm->page_mask[level]; // Get page
    if (!submem[page].data) // Is it allocated?
    {
        if (alloc)
        {
            submem[page] = alloc_page( shm, address, level+1 );
        }
        else
        {
            return NULL;
        }
    }
    if (!submem[page].data) // Is it allocated now?
    {
        return NULL;
    }
    if (shm->page_shift[level+1]==0) // next layer is data
    {
        return submem[page].data + (shm->bytes_per_memory*(address&shm->page_mask[level+1]));
    }
    else
    {
        return find_with_alloc( shm, submem[page].submem, address, level+1, alloc );
    }
}

/*f find_page_at_or_after
  From an address find the first alloced page at or after that address
  Return NULL if the address is not found
 */
static t_sl_hier_mem_data_ptr find_page_at_or_after( t_sl_hier_mem *shm, t_sl_hier_mem_ptr *submem, t_sl_uint64 *address, int level )
{
    int i;
    int page;
    t_sl_uint64 addr;
    t_sl_hier_mem_data_ptr data;

    //printf("find_page_at_or_after %p:%p:%llx:%d\n", shm, submem, *address, level );
    addr = *address;
    page = (addr>>shm->page_shift[level])&shm->page_mask[level]; // Get page to start at
    for (i=page; i<shm->page_size[level]; i++)
    {
        if (submem[i].data) // Is it allocated (data or subpages)?
        {
            addr = addr &~ (shm->page_mask[level]<<shm->page_shift[level]);
            addr = addr | (i<<shm->page_shift[level]);
            if (shm->page_shift[level+1]==0) // next page down is data - so its there!
            {
                *address = addr;
                return submem[i].data;
            }
            else
            {
                data = find_page_at_or_after( shm, submem[i].submem, &addr, level+1 );
                if (data)
                {
                    *address = addr;
                    return data;
                }
            }
        }
    }
    return NULL;
}

/*a Hierarchical memory creation/free functions
 */
/*f sl_hier_mem_create
  Return NULL if failed to create
  Basically we want total_size entries
  log2_page_sizes is a list from bottom to top of page sizes
  We effectively need to reverse the list
  So, if total_size is 10,000 and log2page_sizes is {4,4,4,3,6} we need...
    total_size = 10,000 (which is 14 bits :-)
    log2_page_sizes = 1,4,3,6
 Now, we expect to see numbers like 10 not 3 or 4, but the gist is correct
 */
extern t_sl_hier_mem *sl_hier_mem_create( t_sl_uint64 total_size, int *log2_page_sizes, t_sl_hier_mem_unwritten_data unwritten_data_method, int bytes_per_memory )
{
    int i, j;
    t_sl_uint64 n;
    int data_size; // Size of data required for toplevel page - could be data directly
    t_sl_hier_mem *shm;
    int num_levels;
    int reversed_log2_page_sizes[SL_HIER_MAX_LEVEL];

    for (j=0; log2_page_sizes[j]; j++);
    j--; // log2_page_sizes[j] is the last one
    for (i=0,n=total_size-1; (j>=0) && (i<SL_HIER_MAX_LEVEL) && (n>0); i++,j--)
    {
        if ((((t_sl_uint64) 1)<<log2_page_sizes[j]) > n) // Last one?
        {
            reversed_log2_page_sizes[i] = log2_page_sizes[j];
            while (((((t_sl_uint64) 1)<<(reversed_log2_page_sizes[i]))-1) > n) // Now reduce last page size by 1 bit till we just fit n
                reversed_log2_page_sizes[i]--;
            break;
        }
        else
        {
            n = n >> log2_page_sizes[j];
            reversed_log2_page_sizes[i] = log2_page_sizes[j];
        }
    }
    num_levels = i+1;

    if (num_levels>=SL_HIER_MAX_LEVEL)
        return NULL; // Canna do it captain

    if (num_levels>1)
        data_size = sizeof(t_sl_hier_mem_ptr)*(1<<reversed_log2_page_sizes[num_levels-1]);
    else
        data_size = bytes_per_memory*(1<<reversed_log2_page_sizes[0]);
    shm = (t_sl_hier_mem *)malloc(sizeof(t_sl_hier_mem)+data_size);
    shm->total_size = total_size;
    shm->bytes_per_memory = bytes_per_memory;
    shm->num_levels = num_levels;
    shm->unwritten_data_method = unwritten_data_method;
    for (i=0; i<SL_HIER_MAX_LEVEL; i++)
    {
        shm->page_size[i] = 0;
        shm->page_mask[i] = 0;
        shm->page_shift[i] = 0;
    }
    j=0;
    for (i=num_levels-1; i>=0; i--)
    {
        shm->page_size[i] = 1<<reversed_log2_page_sizes[num_levels-1-i];
        shm->page_mask[i] = (1<<reversed_log2_page_sizes[num_levels-1-i])-1;
        shm->page_shift[i] = j;
        j += reversed_log2_page_sizes[num_levels-1-i];
    }
    if (num_levels>1)
    {
        for (i=0; i<shm->page_size[0]; i++)
        {
            shm->array[i].data = NULL;
        }
    }
    else
    {
        shm->array[0].data = (t_sl_hier_mem_data_ptr)(shm+1);
        init_memory( shm, shm->array[0].data, 0, total_size );
    }
    return shm;
}

/*f sl_hier_mem_free
 */
extern void sl_hier_mem_free( t_sl_hier_mem *shm )
{
    if (!shm)
        return;

    if (shm->page_shift[0]==0)
    {
        // No subdata to free!
    }
    else
    {
        free_level( shm, shm->array, 0 );
    }
    free(shm);
}

/*a MIF file functions
 */
/*f mif_write_callback
 */
static void mif_write_callback( void *handle, t_sl_uint64 address, t_sl_uint64 *data )
{
    t_sl_hier_mem *shm;
    t_sl_uint64 shifted_data;

    shm = (t_sl_hier_mem *)handle;
    shifted_data = data[0]>>shm->bit_start;

    sl_hier_mem_write( shm, address, (unsigned char *)&shifted_data );

}

/*f sl_hier_mem_read_mif
 */
extern t_sl_error_level sl_hier_mem_read_mif( t_sl_hier_mem *shm,
                                              c_sl_error *error,
                                              const char *filename,
                                              const char *user,
                                              t_sl_uint64 address_offset,
                                              int error_on_out_of_range,
                                              int bit_start,
                                              t_sl_mif_file_callback_fn callback,
                                              void *handle )
{
    return sl_mif_read_mif_file( error,
                                 filename,
                                 user,
                                 address_offset,
                                 shm->total_size,
                                 error_on_out_of_range,
                                 mif_write_callback,
                                 (void *)shm  );
}

/*a Read/write/find functions
 */
/*f sl_hier_mem_find
  Return pointer to data from an address, allocating if required
 */
extern t_sl_hier_mem_data_ptr sl_hier_mem_find( t_sl_hier_mem *shm, t_sl_uint64 address, int alloc )
{
    if (address>=shm->total_size)
        return NULL;
    //printf("sl_hier_mem_find %p:%llx:%d:%d\n", shm, address, alloc, shm->page_shift[0]);
    if (shm->page_shift[0]==0) // Not hierarchical after all :-)
        return shm->array[0].data + (shm->bytes_per_memory*address);
    return find_with_alloc( shm, shm->array, address, 0, alloc );
}

/*f sl_hier_mem_read
  Read up to an int from a location
 */
extern unsigned int sl_hier_mem_read( t_sl_hier_mem *shm, t_sl_uint64 address, int alloc )
{
    t_sl_hier_mem_data_ptr ptr;

    ptr = sl_hier_mem_find( shm, address, alloc );
    if (!ptr)
        return 0;
    if (shm->bytes_per_memory==1)
        return ptr[0];
    if (shm->bytes_per_memory==2)
        return ((unsigned short *)ptr)[0];
    return ((unsigned int *)ptr)[0];
}

/*f sl_hier_mem_write - Write data to an address, allocating if required
 */
extern void sl_hier_mem_write( t_sl_hier_mem *shm, t_sl_uint64 address, t_sl_hier_mem_data_ptr data )
{
    t_sl_hier_mem_data_ptr ptr;
    ptr = sl_hier_mem_find( shm, address, 1 );
    if (!ptr)
        return;
    if (shm->bytes_per_memory==1)
    {
        ptr[0]=data[0];
    }
    else if (shm->bytes_per_memory==2)
    {
        ((unsigned short *)ptr)[0] = ((unsigned short *)data)[0];
    }
    else if (shm->bytes_per_memory==4)
    {
        ((unsigned int *)ptr)[0] = ((unsigned int *)data)[0];
    }
    else if (shm->bytes_per_memory==8)
    {
        ((unsigned long long *)ptr)[0] = ((unsigned long long *)data)[0];
    }
    else
    {
        memcpy( ptr, data, shm->bytes_per_memory );
    }
}

/*a Iteration functions
 */
/*f sl_hier_mem_iter - Iterate through the memory
  Returns 1 if range is valid; if range has <=0 length on entry then first range in memory is returned, otherwise first which starts after start_address is returned
 */
extern int sl_hier_mem_iter( t_sl_hier_mem *shm, t_sl_hier_mem_range *range )
{
    t_sl_uint64 address;
    t_sl_hier_mem_data_ptr data;

    if (!range)
        return 0;
    if (range->length<=0)
    {
        address = 0;
    }
    else
    {
        address = range->start_address + shm->page_size[shm->num_levels-1];
        address = address &~ shm->page_mask[shm->num_levels-1];
    }

    if (shm->page_shift[0]==0) // not hierarchical
    {
        if (address!=0)
        {
            return 0;
        }
        range->start_address = address;
        range->length = shm->page_size[0];
        return 1;
    }

    if (address>shm->total_size)
    {
        return 0;
    }
    data = find_page_at_or_after( shm, shm->array, &address, 0 );
    if (!data)
        return 0;

    range->start_address = address;
    range->length = shm->page_size[shm->num_levels-1];
    range->data = data;
    return 1;
}
