/*a Copyright
  
This file 'c_se_engine__internal_types.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_ENGINE__INTERNAL_TYPES
#else
#define __INC_ENGINE__INTERNAL_TYPES

/*a Includes
 */
#include "sl_timer.h"
#include "sl_work_list.h"
#include "c_sl_error.h"
#include "c_se_engine.h"

/*a Defines
 */
#define INPUT_BIT( iptr, bit ) ( (iptr[(bit/64)]>>(bit%64))&1 )

/*a Types
 */
/*t t_thread_pool_submodule_mapping
 */
typedef struct t_thread_pool_submodule_mapping
{
    struct t_thread_pool_submodule_mapping *next_in_list;
    t_sl_wl_thread_ptr thread_ptr;
    char submodule_name[1];
} t_thread_pool_submodule_mapping;

/*t t_thread_pool_mapping
 */
typedef struct t_thread_pool_mapping
{
    struct t_thread_pool_mapping *next_in_list;
    struct t_thread_pool_submodule_mapping *mapping_list;
    char module_name[1];
} t_thread_pool_mapping;

/*t t_engine_function
  for ports on instances.
  basically this structure contains the set of items that may be linked as a port or function of an instance
  the type is implicit from the chain this item is on
*/
typedef struct t_engine_function
{
    struct t_engine_function *next_in_list;
    struct t_engine_module_instance *module_instance;
    char *name;
    void *handle; // Handle for any callback
    union
    {
        struct {
            t_engine_callback_arg_fn reset_fn; // Callback for reset, if this is on the chain of reset functions; there should be just one per module instance
        } reset;
        struct {
            t_engine_callback_fn comb_fn; // Callback for combinatorial function; there should be just one per module instance
        } comb;
        struct {
            t_engine_callback_fn propagate_fn; // Callback for propagate inputs function; there should be just one per module instance
        } propagate;
        struct {
            t_engine_callback_fn prepreclock_fn; // Callback for prepreclock function; there should be just one per module instance
        } prepreclock;
        struct
        {
            t_engine_callback_fn posedge_prepreclock_fn;
            t_engine_callback_fn posedge_preclock_fn;
            t_engine_callback_fn posedge_clock_fn;
            t_engine_callback_fn negedge_prepreclock_fn;
            t_engine_callback_fn negedge_preclock_fn;
            t_engine_callback_fn negedge_clock_fn;
            struct t_engine_clock *driven_by; // Main simulation engine clock that drives this clock
        } clock;
        struct
        {
            int size; // Size of the input in bits
            t_sl_uint64 **value_ptr_ptr; // Pointer to an integer array that contains the driving data; the 'int *' is set when the driver for the input is connected
            int may_be_unconnected;
            struct t_engine_signal *driven_by; // For signals driven by a toplevel, this is a pointer to that signal
            struct t_engine_function_reference *used_by_clocks; // A list of clocks that utilize this inputs; not really necessary, but good for completeness
            int combinatorial; // Indicates the input is used by the combinatorial function; this input is therefore combinatorially connected to an output
            struct t_engine_function_reference *connected_to; // For an input on a module that is directly connected to one/many submodule's input(s), this indicates those inputs; so when the input is driven, the submodules may be too
        } input;
        struct
        {
            int size;
            t_sl_uint64 *value_ptr;
            struct t_engine_signal_reference *drives; // A list of all globals that this output drives
            struct t_engine_function *generated_by_clock;
            int combinatorial;
        } output;
    } data;
} t_engine_function;

/*t t_engine_function_reference
  this is a list of references to ports on an instance
*/
typedef struct t_engine_function_reference
{
    struct t_engine_function_reference *next_in_list;
    t_engine_function *signal;
} t_engine_function_reference;

/*t t_engine_function_list
 */
typedef struct t_engine_function_list
{
    struct t_engine_function_list *next_in_list;
    void *handle;
    struct t_engine_function *signal;
    t_engine_callback_fn callback_fn;
    t_sl_timer timer; // For profiling - time spent in the callback
    int invocation_count; // For profiling - number of times callback has been invoked
} t_engine_function_list;

/*t t_engine_state_desc_list
 */
typedef struct t_engine_state_desc_list
{
    struct t_engine_state_desc_list *next_in_list;
    int num_state;
    t_engine_state_desc *state_desc;
    void *data_base_ptr;
    int data_indirected; // 0 if (data+desc.offset) is a ptr to the approrpriate data (e.g. int* for bit vector), 1 if data+desc.offset is a ptr to that ptr (e.g. int** for bit vector)
    int data_clocked;    // 0 if data is input/output/combinatorial; 1 if data is clocked. Only clocked data is stored in a checkpoint, for example
    char *prefix;
} t_engine_state_desc_list;

