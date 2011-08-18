/*a Copyright
  
  This file 'sl_exec_file.cpp' copyright Gavin J Stark 2003, 2004, 2007, 2008
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "c_sl_error.h"
#include "sl_general.h" 
#include "sl_mif.h"
#include "sl_ef_lib_memory.h"
#include "sl_exec_file.h"

/*a Types
 */
/*t t_sl_memory_object
 */
typedef struct t_sl_memory_object
{
    t_sl_memory_object *next_in_list;
    char *name;
    t_sl_ef_lib_memory memory;
} t_sl_memory_object;

/*t t_mif_compare_info
 */
typedef struct t_mif_compare_info
{
    int okay;
    t_sl_uint64 first_error_address;
    t_sl_ef_lib_memory *mem;
    t_sl_uint64 address_offset;
} t_mif_compare_info;

/*t cmd_* - Commands provided for extending the sl_exec_file
 */
enum
{
    cmd_memory,
};

/*a Static forward function declarations
 */
static t_sl_exec_file_method_fn ef_method_eval_read;
static t_sl_exec_file_method_fn ef_method_eval_write;
static t_sl_exec_file_method_fn ef_method_eval_load;
static t_sl_exec_file_method_fn ef_method_eval_save;
static t_sl_exec_file_method_fn ef_method_eval_compare;

/*a Statics
 */
/*v sl_memory_cmds - Exec file command extensions - map string and argument types to the command number
 */
static t_sl_exec_file_cmd sl_memory_cmds[] =
{
    {cmd_memory,        3, "!memory", "sii",    "memory <name> <num words> <bits per word>" },
    SL_EXEC_FILE_CMD_NONE
};

/*v sl_memory_obj_methods - Exec file object methods
 */
static t_sl_exec_file_method sl_memory_object_methods[] =
{
    {"read",    'i', 1, "i",   "read(<address>) - read data from a memory", ef_method_eval_read, NULL },
    {"write",    0, 2, "ii",   "write(<address>,<data>) - write data to a memory", ef_method_eval_write, NULL },
    {"load",     0,  2, "si",  "load(<filename>,<bit offset>) - load data from a memory", ef_method_eval_load, NULL },
    {"save",     0,  3, "sii", "save(<filename>,<start>,<num words>) - save data to a file", ef_method_eval_save, NULL },
    {"compare", 'i', 2, "si",  "compare(<filename>,<offset>) - compare data to that in a file; file[0]=mem[offset]", ef_method_eval_compare, NULL },
    SL_EXEC_FILE_METHOD_NONE
};

/*a Old stuff

     t_sl_exec_file_memory *mem, *next_mem;
          for (mem = file_data->memories; mem; mem=next_mem)
          {
               next_mem = mem->next_in_list;
               if (mem->contents)
               {
                    free(mem->contents);
               }
               free(mem);
          }
*/

/*a Exec file fns
 */
/*f ensure_contents
 */
static void ensure_contents(t_sl_ef_lib_memory *mem)
{
    int bytes_per_item;
    if (!mem->contents)
    {
        bytes_per_item = BITS_TO_BYTES(mem->bit_size);
        mem->contents = (t_sl_uint64 *)malloc(bytes_per_item*mem->max_size);
    }
}

/*f object_message_handler
 */
static t_sl_error_level object_message_handler( t_sl_exec_file_object_cb *obj_cb )
{
    t_sl_ef_lib_memory *mem;
    mem = &(((t_sl_memory_object *)(obj_cb->object_desc->handle))->memory);

    if (!strcmp(obj_cb->data.message.message,"memory_get"))
    {
        ensure_contents(mem);
        ((t_sl_ef_lib_memory **)obj_cb->data.message.client_handle)[0] = mem;
        return error_level_okay;
    }
    return error_level_serious;
}

/*f find_object
 */
static t_sl_memory_object *find_object( t_sl_memory_object **object_ptr, const char *name )
{
    t_sl_memory_object *object;

    for (object=*object_ptr; object; object=object->next_in_list)
    {
        if (!strcmp(name,object->name))
            return object;
    }
    return NULL;
}

/*f add_object
 */
static t_sl_memory_object *add_object( t_sl_memory_object **object_ptr, const char *name)
{
    t_sl_memory_object *object;

    object = (t_sl_memory_object *)malloc(sizeof(t_sl_memory_object));
    object->next_in_list = *object_ptr;
    *object_ptr = object;
    object->name = sl_str_alloc_copy( name );

    object->memory.contents = NULL;
    object->memory.bit_size = 0;
    object->memory.max_size = 0;
    return object;
}

