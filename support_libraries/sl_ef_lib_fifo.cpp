/*a Copyright
  
  This file 'sl_ef_lib_fifo.cpp' copyright Gavin J Stark 2003, 2004, 2007, 2008
  
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
#include "sl_ef_lib_fifo.h"
#include "sl_exec_file.h"

/*a Defines
 */
#if 0
#define WHERE_I_AM {fprintf(stderr,"%s:%d\n",__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif

/*a Types
 */
/*t t_sl_fifo_object
 */
typedef struct t_sl_fifo_object
{
    t_sl_fifo_object *next_in_list;
    char *name;
    t_sl_ef_lib_fifo fifo;
} t_sl_fifo_object;

/*t cmd_* - Commands provided for extending the sl_exec_file
 */
enum
{
    cmd_fifo,
};


/*a Static forward function declarations
 */
static t_sl_exec_file_method_fn ef_method_eval_reset;
static t_sl_exec_file_method_fn ef_method_eval_flags;
static t_sl_exec_file_method_fn ef_method_eval_is_empty;
static t_sl_exec_file_method_fn ef_method_eval_is_nearly_empty;
static t_sl_exec_file_method_fn ef_method_eval_is_nearly_full;
static t_sl_exec_file_method_fn ef_method_eval_is_full;
static t_sl_exec_file_method_fn ef_method_eval_has_underflowed;
static t_sl_exec_file_method_fn ef_method_eval_has_overflowed;
static t_sl_exec_file_method_fn ef_method_eval_read_ptr;
static t_sl_exec_file_method_fn ef_method_eval_write_ptr;
static t_sl_exec_file_method_fn ef_method_eval_uncommitted_read_ptr;
static t_sl_exec_file_method_fn ef_method_eval_remove;
static t_sl_exec_file_method_fn ef_method_eval_remove_no_commit;
static t_sl_exec_file_method_fn ef_method_eval_peek;
static t_sl_exec_file_method_fn ef_method_eval_add;
static t_sl_exec_file_method_fn ef_method_eval_commit_read;
static t_sl_exec_file_method_fn ef_method_eval_revert_read;
static t_sl_exec_file_method_fn ef_method_eval_wait_for_flags;

/*a Statics
 */
/*v sl_fifo_cmds - Exec file command extensions - map string and argument types to the command number
 */
static t_sl_exec_file_cmd sl_fifo_cmds[] =
{
    {cmd_fifo,             4, "!fifo", "siii", "fifo <name> <size> <ne_wm> <nf_wm>"},
    SL_EXEC_FILE_CMD_NONE
};

/*v sl_fifo_obj_methods - Exec file object methods
 */
