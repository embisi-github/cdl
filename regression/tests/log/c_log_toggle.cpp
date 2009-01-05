/*a Copyright Netronome 2007
 */

/*a Documentation
 */

/*a To do
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "be_model_includes.h"

/*a Defines
 */

/*a Types
 */
/*t c_log_toggle
 */
class c_log_toggle
{
public:
    c_log_toggle( class c_engine *eng, void *eng_handle );
    ~c_log_toggle();
    t_sl_error_level delete_instance( void );
    t_sl_error_level reset( int pass );
    c_engine *engine;
    struct t_engine_log_parser *elp;
private:
    void *engine_handle;
    const char *dut_name;
};

/*a Static wrapper functions for log_toggle
 */
/*f log_toggle_instance_fn
 */
static t_sl_error_level log_toggle_instance_fn( c_engine *engine, void *engine_handle )
{
    c_log_toggle *clt;
    clt = new c_log_toggle( engine, engine_handle );
    if (!clt)
        return error_level_fatal;
    return error_level_okay;
}

/*f log_toggle_delete - simple callback wrapper for the main method
 */
static t_sl_error_level log_toggle_delete( void *handle )
{
    c_log_toggle *mod;
    t_sl_error_level result;
    mod = (c_log_toggle *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f log_toggle_reset
 */
static t_sl_error_level log_toggle_reset( void *handle, int pass )
{
    c_log_toggle *clt;

    clt = (c_log_toggle *)handle;
    return clt->reset( pass );
}

/*a Constructors and destructors for log_toggle
 */
/*f c_log_toggle::c_log_toggle
 */
static t_sl_error_level log_callback( void *eng_handle, int cycle, struct t_engine_log_interest *log_interest, void *handle, int event_number, struct t_engine_log_event_array_entry *log_event_interest )
{
    t_se_signal_value *log_data[1];
    c_log_toggle *clt;
    clt = (c_log_toggle *)handle;
    clt->engine->log_parse( log_event_interest, clt->elp, log_data );
    printf("Toggle happened at cycle %d toggle value %llx\n", cycle, log_data[0]?*log_data[0]:0xdeaddeadll );

    {const char *name; t_se_signal_value *value_base; t_engine_text_value_pair *args; int num_args; int i;
        num_args = clt->engine->log_get_event_desc( log_event_interest, &name, &value_base, &args );
        if (num_args>=0)
        {
            printf ("%d:Log event '%s'\n", cycle, name );
            for (i=0; i<num_args; i++)
            {
                printf ("\t\t'%s' : %llx\n", args[i].text, value_base[args[i].value] );
            }
        }
    }
    return error_level_okay;
}
c_log_toggle::c_log_toggle( class c_engine *eng, void *eng_handle )
{
    engine = eng;
    engine_handle = eng_handle;

    dut_name = engine->get_option_string( engine_handle, "module_name", "" );

    engine->register_delete_function( engine_handle, (void *)this, log_toggle_delete );
    engine->register_reset_function( engine_handle, (void *)this, log_toggle_reset );

}

/*f c_log_toggle::~c_log_toggle
 */
c_log_toggle::~c_log_toggle()
{
    delete_instance();
}

/*f c_log_toggle::delete_instance
 */
t_sl_error_level c_log_toggle::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for log_toggle
 */
/*f c_log_toggle::reset
 */
t_sl_error_level c_log_toggle::reset( int pass )
{
    struct t_engine_log_interest *eli;
    if (pass==0)
    {
//        eli = engine->log_interest( engine_handle, engine->find_module_instance(dut_name), "toggle", log_callback, (void *)this );
        eli = engine->log_interest( engine_handle, engine->find_module_instance(dut_name), NULL, log_callback, (void *)this );
        elp = engine->log_parse_setup( engine_handle, eli, "toggle", 1, "toggle", 0 );
    }
    return error_level_okay;
}

/*a Initialization functions
 */
/*f c_log_toggle__init
 */
extern void c_log_toggle__init( void )
{
    se_external_module_register( 1, "log_toggle", log_toggle_instance_fn );
}

/*a Scripting support code
 */
/*f initc_log_toggle
 */
extern "C" void initc_log_toggle( void )
{
    c_log_toggle__init( );
    scripting_init_module( "c_log_toggle" );
}

/*a Editor preferences and notes
  mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

