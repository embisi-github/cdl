/*a Copyright
  
  This file 'c_se_engine__log.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Documentation
// Want module instances such as SRAM to have calls such as
// log_read_event = log_event_register( "read", "address", &value_store, NULL); OR
// log_event_array_handle = log_event_register_array( &descriptor, values_base );
// with descriptor of { { "read", 1, { "address", (&value_store-values_base) } }, {NULL, 0}}
// Then
// log_event_occurred(log_read_event);
// possibly first with value_store=address
// The consume would do
// log_handle = log_interest("hierarchy", callback, handle ); // log_handle includes all events in the module specified by the hierarchy - it is a set, not a single event
// log_parse_read_handle = log_parse_setup( log_handle, "read", 1, "address" ); // parse all 'read' events in log_handle so that their 'address' field values are visible as arg 0 to 
// callback( engine_handle, log_handle, handle, log_event_instance ) // engine_handle is instance that invoked log_event_occurred
//    t_se_signal_value args[1];
//    if (log_parse( log_event_instance, log_parse_read_handle, args )) // using above structure, check it is a read and find the args, i.e. args[0]=address
// 
 */
/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "se_engine_function.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*t t_engine_log_event_callback_fn_instance
  The ownership is actually in an array within t_engine_log_interest - the list is per individual event
 */
typedef struct t_engine_log_event_callback_fn_instance
{
    struct t_engine_log_event_callback_fn_instance *next_in_list; // List of callbacks for a particular event
    struct t_engine_log_interest *log_handle; // Which t_engine_log_interest that this callback is part of
    struct t_engine_log_event_array_entry *event; // Which event this is a callback for (i.e. event->list of callbacks includes us, us->next_in_list etc)
} t_engine_log_event_callback_fn_instance;

/*t t_engine_log_interest
  An array of t_engine_log_event_callback_fn_instance's belonging to a client, not the logging module
 */
typedef struct t_engine_log_interest
{
    int num_events;
    t_engine_log_event_callback_fn callback_fn;
    void *handle;
    t_engine_log_event_callback_fn_instance callbacks[1]; // extends
} t_engine_log_interest;

/*t t_engine_log_event_array_entry
 */
typedef struct t_engine_log_event_array_entry
{
    const char *name;
    int num_args;
    t_se_signal_value *value_base;
    t_engine_log_event_callback_fn_instance *callback_list; // Not a mallocked link
    t_engine_text_value_pair *entries; // Not a mallocked link - array of (text,value offset) pairs
} t_engine_log_event_array_entry;

/*t t_engine_log_event_array
  Should be allocked of size t_engine_log_event_array_entry*num_events + t_engine_text_value_pair*SUM(num_args for all events)
 */
typedef struct t_engine_log_event_array
{
    struct t_engine_log_event_array *next_in_list;
    int num_events;
    t_engine_log_event_array_entry events[1];
} t_engine_log_event_array;

/*t t_engine_log_parser
  Matches a number of t_engine_log_event_array's, possibly, all with the same name
  Is a set of these events, with for each a reference back to the event, plus a mapping for 'n' consecutive client slots to 'n' event args, i.e. an array length 'n' of event arg numbers for that event
 */
typedef struct t_engine_log_parser
{
    int num_args;
    int num_events;
    t_engine_log_event_array_entry **events; // num_events
    int                            *arg_mapping; // num_args*num_events
} t_engine_log_parser;

/*t t_log_event_ef_lib
 */
typedef struct t_log_event_ef_lib
{
    struct t_sl_exec_file_data *file_data;
    c_engine *engine;
    void *engine_handle;
    int phase;
    int num_events;
    int total_args;
    int next_event;
    int next_arg;
    struct t_log_event_ef_object *log_events;
    t_engine_text_value_pair *log_desc; // malloced
    t_se_signal_value *values; // malloced
    t_engine_log_event_array *log_event_array;
} t_log_event_ef_lib;

/*t t_log_event_ef_object
 */
typedef struct t_log_event_ef_object
{
    struct t_log_event_ef_object *next_in_list;
    t_log_event_ef_lib *ef_lib;
    int num_args;
    int event_number;
    int arg_offset;
    const char *name;
    const char *arg_name[SL_EXEC_FILE_MAX_CMD_ARGS];
} t_log_event_ef_object;

/*a Static function declarations
 */
static t_sl_exec_file_method_fn ef_method_eval_occurred;
static void ef_poststate_callback( void *handle );

/*a Statics
 */
/*v sim_file_cmds
 */
static t_sl_exec_file_cmd sim_file_cmds[] =
{
     {0,        1, "!log_event", "ssssssssssssssss", "log_event <name> [<args>]"},
     SL_EXEC_FILE_CMD_NONE
};

