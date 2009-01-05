/*a Copyright
  
  This file 'register_file.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_model_includes.h"

/*a RF decoder types
 */
/*t t_rf_dec_inputs
 */
typedef struct t_rf_dec_inputs
{
    int *address;
} t_rf_dec_inputs;

/*t t_rf_dec_combinatorials
 */
typedef struct t_rf_dec_combinatorials
{
    int decode_out;
} t_rf_dec_combinatorials;

/*t	c_rf_dec
*/
class c_rf_dec
{
public:
     c_rf_dec::c_rf_dec( class c_engine *eng, void *engine_handle );
     c_rf_dec::~c_rf_dec();
     t_sl_error_level c_rf_dec::delete_instance( void );
     t_sl_error_level c_rf_dec::reset( void );
     t_sl_error_level c_rf_dec::comb( void );
     c_engine *engine;
     void *engine_handle;
private:
    t_rf_dec_inputs inputs;
    t_rf_dec_combinatorials combinatorials;
};

/*a RF decoder static variables
 */
/*f state_desc
 */
//static t_state *_ptr;
//#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
//static t_engine_state_desc state_desc[] =
//{
//     {"sum", engine_state_desc_type_bits, NULL, struct_offset(_ptr, sum), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
//     {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
//};

/*a RF decoder static callback functions
 */
/*f rf_dec_instance_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_dec_instance_fn( c_engine *engine, void *engine_handle )
{
     c_rf_dec *mod;
     mod = new c_rf_dec( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f rf_dec_delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_dec_delete_fn( void *handle )
{
     c_rf_dec *mod;
     t_sl_error_level result;
     mod = (c_rf_dec *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f rf_dec_reset_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_dec_reset_fn( void *handle )
{
     c_rf_dec *mod;
     mod = (c_rf_dec *)handle;
     return mod->reset();
}

/*f rf_dec_comb_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_dec_comb_fn( void *handle )
{
     c_rf_dec *mod;
     mod = (c_rf_dec *)handle;
     return mod->comb();
}

/*a RF decoder constructors/destructors
 */
/*f c_rf_dec::c_rf_dec
 */
c_rf_dec::c_rf_dec( class c_engine *eng, void *eng_handle )
{
     this->engine = eng;
     this->engine_handle = eng_handle;

     SL_DEBUG( sl_debug_level_info, "c_rf_dec::c_rf_dec", "New instance %p", eng );

     engine->register_delete_function( engine_handle, (void *)this, rf_dec_delete_fn );
     engine->register_reset_function( engine_handle, (void *)this, rf_dec_reset_fn );

     engine->register_comb_fn( engine_handle, (void *)this, rf_dec_comb_fn );

     engine->register_output_signal( engine_handle, "decode", 32, &combinatorials.decode_out );
     engine->register_comb_output( engine_handle, "decode" );

     engine->register_input_signal( engine_handle, "address", 5, &inputs.address );
     engine->register_comb_input( engine_handle, "address" );

}

/*f c_rf_dec::~c_rf_dec
 */
c_rf_dec::~c_rf_dec()
{
     delete_instance();
}

/*f c_rf_dec::delete_instance
 */
t_sl_error_level c_rf_dec::delete_instance( void )
{
     return error_level_okay;
}

/*a RF decoder engine invoked methods
 */
/*f c_rf_dec::reset
 */
t_sl_error_level c_rf_dec::reset( void )
{
     SL_DEBUG( sl_debug_level_info,
              "c_rf_dec::reset",
              "reset");
     return error_level_okay;
}

/*f c_rf_dec::comb
 */
t_sl_error_level c_rf_dec::comb( void )
{
    combinatorials.decode_out = 1<<(inputs.address[0]);
    return error_level_okay;
}

/*a RF array types
 */
/*t t_rf_array_state
 */
typedef struct t_rf_array_state
{
    int array[32];
} t_rf_array_state;

/*t t_rf_array_inputs
 */
typedef struct t_rf_array_inputs
{
    int *read_decode;
    int *write_decode;
    int *write_enable;
    int *write_data;
} t_rf_array_inputs;

/*t t_rf_array_combinatorials
 */
typedef struct t_rf_array_combinatorials
{
    int read_data;
} t_rf_array_combinatorials;

