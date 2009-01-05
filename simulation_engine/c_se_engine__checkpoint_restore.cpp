/*a Copyright
  
  This file 'c_se_engine__checkpoint_restore.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Defines
 */
#define GROWING_ARRAY_BLOCK(a,t,i) (&(((t *)((a).data))[i]))
#define ALIGN_UP(x) (((x)+sizeof(unsigned long long)) & -sizeof(unsigned long long ))
#define OFFSET_AS_PTR(a,b,t) ((t)( ( ((char *)(a)) - ((char *)(b)) ) + (char *)0))
#define PTR_FROM_OFFSET(a,b,t) ((t)( ((char *)(b)) + (long int)(a) ))

/*a Types
 */
/*t t_mem_block
 */
typedef struct t_mem_block
{
    unsigned char *start;
    int length;
} t_mem_block;

/*t t_state_reference
 */
typedef struct t_state_reference
{
    t_mem_block mem_block;
    t_engine_state_desc_list *sdl; // state desc list it belongs to (module instance is already implied by parent)
    int state_number; // state number within the state desc list, not within the module instance
    int mem_block_num; // Filled in once memory blocks are amalgamated and sorted
    int offset; // Filled in once memory blocks are amalgamated and sorted
} t_state_reference;

/*t t_checkpoint_state_entry
 */
typedef struct t_checkpoint_state_entry
{
    t_engine_state_desc_list *sdl; // state desc list it belongs to (module instance is already implied by parent)
    int state_number; // state number within the state desc list, not within the module instance
    int mem_block; // memory block that it is within
    int offset; // offset in to memory block
} t_checkpoint_state_entry;

/*t t_engine_checkpoint_entry
  One of a list for a module instance, containing the set of data ptrs and corresponding state entries for a checkpoint for a portion of the state of a module
 */
typedef struct t_engine_checkpoint_entry
{
    struct t_engine_checkpoint_entry *next_in_list;
    t_engine_module_instance *emi; // what we are checkpointing here
    t_mem_block *memory_blocks; // part of this mallocked structure, contains memory blocks used (and stored in a checkpoint)
    int num_mem_blocks;
    t_checkpoint_state_entry *state_entries; // part of this mallocked structure, list of what state is stored where in the memory block
    int num_state_entries; // total number of state entries within the memory blocks
} t_engine_checkpoint_entry;

/*t t_engine_checkpoint
  The structure containing all the data for a checkpoint
  These can be hierarchical deltas, and so each one is a delta from a parent
 */
typedef struct t_engine_checkpoint
{
    struct t_engine_checkpoint *next_in_list; // Next in list for the engine, so a particular checkpoint may be recovered; they are stored in cycle order, so that they may be deleted readily if simulations restart after a force or similar
    struct t_engine_checkpoint *prev_in_list; // Previous in list for the engine, so a checkpoint may be removed easily
    struct t_engine_checkpoint *parent; // parent if this is a delta
    char *name; // name of this checkpoint (within this malloced t_engine_checkpoint)
    int allocated_size; // Allocated size of this structure
    int used_size; // Actual used size of this structure
    int cycle; // cycle number for this checkpoint
    int num_instances; // number of instances in this checkpoint
    t_mem_block *memory_blocks; // one for each instance, whose data is actually within this malloced t_engine_checkpoint
    char data[1]; // Start of data - must be at end of the struct
} t_engine_checkpoint;

/*t t_growing_array
 */
typedef struct t_growing_array
{
    void *data;
    int alloced;
    int used;
    int size_of_entry;
} t_growing_array;

/*a Growing array functions
 */
/*f growing_array_alloc_one
 */
static int growing_array_alloc_one( t_growing_array *array )
{
    if (array->used == array->alloced)
    {
        void *new_memory;
        new_memory = realloc( array->data, (array->alloced+64)*array->size_of_entry );
        if (!new_memory)
        {
            return 0;
        }
        array->data = (void *)new_memory;
        array->alloced += 64;
    }
    return 1;
}

/*a Checkpoint/restore methods
  Each module must supply methods for:
    initialize_checkpointing
        Basically let the module instance sort out what data from where it may need to checkpoint, and how that corresponds to actual state
    checkpoint_size
        Estimate (pretty accurately) the size of data required for a checkpoint, possibly as a delta from a previous checkpoint
    checkpoint_add
        Add data for a checkpoint, possibly as a delta from a previous checkpoint

  void *initialize_checkpointing( void );
     void *add_checkpoint( const char *name, void *previous );
     t_sl_error_level restore_checkpoint( void *handle );
     void *find_checkpoint( const char *name );
     void *delete_checkpoint( void *handle );
     void *delete_all_checkpoints( void );
     t_sl_error_level write_checkpoint_to_file( void *handle, const char *filename, int type );
     t_sl_error_level read_checkpoint_from_file( const char *name, const char *filename );
 */
