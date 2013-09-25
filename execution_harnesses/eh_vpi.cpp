/*a Copyright
  
  This file 'eh_vpi_harness.cpp' copyright Gavin J Stark 2003, 2004, 2007
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/
/*a Constraint compiler source code
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sl_debug.h"
#include "sl_cons.h"
#include "sl_indented_file.h"
#include "sl_general.h"
#include "se_external_module.h"
#include "model_list.h"
#include "c_se_engine.h"

#include "vpi_user.h"
//#include "vpi_user_cds.h"

/*a Defines
 */
#define MAX_VALUE_SIZE ((512)/sizeof(int)/8)
#if 1
#define WHERE_I_AM {}
#else
#define WHERE_I_AM {fprintf(stderr,"%s:%s:%d\n",__FILE__,__func__,__LINE__ );}
#endif

/*a Types
 */
/*t t_globals
 */
typedef struct t_globals
{
    c_sl_error *error;
    c_engine *engine;
    t_sl_option_list plus_args;
} t_globals;

/*t t_vpi_input
 */
typedef struct t_vpi_input
{
    char *signal_name;
    int width;
    t_se_signal_value value[MAX_VALUE_SIZE];
    vpiHandle signal_handle;
} t_vpi_input;

/*t t_vpi_output
 */
typedef struct t_vpi_output
{
    char *signal_name;
    int width;
    t_se_signal_value *value_ptr;
    s_vpi_vecval vector[MAX_VALUE_SIZE];
    vpiHandle signal_handle;
} t_vpi_output;

/*t t_vpi_clock
 */
typedef struct t_vpi_clock
{
    struct t_vpi_module_instance *vmi;
    char *signal_name;
    int edges;
    vpiHandle signal_handle;
    s_cb_data callback;
    s_vpi_value value;
    vpiHandle callback_handle;
} t_vpi_clock;

/*t t_vpi_module_instance
 */
typedef struct t_vpi_module_instance
{
    void *engine_handle;
    vpiHandle task_handle;
    vpiHandle task_scope;
    vpiHandle callback_handle;
    s_cb_data callback;
    char *instance_name;
    char *module_type;
    char *task_name;
    t_sl_option_list option_list;
    int num_inputs;
    int num_outputs;
    int num_clocks;
    t_vpi_input *inputs;
    t_vpi_output *outputs;
    t_vpi_clock *clocks;
} t_vpi_module_instance;

/*a Declarations
 */
static void setup_vpi_callbacks( void );

/*a Statics (local and extern) for vpi
 */
/*v vlog_startup_routines - this extern is used by the vpi to invoke the init routines required for this library
 */
void (*vlog_startup_routines[])() =
{
  setup_vpi_callbacks,
  0
};

/*v globals
 */
static t_globals globals;

/*a VPI functions
 */
/*f reset_callback - called when a clock on a module instance toggles; check for the edge, scan inputs, preclock, clock, and then drive outputs
static PLI_INT32 reset_callback( t_cb_data *cb_data )
{
    t_vpi_module_instance *vmi;
    vmi = (t_vpi_module_instance *)cb_data->user_data;
    globals.engine->simulation_assist_reset_instance( vmi->engine_handle, 0 );
    return 1;
}
 */

/*f clock_callback - called when a clock on a module instance toggles; check for the edge, scan inputs, preclock, clock, and then drive outputs
 */