/*f static exec_file_cmd_handler_cb
 */
static t_sl_error_level exec_file_cmd_handler_cb( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_sl_memory_object *mem;

    switch (cmd_cb->cmd)
    {
    case cmd_memory:
        mem = find_object((t_sl_memory_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
        if (!mem)
        {
            t_sl_exec_file_object_desc object_desc;
            mem = add_object((t_sl_memory_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
            mem->memory.max_size = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
            mem->memory.bit_size = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 2 );
            mem->memory.contents = NULL;
            memset(&object_desc,0,sizeof(object_desc));
            object_desc.version = sl_ef_object_version_checkpoint_restore;
            object_desc.name = mem->name;
            object_desc.handle = (void *)mem;
            object_desc.message_handler = object_message_handler;
            object_desc.methods = sl_memory_object_methods;
            sl_exec_file_add_object_instance( cmd_cb->file_data, &object_desc );
            ensure_contents(&(mem->memory));
            sl_exec_file_object_instance_register_state( cmd_cb, mem->name, "memory", (void *)(mem->memory.contents), mem->memory.bit_size, mem->memory.max_size );
        }
        break;
    default:
        return error_level_serious;
    }
    return error_level_okay;
}

/*f sl_memory_add_exec_file_enhancements
 */
extern void sl_ef_lib_memory_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;
    t_sl_memory_object **object_ptr;
    object_ptr = (t_sl_memory_object **)malloc(sizeof(t_sl_memory_object *));
    *object_ptr = NULL;

    lib_desc.version = sl_ef_lib_version_initial;
    lib_desc.library_name = "sys_memory";
    lib_desc.handle = (void *)object_ptr;
    lib_desc.cmd_handler = exec_file_cmd_handler_cb;
    lib_desc.file_cmds = sl_memory_cmds;
    lib_desc.file_fns = NULL;
    sl_exec_file_add_library( file_data, &lib_desc );
}

/*a Memory object methods
 */
/*f mif_compare_callback
 */
static void mif_compare_callback( void *handle, t_sl_uint64 address, t_sl_uint64 *data )
{
    t_mif_compare_info *info;
    t_sl_ef_lib_memory *mem;
    int okay;
    info = (t_mif_compare_info *)handle;
    mem = info->mem;
    address -= info->address_offset;

    okay=0;
    if (mem->bit_size<=8)
    {
        okay = (((const unsigned char *)mem->contents)[address] == ((*data)&0xff));
    }
    else if (mem->bit_size<=16)
    {
        okay = (((const unsigned short *)mem->contents)[address] == ((*data)&0xffff));
    }
    else if (mem->bit_size<=32)
    {
        okay = (((t_sl_uint32 *)mem->contents)[address] == ((*data)&0xffffffff));
    }
    else if (mem->bit_size<=64)
    {
        okay = (((t_sl_uint64 *)mem->contents)[address] == (*data));
    }
    if (!okay && info->okay)
    {
        info->okay = 0;
        info->first_error_address = address;
    }
}

/*f ef_method_eval_compare
 */
static t_sl_error_level ef_method_eval_compare( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_memory *mem;
    mem = &(((t_sl_memory_object *)(object_desc->handle))->memory);

    t_mif_compare_info info;

    ensure_contents(mem);

    info.address_offset = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
    info.okay = 1;
    info.mem = mem;
    info.first_error_address = 0;
    sl_mif_read_mif_file( cmd_cb->error,
                          sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ),
                          "exec file memory", //file_data->user,
                          0, // address offset
                          mem->max_size, // size
                          0, // error on out of range
                          mif_compare_callback,
                          (void *)&info );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, info.okay?((t_sl_uint64)-1):info.first_error_address ))
        return error_level_fatal;
    return error_level_okay;
}

/*f ef_method_eval_read
 */
