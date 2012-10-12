/*a Copyright
  
  This file 'se_cmodel_assist.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "se_cmodel_assist.h"
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

/*a Defines
 */
#define struct_resolve( t, ptr, a ) ((t)(((char *)(ptr))+(a)))

/*a Static variables
 */
static t_sl_uint64 dummy_input[16]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/*a Statics
 */
static t_sl_uint64 bit_mask[] = {
     0LL, 1LL, 3LL, 7LL,
     0xfLL, 0x1fLL, 0x3fLL, 0x7fLL,
     0xffLL, 0x1ffLL, 0x3ffLL, 0x7ffLL,
     0xfffLL, 0x1fffLL, 0x3fffLL, 0x7fffLL,
     0xffffLL, 0x1ffffLL, 0x3ffffLL, 0x7ffffLL,
     0xfffffLL, 0x1fffffLL, 0x3fffffLL, 0x7fffffLL,
     0xffffffLL, 0x1ffffffLL, 0x3ffffffLL, 0x7ffffffLL,
     0xfffffffLL, 0x1fffffffLL, 0x3fffffffLL, 0x7fffffffLL,
     0xffffffffLL, 0x1ffffffffLL, 0x3ffffffffLL, 0x7ffffffffLL,
     0xfffffffffLL, 0x1fffffffffLL, 0x3fffffffffLL, 0x7fffffffffLL,
     0xffffffffffLL, 0x1ffffffffffLL, 0x3ffffffffffLL, 0x7ffffffffffLL,
     0xfffffffffffLL, 0x1fffffffffffLL, 0x3fffffffffffLL, 0x7fffffffffffLL,
     0xffffffffffffLL, 0x1ffffffffffffLL, 0x3ffffffffffffLL, 0x7ffffffffffffLL,
     0xfffffffffffffLL, 0x1fffffffffffffLL, 0x3fffffffffffffLL, 0x7fffffffffffffLL,
     0xffffffffffffffLL, 0x1ffffffffffffffLL, 0x3ffffffffffffffLL, 0x7ffffffffffffffLL,
     0xfffffffffffffffLL, 0x1fffffffffffffffLL, 0x3fffffffffffffffLL, 0x7fffffffffffffffLL,
     0xffffffffffffffffLL
};

/*a External functions
 */
/*f se_cmodel_assist_assign_to_bit
 */
extern void se_cmodel_assist_assign_to_bit( t_sl_uint64 *vector, int size, int bit, unsigned int value)
{
    t_sl_uint64 old;
    int shift;

    if ((bit<size) && (bit>=0))
    {
        old = vector[bit/64];
        shift = bit%64;
        vector[bit/64] = (old &~ (1LL<<shift)) | ((value&1LL)<<shift);
    }
}

/*f se_cmodel_assist_assign_to_bit_range
 */
extern void se_cmodel_assist_assign_to_bit_range( t_sl_uint64 *vector, int size, int bit, int length, t_sl_uint64 value)
{
    t_sl_uint64 old;
    int shift;

    if ((bit+length<=size) && (bit>=0) && (length>=1))
    {
        old = vector[bit/64];
        shift = bit%64;
        vector[bit/64] = (old &~ (bit_mask[length]<<shift)) | ((value&bit_mask[length])<<shift);
    }
}

/*f arg_from_vprintf_string
 */
static int arg_from_vprintf_string( const char *string, int *used, int num_args )
{
     int j, k;

     j=string[0]-'0';
     if ((j>=0) && (j<=9))
     {
          for( k=1; isdigit(string[k]); k++ )
          {
               j = (j*10)+(string[k]-'0');
          }
          if ((string[k]=='%') && (j>=0) && (j<num_args))
          {
               *used = k+1;
               return j;
          }
     }
     return -1;
}

/*f se_cmodel_assist_vsnprintf
 */