static PLI_INT32 clock_callback( t_cb_data *cb_data )
{
    t_vpi_clock *vck;
    t_vpi_module_instance *vmi;
    WHERE_I_AM;
    vck = (t_vpi_clock *)cb_data->user_data;
    vmi = vck->vmi;
    //vpi_printf( "Clock callback %s\n",vck->signal_name);
    vck->value.format = vpiIntVal;
    if (vck->signal_handle == NULL)
    {
        fprintf(stderr, "NULL to vck->signal_handle for vpi_get_value!\n");
    }
     vpi_get_value( vck->signal_handle, &(vck->value) );
    if ( vck->value.format==vpiIntVal )
    {
        int i, posedge;
        posedge = (vck->value.value.integer==1);
        if ( (posedge && (vck->edges&1)) ||
             ((!posedge) && (vck->edges&2)) )
        {
            WHERE_I_AM;
            for (i=0; i<vmi->num_inputs; i++)
            {
                if (vmi->inputs[i].width>0)
                {
                    int j;
                    vck->value.format = vpiVectorVal;
                    if (vmi->inputs[i].signal_handle == NULL)
                    {
                        fprintf(stderr, "NULL to vmi->inputs[i].signal_handle for vpi_get_value!\n");
                    }
                    vpi_get_value( vmi->inputs[i].signal_handle, &(vck->value) );
                    if (vck->value.format==vpiVectorVal)
                    {
                        for (j=0; j<(vmi->inputs[i].width+31)/32; j++)
                        {
                            if (j&1)
                            {
                                vmi->inputs[i].value[j/2] |= ((t_se_signal_value)(vck->value.value.vector[j].aval))<<32;
                            }
                            else
                            {
                                vmi->inputs[i].value[j/2] = (t_se_signal_value)((unsigned int)vck->value.value.vector[j].aval);
                            }
                        }
                    }
                }
            }
            WHERE_I_AM;
            if (posedge)
            {
                if (vck->edges&1)
                {
                    globals.engine->simulation_assist_prepreclock_instance( vmi->engine_handle, 1, vck->signal_name );
                    globals.engine->simulation_assist_preclock_instance( vmi->engine_handle, 1, vck->signal_name );
                    globals.engine->simulation_assist_clock_instance( vmi->engine_handle, 1, vck->signal_name );
                }
            }
            if (vck->value.value.integer==0)
            {
                if (vck->edges&2)
                {
                    globals.engine->simulation_assist_prepreclock_instance( vmi->engine_handle, 0, vck->signal_name );
                    globals.engine->simulation_assist_preclock_instance( vmi->engine_handle, 0, vck->signal_name );
                    globals.engine->simulation_assist_clock_instance( vmi->engine_handle, 0, vck->signal_name );
                }
            }
            WHERE_I_AM;
            globals.engine->simulation_assist_comb_instance( vmi->engine_handle );
            globals.engine->simulation_assist_propagate_instance( vmi->engine_handle );
            WHERE_I_AM;
            for (i=0; i<vmi->num_outputs; i++)
            {
                if (vmi->outputs[i].width>0)
                {
                    int j;
                    s_vpi_vecval vector[MAX_VALUE_SIZE];
                    for (j=0; j<(vmi->outputs[i].width+31)/32; j++)
                    {
                        vector[j].aval = (vmi->outputs[i].value_ptr[j/2])>>(32*(j&1));
                        vector[j].bval = 0;
                    }
                    if (memcmp(vector, vmi->outputs[i].vector, sizeof(vector[0])*j)!=0) {
                        memcpy(vmi->outputs[i].vector, vector, sizeof(vector[0])*j);
#if 0
                        vpi_printf("%d: %s value ",globals.engine->cycle(),vmi->outputs[i].signal_name);
                        while (j > 0) {
                            vpi_printf("%08x", vector[j-1].aval);
                            j--;
                        }
                        vpi_printf("\n");
#endif
                        vck->value.format = vpiVectorVal;
                        vck->value.value.vector = vector;
                        vpi_put_value( vmi->outputs[i].signal_handle, &(vck->value), NULL, vpiNoDelay);
                    }
                }
            }

            /* Invoke the callbacks to generate the waveforms */
            globals.engine->simulation_set_cycle(globals.engine->cycle()+1);
            globals.engine->simulation_invoke_callbacks();

        }
    }

    {
        char error_accumulator[16384];
        void *handle;
        handle = NULL;
        while (globals.engine->message->check_errors_and_reset( error_accumulator, sizeof(error_accumulator), error_level_info, error_level_info, &handle )>0)
        {
            vpi_printf("%s", error_accumulator);
        }
        handle = NULL;
        while (globals.engine->error->check_errors_and_reset( error_accumulator, sizeof(error_accumulator), error_level_info, error_level_info, &handle )>0)
        {
            vpi_printf("%s", error_accumulator);
        }
    }
    WHERE_I_AM;
    return 1;
}