/*v sl_vcd_file_obj_methods - Exec file object methods
 */
static t_sl_exec_file_method sl_log_event_object_methods[] =
{
    {"occurred", 'i',  0, "iiiiiiiiiiiiiiii",  "occurred() - event occurrence", ef_method_eval_occurred },
    SL_EXEC_FILE_METHOD_NONE
};

/*a Allocation functions
 */
/*t log_event_array_create
 */
t_engine_log_event_array *c_engine::log_event_array_create( void *engine_handle, int num_events, int total_args )
{
    t_engine_log_event_array *lea;
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *) engine_handle;

    lea = (t_engine_log_event_array *)malloc( sizeof(t_engine_log_event_array) + 
                                              (num_events-1)*sizeof(t_engine_log_event_array) + 
                                              total_args*sizeof(t_engine_text_value_pair) );
    if (!lea) return NULL;
    lea->next_in_list = emi->log_event_list;
    emi->log_event_list = lea;
    lea->num_events = num_events;
    return lea;
}

/*t log_interest_create
 */
struct t_engine_log_interest *c_engine::log_interest_create( void *engine_handle, int num_events, t_engine_log_event_callback_fn callback_fn, void *handle )
{
    t_engine_log_interest *eli;
    int i;

    eli = (t_engine_log_interest *)malloc(sizeof(t_engine_log_interest)+(num_events-1)*sizeof(t_engine_log_event_callback_fn_instance));
    eli->num_events = num_events;
    eli->callback_fn = callback_fn;
    eli->handle = handle;
    for (i=0; i<num_events; i++)
    {
        eli->callbacks[i].next_in_list = NULL;
        eli->callbacks[i].log_handle = NULL;
        eli->callbacks[i].event = NULL;
    }
    return eli;
}

/*t log_parse_create
 */
struct t_engine_log_parser *c_engine::log_parse_create( void *engine_handle, int num_events, int num_args )
{
    t_engine_log_parser *elp;
    elp = (t_engine_log_parser *)malloc(sizeof(t_engine_log_parser) + sizeof(t_engine_log_event_array_entry *)*num_events + num_events*num_args*sizeof(int));
    elp->num_events = num_events;
    elp->num_args = num_args;
    elp->events      = (t_engine_log_event_array_entry **)(&elp[1]);
    elp->arg_mapping = (int *)(&elp->events[num_events]);
    return elp;
}

/*a Registration by loggers
 */
/*f c_engine::log_event_register
 */
struct t_engine_log_event_array *c_engine::log_event_register( void *engine_handle, const char *event_name, t_se_signal_value *value_base, ... )
{
    const char *value_name;
    int value_offset;
    int i;
    int num_args;
    t_engine_log_event_array *event_array;

    va_list ap;
    va_start( ap, value_base );
    num_args=0;
    value_name = va_arg( ap, const char * );
    while (value_name)
    {
        value_offset = va_arg( ap, int );
        num_args++;
        value_name = va_arg( ap, const char * );
    }
    va_end(ap);
    
    event_array = log_event_array_create( engine_handle, 1, num_args );
    if (!event_array) return NULL;

    event_array->events[0].name = event_name;
    event_array->events[0].num_args = num_args;
    event_array->events[0].callback_list = NULL;
    event_array->events[0].value_base = value_base;
    event_array->events[0].entries = (t_engine_text_value_pair *)&event_array->events[1];
    va_start( ap, value_base );
    for (i=0; i<num_args; i++)
    {
        event_array->events[0].entries[num_args].text = va_arg( ap, const char * );
        event_array->events[0].entries[num_args].value = va_arg( ap, int );
    }
    va_end(ap);
    return event_array;
}

/*f log_event_register_array
 */
struct t_engine_log_event_array *c_engine::log_event_register_array( void *engine_handle, const t_engine_text_value_pair *descriptor, t_se_signal_value *value_base )
{
    int i, j, k;
    int num_events;
    int total_args;
    struct t_engine_log_event_array *event_array;
    t_engine_text_value_pair *next_entry;

    total_args=0;
    for (num_events=i=0; descriptor[i].text!=NULL; num_events++ )
    {
        total_args += descriptor[i].value;
        i += 1+descriptor[i].value;
    }

    event_array = log_event_array_create( engine_handle, num_events, total_args );
    if (!event_array) return NULL;

