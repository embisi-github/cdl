/*a Copyright
  
  This file 'sl_ef_lib_event.cpp' copyright Gavin J Stark 2003, 2004, 2007, 2008
  
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
#include "sl_ef_lib_event.h"
#include "sl_exec_file.h"

/*a Defines
 */
#if 0
#define WHERE_I_AM {fprintf(stderr,"%s:%s:%d\n",__FILE__,__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif

/*a Types
 */
/*t t_sl_ef_lib_event
 */
typedef struct t_sl_ef_lib_event
{
    int fired;
} t_sl_ef_lib_event;

/*t t_sl_event_object
 */
typedef struct t_sl_event_object
{
    t_sl_event_object *next_in_list;
    char *name;
    t_sl_ef_lib_event event;
} t_sl_event_object;

/*t cmd_* - Commands provided for extending the sl_exec_file
 */
enum
{
    cmd_event,
};

/*a Static forward function declarations
 */
static t_sl_exec_file_method_fn ef_method_eval_reset;
static t_sl_exec_file_method_fn ef_method_eval_fire;
static t_sl_exec_file_method_fn ef_method_eval_wait;
static t_sl_exec_file_method_fn ef_method_eval_fired;

/*a Statics
 */
/*v sl_event_cmds - Exec file command extensions - map string and argument types to the command number
 */
static t_sl_exec_file_cmd sl_event_cmds[] =
{
    {cmd_event,         1, "!event", "s",       "event <name>" },
    SL_EXEC_FILE_CMD_NONE
};

/*v sl_event_obj_methods - Exec file object methods
 */
static t_sl_exec_file_method sl_event_object_methods[] =
{
    {"reset",     0,   0, "",  "reset() - reset the event, so it can fire", ef_method_eval_reset, NULL },
    {"fire",      0,   0, "",  "fire() - fire the event", ef_method_eval_fire, NULL },
    {"wait",      0,   0, "",  "wait() - wait for the event", ef_method_eval_wait, NULL },
    {"fired",     'i', 0, "",  "fired() - return the state of the event - 1 if it has fired, else 0", ef_method_eval_fired, NULL },
    SL_EXEC_FILE_METHOD_NONE
};

/*a Event methods
 */
/*f sl_ef_lib_event_reset - reset the event, and return whether it was fired beforehand
 */
extern int sl_ef_lib_event_reset( t_sl_ef_lib_event_ptr event )
{
    int i;
    WHERE_I_AM;
    i = event->fired;
    event->fired = 0;
    return i;
}

/*f sl_ef_lib_event_fire - fire the event, and return whether it was previously fired
 */
extern int sl_ef_lib_event_fire( t_sl_ef_lib_event_ptr event )
{
    int i;
    WHERE_I_AM;
    i = event->fired;
    event->fired = 1;
    return i;
}

/*f sl_ef_lib_event_fired - return state of the event
 */
extern int sl_ef_lib_event_fired( t_sl_ef_lib_event_ptr event )
{
    WHERE_I_AM;
    return event->fired;
}

/*a Exec file fns
 */
/*f object_message_handler
 */
static t_sl_error_level object_message_handler( t_sl_exec_file_object_cb *obj_cb )
{
    t_sl_ef_lib_event *event;
    WHERE_I_AM;
    event = &(((t_sl_event_object *)(obj_cb->object_desc->handle))->event);

    if (!strcmp(obj_cb->data.message.message,"event_handle"))
    {
        ((t_sl_ef_lib_event **)obj_cb->data.message.client_handle)[0] = event;
        return error_level_okay;
    }
    WHERE_I_AM;
    return error_level_serious;
}

/*f find_object
 */
static t_sl_event_object *find_object( t_sl_event_object **object_ptr, const char *name )
{
    t_sl_event_object *object;

    for (object=*object_ptr; object; object=object->next_in_list)
    {
        if (!strcmp(name,object->name))
            return object;
    }
    return NULL;
}

/*f add_object
 */