/*f mem_block_compare
 */
static int mem_block_compare( const void *a, const void *b )
{
    const t_mem_block *mem_a = (const t_mem_block *)a;
    const t_mem_block *mem_b = (const t_mem_block *)b;
    if (mem_a->start < mem_b->start) return -1;
    return 1;
}

/*f checkpoint_create_descriptor_entry
 */
t_engine_checkpoint_entry *c_engine::checkpoint_create_descriptor_entry( t_engine_module_instance *emi, int num_mem_blocks, int num_state_entries )
{
    t_engine_checkpoint_entry *ce;

    ce = (t_engine_checkpoint_entry *)malloc( sizeof(t_engine_checkpoint_entry) + num_state_entries*sizeof(t_checkpoint_state_entry) + num_mem_blocks*sizeof(t_mem_block) );
    if (!ce)
    {
        fprintf(stderr,"Out of memory in checkpoint allocation\n!");
        exit(0);
    }
    ce->next_in_list = NULL;
    ce->emi = emi;
    ce->memory_blocks = (t_mem_block *)( ((char *)(ce)) + sizeof(t_engine_checkpoint_entry) );
    ce->num_mem_blocks = num_mem_blocks;
    ce->state_entries = (t_checkpoint_state_entry *)( ((char *)(ce->memory_blocks)) + num_mem_blocks*sizeof(t_mem_block) );
    ce->num_state_entries = num_state_entries;
    return ce;
}

/*f checkpoint_initialize_instance_declared_state
 */