/*t	c_rf_array
*/
class c_rf_array
{
public:
     c_rf_array::c_rf_array( class c_engine *eng, void *engine_handle );
     c_rf_array::~c_rf_array();
     t_sl_error_level c_rf_array::delete_instance( void );
     t_sl_error_level c_rf_array::reset( void );
     t_sl_error_level c_rf_array::comb( void );
     t_sl_error_level c_rf_array::preclock( void );
     t_sl_error_level c_rf_array::clock( void );
     c_engine *engine;
     void *engine_handle;
private:
    t_rf_array_state state;
    t_rf_array_state next_state;
    t_rf_array_inputs inputs;
    t_rf_array_combinatorials combinatorials;
};

/*a RF static variables
 */
/*f rf_array_state_desc
 */
static t_rf_array_state *_ptr;
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_engine_state_desc state_desc[] =
{
     {"array", engine_state_desc_type_memory, NULL, struct_offset(_ptr, array), {32,32,0,0}, {NULL,NULL,NULL,NULL} },
     {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};
#undef struct_offset

/*a RF array static callback functions
 */
/*f rf_array_instance_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_array_instance_fn( c_engine *engine, void *engine_handle )
{
     c_rf_array *mod;
     mod = new c_rf_array( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f rf_array_delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_array_delete_fn( void *handle )
{
     c_rf_array *mod;
     t_sl_error_level result;
     mod = (c_rf_array *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f rf_array_reset_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_array_reset_fn( void *handle )
{
     c_rf_array *mod;
     mod = (c_rf_array *)handle;
     return mod->reset();
}

/*f rf_array_comb_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_array_comb_fn( void *handle )
{
     c_rf_array *mod;
     mod = (c_rf_array *)handle;
     return mod->comb();
}

/*f rf_array_preclock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_array_preclock_fn( void *handle )
{
     c_rf_array *mod;
     mod = (c_rf_array *)handle;
     return mod->preclock();
}

/*f rf_array_clock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_array_clock_fn( void *handle )
{
     c_rf_array *mod;
     mod = (c_rf_array *)handle;
     return mod->clock();
}

/*a RF constructors/destructors
 */
/*f c_rf_array::c_rf_array
 */
c_rf_array::c_rf_array( class c_engine *eng, void *eng_handle )
{
     this->engine = eng;
     this->engine_handle = eng_handle;

     SL_DEBUG( sl_debug_level_info, "c_rf_array::c_rf_array", "New instance %p", eng );

     memset( &state, 0, sizeof(state) );
     memset( &next_state, 0, sizeof(next_state) );

     engine->register_delete_function( engine_handle, (void *)this, rf_array_delete_fn );
     engine->register_reset_function( engine_handle, (void *)this, rf_array_reset_fn );

     engine->register_clock_fns( engine_handle, (void *)this, "main_clock", rf_array_preclock_fn, rf_array_clock_fn );
     engine->register_output_signal( engine_handle, "read_data", 32, &combinatorials.read_data );
     engine->register_comb_output( engine_handle, "read_data" );
     engine->register_output_generated_on_clock( engine_handle, "read_data", "main_clock", 1 );

     engine->register_comb_fn( engine_handle, (void *)this, rf_array_comb_fn );

     engine->register_input_signal( engine_handle, "write_decode", 32, &inputs.write_decode );
     engine->register_input_used_on_clock( engine_handle, "write_decode", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "write_enable", 1, &inputs.write_enable );
     engine->register_input_used_on_clock( engine_handle, "write_enable", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "write_data",  32, &inputs.write_data );
     engine->register_input_used_on_clock( engine_handle, "write_data", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "read_decode", 32, &inputs.read_decode );
     engine->register_comb_input( engine_handle, "read_decode" );

}

/*f c_rf_array::~c_rf_array
 */
c_rf_array::~c_rf_array()
{
     delete_instance();
}

/*f c_rf_array::delete_instance
 */
t_sl_error_level c_rf_array::delete_instance( void )
{
     return error_level_okay;
}

/*a RF array engine invoked methods
 */
/*f c_rf_array::reset
 */
t_sl_error_level c_rf_array::reset( void )
{
    int i;
    SL_DEBUG( sl_debug_level_info,
             "c_rf_array::reset",
             "reset" );
    for (i=0; i<32; i++)
    {
        state.array[i] = 0;
    }
    return error_level_okay;
}

/*f c_rf_array::comb
 */
t_sl_error_level c_rf_array::comb( void )
{
    int i, j;

    for (i=0,j=-1; i<32; i++)
    {
        if ((inputs.read_decode[0]>>i)&1)
        {
            if (j==-1)
            {
                j = i;
            }
            else
            {
                j = -2;
            }
        }
    }
    switch (j)
    {
    case -2:
        combinatorials.read_data = 0xdeaddead; // Reading 2 array slots
        break;
    case -1:
        combinatorials.read_data = 0xcafecafe; // Reading 0 array slots
        break;
    default:
        combinatorials.read_data = state.array[j]; // Reading an array slot
        break;
    }

    return error_level_okay;
}

/*f c_rf_array::preclock
 */
t_sl_error_level c_rf_array::preclock( void )
{
    int i, j;

    memcpy( &next_state, &state, sizeof(state) );
    if (inputs.write_enable[0])
    {
        for (i=0,j=-1; i<32; i++)
        {
            if ((inputs.write_decode[0]>>i)&1)
            {
                if (j==-1)
                {
                    j = i;
                }
                else
                {
                    j = -2;
                }
            }
        }
        switch (j)
        {
        case -2:
            break;
        case -1:
            break;
        default:
            next_state.array[j] = inputs.write_data[0];
            break;
        }
    }
    return error_level_okay;
}

/*f c_rf_array::clock
 */
t_sl_error_level c_rf_array::clock( void )
{

     memcpy( &state, &next_state, sizeof(state) );
     return error_level_okay;
}

/*a RF types
 */
/*t t_inputs
 */
typedef struct t_inputs
{
    int *write_data;
    int *write_enable;
    int *write_address;
    int *read_address;
} t_inputs;

/*t t_outputs
 */
typedef struct t_outputs
{
    int read_data;
} t_outputs;

/*t t_locals
 */
typedef struct t_locals
{
} t_locals;

/*t	c_register_file
*/
class c_register_file
{
public:
     c_register_file::c_register_file( class c_engine *eng, void *engine_handle );
     c_register_file::~c_register_file();
     t_sl_error_level c_register_file::delete_instance( void );
     t_sl_error_level c_register_file::reset( void );
     t_sl_error_level c_register_file::preclock( void );
     t_sl_error_level c_register_file::clock( void );
     t_sl_error_level c_register_file::comb( void );
     c_engine *engine;
     void *engine_handle;
private:
    t_inputs inputs;
    t_outputs outputs;
    t_locals locals;

    void *read_decode_handle;
    void *write_decode_handle;
    void *array_handle;
    void *array_clock_handle;
};

/*a RF static callback functions
 */
/*f instance_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_instance_fn( c_engine *engine, void *engine_handle )
{
     c_register_file *mod;
     mod = new c_register_file( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_delete_fn( void *handle )
{
     c_register_file *mod;
     t_sl_error_level result;
     mod = (c_register_file *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f reset_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_reset_fn( void *handle )
{
     c_register_file *mod;
     mod = (c_register_file *)handle;
     return mod->reset();
}

/*f comb_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_comb_fn( void *handle )
{
     c_register_file *mod;
     mod = (c_register_file *)handle;
     return mod->comb();
}

/*f preclock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_preclock_fn( void *handle )
{
     c_register_file *mod;
     mod = (c_register_file *)handle;
     return mod->preclock();
}

/*f clock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level rf_clock_fn( void *handle )
{
     c_register_file *mod;
     mod = (c_register_file *)handle;
     return mod->clock();
}

/*a RF constructors/destructors
 */
/*f c_register_file::c_register_file
 */
c_register_file::c_register_file( class c_engine *eng, void *eng_handle )
{
     this->engine = eng;
     this->engine_handle = eng_handle;

     SL_DEBUG( sl_debug_level_info, "c_register_file::c_register_file", "New instance %p", eng );

     engine->register_delete_function( engine_handle, (void *)this, rf_delete_fn );
     engine->register_reset_function( engine_handle, (void *)this, rf_reset_fn );

     engine->register_clock_fns( engine_handle, (void *)this, "main_clock", rf_preclock_fn, rf_clock_fn );
     engine->register_output_signal( engine_handle, "read_data", 32, NULL ); // Will be tied to a submodule output
     engine->register_comb_output( engine_handle, "read_data" ); // Output changes combinatorially on read_address
     engine->register_output_generated_on_clock( engine_handle, "read_data", "main_clock", 1 ); // Output changes on clock edge due to array contents

     engine->register_comb_fn( engine_handle, (void *)this, rf_comb_fn );

     engine->register_input_signal( engine_handle, "read_address", 5, &inputs.read_address );
     engine->register_input_signal( engine_handle, "write_address", 5, &inputs.write_address );
     engine->register_input_signal( engine_handle, "write_enable", 1, &inputs.write_enable );
     engine->register_input_signal( engine_handle, "write_data", 32, &inputs.write_data );
     engine->register_comb_input( engine_handle, "read_address" );
     engine->register_input_used_on_clock( engine_handle, "write_address", "main_clock", 1 );
     engine->register_input_used_on_clock( engine_handle, "write_enable", "main_clock", 1 );
     engine->register_input_used_on_clock( engine_handle, "write_data", "main_clock", 1 );

     engine->instantiate( engine_handle, "rf_array", "array", NULL );
     engine->instantiate( engine_handle, "rf_decode", "read_decode", NULL );
     engine->instantiate( engine_handle, "rf_decode", "write_decode", NULL );

     read_decode_handle = engine->submodule_get_handle( engine_handle, "read_decode" );
     write_decode_handle = engine->submodule_get_handle( engine_handle, "write_decode" );
     array_handle = engine->submodule_get_handle( engine_handle, "array" );
     array_clock_handle = engine->submodule_get_clock_handle( array_handle, "main_clock" );

     engine->submodule_drive_input_with_module_input( engine_handle, "read_address", read_decode_handle, "address" );
     engine->submodule_drive_input_with_module_input( engine_handle, "write_address", write_decode_handle, "address" );

     engine->submodule_drive_input_with_module_input( engine_handle, "write_enable", array_handle, "write_enable" );
     engine->submodule_drive_input_with_module_input( engine_handle, "write_data", array_handle, "write_data" );
     engine->submodule_drive_input_with_submodule_output( write_decode_handle, "decode", array_handle, "write_decode" );
     engine->submodule_drive_input_with_submodule_output( read_decode_handle, "decode", array_handle, "read_decode" );
     engine->submodule_output_drive_module_output( array_handle, "read_data", engine_handle, "read_data" );

}

/*f c_register_file::~c_register_file
 */
c_register_file::~c_register_file()
{
     delete_instance();
}

/*f c_register_file::delete_instance
 */
t_sl_error_level c_register_file::delete_instance( void )
{
     return error_level_okay;
}

/*a RF engine invoked methods
 */
/*f c_register_file::reset
 */
t_sl_error_level c_register_file::reset( void )
{
     SL_DEBUG( sl_debug_level_info,
              "c_register_file::reset",
              "reset" );
     return error_level_okay;
}

/*f c_register_file::comb
 */
t_sl_error_level c_register_file::comb( void )
{
    engine->submodule_call_comb( read_decode_handle ); // read_decode depends combinatorially on input read_address
    engine->submodule_call_comb( array_handle ); // array output 'read_data' depends combinatorially on read_decode
    return error_level_okay;
}

/*f c_register_file::preclock
 */
t_sl_error_level c_register_file::preclock( void )
{
    engine->submodule_call_comb( write_decode_handle ); // write_decode depends combinatorially on write_address
    engine->submodule_call_preclock( array_clock_handle, 1 );
    return error_level_okay;
}

/*f c_register_file::clock
 */
t_sl_error_level c_register_file::clock( void )
{
    engine->submodule_call_clock( array_clock_handle, 1 );
    comb(); // Safe to do at any time, but particularly required as read_data depends on clocked array data
    return error_level_okay;
}

/*a Initialization functions
 */
/*f register_file__init
 */
extern void register_file__init( void )
{
     se_external_module_register( 1, "rf_decode", rf_dec_instance_fn );
     se_external_module_register( 1, "rf_array", rf_array_instance_fn );
     se_external_module_register( 1, "register_file", rf_instance_fn );
}

/*a Scripting support code
 */
/*f initc_register_file
 */
extern "C" void initc_register_file( void )
{
     register_file__init( );
     scripting_init_module( "register_file" );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