/*t t_engine_signal
 */
typedef struct t_engine_signal
{
    struct t_engine_signal *next_in_list;
    struct t_engine_signal *next_in_hash;
    char *global_name;
    int size;
    t_engine_function_reference *drives_list;
    t_engine_function_reference *driven_by;
    int valid;
    int changed;
} t_engine_signal;

/*t t_engine_signal_reference
  this is a list of references to signals
*/
typedef struct t_engine_signal_reference
{
    struct t_engine_signal_reference *next_in_list;
    t_engine_signal *signal;
} t_engine_signal_reference;

/*t t_engine_clock_fns
 */
typedef struct t_engine_clock_fns
{
    t_engine_function_list *prepreclock; // Copied from module instance in the scheduler
    t_engine_function_list *preclock;
    t_engine_function_list *clock;
    t_engine_function_list *comb;
    t_engine_function_list *propagate; // Copied from module instance in the scheduler
} t_engine_clock_fns;

/*t t_engine_clock
 */
typedef struct t_engine_clock
{
    struct t_engine_clock *next_in_list;
    char *global_name;
    t_engine_function_reference *clocks_list; // List of module instances' clocks (efr's) bound to this global clock
    t_engine_clock_fns posedge;
    t_engine_clock_fns negedge;
    int delay; // delay in cycles before first posedge
    int high_cycles;
    int low_cycles;
    int cycle_length; // sum of high and low cycles, zero for inoperative
    int posedge_remainder; // if cycle%cycle_length==this, its a posedge (and value will be set)
    int negedge_remainder; // if cycle%cycle_length==this, its a negedge (and value will be cleared)
    int edge; // in running the schedule, 0 is no edge, 1 is posedge, -1 is negedge
    t_se_signal_value next_value; // clock starts at zero, waits for 'delay' cycles, goes high for 'high_cycles', low for 'low_cycles', hih, etc
    t_se_signal_value value;      // current clock value for simulation view
} t_engine_clock;

/*t t_engine_coverage_desc
 */
typedef struct t_engine_coverage_desc
{
    int stmt_data_size;
    unsigned char *stmt_data;
    int code_map_size; // size in entries
    t_sl_uint64 *code_map;
} t_engine_coverage_desc;

/*t t_engine_module_instance
 */
typedef struct t_engine_module_instance
{
    struct t_engine_module_instance *next_instance;
    struct t_engine_module_instance *parent_instance; // Parent in a hierarchy, if this is not a root module instance
    struct t_engine_module_instance *next_sibling_instance;
    struct t_engine_module_instance *first_child_instance;

    void *module_handle; // Handle from instantiation function; used in all callbacks
    char *name; // Instance name inside this level of hierarchy 
    unsigned int name_hash;
    char *full_name; // Full instance name from global level
    unsigned int full_name_hash;
    char *type; // Module type

    t_sl_option_list option_list;
    t_engine_function_list *delete_fn_list;
    t_engine_function_list *reset_fn_list;
    t_engine_function *clock_fn_list;
    t_engine_function *comb_fn_list;
    t_engine_function *propagate_fn_list;
    t_engine_function *prepreclock_fn_list;
    t_engine_function *input_list;
    t_engine_function *output_list;
    t_engine_state_desc_list *state_desc_list;
    t_engine_coverage_desc *coverage_desc;
    t_engine_function_list *checkpoint_fn_list;
    t_engine_function_list *message_fn_list;
    struct t_engine_log_event_array *log_event_list;

    void *thread_pool; // Used for multithreaded modules
    void *worklist; // Used for multithreaded modules

    t_engine_state_desc_list *sdl_to_view;
    int state_to_view;

    struct t_engine_checkpoint_entry *checkpoint_entry;

    t_sl_timer timer_clk_fns; // For profiling - time spent in clock functions for this module instance (when invoked as a submodule)
    t_sl_timer timer_comb_fns; // For profiling - time spent in comb functions for this module instance (when invoked as a submodule)
    t_sl_timer timer_propagate_fns; // For profiling - time spent in propagate functions for this module instance (when invoked as a submodule)

} t_engine_module_instance;

/*t t_se_engine_monitor
 */
typedef struct t_se_engine_monitor
{
    struct t_se_engine_monitor *next_in_list;
    t_se_interrogation_handle entity;
    t_se_engine_monitor_callback_fn callback;
    void *handle;
    void *handle_b;
    int reset;
    int *entity_data;
    int int_size;
    int data[1];
} t_se_engine_monitor;

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