/*f hierarchy_tail
 */
static char *hierarchy_tail( char *hierarchy )
{
    char *tail;
    tail = hierarchy+strlen(hierarchy)-1;
    while ((tail>=hierarchy) && (tail[0]!='.')) tail--;
    if (tail>=hierarchy)
        return tail+1;
    return NULL;
}

/*f external_module_instantiation - called when one of the system tasks we create is invoked, which instantiates a module in the simulation engine
 */
static PLI_INT32 external_module_instantiation( PLI_BYTE8 *module_type )
{
    t_vpi_module_instance *vmi;

    /*b Create VPI module instance
     */
    vmi = (t_vpi_module_instance*)malloc(sizeof(t_vpi_module_instance));

    vmi->module_type = sl_str_alloc_copy( module_type );
    vmi->task_handle = vpi_handle( vpiSysTfCall, NULL );
    vmi->task_scope = vpi_handle( vpiScope, vmi->task_handle );
    vmi->task_name = sl_str_alloc_copy(vpi_get_str( vpiName, vmi->task_handle ));

    /*b Get option list
     */
    {
        vpiHandle iter;
        vmi->option_list = NULL;
        iter = vpi_iterate( vpiArgument, vmi->task_handle );
        if (iter)
        {
            vpiHandle arg;
            char *keyword;
            keyword = NULL;
            while ( (arg = vpi_scan(iter))!=NULL )
            {
                PLI_INT32 arg_type;
                s_vpi_value value;
                arg_type = vpi_get(vpiType, arg);
                if ( (arg_type == vpiConstant) ||
                     (arg_type == vpiParameter) ||
                     (arg_type == vpiReg) ||
                     (arg_type == vpiNet) )
                {
                    value.format = vpiObjTypeVal;
                    if (arg == NULL)
                    {
                        fprintf(stderr, "NULL to arg for vpi_get_value!\n");
                    }
                    vpi_get_value( arg, &value );
                    if (!keyword)
                    {
                        if (value.format==vpiStringVal)
                        {
                            keyword = sl_str_alloc_copy(value.value.str);
                        }
                    }
                    else
                    {
                        if (value.format==vpiVectorVal)
                        {
                            /* Try to coerce this into a string. */
                            value.format = vpiStringVal;
                            vpi_get_value( arg, &value );
                        }
                        if (value.format==vpiStringVal)
                        {
                            char *plusloc = strstr(value.value.str, "+{");
                            if (globals.plus_args && plusloc) 
                            {
                                /* Replace each occurrence of +{KEYWORD} with the keyvalue from the plus_args. */
                                char *old_string = value.value.str;
                                char substituted_string[256];

                                substituted_string[0] = '\0';
                                while (plusloc)
                                {
                                    char *close_brace = strchr(old_string, '}');
                                    if (close_brace && close_brace > plusloc)
                                    {
                                        const char *substitution;
                                        int substituted_length = strlen(substituted_string);
                                        char *plusloc_in_new = (substituted_string + substituted_length) + (plusloc - old_string);
                                        size_t cat_length = close_brace - old_string;

                                        if (cat_length > sizeof(substituted_string) - substituted_length)
                                        {
                                            cat_length = sizeof(substituted_string) - substituted_length;
                                        }
                                        strncat(substituted_string, old_string, cat_length);
                                        substitution = sl_option_get_string(globals.plus_args, plusloc_in_new + 2);
                                        if (substitution)
                                        {
                                            *plusloc_in_new = '\0';
                                            strncat(substituted_string, substitution, 
                                                    sizeof(substituted_string) - strlen(substituted_string));
                                        }
                                        old_string = close_brace+1;
                                    }
                                    else
                                    {
                                        /* Oops. Malformed. Do what we can. */
                                        old_string = plusloc+2;
                                    }
                                    plusloc = strstr(old_string, "+{");
                               }
                               strncat(substituted_string, old_string, sizeof(substituted_string) - strlen(substituted_string));
                               vmi->option_list = sl_option_list( vmi->option_list, keyword, substituted_string );
                            }
                            else
                            {
                                vmi->option_list = sl_option_list( vmi->option_list, keyword, value.value.str );
                            }
                        }
                        if (value.format==vpiIntVal)
                        {
                            vmi->option_list = sl_option_list( vmi->option_list, keyword, value.value.integer );
                        }
                        free(keyword);
                        keyword = NULL;
                    }
                }
            }
        }
    }

    /*b Instantiate simulation model
     */
    vpi_printf("Instantiate %s %p %s\n", vmi->module_type, vmi->task_handle, vmi->task_name );
    vmi->instance_name = globals.engine->create_unique_instance_name( NULL, vmi->module_type, NULL );
    globals.engine->instantiate( NULL, vmi->module_type, vmi->instance_name, vmi->option_list );
    vmi->engine_handle = globals.engine->find_module_instance( vmi->instance_name );

    /*b Find inputs, outputs and clocks
     */
    {
        int i;
        t_se_interrogation_handle handle, sub_handle;

        /*b Init
         */
        handle = globals.engine->find_entity( vmi->instance_name );
        sub_handle = NULL;
        vmi->inputs = NULL;
        vmi->outputs = NULL;
        vmi->clocks = NULL;

        /*b Find inputs
         */
        vmi->num_inputs = globals.engine->interrogate_count_hierarchy( handle, engine_state_desc_type_mask_bits, engine_interrogate_include_mask_inputs );
        if (vmi->num_inputs>0)
        {
            vmi->inputs = (t_vpi_input *)malloc(vmi->num_inputs*sizeof(t_vpi_input));
            for (i=0; i<vmi->num_inputs; i++)
            {
                vmi->inputs[i].width = 0;
                if (globals.engine->interrogate_enumerate_hierarchy( handle, i, engine_state_desc_type_mask_bits, engine_interrogate_include_mask_inputs, &sub_handle ))
                {
                    t_se_signal_value *data[4];
                    int sizes[4];
                    char signal_name[1024];
                    globals.engine->interrogate_get_entity_path_string( sub_handle, signal_name, sizeof(signal_name) );
                    vmi->inputs[i].signal_name = sl_str_alloc_copy( hierarchy_tail(signal_name) );
                    if (globals.engine->interrogate_get_data_sizes_and_type( sub_handle, data, sizes )==engine_state_desc_type_bits)
                    {
                        vmi->inputs[i].width = sizes[0];
                        vmi->inputs[i].signal_handle = vpi_handle_by_name( vmi->inputs[i].signal_name, vmi->task_scope );
                        void *fn;
                        globals.engine->interrogate_get_internal_fn( sub_handle, &fn );
                        globals.engine->module_drive_input( (struct t_engine_function *)fn, &(vmi->inputs[i].value[0]) );
                    }
                }
            }
        }
        vmi->num_outputs = globals.engine->interrogate_count_hierarchy( handle, engine_state_desc_type_mask_bits, engine_interrogate_include_mask_outputs );
        if (vmi->num_outputs>0)
        {
            vmi->outputs = (t_vpi_output *)malloc(vmi->num_outputs*sizeof(t_vpi_output));
            for (i=0; i<vmi->num_outputs; i++)
            {
                vmi->outputs[i].width = 0;
                /* This will ensure that the initial values are flushed */
                vmi->outputs[i].vector[0].bval = 1;
                if (globals.engine->interrogate_enumerate_hierarchy( handle, i, engine_state_desc_type_mask_bits, engine_interrogate_include_mask_outputs, &sub_handle ))
                {
                    t_se_signal_value *data[4];
                    int sizes[4];
                    char signal_name[1024];
                    globals.engine->interrogate_get_entity_path_string( sub_handle, signal_name, sizeof(signal_name) );
                    vmi->outputs[i].signal_name = sl_str_alloc_copy( hierarchy_tail(signal_name) );
                    if (globals.engine->interrogate_get_data_sizes_and_type( sub_handle, data, sizes )==engine_state_desc_type_bits)
                    {
                        vmi->outputs[i].width = sizes[0];
                        vmi->outputs[i].value_ptr = data[0];
                        vmi->outputs[i].signal_handle = vpi_handle_by_name( vmi->outputs[i].signal_name, vmi->task_scope );

                        //printf("Output type %d\n",vpi_get(vpiType,vmi->outputs[i].signal_handle));
                    }
                }
            }
        }
        vmi->num_clocks = globals.engine->interrogate_count_hierarchy( handle, engine_state_desc_type_mask_bits, engine_interrogate_include_mask_clocks );
        if (vmi->num_clocks>0)
        {
            vmi->clocks = (t_vpi_clock *)malloc(vmi->num_clocks*sizeof(t_vpi_clock));
            for (i=0; i<vmi->num_clocks; i++)
            {
                vmi->clocks[i].edges = 0;
                vmi->clocks[i].vmi = vmi;
                if (globals.engine->interrogate_enumerate_hierarchy( handle, i, engine_state_desc_type_mask_bits, engine_interrogate_include_mask_clocks, &sub_handle ))
                {
                    t_se_signal_value *data[4];
                    int sizes[4];
                    char signal_name[1024];
                    globals.engine->interrogate_get_entity_path_string( sub_handle, signal_name, sizeof(signal_name) );
                    vmi->clocks[i].signal_name = sl_str_alloc_copy( hierarchy_tail(signal_name) );
                    if (globals.engine->interrogate_get_data_sizes_and_type( sub_handle, data, sizes )==engine_state_desc_type_bits)
                    {
                        t_vpi_clock *vck;
                        vck = &(vmi->clocks[i]);
                        vck->edges = sizes[1]; // bit 0 for posedge, bit 1 for negedge
                        vck->signal_handle = vpi_handle_by_name( vck->signal_name, vmi->task_scope );
                        vck->callback.reason = cbValueChange;
                        vck->callback.cb_rtn = clock_callback;
                        vck->callback.obj = vck->signal_handle;
                        vck->callback.time = NULL;
                        vck->callback.value = NULL;
                        vck->callback.user_data = (PLI_BYTE8 *)((void *)vck);
                        vck->callback_handle = vpi_register_cb( &(vck->callback) );
                        vpi_free_object( vck->callback_handle );
                    }
                }
            }
        }
        globals.engine->interrogation_handle_free( sub_handle );
        globals.engine->interrogation_handle_free( handle );
    }

    /*b Add initial reset callback
     */
    globals.engine->simulation_assist_reset_instance( vmi->engine_handle, 0 );

    /*b Attach to a VCD file, if desired
     */
    {
        const char *vcd_file = sl_option_get_string(vmi->option_list, "vcd_file");
        if (vcd_file)
        {
            t_waveform_vcd_file *vcd;
            vcd = globals.engine->waveform_vcd_file_find(vmi->instance_name);
            if (vcd == NULL)
            {
                vcd = globals.engine->waveform_vcd_file_create(vmi->instance_name);
            }
            else
            {
                globals.engine->waveform_vcd_file_reset(vcd);
            }
            globals.engine->waveform_vcd_file_open(vcd, vcd_file);
            if (vcd)
            {
                const char *entities = sl_option_get_string(vmi->option_list, "vcd_entities");
                if (entities == NULL)
                    entities = vmi->instance_name;

                char *elist = sl_str_alloc_copy(entities);
                char *efree = elist;
                char *cp;

                while ((cp = strsep(&elist, " \t\n\r")))
                {
                    char *dc;
                    int depth;
                    t_se_interrogation_handle entity;

                    dc = strchr(cp, ',');
                    if (dc)
                    {
                        *(dc++) = 0;
                        depth = strtol(dc, NULL, 0);
                    }
                    else
                    {
                        depth = 1;
                    }

                    entity = globals.engine->find_entity(cp);
                    if (entity)
                    {
                        globals.engine->waveform_vcd_file_add_hierarchy(vcd, entity, depth);
                        globals.engine->waveform_vcd_file_enable(vcd);
                        vpi_printf("%s: Logging to %s,%d\n", vcd_file, cp, depth);
                    }
                    else
                    {
                        vpi_printf("%s: Can't find entity %s\n", vcd_file, cp);
                    }
                }
                free(efree);
            }
            else
            {
                vpi_printf("%s: Can't open waveform file \"%s\"\n", vmi->instance_name, vcd_file);
            }
        }
    }

    /*b Done
     */
    return 1;
}