    next_entry = (t_engine_text_value_pair *)&event_array->events[num_events];
    for (i=k=0; i<num_events; i++)
    {
        event_array->events[i].name = descriptor[k].text;
        event_array->events[i].num_args = descriptor[k].value;
        event_array->events[i].callback_list = NULL;
        event_array->events[i].value_base = value_base;
        event_array->events[i].entries = next_entry;
        k++;
        for (j=0; j<event_array->events[i].num_args; j++)
        {
            event_array->events[i].entries[j].text = descriptor[k].text;
            event_array->events[i].entries[j].value = descriptor[k].value;
            k++;
        }
        next_entry += event_array->events[i].num_args;
    }
    return event_array;
}

/*f log_event_occurred
 */
void c_engine::log_event_occurred( void *engine_handle, struct t_engine_log_event_array *event_array, int event_number )
{
    t_engine_log_event_callback_fn_instance *cfi;
    for (cfi=event_array->events[event_number].callback_list; cfi; cfi=cfi->next_in_list)
    {
        cfi->log_handle->callback_fn( engine_handle, cycle(), cfi->log_handle, cfi->log_handle->handle, cfi - cfi->log_handle->callbacks,  &(event_array->events[event_number]) );
    }
}

/*a Client functions
 */
/*f c_engine::log_interest - declare a callback for all log events in a module instance, or a particular named event type within a module instance
  module_instance comes from a call to find_module_instance
 */
struct t_engine_log_interest *c_engine::log_interest( void *engine_handle, void *module_instance, const char *event_name, t_engine_log_event_callback_fn callback_fn, void *handle )
{
    t_engine_module_instance *emi;
    t_engine_log_event_array *event_list;
    t_engine_log_interest *log_handle;
    int num_events;
    int i;

    emi = (t_engine_module_instance *) module_instance;
    if (!emi) return NULL;

    // Count events that match the name
    num_events=0;
    for (event_list=emi->log_event_list; event_list; event_list=event_list->next_in_list )
    {
        for (i=0; i<event_list->num_events; i++)
        {
            if ((!event_name) || (!strcmp(event_name,event_list->events[i].name)))
            {
                num_events++;
            }
        }
    }

    // Create array
    log_handle = log_interest_create( engine_handle, num_events, callback_fn, handle );

    // Fill array
    num_events=0;
    for (event_list=emi->log_event_list; event_list; event_list=event_list->next_in_list )
    {
        for (i=0; i<event_list->num_events; i++)
        {
            if ((!event_name) || (!strcmp(event_name,event_list->events[i].name)))
            {
                t_engine_log_event_array_entry *event;
                event = &(event_list->events[i]);
                log_handle->callbacks[num_events].next_in_list = event->callback_list;
                event->callback_list = &log_handle->callbacks[num_events];
                log_handle->callbacks[num_events].log_handle = log_handle;
                log_handle->callbacks[num_events].event = event;
                num_events++;
            }
        }
    }

    return log_handle;
}

/*f c_engine::log_parse_setup
 */
struct t_engine_log_parser *c_engine::log_parse_setup( void *engine_handle, struct t_engine_log_interest *log_handle, const char *event_name, const char **value_names )
{
    int num_events;
    t_engine_log_event_array_entry *event;
    t_engine_log_parser *parse;
    int num_args;
    int i, j, k;

    for (num_args=0; value_names[num_args]; num_args++);

    num_events=0;
    for (i=0; i<log_handle->num_events; i++)
    {
        event = log_handle->callbacks[i].event;
        if (!strcmp(event->name, event_name))
        {
            num_events++;
        }
    }

    parse = log_parse_create( engine_handle, num_events, num_args );

    parse->num_args = num_args;
    parse->num_events = num_events;

    num_events=0;
    for (i=0; i<log_handle->num_events; i++)
    {
        event = log_handle->callbacks[i].event;
        if (!strcmp(event->name, event_name))
        {
            parse->events[num_events] = event;
            for (j=0; j<num_args; j++)
            {
                parse->arg_mapping[num_events*num_args+j] = -1;
                for (k=0; k<event->num_args; k++)
                {
                    if (!strcmp(value_names[j], event->entries[k].text ))
                    {
                        parse->arg_mapping[num_events*num_args+j] = k;
                    }
                }
            }
            num_events++;
        }
    }
    return parse;
}

/*f c_engine::log_parse_setup
 */
struct t_engine_log_parser *c_engine::log_parse_setup( void *engine_handle, struct t_engine_log_interest *log_handle, const char *event_name, int num_args, ... )
{  // args are const char *value_names
    va_list ap;
    int num_events;
    t_engine_log_event_array_entry *event;
    t_engine_log_parser *parse;
    int i, j, k;