static t_sl_exec_file_method sl_fifo_object_methods[] =
{
    {"reset",    0, 2, "si",  "compare(<filename>,<offset>) - ]", ef_method_eval_reset, NULL },
    {"flags",    'i', 0, "",  "flags() - return e, f, ewm, fwm flags as 4 bits 0 thru 3", ef_method_eval_flags, NULL },
    {"is_empty", 'i', 0, "",  "is_empty() - return 1 if empty, 0 if not", ef_method_eval_is_empty, NULL },
    {"is_full",  'i', 0, "",  "is_full() - return 1 if full, 0 if not", ef_method_eval_is_full, NULL },
    {"is_nearly_full",  'i', 0, "",  "is_nearly_full() - return 1 if nearly full, 0 if not", ef_method_eval_is_nearly_full, NULL },
    {"is_nearly_empty",  'i', 0, "",  "is_nearly_empty() - return 1 if nearly empty, 0 if not", ef_method_eval_is_nearly_empty, NULL },
    {"has_underflowed", 'i', 0, "",  "has_underflowed() - return 1 if fifo has underflowed, 0 if not", ef_method_eval_has_underflowed, NULL },
    {"has_overflowed",  'i', 0, "",  "has_overflowed() - return 1 if fifo has overflowed, 0 if not", ef_method_eval_has_overflowed, NULL },
    {"read_ptr", 'i', 0, "",  "read_ptr() - return read ptr for the FIFO", ef_method_eval_read_ptr, NULL },
    {"write_ptr", 'i', 0, "",  "write_ptr() - return write ptr for the FIFO", ef_method_eval_write_ptr, NULL },
    {"uncommitted_read_ptr", 'i', 0, "",  "uncommitted_read_ptr() - return uncommitted_read ptr for the FIFO", ef_method_eval_uncommitted_read_ptr, NULL },
    {"remove", 'i', 0, "",  "remove() - remove an element from the FIFO, error if underflow", ef_method_eval_remove, NULL },
    {"peek", 'i', 0, "",  "peek() - look at the top element from the FIFO but do not change pointers, error if underflow", ef_method_eval_peek, NULL },
    {"remove_no_commit", 'i', 0, "",  "remove_no_commit() - remove an element from the FIFO without committing the read ptr, error if underflow", ef_method_eval_remove_no_commit, NULL },
    {"add", 0, 1, "i",  "add(<value>) - add an element to the FIFO, error if overflow", ef_method_eval_add, NULL },
    {"commit_read", 0, 0, "",  "commit_read() - commit the uncommited read ptr of the FIFO after uncommitted reads", ef_method_eval_commit_read, NULL },
    {"revert_read", 0, 0, "",  "revert_read() - revert the uncommited read ptr of the FIFO back to the last committed value", ef_method_eval_revert_read, NULL },
    {"wait_for_flags", 0, 2, "ii",  "wait_for_flags(<mask>,<value>) - wait for FIFO flags AND mask to equal value", ef_method_eval_wait_for_flags, NULL },
    SL_EXEC_FILE_METHOD_NONE
};

/*v fifo_state_desc
 */
SL_EF_STATE_DESC_PTR( __ptr, t_sl_ef_lib_fifo );
static t_sl_exec_file_state_desc_entry fifo_state_desc[] =
{
    SL_EF_STATE_DESC_ENTRY(__ptr, nearly_empty_watermark, 32, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, nearly_full_watermark, 32, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, read_ptr, 32, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, committed_read_ptr, 32, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, write_ptr, 32, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, empty, 1, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, full, 1, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, nearly_empty, 1, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, nearly_full, 1, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, underflowed, 1, 0),
    SL_EF_STATE_DESC_ENTRY(__ptr, overflowed, 1, 0),
    SL_EF_STATE_DESC_END
};

/*a Exec file fns
 */
/*f object_message_handler
 */
static t_sl_error_level object_message_handler( t_sl_exec_file_object_cb *obj_cb )
{
    t_sl_ef_lib_fifo *fifo;
    fifo = &(((t_sl_fifo_object *)(obj_cb->object_desc->handle))->fifo);

    WHERE_I_AM;
    if (!strcmp(obj_cb->data.message.message,"fifo_get"))
    {
        ((t_sl_ef_lib_fifo **)obj_cb->data.message.client_handle)[0] = fifo;
        return error_level_okay;
    }
    WHERE_I_AM;
    return error_level_serious;
}

/*f find_object
 */
static t_sl_fifo_object *find_object( t_sl_fifo_object **object_ptr, const char *name )
{
    t_sl_fifo_object *object;

    for (object=*object_ptr; object; object=object->next_in_list)
    {
        if (!strcmp(name,object->name))
            return object;
    }
    return NULL;
}

/*f add_object
 */
static t_sl_fifo_object *add_object( t_sl_fifo_object **object_ptr, const char *name)
{
    t_sl_fifo_object *object;

    object = (t_sl_fifo_object *)malloc(sizeof(t_sl_fifo_object));
    object->next_in_list = *object_ptr;
    *object_ptr = object;
    object->name = sl_str_alloc_copy( name );

    object->fifo.contents = NULL;
    return object;
}

/*f static exec_file_cmd_handler_cb
 */
