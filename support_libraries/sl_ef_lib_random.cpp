/*a Copyright
  
  This file 'sl_random.cpp' copyright Gavin J Stark 2003, 2004, 2007, 2008
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdlib.h>
#include <string.h>
#include "sl_random.h"
#include "sl_exec_file.h"

/*a Defines
 */
#if 0
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif

/*a Types
 */
/*t t_sl_random_object
 */
typedef struct t_sl_random_object
{
    t_sl_random_object *next_in_list;
    char *name;
    t_sl_random random;
} t_sl_random_object;

/*t cmd_* - Commands provided for extending the sl_exec_file
 */
enum
{
    cmd_random_nial,
    cmd_random_cyclic,
};

/*a Static forward function declarations
 */
static t_sl_exec_file_method_fn ef_method_eval_next;
static t_sl_exec_file_method_fn ef_method_eval_seed;

/*a Statics
 */
/*v sl_random_cmds - Exec file command extensions - map string and argument types to the command number
 */
static t_sl_exec_file_cmd sl_random_cmds[] =
{
    {cmd_random_nial,             4, "!random_nial", "siii", "random_nial <name> <iters> <base> <range>"},
    {cmd_random_cyclic,           5, "!random_cyclic", "siiii", "random_cyclic <name> <value> <base> <range> <step>"},
    SL_EXEC_FILE_CMD_NONE
};

/*v sl_random_obj_methods - Exec file object methods
 */
static t_sl_exec_file_method sl_random_object_methods[] =
{
    {"next",  'i', 0, "",  "next() - return next value", ef_method_eval_next, NULL },
    {"seed",  0,  1, "s", "seed(<seed string>) - reseed the random number generator", ef_method_eval_seed, NULL },
    SL_EXEC_FILE_METHOD_NONE
};

/*a Exec file fns
 */
/*f object_message_handler
 */
static t_sl_error_level object_message_handler( t_sl_exec_file_object_cb *obj_cb )
{
    t_sl_random *rnd;
    rnd = &(((t_sl_random_object *)(obj_cb->object_desc->handle))->random);

    if (!strcmp(obj_cb->data.message.message,"random_get"))
    {
        ((t_sl_random **)obj_cb->data.message.client_handle)[0] = rnd;
        return error_level_okay;
    }
    return error_level_serious;
}

/*f find_object
 */
static t_sl_random_object *find_object( t_sl_random_object **object_ptr, const char *name )
{
    t_sl_random_object *object;

    for (object=*object_ptr; object; object=object->next_in_list)
    {
        if (!strcmp(name,object->name))
            return object;
    }
    return NULL;
}

/*f add_object
 */
static t_sl_random_object *add_object( t_sl_random_object **object_ptr, const char *name, int extra_numbers )
{
    t_sl_random_object *object;

    object = (t_sl_random_object *)malloc(sizeof(t_sl_random_object) + sizeof(t_sl_uint64)*extra_numbers );
    object->next_in_list = *object_ptr;
    *object_ptr = object;
    object->name = sl_str_alloc_copy( name );

    object->random.extra_values = extra_numbers;
    object->random.seed[0] = 0;
    object->random.seed[1] = 0;
    object->random.value = 0;
    object->random.start = 0;
    object->random.range = 1;
    return object;
}

/*f exec_file_cmd_handler_cb
 */
static t_sl_error_level exec_file_cmd_handler_cb( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_sl_random_object *rnd;

    WHERE_I_AM;
    //fprintf(stderr, "%p:%p:%p:%p\n",cmd_cb,handle,cmd_cb->args,cmd_cb->args[0].p.ptr);
    switch (cmd_cb->cmd)
    {
    case cmd_random_cyclic: // Declarative command - creates an object
        rnd = find_object((t_sl_random_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
        if (!rnd)
        {
            t_sl_exec_file_object_desc object_desc;
            rnd = add_object((t_sl_random_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ), 0);
            rnd->random.type = sl_random_type_cyclic;
            rnd->random.value = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
            rnd->random.start = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 2 );
            rnd->random.range = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 3 );
            rnd->random.data.cyclic = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 4 );
            memset(&object_desc,0,sizeof(object_desc));
            object_desc.version = sl_ef_object_version_checkpoint_restore;
            object_desc.name = rnd->name;
            object_desc.handle = (void *)rnd;
            object_desc.message_handler = object_message_handler;
            object_desc.methods = sl_random_object_methods;
            sl_exec_file_add_object_instance( cmd_cb->file_data, &object_desc );
        }
        break;
    case cmd_random_nial:
        rnd = find_object((t_sl_random_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
        if (!rnd)
        {
            t_sl_exec_file_object_desc object_desc;
            rnd = add_object((t_sl_random_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ), 0);
            rnd->random.type = sl_random_type_nial;
            rnd->random.data.nial = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 1 );
            rnd->random.start = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 2 );
            rnd->random.range = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 3 );
            memset(&object_desc,0,sizeof(object_desc));
            object_desc.version = sl_ef_object_version_checkpoint_restore;
            object_desc.name = rnd->name;
            object_desc.handle = (void *)rnd;
            object_desc.message_handler = object_message_handler;
            object_desc.methods = sl_random_object_methods;
            sl_exec_file_add_object_instance( cmd_cb->file_data, &object_desc );
        }
        break;
    default:
        return error_level_serious;
    }
    return error_level_okay;
}

/*f sl_ef_lib_random_add_exec_file_enhancements
 */
extern void sl_ef_lib_random_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;
    t_sl_random_object **object_ptr;
    object_ptr = (t_sl_random_object **)malloc(sizeof(t_sl_random_object *));
    *object_ptr = NULL;

    lib_desc.version = sl_ef_lib_version_initial;
    lib_desc.library_name = "sys_random";
    lib_desc.handle = (void *)object_ptr;
    lib_desc.cmd_handler = exec_file_cmd_handler_cb;
    lib_desc.file_cmds = sl_random_cmds;
    lib_desc.file_fns = NULL;
    sl_exec_file_add_library( file_data, &lib_desc );
}

/*f ef_method_eval_next
 */
static t_sl_error_level ef_method_eval_next( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_random *rnd;
    rnd = &(((t_sl_random_object *)(object_desc->handle))->random);
    return sl_exec_file_eval_fn_set_result( cmd_cb->file_data, sl_random_number(rnd) )?error_level_okay:error_level_fatal;
}

/*f ef_method_eval_seed
 */
static t_sl_error_level ef_method_eval_seed( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_random *rnd;
    rnd = &(((t_sl_random_object *)(object_desc->handle))->random);
    sl_random_setstate( rnd, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ) );
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