t_sl_error_level c_engine::checkpoint_initialize_instance_declared_state( t_engine_module_instance *emi )
{
    t_engine_state_desc_list *sdl;
    t_growing_array mem_block_array;
    t_growing_array state_reference_array;

    /*b Init mem_block_array and state_reference_aray
     */
    mem_block_array.data = NULL;
    mem_block_array.used = mem_block_array.alloced = 0;
    mem_block_array.size_of_entry = sizeof(t_mem_block);
    state_reference_array.data = NULL;
    state_reference_array.used = state_reference_array.alloced = 0;
    state_reference_array.size_of_entry = sizeof(t_state_reference);

    /*b Run through state desc lists and add lumps to memory blocks for the state
     */
    for (sdl = emi->state_desc_list; sdl; sdl = sdl->next_in_list )
    {
        int i;
        void *ptr;

        /*b Handle each state entry
         */
        ptr = sdl->data_base_ptr;
        for (i=0; i<sdl->num_state; i++)
        {
            t_engine_state_desc *desc;
            unsigned char *base;
            int size;

            /*b Interpret state desc 
             */
            desc = &(sdl->state_desc[i]);
            size = 0;
            base = (unsigned char *)ptr;
            if (desc->ptr)
            {
                base = ((unsigned char *)desc->ptr);
            }
            base += desc->offset;
            if (sdl->data_indirected)
            {
                base = *((unsigned char **)base);
            }
            switch (desc->type)
            {
            case engine_state_desc_type_none:
                break;
            case engine_state_desc_type_bits:
            case engine_state_desc_type_fsm:
                size = sizeof(int)*BITS_TO_INTS(desc->args[0]);
                break;
            case engine_state_desc_type_memory:
                size = sizeof(int)*desc->args[1]*BITS_TO_INTS(desc->args[0]);
                break;
            default:
                printf("Cannot checkpoint type %d\n", desc->type);
                break;
            }

            /*b Add state reference
             */
            if (size>0)
            {
                if (!growing_array_alloc_one( &state_reference_array ))
                {
                    fprintf(stderr,"Out of memory in checkpoint allocation\n!");
                    exit(0);
                }
                t_state_reference *new_ref;
                new_ref = GROWING_ARRAY_BLOCK(state_reference_array, t_state_reference, state_reference_array.used);
                new_ref->mem_block.start = base;
                new_ref->mem_block.length = size;
                new_ref->sdl = sdl;
                new_ref->state_number = i;
                new_ref->mem_block_num = 0;
                new_ref->offset = 0;
                state_reference_array.used++;
            }

            /*b Attempt to append to previous memory block - common occurrence
             */
            if (size>0)
            {
                if (mem_block_array.used>0)
                {
                    t_mem_block *last_block;
                    last_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, mem_block_array.used-1);
                    if (base==(last_block->start+last_block->length))
                    {
                        last_block->length += size;
                        size = 0;
                    }
                }
            }

            /*b Add to list of blocks if required
             */
            if (size>0)
            {
                if (!growing_array_alloc_one( &mem_block_array ))
                {
                    fprintf(stderr,"Out of memory in checkpoint allocation\n!");
                    exit(0);
                }
                t_mem_block *new_block;
                new_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, mem_block_array.used);
                new_block->start = base;
                new_block->length = size;
                mem_block_array.used++;
            }
        }
    }

    /*b Sort mem_blocks, amlgamate adjacent blocks, and create actual final list
     */
    if (mem_block_array.used==0)
    {
        emi->checkpoint_entry = NULL;
        return error_level_okay;
    }
    qsort( mem_block_array.data, mem_block_array.used, mem_block_array.size_of_entry, mem_block_compare );
    {
        int i, j;
        t_mem_block *prev_block, *new_block;
        j=0;
        prev_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, 0 ); // 0 is the tail
        for (i=1; i<mem_block_array.used; i++) // start from the second block trying to amalgamate with previous block
        {
            new_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, i );
            if (prev_block->start+prev_block->length == new_block->start)
            {
                prev_block->length += new_block->length;
            }
            else
            {
                j = j+1;
                prev_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, j );
                *prev_block = *new_block;
            }
        }
        mem_block_array.used = j+1;
        if (1)
        {
            for (i=0; i<mem_block_array.used; i++)
            {
                t_mem_block *mem_block;
                mem_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, i );
                //printf("Mem block %d: %p->%p, %d\n", i, mem_block->start, mem_block->start+mem_block->length, mem_block->length);
            }
        }
    }

    /*b Cross reference the state references for memory block
     */
    {
        int i;
        int mem_block_num;
        t_mem_block *prev_mem_block;

        mem_block_num = -1;
        prev_mem_block = NULL;
        for (i=0; i<state_reference_array.used; i++)
        {
            t_state_reference *ref;
            int j;

            /*b Get reference
             */
            ref = GROWING_ARRAY_BLOCK( state_reference_array, t_state_reference, i );

            /*b Check to see if it is within the mem_block in hand
             */
            if (prev_mem_block)
            {
                if ( (ref->mem_block.start >= prev_mem_block->start) &&
                     (ref->mem_block.start < (prev_mem_block->start+prev_mem_block->length)) )
                {
                    mem_block_num = mem_block_num;
                }
                else
                {
                    mem_block_num = -1;
                }
            }
            else
            {
                mem_block_num = -1;
            }

            /*b Search mem_blocks to find it
             */
            for (j=0; (mem_block_num<=0) && (j<mem_block_array.used); j++)
            {
                t_mem_block *mem_block;
                mem_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, j );
                if ( (ref->mem_block.start >= mem_block->start) &&
                     (ref->mem_block.start < (mem_block->start+mem_block->length)) )
                {
                    mem_block_num = j;
                }
            }

            /*b Cross reference
             */
            if (mem_block_num>=0) // SHOULD BE!!!
            {
                prev_mem_block = GROWING_ARRAY_BLOCK(mem_block_array, t_mem_block, mem_block_num );
                ref->mem_block_num = mem_block_num;
                ref->offset = ref->mem_block.start - prev_mem_block->start;
            }
            else
            {
                fprintf(stderr,"Bug in checkpointing - state reference %d not in memory block list (%p/%p)\n",
                        i,
                        ref->mem_block.start,
                        ref->mem_block.start+ref->mem_block.length);
                prev_mem_block = NULL;
            }
        }
    }

    /*b Create checkpoint entry
     */
    {
        t_engine_checkpoint_entry *ce;
        int i;

        ce = checkpoint_create_descriptor_entry( emi, mem_block_array.used, state_reference_array.used );
        if (!ce)
            return error_level_fatal;

        memcpy( ce->memory_blocks, mem_block_array.data, mem_block_array.used*sizeof(t_mem_block) );
        for (i=0; i<ce->num_state_entries; i++)
        {
            t_state_reference *ref;
            ref = GROWING_ARRAY_BLOCK( state_reference_array, t_state_reference, i );
            ce->state_entries[i].sdl = ref->sdl;
            ce->state_entries[i].state_number = ref->state_number;
            ce->state_entries[i].mem_block = ref->mem_block_num;
            ce->state_entries[i].offset = ref->offset;
        }
        emi->checkpoint_entry = ce;
    }

    /*b Free temporary memory
     */
    if (mem_block_array.data)
        free(mem_block_array.data);
    if (state_reference_array.data)
        free(state_reference_array.data);

    /*b Return
      */
    return error_level_okay;
}