/*f setup_vpi_callbacks
 */
static void setup_vpi_callbacks()
{
    int i;
    s_vpi_vlog_info vlog_info;

    /*b Initialize simulation engine
     */
    se_c_engine_init();

    /*b Initialize models
     */
    for (i=0; model_init_fns[i]; i++)
    {
        model_init_fns[i]();
    }

    /*b Create simulation engine
     */
    globals.error = new c_sl_error( 4, 8 ); // GJS
    globals.engine = new c_engine( globals.error, "cycle simulation engine" );

    /*b Grab plus_args
     */
    globals.plus_args = NULL;
    if (vpi_get_vlog_info(&vlog_info))
    {
        for (i=1; i<vlog_info.argc; i++) // ignore argv[0]
        {
            if (vlog_info.argv[i][0] == '+')
            {
                char *equal_pos;
                char *keyword;
                char *keyvalue;
                
                equal_pos = strchr(vlog_info.argv[i]+1, '=');
                if (equal_pos == NULL)
                {
                    equal_pos = strchr(vlog_info.argv[i]+1, '+');
                }
                if (equal_pos)
                {
                    keyword = sl_str_alloc_copy(vlog_info.argv[i]+1, equal_pos - vlog_info.argv[i] - 1);
                    keyvalue = sl_str_alloc_copy(equal_pos+1);
                    globals.plus_args = sl_option_list( globals.plus_args, keyword, keyvalue );
                }
            }
        }
    }

    /*b Register all simulation external modules with VPI
     */
    for (i=0; i>=0; i++)
    {
        char *name;
        name = se_external_module_name(i);
        if (!name)
        {
            i = -99;
        }
        else
        {
            s_vpi_systf_data vpi_task;
            char *buffer;
            vpi_task.type = vpiSysTask;
            vpi_task.sysfunctype = 0;
            buffer = (char *)malloc(strlen(name)+32);
            sprintf(buffer,"$__sim_ext_mod__%s",name);
            vpi_task.tfname = buffer;
            vpi_task.calltf = external_module_instantiation;
            vpi_task.compiletf = NULL;
            vpi_task.sizetf = NULL;
            vpi_task.user_data = name;
            vpi_register_systf(&vpi_task);
            if (vpi_chk_error(NULL))
            {
                vpi_printf( "Failed to set up VPI callback for simulation module %s", name );
            }
        }
    }
}

/*f scripting_init_module
  This is not actually called
 */
extern void scripting_init_module( const char *name )
{
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