static t_sl_event_object *add_object( t_sl_event_object **object_ptr, const char *name)
{
    t_sl_event_object *object;

    object = (t_sl_event_object *)malloc(sizeof(t_sl_event_object));
    object->next_in_list = *object_ptr;
    *object_ptr = object;
    object->name = sl_str_alloc_copy( name );

    sl_ef_lib_event_reset( &(object->event) );
    return object;
}

/*f exec_file_cmd_handler_cb
 */
static t_sl_error_level exec_file_cmd_handler_cb( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_sl_event_object *event;

    switch (cmd_cb->cmd)
    {
    case cmd_event:
        event = find_object((t_sl_event_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
        if (!event)
        {
            t_sl_exec_file_object_desc object_desc;
            event = add_object((t_sl_event_object **)handle, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));

            memset(&object_desc,0,sizeof(object_desc));
            object_desc.version = sl_ef_object_version_checkpoint_restore;
            object_desc.name = event->name;
            object_desc.handle = (void *)event;
            object_desc.message_handler = object_message_handler;
            object_desc.methods = sl_event_object_methods;
            sl_exec_file_add_object_instance( cmd_cb->file_data, &object_desc );
        }
        break;
    default:
        return error_level_serious;
    }
    return error_level_okay;
}

/*f sl_event_add_exec_file_enhancements
 */
extern void sl_ef_lib_event_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;
    t_sl_event_object **object_ptr;
    object_ptr = (t_sl_event_object **)malloc(sizeof(t_sl_event_object *));
    *object_ptr = NULL;

    lib_desc.version = sl_ef_lib_version_initial;
    lib_desc.library_name = "sys.event";
    lib_desc.handle = (void *)object_ptr;
    lib_desc.cmd_handler = exec_file_cmd_handler_cb;
    lib_desc.file_cmds = sl_event_cmds;
    lib_desc.file_fns = NULL;
    sl_exec_file_add_library( file_data, &lib_desc );
}

/*f static sl_ef_lib_event_wait_cb - callback for exec file thread wait
 */
static int sl_ef_lib_event_wait_cb( t_sl_exec_file_wait_cb *wait_cb )
{
    return sl_ef_lib_event_fired( (t_sl_ef_lib_event *)wait_cb->args[0].pointer );
}
/*a Event object methods
 */
/*f sl_ef_lib_event_create
 */
extern t_sl_ef_lib_event_ptr sl_ef_lib_event_create( char *name )
{
    t_sl_ef_lib_event_ptr event;
    event = (t_sl_ef_lib_event_ptr)malloc(sizeof(t_sl_ef_lib_event));
    if (event) sl_ef_lib_event_reset( event );
    return event;
}

/*f sl_ef_lib_event_free
 */
extern void sl_ef_lib_event_free( t_sl_ef_lib_event_ptr event )
{
    if (event)
        free(event);
}

/*f ef_method_eval_reset
 */
static t_sl_error_level ef_method_eval_reset( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_event *event;
    WHERE_I_AM;
    event = &(((t_sl_event_object *)(object_desc->handle))->event);

    sl_ef_lib_event_reset( event );
    return error_level_okay;
}

/*f ef_method_eval_fire
 */
static t_sl_error_level ef_method_eval_fire( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_event *event;
    WHERE_I_AM;
    event = &(((t_sl_event_object *)(object_desc->handle))->event);

    sl_ef_lib_event_fire( event );
    return error_level_okay;
}

/*f ef_method_eval_fired
 */
static t_sl_error_level ef_method_eval_fired( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_event *event;
    WHERE_I_AM;
    event = &(((t_sl_event_object *)(object_desc->handle))->event);

    int result;
    result = event->fired;

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64)result))
        return error_level_fatal;
    return error_level_okay;
}

/*f ef_method_eval_wait
 */
static t_sl_error_level ef_method_eval_wait( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_sl_ef_lib_event *event;
    WHERE_I_AM;
    event = &(((t_sl_event_object *)(object_desc->handle))->event);

    if (!event->fired)
    {
        t_sl_exec_file_wait_cb wait_cb;
        wait_cb.args[0].pointer = (void *)event;
        sl_exec_file_thread_wait_on_callback( cmd_cb, sl_ef_lib_event_wait_cb, &wait_cb );
    }
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