    num_events=0;
    for (i=0; i<log_handle->num_events; i++)
    {
        event = log_handle->callbacks[i].event;
        if (!strcmp(event->name, event_name))
        {
            num_events++;
        }
    }

    parse = log_parse_create( engine_handle, num_events, num_args );

    parse->num_args = num_args;
    parse->num_events = num_events;

    num_events=0;
    for (i=0; i<log_handle->num_events; i++)
    {
        event = log_handle->callbacks[i].event;
        if (!strcmp(event->name, event_name))
        {
            const char *value_name;
            parse->events[num_events] = event;
            va_start( ap, num_args );
            for (j=0; j<num_args; j++)
            {
                value_name=va_arg( ap, const char *);
                parse->arg_mapping[num_events*num_args+j] = -1;
                for (k=0; k<event->num_args; k++)
                {
                    if (!strcmp(value_name, event->entries[k].text ))
                    {
                        parse->arg_mapping[num_events*num_args+j] = k;
                    }
                }
            }
            va_end( ap );
            num_events++;
        }
    }
    return parse;
}

/*f c_engine::log_parse
 */
int c_engine::log_parse( struct t_engine_log_event_array_entry *event, struct t_engine_log_parser *log_parser, t_se_signal_value **arg_array )
{
    int i, j;

    for (i=0; i<log_parser->num_events; i++)
    {
        if (event==log_parser->events[i])
        {
            for (j=0; j<log_parser->num_args; j++)
            {
                int maps_to;
                maps_to = log_parser->arg_mapping[log_parser->num_args*i+j];
                if (maps_to<0)
                {
                    arg_array[j] = NULL;
                }
                else
                {
                    arg_array[j] = event->value_base + event->entries[maps_to].value;
                }
            }
        }
        return 1;
    }
    return 0;
}

/*f c_engine::log_get_event_interest_desc - returns number of events, and detail of a specific event
 */
int c_engine::log_get_event_interest_desc( struct t_engine_log_interest *log_handle, int n, struct t_engine_log_event_array_entry **event )
{
    if ((n<0) || (n>=log_handle->num_events))
        return log_handle->num_events;

    if (event) { (*event) = log_handle->callbacks[n].event; }
    return log_handle->num_events;
}

/*f c_engine::log_get_event_desc
 */
int c_engine::log_get_event_desc( struct t_engine_log_event_array_entry *event, const char **event_name, t_se_signal_value **value_base, t_engine_text_value_pair **args )
{
    if (!event) return -1;
    if (value_base) *value_base = event->value_base;
    if (args) *args = event->entries;
    if (event_name) *event_name = event->name;
    return event->num_args;
}

/*a Exec file functions
 */
/*f static exec_file_cmd_handler
 */
static t_sl_error_level exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_log_event_ef_lib *log_event_ef_lib;
    log_event_ef_lib = (t_log_event_ef_lib *)handle;
    if (!log_event_ef_lib->engine->log_handle_exec_file_command( log_event_ef_lib, cmd_cb->file_data, cmd_cb->cmd, cmd_cb->num_args, cmd_cb->args ))
        return error_level_serious;
    return error_level_okay;
}

/*f c_engine::log_add_exec_file_enhancements
 */
int c_engine::log_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle )
{
    t_sl_exec_file_lib_desc lib_desc;
    t_log_event_ef_lib *log_event_ef_lib;
    log_event_ef_lib = (t_log_event_ef_lib *)malloc(sizeof(t_log_event_ef_lib));
    log_event_ef_lib->file_data = file_data;
    log_event_ef_lib->engine = this;
    log_event_ef_lib->engine_handle = engine_handle;
    log_event_ef_lib->log_events = NULL;
    log_event_ef_lib->num_events = 0;
    log_event_ef_lib->total_args = 0;
    log_event_ef_lib->log_desc = NULL;
    log_event_ef_lib->values = NULL;
    log_event_ef_lib->log_event_array = NULL;
    if (!sl_exec_file_add_poststate_callback( file_data, ef_poststate_callback, (void *)log_event_ef_lib )) return 0;
    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "cdlsim.log";
    lib_desc.handle = (void *) log_event_ef_lib;
    lib_desc.cmd_handler = exec_file_cmd_handler;
    lib_desc.file_cmds = sim_file_cmds;
    lib_desc.file_fns = NULL;
    return sl_exec_file_add_library( file_data, &lib_desc );
}

/*f ef_event_object_message_handler
 */