static t_sl_error_level exec_file_cmd_handler_cb( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_sl_fifo_object *fifo;

    switch (cmd_cb->cmd)
    {
    case cmd_fifo:
        fifo = find_object((t_sl_fifo_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
        if (!fifo)
        {
            t_sl_exec_file_object_desc object_desc;
            fifo = add_object((t_sl_fifo_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
            sl_fifo_init( &(fifo->fifo),
                          sizeof(t_sl_uint64),
                          sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 ), // Number of elemtnts
                          sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 2 ), // empty wm
                          sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 3 ) ); // full wm
            memset(&object_desc,0,sizeof(object_desc));
            object_desc.version = sl_ef_object_version_checkpoint_restore;
            object_desc.name = fifo->name;
            object_desc.handle = (void *)fifo;
            object_desc.message_handler = object_message_handler;
            object_desc.methods = sl_fifo_object_methods;
            sl_exec_file_add_object_instance( cmd_cb->file_data, &object_desc );
            if (!fifo->fifo.contents)
            {
                fifo->fifo.contents = (t_sl_uint64 *)malloc(fifo->fifo.size * fifo->fifo.entry_int_size * sizeof(int));
            }
            if (fifo->fifo.contents) // This will in general not be true, as we have not created the contents yet...
            {
                sl_exec_file_object_instance_register_state( cmd_cb, fifo->name, "contents", (void *)(fifo->fifo.contents), fifo->fifo.entry_int_size*sizeof(int)*8, fifo->fifo.size );
            }
            sl_exec_file_object_instance_register_state_desc( cmd_cb, fifo->name, fifo_state_desc, (void *)&(fifo->fifo) );
        }
        break;
    default:
        return error_level_serious;
    }
    return error_level_okay;
}

/*f sl_fifo_add_exec_file_enhancements
 */
extern void sl_ef_lib_fifo_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;
    t_sl_fifo_object **object_ptr;
    object_ptr = (t_sl_fifo_object **)malloc(sizeof(t_sl_fifo_object *));
    *object_ptr = NULL;

    lib_desc.version = sl_ef_lib_version_initial;
    lib_desc.library_name = "sys_fifo";
    lib_desc.handle = (void *)object_ptr;
    lib_desc.cmd_handler = exec_file_cmd_handler_cb;
    lib_desc.file_cmds = sl_fifo_cmds;
    lib_desc.file_fns = NULL;
    sl_exec_file_add_library( file_data, &lib_desc );
}

/*a Fifo object methods
 */
/*f ef_method_eval_reset
 */
static t_sl_error_level ef_method_eval_reset( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    sl_fifo_reset( fifo );
    return error_level_okay;
}

/*f ef_method_eval_flags
 */
static t_sl_error_level ef_method_eval_flags( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int flags;
    flags = sl_fifo_flags( fifo, NULL, NULL, NULL, NULL, NULL, NULL );
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)flags))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_is_empty
 */
static t_sl_error_level ef_method_eval_is_empty( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int empty;
    sl_fifo_flags( fifo, &empty, NULL, NULL, NULL, NULL, NULL );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)empty))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_is_nearly_empty
 */
static t_sl_error_level ef_method_eval_is_nearly_empty( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int nearly_empty;
    sl_fifo_flags( fifo, NULL, NULL, &nearly_empty, NULL, NULL, NULL );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)nearly_empty))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_is_full
 */
static t_sl_error_level ef_method_eval_is_full( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int full;
    sl_fifo_flags( fifo, NULL, &full, NULL, NULL, NULL, NULL );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)full))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_is_nearly_full
 */
static t_sl_error_level ef_method_eval_is_nearly_full( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int nearly_full;
    sl_fifo_flags( fifo, NULL, NULL, NULL, &nearly_full, NULL, NULL );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)nearly_full))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_has_underflowed
 */
static t_sl_error_level ef_method_eval_has_underflowed( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int underflowed;
    sl_fifo_flags( fifo, NULL, NULL, NULL, NULL, &underflowed, NULL );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)underflowed))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_has_overflowed
 */