extern char *se_cmodel_assist_vsnprintf( char *buffer, int buffer_size, const char *format, int num_args, ... )
{
    va_list ap;
    int i, j, k;
    int ch;
    int pos;
    int done;
    t_sl_uint64 arguments[64];

    va_start( ap, num_args );
    for (i=0; (i<num_args) && (i<(int)(sizeof(arguments)/sizeof(t_sl_uint64))); i++)
    {
        arguments[i] = (t_sl_uint64)va_arg( ap, t_sl_uint64);
    }
    va_end( ap );

    for (i=0, pos=0; format[i]; i++)
    {
        if (pos>=buffer_size)
        {
            break;
        }
        switch (format[i])
        {
        case '%':
            i++;
            done = 0;
            ch = format[i];
            switch (ch)                   
            {
            case 'd':
                j = arg_from_vprintf_string( format+i+1, &k, num_args );
                if (j>=0)
                {
                    snprintf( buffer+pos, buffer_size-pos, "%lld", arguments[j] );
                    done = 1;
                    i+=k;
                }
                break;
            case 'x':
            case 'X':
                j = arg_from_vprintf_string( format+i+1, &k, num_args );
                if (j>=0)
                {
                    snprintf( buffer+pos, buffer_size-pos, (ch=='x')?"%08llx":"%016llx", arguments[j] );
                    done = 1;
                    i+=k;
                }
                break;
            default:
                break;
            }
            if (done)
            {
                pos = strlen(buffer);
            }
            else
            {
                i--;
                buffer[pos++] = format[i];
            }
            break;
        default:
            buffer[pos++] = format[i];
            break;
        }
    }
    if (pos>=buffer_size)
        pos = buffer_size-1;
    buffer[pos]=0;
    return buffer;
}

/*f se_cmodel_assist_module_declaration
*/
extern void se_cmodel_assist_module_declaration( c_engine *engine, void*engine_handle, void *base, t_se_cma_module_desc *module_desc )
{
    for (int i=0; module_desc->input_descs && module_desc->input_descs[i].port_name; i++)
    {
        engine->register_input_signal( engine_handle, module_desc->input_descs[i].port_name, module_desc->input_descs[i].width, struct_resolve( t_sl_uint64 **, base, module_desc->input_descs[i].driver_ofs )  );
        if (module_desc->input_descs[i].is_comb) engine->register_comb_input( engine_handle, module_desc->input_descs[i].port_name );
    }
    for (int i=0; module_desc->output_descs && module_desc->output_descs[i].port_name; i++)
    {
        engine->register_output_signal( engine_handle, module_desc->output_descs[i].port_name, module_desc->output_descs[i].width, struct_resolve( t_sl_uint64 *, base, module_desc->output_descs[i].value_ofs )  );
        if (module_desc->output_descs[i].is_comb) engine->register_comb_output( engine_handle, module_desc->output_descs[i].port_name );
    }
    for (int i=0; module_desc->clock_descs_list[i]; i++)
    {
        t_se_cma_clock_desc *clock_desc = module_desc->clock_descs_list[i];
        for (int j=0; clock_desc->posedge_inputs && clock_desc->posedge_inputs[j]>=0; j++)
        {
            int input_number=clock_desc->posedge_inputs[j];
            engine->register_input_used_on_clock( engine_handle, module_desc->input_descs[input_number].port_name, clock_desc->name, 1 );
        }
        for (int j=0; clock_desc->posedge_outputs && clock_desc->posedge_outputs[j]>=0; j++)
        {
            int output_number=clock_desc->posedge_outputs[j];
            engine->register_output_generated_on_clock( engine_handle, module_desc->output_descs[output_number].port_name, clock_desc->name, 1 );
        }
        for (int j=0; clock_desc->negedge_inputs && clock_desc->negedge_inputs[j]>=0; j++)
        {
            int input_number=clock_desc->negedge_inputs[j];
            engine->register_input_used_on_clock( engine_handle, module_desc->input_descs[input_number].port_name, clock_desc->name, 0 );
        }
        for (int j=0; clock_desc->negedge_outputs && clock_desc->negedge_outputs[j]>=0; j++)
        {
            int output_number=clock_desc->negedge_outputs[j];
            engine->register_output_generated_on_clock( engine_handle, module_desc->output_descs[output_number].port_name, clock_desc->name, 0 );
        }
    }
}

