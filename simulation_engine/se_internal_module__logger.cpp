/*a Copyright
  
  This file 'se_internal_module__sram_mrw.cpp' copyright Gavin J Stark 2007
  
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
#include "sl_general.h"
#include "sl_token.h"
#include "se_errors.h"
#include "se_external_module.h"
#include "se_internal_module.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Defines
 */

/*a Types
 */
/*t t_log_cb
 */
typedef struct t_log_cb
{
    struct t_log_cb *next_in_list;
    class c_logger *lgr;
    struct t_engine_log_interest *eli;
    const char *module_instance_name;
    int log_id;
    int num_events;
    int *occurences; // one integer per event
} t_log_cb;

/*t c_logger
*/
class c_logger
{
public:
    c_logger( class c_engine *eng, void *eng_handle );
    ~c_logger();
    t_sl_error_level delete_instance( void );
    t_sl_error_level reset( int pass );
    t_sl_error_level log_callback( void *eng_handle, int cycle, struct t_engine_log_interest *log_interest, int event_number, struct t_engine_log_event_array_entry *log_event_interest, t_log_cb *log_cb );
    c_engine *engine;
    struct t_engine_log_parser *elp;
private:
    void *engine_handle;

    const char *module_names;
    const char *filename;
    int verbose;

    int num_modules;
    t_log_cb *log_cbs;
    FILE *outfile;
};

/*a Statics
 */

/*a Static wrapper functions for SRAM module
 */
/*f logger_delete - simple callback wrapper for the main method
*/
static t_sl_error_level logger_delete( void *handle )
{
    c_logger *mod;
    t_sl_error_level result;
    mod = (c_logger *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f logger_reset
 */
static t_sl_error_level logger_reset( void *handle, int pass )
{
    c_logger *sram;
    sram = ((c_logger *)handle);
    return sram->reset( pass );
}

/*f logger_log_callback
 */
static t_sl_error_level logger_log_callback( void *eng_handle, int cycle, struct t_engine_log_interest *log_interest, void *handle, int event_number, struct t_engine_log_event_array_entry *log_event_interest )
{
    t_log_cb *lcb;
    lcb = ((t_log_cb *)handle);
    return lcb->lgr->log_callback( eng_handle, cycle, log_interest, event_number, log_event_interest, lcb );
}

/*a Extern wrapper functions for SRAM module
 */
/*f se_internal_module__logger_instantiate
 */
extern t_sl_error_level se_internal_module__logger_instantiate( c_engine *engine, void *engine_handle )
{
    c_logger *mod;
    mod = new c_logger( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*a Constructors and destructors for logger
*/
/*f c_logger::c_logger
*/
c_logger::c_logger( class c_engine *eng, void *eng_handle )
{
    int i;
    char *module_name[256];

    engine = eng;
    engine_handle = eng_handle;

    outfile = NULL;
    log_cbs = NULL;

    module_names = engine->get_option_string( engine_handle, "modules", "" );
    filename = engine->get_option_string( engine_handle, "filename", "" );
    verbose = engine->get_option_int( engine_handle, "verbose", 0 );

    engine->register_delete_function( engine_handle, (void *)this, logger_delete );
    engine->register_reset_function( engine_handle, (void *)this, logger_reset );

    if (filename && filename[0])
    {
        outfile = fopen( filename, "w" );
        if (!outfile)
        {
            fprintf(stderr, "Failed to open log file '%s'\n", filename );
        }
    }

    char *tkn_in;
    tkn_in = sl_str_alloc_copy( module_names );
    sl_tokenize_line( tkn_in, module_name, sizeof(module_name)/sizeof(char *), &num_modules );
    for (i=0; i<num_modules; i++)
    {
        t_log_cb *lcb;
        lcb = (t_log_cb *)malloc(sizeof(t_log_cb));
        lcb->eli = NULL;
        lcb->next_in_list = log_cbs;
        log_cbs = lcb;
        lcb->lgr = this;
        lcb->module_instance_name = sl_str_alloc_copy( module_name[i] );
        lcb->log_id=i;
    }
    free(tkn_in);
}

/*f c_logger::~c_logger
*/
c_logger::~c_logger()
{
    delete_instance();
}

/*f c_logger::delete_instance
*/
t_sl_error_level c_logger::delete_instance( void )
{
    if (outfile)
    {
        fclose(outfile);
        outfile = NULL;
    }
    return error_level_okay;
}

/*a Class reset/log_callback methods for logger
*/
/*f c_logger::reset
*/
t_sl_error_level c_logger::reset( int pass )
{
    if (pass==0)
    {
        t_log_cb *lcb;
        for (lcb=log_cbs; lcb; lcb=lcb->next_in_list)
        {
            lcb->num_events = 0;
            lcb->occurences = 0;
            lcb->eli = engine->log_interest( engine_handle, engine->find_module_instance(lcb->module_instance_name), NULL, logger_log_callback, (void *)lcb );
            if (lcb->eli)
            {
                lcb->num_events = engine->log_get_event_interest_desc( lcb->eli, -1, NULL );
            }
            else
            {
                fprintf(stderr,"****LOGGER: Failed to find module instance logs for instance '%s' in se_logger\n", lcb->module_instance_name );
            }
            if (lcb->num_events>0)
            {
                int i;
                lcb->occurences = (int *)malloc(sizeof(int)*lcb->num_events);
                for (i=0; i<lcb->num_events; i++)
                {
                    lcb->occurences[i] = 0;
                }
            }
        }
    }
    return error_level_okay;
}

/*f c_logger::log_callback
 */
t_sl_error_level c_logger::log_callback( void *eng_handle, int cycle, struct t_engine_log_interest *log_interest, int event_number, struct t_engine_log_event_array_entry *log_event_interest, t_log_cb *log_cb )
{
    const char *name;
    t_se_signal_value *value_base;
    t_engine_text_value_pair *args;

    int num_args;
    num_args = engine->log_get_event_desc( log_event_interest, &name, &value_base, &args );
    if (num_args<0) return error_level_okay;

    if (outfile)
    {
        int i;
        if (log_cb->occurences[event_number]==0)
        {
            fprintf(outfile,"#%s,%d,\"%s\",%d", log_cb->module_instance_name, event_number, name, num_args );
            for (i=0; i<num_args; i++)
            {
                fprintf(outfile,",\"%s\"", args[i].text );
            }
            fprintf(outfile,"\n" );
        }
        fprintf(outfile,"%d,%d,%s,%d,%d", cycle, log_cb->occurences[event_number], log_cb->module_instance_name, event_number, num_args );
        for (i=0; i<num_args; i++)
        {
            fprintf(outfile,",%llx", value_base[args[i].value] );
        }
        fprintf(outfile,"\n" );
    }
    if (verbose)
    {
        int i;
        printf ("%d:Log event %s:%s:%d\n", cycle, log_cb->module_instance_name, name, log_cb->occurences[event_number] );
        for (i=0; i<num_args; i++)
        {
            printf ("\t\t'%s' : %llx\n", args[i].text, value_base[args[i].value] );
        }
    }
    log_cb->occurences[event_number]++;
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