static t_sl_error_level ef_event_object_message_handler( t_sl_exec_file_object_cb *obj_cb )
{
    t_log_event_ef_object *leeo;
    leeo = (t_log_event_ef_object *)(obj_cb->object_desc->handle);

    if (!strcmp(obj_cb->data.message.message,"log_event_post_state"))
    {
        int i, x;
        leeo->event_number = leeo->ef_lib->next_event;
        leeo->arg_offset = leeo->ef_lib->next_arg;
        x = leeo->ef_lib->next_event + leeo->ef_lib->next_arg;
        leeo->ef_lib->log_desc[x+0].text = leeo->name;
        leeo->ef_lib->log_desc[x+0].value = leeo->num_args;
        for (i=0; i<leeo->num_args; i++)
        {
            leeo->ef_lib->log_desc[x+1+i].text = leeo->arg_name[i];
            leeo->ef_lib->log_desc[x+1+i].value = leeo->arg_offset+i;
        }
        leeo->ef_lib->next_event++;
        leeo->ef_lib->next_event+=leeo->num_args;
        return error_level_okay;
    }
    return error_level_serious;
}

/*f ef_poststate_callback
 */
void ef_poststate_callback( void *handle )
{
    t_log_event_ef_lib *log_event_ef_lib;
    log_event_ef_lib = (t_log_event_ef_lib *)handle;
    if (log_event_ef_lib->num_events<1) return;
    // Malloc structure, link to ?; structure has text/value pairs for event names/num_args plus each arg/offset
    log_event_ef_lib->values = (t_se_signal_value *) malloc(sizeof(t_se_signal_value)*log_event_ef_lib->total_args);
    log_event_ef_lib->log_desc = (t_engine_text_value_pair *) malloc(sizeof(t_engine_text_value_pair)*(log_event_ef_lib->num_events+log_event_ef_lib->total_args+1));
    log_event_ef_lib->next_event = 0;
    log_event_ef_lib->next_arg = 0;
    sl_exec_file_send_message_to_all_objects( log_event_ef_lib->file_data, "log_event_post_state", (void *)log_event_ef_lib ); // Fill structure, and fill events with their pointers ready for events to occur
    log_event_ef_lib->log_desc[log_event_ef_lib->next_event+log_event_ef_lib->next_arg].text = NULL; // terminator
    // Declare the log events so interest can be registered and so occurrences can, er, occur
    log_event_ef_lib->log_event_array = log_event_ef_lib->engine->log_event_register_array( log_event_ef_lib->engine_handle, log_event_ef_lib->log_desc, log_event_ef_lib->values );
    free(log_event_ef_lib->log_desc);
}

/*f c_engine::log_handle_exec_file_command
 */
int c_engine::log_handle_exec_file_command( t_log_event_ef_lib *log_event_ef_lib, struct t_sl_exec_file_data *exec_file_data, int cmd, int num_args, struct t_sl_exec_file_value *args )
{
    t_log_event_ef_object *leeo;
    t_sl_exec_file_object_desc object_desc;
    int i;

    leeo = (t_log_event_ef_object *)malloc(sizeof(t_log_event_ef_object));
    if (!leeo)
    {
        fprintf(stderr, "NNE: log handler failed to add event\n" );
        return 0;
    }
    leeo->next_in_list = log_event_ef_lib->log_events;
    log_event_ef_lib->log_events = leeo;
    leeo->ef_lib = log_event_ef_lib;
    leeo->name = sl_str_alloc_copy(sl_exec_file_eval_fn_get_argument_string( exec_file_data, args, 0 ));
    leeo->num_args = num_args-1;
    for (i=0; i<leeo->num_args; i++)
    {
        leeo->arg_name[i] = sl_str_alloc_copy(sl_exec_file_eval_fn_get_argument_string( exec_file_data, args, i+1 ));
    }
    log_event_ef_lib->num_events++;
    log_event_ef_lib->total_args += leeo->num_args;
    memset(&object_desc,0,sizeof(object_desc));
    object_desc.version = sl_ef_object_version_checkpoint_restore;
    object_desc.name = leeo->name;
    object_desc.handle = (void *)leeo;
    object_desc.methods = sl_log_event_object_methods;
    object_desc.message_handler = ef_event_object_message_handler;
    sl_exec_file_add_object_instance( exec_file_data, &object_desc );
    return 1;
}

/*f ef_method_eval_occurred
 */
static t_sl_error_level ef_method_eval_occurred( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_log_event_ef_object *leeo;
    int i;

    leeo = (t_log_event_ef_object *)object_desc->handle;
    for (i=0; i<cmd_cb->num_args; i++)
    {
        leeo->ef_lib->values[leeo->arg_offset+i] = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, i );
    }
    leeo->ef_lib->engine->log_event_occurred( leeo->ef_lib->engine_handle, leeo->ef_lib->log_event_array, leeo->event_number );
    return error_level_okay;
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