/*f c_engine::initialize_checkpointing
 */
t_sl_error_level c_engine::initialize_checkpointing( void )
{
    t_engine_module_instance *emi;

    /*b We need to run through the state and build a description of the state that will be placed in a checkpoint file
      This should call any checkpointing fns required by modules to get their state requirements
      For each module instance set of SDLs we want to build a list of state
      which we sort
      and then turn in to a start address, size, list of data; all data is assumed to be uint32 in size at least.
      We will end up with a list of these that can be used for the checkpointing of the SDL-based module instances
    */
    for (emi=module_instance_list; emi; emi=emi->next_instance )
    {
        if (emi->checkpoint_entry)
        {
            free(emi->checkpoint_entry);
            emi->checkpoint_entry = NULL;
        }

        if (emi->checkpoint_fn_list)
        {
            printf("Should invoke some sort of checkpointing function\n");
        }
        else
        {
            checkpoint_initialize_instance_declared_state( emi );
        }
    }
    return error_level_okay;
}

/*f c_engine::checkpointing_display_information
 */
void c_engine::checkpointing_display_information( int level )
{
    t_engine_module_instance *emi;

    /*b Run through all the checkpointing information we have and display it
      We should also display any checkpoints we have too
    */
    for (emi=module_instance_list; emi; emi=emi->next_instance )
    {
        t_engine_checkpoint_entry *ce;
        printf("Module instance %s : %s\n",emi->full_name, emi->type );
        for (ce = emi->checkpoint_entry; ce; ce=ce->next_in_list)
        {
            int i;
            printf("\tCheckpoint entry (mem blocks %d, state entries %d)\n",
                   ce->num_mem_blocks,
                   ce->num_state_entries );
            for (i=0; i<ce->num_mem_blocks; i++)
            {
                t_mem_block *mem_block = ce->memory_blocks+i;
                printf("\t\tMemory block %d: %p->%p, %d\n", i, mem_block->start, mem_block->start+mem_block->length, mem_block->length);
            }
            for (i=0; i<ce->num_state_entries; i++)
            {
                printf("\t\tState %s.%s in block %d @ %d\n",
                       ce->state_entries[i].sdl->prefix?ce->state_entries[i].sdl->prefix:"",
                       ce->state_entries[i].sdl->state_desc[ce->state_entries[i].state_number].name,
                       ce->state_entries[i].mem_block,
                       ce->state_entries[i].offset );
            }
        }
        /*b Next module
         */
    }
}

/*f c_engine::checkpoint_size
 */
int c_engine::checkpoint_size( const char *name, const char *previous )
{
    t_engine_module_instance *emi;
    int total_size;

    /*b Run through all the checkpointing information and calculate the size
    */
    for (emi=module_instance_list; emi; emi=emi->next_instance )
    {
        t_engine_checkpoint_entry *ce;
        if (emi->checkpoint_fn_list)
        {
            printf("Should invoke some sort of checkpointing function\n");
        }
        else
        {
            for (ce = emi->checkpoint_entry; ce; ce=ce->next_in_list)
            {
                int i;
                for (i=0; i<ce->num_mem_blocks; i++)
                {
                    total_size += ce->memory_blocks[i].length;
                }
            }
        }
    }
    return total_size;
}

/*f c_engine::checkpoint_find_from_name
 */
t_engine_checkpoint *c_engine::checkpoint_find_from_name( const char *name )
{
    t_engine_checkpoint *ckpt;
    for (ckpt=checkpoint_list; ckpt; ckpt=ckpt->next_in_list)
    {
        if (!strcmp(name,ckpt->name))
        {
            return ckpt;
        }
    }
    return NULL;
}

/*f c_engine::checkpoint_find_up_to_time
  Find the checkpoint up to or at a cycle (i.e. just prior to the 'cycle'th system clock edge)
 */
t_engine_checkpoint *c_engine::checkpoint_find_up_to_time( int time )
{
    t_engine_checkpoint *ckpt;
    for (ckpt=checkpoint_list; ckpt; ckpt=ckpt->next_in_list)
    {
        if (ckpt->cycle>time)
        {
            if (ckpt->prev_in_list)
            {
                return ckpt;
            }
            return NULL;
        }
    }
    return NULL;
}