static t_sl_error_level ef_method_eval_read( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_memory *mem;
    mem = &(((t_sl_memory_object *)(object_desc->handle))->memory);

    int address;
    t_sl_uint64 data;

    address = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 );
    if ( (address<0) || (address>=mem->max_size) )
    {
        data = 0xdeadbeef000add00LL;
        //cmd_cb->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_memory_address_out_of_range, error_id_sl_exec_file_get_next_cmd,
//                                     error_arg_type_integer, (int)address,
//                                     error_arg_type_malloc_string, thread->id,
//                                     error_arg_type_malloc_filename, file_data->filename,
//                                     error_arg_type_line_number, i,
//                                     error_arg_type_none );
    }
    else
    {
        data = 0xdeadbeef000add00LL;
        if (mem->contents)
        {
            if (mem->bit_size<=8)
            {
                data = ((unsigned char *)(mem->contents))[ address ];
            }
            else if (mem->bit_size<=16)
            {
                data = ((unsigned short *)(mem->contents))[ address ];
            }
            else if (mem->bit_size<=32)
            {
                data = ((t_sl_uint32 *)(mem->contents))[ address ];
            }
            else if (mem->bit_size<=64)
            {
                data = ((t_sl_uint64 *)(mem->contents))[ address ];
            }
        }
    }
    //printf("%p:%d:Reading address %d value %08llx\n",mem->contents, mem->bit_size,address,data);

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, data ))
        return error_level_fatal;
    return error_level_okay;
}

/*f ef_method_eval_write
 */
static t_sl_error_level ef_method_eval_write( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_memory *mem;
    mem = &(((t_sl_memory_object *)(object_desc->handle))->memory);

    int address;
    t_sl_uint64 data;

    ensure_contents(mem);

    address = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 );
    data = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
    //printf("Writing address %d value %08llx\n",address,data);

    if ( (address<0) || (address>=mem->max_size) )
    {
        return cmd_cb->error->add_error( (void *)(((t_sl_memory_object *)(object_desc->handle))->name), error_level_serious, error_number_sl_exec_file_memory_address_out_of_range, error_id_sl_exec_file_get_next_cmd,
                                         error_arg_type_integer, (int)address,
                                         error_arg_type_malloc_string, "unknown thread",
                                         error_arg_type_malloc_filename, cmd_cb->filename,
                                         error_arg_type_line_number, cmd_cb->line_number,
                                         error_arg_type_none );
    }
    if (mem->contents)
    {
        if (mem->bit_size<=8)
        {
            ((unsigned char *)(mem->contents))[ address ] = data;
        }
        else if (mem->bit_size<=16)
        {
            ((unsigned short *)(mem->contents))[ address ] = data;
        }
        else if (mem->bit_size<=32)
        {
            ((t_sl_uint32 *)(mem->contents))[ address ] = data;
        }
        else if (mem->bit_size<=64)
        {
            ((t_sl_uint64 *)(mem->contents))[ address ] = data;
        }
    }

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)1 ))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_load
 */
static t_sl_error_level ef_method_eval_load( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_memory *mem;
    mem = &(((t_sl_memory_object *)(object_desc->handle))->memory);

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)1 ))
        return error_level_fatal;

    if (mem->contents)
    {
        return sl_mif_reset_and_read_mif_file( cmd_cb->error,
                                               sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ),
                                               "exec file memory", //file_data->user,
                                               0, // address offset
                                               mem->max_size,
                                               mem->bit_size,
                                               (int)sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 ),
                                               2, 0, // Reset
                                               (int *)(mem->contents),
                                               NULL,
                                               NULL);
    }
    return error_level_fatal;
}

/*f ef_method_eval_save
 */
static t_sl_error_level ef_method_eval_save( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_memory *mem;
    mem = &(((t_sl_memory_object *)(object_desc->handle))->memory);

    const char *filename;
    int address;
    int size;

    ensure_contents(mem);

    filename = sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 );
    address = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
    size = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 2 );

    //printf("Filename %s add %d size %d\n", filename, address, size );
    if ( (address<0) || (address+size>mem->max_size) )
    {
        return cmd_cb->error->add_error( (void *)(((t_sl_memory_object *)(object_desc->handle))->name), error_level_serious, error_number_sl_exec_file_memory_address_out_of_range, error_id_sl_exec_file_get_next_cmd,
                                         error_arg_type_integer, (int)address,
                                         error_arg_type_malloc_string, "unknown thread",
                                         error_arg_type_malloc_filename, cmd_cb->filename,
                                         error_arg_type_line_number, cmd_cb->line_number,
                                         error_arg_type_none );
    }

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)1 ))
        return error_level_fatal;

    return sl_mif_write_mif_file( cmd_cb->error,
                                  filename,
                                  "exec file memory",
                                  (int *)mem->contents,
                                  mem->bit_size,
                                  address,
                                  size );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

