/*a Copyright
  
  This file 'se_cmodel_assist.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_SE_CMODEL_ASSIST
#else
#define __INC_SE_CMODEL_ASSIST

/*a Includes
 */
#include "sl_general.h"
#include "c_se_engine.h"

/*a Defines
 */
#define se_cmodel_assist_stmt_coverage_register(se,se_handle,data) se->coverage_stmt_register(se_handle,(void *)(&data),sizeof(data))
#define se_cmodel_assist_stmt_coverage_statement_reached(coverage,number) {coverage.map[number]=(coverage.map[number]==255)?255:coverage.map[number]+1;}
#define se_cmodel_assist_code_coverage_register(se,se_handle,data) se->coverage_code_register(se_handle,(void *)(&data),sizeof(data))
#define se_cmodel_assist_code_coverage_case_already_reached(coverage,c,b) (!((coverage.bitmap[c]>>(b))&1))
#define se_cmodel_assist_code_coverage_case_now_reached(coverage,c,b) {coverage.bitmap[c]|=(1<<(b));}

/*a Types
 */
/*a Global types
*/
/*t t_se_cma_clock_desc
*/
typedef struct t_se_cma_clock_desc
{
    const char *name;
    int *posedge_inputs;
    int *negedge_inputs;
    int *posedge_outputs;
    int *negedge_outputs;
} t_se_cma_clock_desc;

/*t t_se_cma_input_desc
*/
typedef struct t_se_cma_input_desc
{
    const char *port_name;
    int driver_ofs;
    int input_state_ofs;
    char width;
    char is_comb;
} t_se_cma_input_desc;

/*t t_se_cma_output_desc
*/
typedef struct t_se_cma_output_desc
{
    const char *port_name;
    int value_ofs;
    char width;
    char is_comb;
} t_se_cma_output_desc;

/*t t_se_cma_net_desc
*/
typedef struct t_se_cma_net_desc
{
    const char *port_name;
    int net_driver_offset;
    char width;
    char vector_driven_in_parts;
} t_se_cma_net_desc;

/*t t_se_cma_instance_port
*/
typedef struct t_se_cma_instance_port
{
    const char *port_name;
    const char *output_port_name;
    int instance_port_offset;
    int net_port_offset;
    char width;
    char is_input;
    char comb;
} t_se_cma_instance_port;

/*t t_se_cma_module_desc
*/
typedef struct t_se_cma_module_desc
{
    t_se_cma_input_desc  *input_descs;
    t_se_cma_output_desc *output_descs;
    t_se_cma_clock_desc  **clock_descs_list;
} t_se_cma_module_desc;

/*a Inline function definitions
 */
/*f se_cmodel_assist_assign_to_bit
 */
static inline void se_cmodel_assist_assign_to_bit( t_sl_uint64 *vector, int size, int bit, unsigned int value)
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
static inline void se_cmodel_assist_assign_to_bit_range( t_sl_uint64 *vector, int size, int bit, int length, t_sl_uint64 value)
{
    t_sl_uint64 old, bit_mask;
    int shift;
    bit_mask = (length >= 64) ? ~0ULL : ((1ULL << length)-1);

    if ((bit+length<=size) && (bit>=0) && (length>=1))
    {
        old = vector[bit/64];
        shift = bit%64;
        vector[bit/64] = (old &~ (bit_mask<<shift)) | ((value&bit_mask)<<shift);
    }
}

/*a Function declarations - for functions that are called once per instantiation, or once per error message etc
 */
extern char *se_cmodel_assist_vsnprintf( char *buffer, int buffer_size, const char *format, int num_args, ... );
extern void se_cmodel_assist_module_declaration( c_engine *engine, void*engine_handle, void *base, t_se_cma_module_desc *module_desc );
extern void se_cmodel_assist_instantiation_wire_ports( c_engine *engine, void*engine_handle, void *base, const char *module_name, const char *module_instance_name, void *instance_handle, t_se_cma_instance_port *port_list );
extern void se_cmodel_assist_check_unconnected_inputs( c_engine *engine, void*engine_handle, void *base, t_se_cma_input_desc *input_descs, const char *module_type );
extern void se_cmodel_assist_check_unconnected_nets( c_engine *engine, void*engine_handle, void *base, t_se_cma_net_desc *net_descs, const char *module_type );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