static t_sl_error_level ef_method_eval_has_overflowed( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int overflowed;
    sl_fifo_flags( fifo, NULL, NULL, NULL, NULL, NULL, &overflowed );

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)overflowed))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_read_ptr
 */
static t_sl_error_level ef_method_eval_read_ptr( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int committed_read_ptr;
    sl_fifo_ptrs( fifo, NULL, &committed_read_ptr, NULL );
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)committed_read_ptr))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_write_ptr
 */
static t_sl_error_level ef_method_eval_write_ptr( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int write_ptr;
    sl_fifo_ptrs( fifo, NULL, NULL, &write_ptr );
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)write_ptr))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_uncommitted_read_ptr
 */
static t_sl_error_level ef_method_eval_uncommitted_read_ptr( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int read_ptr;
    sl_fifo_ptrs( fifo, &read_ptr, NULL, NULL );
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)read_ptr))
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_remove
 */
static t_sl_error_level ef_method_eval_remove( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int ptr;
    ptr = sl_fifo_remove_entry( fifo, NULL );
    sl_fifo_commit_reads( fifo );

    if ( (ptr<0) ||
         (!fifo->contents) ||
         (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64) (fifo->contents[ptr]))) )
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_remove_no_commit
 */
static t_sl_error_level ef_method_eval_remove_no_commit( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int ptr;
    ptr = sl_fifo_remove_entry( fifo, NULL );

    if ( (ptr<0) ||
         (!fifo->contents) ||
         (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, fifo->contents[ptr])) )
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_peek
 */
static t_sl_error_level ef_method_eval_peek( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    int ptr;
    sl_fifo_commit_reads( fifo );
    ptr = sl_fifo_remove_entry( fifo, NULL );
    sl_fifo_revert_reads( fifo );

    if ( (ptr<0) ||
         (!fifo->contents) ||
         (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, fifo->contents[ptr])) )
        return error_level_fatal;

    return error_level_okay;
}

/*f ef_method_eval_add
 */
static t_sl_error_level ef_method_eval_add( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    WHERE_I_AM;
    int ptr;
    ptr = sl_fifo_add_entry( fifo, NULL );

    if ( (ptr<0) ||
         !sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64) (ptr>=0)))
        return error_level_fatal;

    if (fifo->contents)
    {
        fifo->contents[ptr] = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 );
    }

    return error_level_okay;
}

/*f ef_method_eval_commit_read
 */
static t_sl_error_level ef_method_eval_commit_read( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    sl_fifo_commit_reads( fifo );

    return error_level_okay;
}

/*f ef_method_eval_revert_read
 */
static t_sl_error_level ef_method_eval_revert_read( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    sl_fifo_revert_reads( fifo );

    return error_level_okay;
}

/*f static sl_ef_lib_fifo_wait_cb - callback for exec file thread wait
 */
static int sl_ef_lib_fifo_wait_cb( t_sl_exec_file_wait_cb *wait_cb )
{
    t_sl_ef_lib_fifo *fifo;
    int flags;
    fifo = (t_sl_ef_lib_fifo *)wait_cb->args[0].pointer;
    flags = sl_fifo_flags( fifo, NULL, NULL, NULL, NULL, NULL, NULL );
    return ((flags & wait_cb->args[1].uint64)==wait_cb->args[2].uint64);
}

/*f ef_method_eval_wait_for_flags
 */
static t_sl_error_level ef_method_eval_wait_for_flags( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_fifo *fifo;
    WHERE_I_AM;
    fifo = &(((t_sl_fifo_object *)(object_desc->handle))->fifo);

    t_sl_exec_file_wait_cb wait_cb;
    wait_cb.args[0].pointer = (void *)fifo;
    wait_cb.args[1].uint64 = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 );
    wait_cb.args[2].uint64 = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
    if (!sl_ef_lib_fifo_wait_cb(&wait_cb))
        sl_exec_file_thread_wait_on_callback( cmd_cb, sl_ef_lib_fifo_wait_cb, &wait_cb );
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