/*f c_engine::checkpoint_get_information
 */
t_sl_error_level c_engine::checkpoint_get_information( const char *name, const char **parent, int *time, const char **next_ckpt )
{
    t_engine_checkpoint *ckpt;
    if (parent) *parent = NULL;
    if (time) *time = 0;
    if (next_ckpt) *next_ckpt = NULL;
    ckpt = checkpoint_find_from_name( name );
    if (!ckpt)
        return error_level_serious;
    if (parent)
        *parent = (ckpt->parent) ? ckpt->parent->name : NULL;
    if (time)
        *time = ckpt->cycle;
    if (next_ckpt)
        *next_ckpt = (ckpt->next_in_list) ? ckpt->next_in_list->name : NULL;
    return error_level_okay;
}

/*f c_engine::checkpoint_add
 */
t_sl_error_level c_engine::checkpoint_add( const char *name, const char *previous )
{
    t_engine_module_instance *emi;
    t_engine_checkpoint *ckpt;
    int num_instances, instance;

    /*b Check we are not duplicating nor adding a checkpoint wen there is already a later checkpoint (should this latter be permitted?)
     */
    if (checkpoint_find_from_name(name))
    {
        fprintf(stderr,"Duplicate checkpoint name requested\n");
        return error_level_serious;
    }
    ckpt = checkpoint_find_up_to_time(cycle_number);
    if ((ckpt) && (ckpt->next_in_list)) // if another exists after this cycle then that's a serious error or warning?
    {
        fprintf(stderr,"Warning = ading checkpoint '%s' at time before checkpoint '%s'\n", name, ckpt->next_in_list->name );
    }

    /*b Count the module instances as each one has a slot in the checkpoint
    */
    num_instances = 0;
    for (emi=module_instance_list; emi; emi=emi->next_instance )
        num_instances++;

    /*b Allocate room for the checkpoint
     */
    {
        int size;
        size = sizeof(t_engine_checkpoint) + strlen(name)+1  + num_instances*sizeof(t_mem_block) + 256; // Have some extra to spare
        ckpt = (t_engine_checkpoint *)malloc( size );
        if (!ckpt)
        {
            fprintf(stderr,"Failed to allocate room for checkpoint\n");
            return error_level_serious;
        }
        ckpt->next_in_list = NULL;
        ckpt->prev_in_list = NULL;
        ckpt->parent = NULL;
        ckpt->name = OFFSET_AS_PTR( ckpt->data, ckpt, char * );
        ckpt->allocated_size = size;
        ckpt->used_size = ALIGN_UP( (((char *)ckpt->data-(char *)ckpt)+strlen(name)+1) );
        ckpt->cycle = cycle_number;
        printf("Set cycle %d\n",ckpt->cycle);
        ckpt->num_instances = num_instances;
        ckpt->memory_blocks = OFFSET_AS_PTR( ((char *)ckpt)+ckpt->used_size, ckpt, t_mem_block * );
        ckpt->used_size += num_instances * sizeof(t_mem_block);
    }

    /*b Run through all the module instances and checkpoint them
    */
    for (emi=module_instance_list, instance=0; emi; emi=emi->next_instance, instance++ )
    {
        char *instance_data;
        instance_data = ((char *)ckpt)+ckpt->used_size;
        {
            t_mem_block *mem_blocks;
            mem_blocks = PTR_FROM_OFFSET( ckpt->memory_blocks, ckpt, t_mem_block *);
            mem_blocks[instance].start = 0;
            mem_blocks[instance].length = 0;
        }
        if (emi->checkpoint_fn_list)
        {
            printf("Should invoke some sort of checkpointing function\n");
        }
        else
        {
            t_engine_checkpoint_entry *ce;
            int size;
            size = 0;
            for (ce = emi->checkpoint_entry; ce; ce=ce->next_in_list)
            {
                int i;
                for (i=0; i<ce->num_mem_blocks; i++)
                {
                    size += ce->memory_blocks[i].length;
                }
            }
            size = ALIGN_UP(size);
            if (ckpt->used_size+size > ckpt->allocated_size)
            {
                void *new_ptr;
                int new_size;
                new_size = ckpt->used_size+size+256;
                new_ptr = realloc( ckpt, new_size );
                if (!new_ptr)
                {
                    fprintf(stderr,"Failed to expand room for checkpoint\n");
                    return error_level_serious;
                }
                ckpt = (t_engine_checkpoint *)new_ptr;
                ckpt->allocated_size = new_size;
                instance_data = ((char *)ckpt)+ckpt->used_size;
            }
            {
                t_mem_block *mem_blocks;
                mem_blocks = PTR_FROM_OFFSET( ckpt->memory_blocks, ckpt, t_mem_block *);
                mem_blocks[instance].start = OFFSET_AS_PTR( instance_data, ckpt, unsigned char * );
                mem_blocks[instance].length = size;
            }
            for (ce = emi->checkpoint_entry; ce; ce=ce->next_in_list)
            {
                int i;
                for (i=0; i<ce->num_mem_blocks; i++)
                {
                    memcpy( instance_data, ce->memory_blocks[i].start,  ce->memory_blocks[i].length );
                    instance_data += ce->memory_blocks[i].length;
                }
            }
            ckpt->used_size += size;
        }
    }

    /*b Tidy up pointers (which are currently offsets)
     */
    ckpt->name = PTR_FROM_OFFSET( ckpt->name, ckpt, char * );
    ckpt->memory_blocks = PTR_FROM_OFFSET( ckpt->memory_blocks, ckpt, t_mem_block * );
    for (instance=0; instance<ckpt->num_instances; instance++)
    {
        ckpt->memory_blocks[instance].start = PTR_FROM_OFFSET( ckpt->memory_blocks[instance].start, ckpt, unsigned char *);
    }

    /*b Chain in the checkpoint
     */
    {
        t_engine_checkpoint *prev_ckpt;
        strcpy(ckpt->name,name);
        prev_ckpt = checkpoint_find_up_to_time(cycle_number);
        if (prev_ckpt)
        {
            ckpt->prev_in_list = prev_ckpt;
            ckpt->next_in_list = prev_ckpt->next_in_list;
            if (prev_ckpt->next_in_list)
            {
                prev_ckpt->next_in_list->prev_in_list = ckpt->next_in_list;
            }
            else
            {
                checkpoint_tail = ckpt;
            }
            prev_ckpt->next_in_list = ckpt;
        }
        else
        {
            ckpt->prev_in_list = NULL;
            ckpt->next_in_list = checkpoint_list;
            if (checkpoint_list)
            {
                checkpoint_list->prev_in_list = ckpt;
            }
            else
            {
                checkpoint_list = ckpt;
                checkpoint_tail = ckpt;
            }
        }
    }

    /*b Return okay
     */
    return error_level_okay;
}