/*f se_cmodel_assist_instantiation_wire_ports
*/
extern void se_cmodel_assist_instantiation_wire_ports( c_engine *engine, void*engine_handle, void *base, const char *module_name, const char *module_instance_name, void *instance_handle, t_se_cma_instance_port *port_list )
{
    for (int i=0; port_list[i].port_name; i++)
    {
        t_se_cma_instance_port *port = &(port_list[i]);
        int comb, size;
        if (port->is_input)
        {
            engine->submodule_input_type( instance_handle, port->port_name, &comb, &size );
            if (!comb && port->comb)
            {
                fprintf(stderr,"Warning, incorrect timing file (no impact to simulation): submodule input %s.%s.%s is used for flops (no combinatorial input-to-output path) in that submodule, but timing file used by the calling module indicated that the input was used combinatorially (with 'timing comb input').\n",module_name,module_instance_name,port->port_name);
            }
            if (comb && !port->comb)
            {
                fprintf(stderr,"Serious error, potential missimulation: submodule input %s.%s.%s is used combinatorially in that submodule to produce an output, but timing file used by the calling module indicated that it was not. If the input is part of a structure and ANY element is combinatorial, then all elements of the structure are deemed combinatorial.\n",module_name,module_instance_name,port->port_name);
            }
            engine->submodule_drive_input( instance_handle, port->port_name, struct_resolve( t_sl_uint64 *, base, port->instance_port_offset), port->width );
        }
        else
        {
            engine->submodule_output_type( instance_handle, port->port_name, &comb, &size );
            if (!comb && port->comb)
            {
                fprintf(stderr,"Warning, incorrect timing file (no impact to simulation): submodule output %s.%s.%s is generated only off a clock (no combinatorial input-to-output path) in that submodule, but timing file used by the calling module indicated that the output was generated combinatorially from an input (with 'timing comb output').\n",module_name, module_instance_name,port->port_name);
            }
            if (comb && !port->comb)
            {
                fprintf(stderr,"Serious error, potential missimulation: submodule output %s.%s.%s is generated combinatorially in that submodule from an input, but timing file used by the calling module indicated that it was not. If the output is part of a structure and ANY element is combinatorial, then all elements of the structure are deemed combinatorial.\n",module_name,module_instance_name,port->port_name);
            }
            engine->submodule_output_add_receiver( instance_handle, port->port_name, struct_resolve( t_sl_uint64 **, base, port->instance_port_offset), port->width );
            if (port->net_port_offset>=0) {engine->submodule_output_add_receiver( instance_handle, port->port_name, struct_resolve( t_sl_uint64 **, base, port->net_port_offset), port->width );}
            if (port->output_port_name)   {engine->submodule_output_drive_module_output( instance_handle, port->port_name, engine_handle, port->output_port_name );}
        }
    }
}

/*f se_cmodel_assist_check_unconnected_inputs
*/
extern void se_cmodel_assist_check_unconnected_inputs( c_engine *engine, void*engine_handle, void *base, t_se_cma_input_desc *input_descs, const char *module_type )
{
    t_sl_uint64 **ptr;
    for (int i=0; input_descs[i].port_name; i++)
    {
        ptr = struct_resolve( t_sl_uint64 **, base, input_descs[i].driver_ofs );
        if (ptr[0]==NULL)
        {
            fprintf( stderr,"FATAL: Unconnected input port '%s' on module '%s' (of type %s) at initial reset\n",
                     input_descs[i].port_name,
                     engine->get_instance_full_name(engine_handle),
                     module_type
                );
            ptr[0]=&dummy_input[0];
        }
    }
}

/*f se_cmodel_assist_check_unconnected_nets
*/
extern void se_cmodel_assist_check_unconnected_nets( c_engine *engine, void*engine_handle, void *base, t_se_cma_net_desc *net_descs, const char *module_type )
{
    t_sl_uint64 **ptr;
    for (int i=0; net_descs[i].port_name; i++)
    {
        if (net_descs[i].net_driver_offset>=0)
        {
            ptr = struct_resolve( t_sl_uint64 **, base, net_descs[i].net_driver_offset );
            if (ptr[0]==NULL)
            {
                fprintf( stderr, "POSSIBLY FATAL: Undriven internal net '%s' in module '%s' (of type %s) at initial reset (did its driver module instantiate okay?)\n",
                         net_descs[i].port_name,
                         engine->get_instance_full_name(engine_handle),
                         module_type
                    );
                ptr[0]=&dummy_input[0];
            }
        }
    }
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