/*f c_engine::checkpoint_restore
 */
t_sl_error_level c_engine::checkpoint_restore( const char *name )
{
    t_engine_module_instance *emi;
    t_engine_checkpoint *ckpt;
    int instance;

    /*b Find checkpoint
     */
    ckpt = checkpoint_find_from_name(name);
    fprintf(stderr,"ckpt %p %d\n",ckpt,ckpt->cycle);
    if (!ckpt)
    {
        fprintf(stderr,"Could not find checkpoint name requested\n");
        return error_level_serious;
    }

    /*b Run through all the module instances and recover their state from the checkpoint
    */
    fprintf(stderr,"ckpt %p %d\n",ckpt,ckpt->cycle);
    for (emi=module_instance_list, instance=0; emi; emi=emi->next_instance, instance++ )
    {
        if (emi->checkpoint_fn_list)
        {
            printf("Should invoke some sort of checkpointing function\n");
        }
        else
        {
            unsigned char *data;
            size_t data_size;
            t_engine_checkpoint_entry *ce;

            data = ckpt->memory_blocks[instance].start;
            data_size = ckpt->memory_blocks[instance].length;
            for (ce = emi->checkpoint_entry; ce; ce=ce->next_in_list)
            {
                int i;
                for (i=0; i<ce->num_mem_blocks; i++)
                {
                    memcpy( ce->memory_blocks[i].start, data, ce->memory_blocks[i].length );
                    data += ce->memory_blocks[i].length;
                    data_size -= ce->memory_blocks[i].length;
                }
            }
            if ((data_size<0) || (data_size>ALIGN_UP(1)))
            {
                fprintf(stderr,"arrgh %d\n",(int)data_size);
                return error_level_serious;
            }
        }
    }
    fprintf(stderr,"ckpt %p %d\n",ckpt,ckpt->cycle);
    cycle_number = ckpt->cycle;
    fprintf(stderr,"Recvr cycle %d\n",cycle_number);

    /*b Return okay
     */
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

