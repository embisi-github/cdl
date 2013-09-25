/*a Copyright
  
  This file 'c_se_engine.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_ENGINE
#else
#define __INC_ENGINE

/*a Includes
 */
#include "c_sl_error.h"
#include "sl_option.h"
#include "sl_general.h"
#ifndef Py_PYTHON_H
#ifdef SE_ENGINE_PYTHON
#include <Python.h>
#endif
#ifndef SE_ENGINE_PYTHON
struct _object;
typedef struct _object PyObject; // To make non-Python builds link
#endif
#endif

/*a Defines
 */
#ifdef _WIN32
    #define DLLEXPORT __declspec(dllexport)
#else
    #define DLLEXPORT
#endif

/*a Types
 */
/*t t_se_signal_value
 */
typedef t_sl_uint64 t_se_signal_value;

/*t t_se_message_reason
 */
typedef enum
{
    se_message_reason_set_option = 1,           // ptrs[0] = const char *option_name, ptrs[1] = const char *value (or size_t value)
    se_message_reason_force_reset = 2,          // ints[0] = pass
    se_message_reason_add_callback = 3,
    se_message_reason_remove_callback = 4,
    se_message_reason_interrogate_state = 5,    // ptrs[0] = const char *name (interpreted by module); standard return ptrs[0] = ptr to data (if array/vector); values[1] = data_width (width of vector/array), values[2] = 0 or size of array
    se_message_reason_log_filter = 6,           // ptrs[0] = const char *name (interpreted by module), values[1-3] are interpreted by module; for example SRAMS use name=read/write, values[1]=address mask, values[2]=address match (mask=0 => disable)
    se_message_reason_read = 8,                 // interpreted by module - standard is values[0]=n, values[1]=address, values[2]=byte size of buffer, ptrs[3]=data buffer; values[0]==0 => values[2..] on return is the data read
    se_message_reason_write = 9,                // interpreted by module
    se_message_reason_checkpoint  = 10,         // to be specified by checkpoint/recover
    se_message_reason_checkpoint2 = 11,         // to be specified by checkpoint/recover
    se_message_reason_action_one = 16,          // interpreted by module
} t_se_message_reason;

/*t t_se_message_response_type
 */
typedef enum
{
    se_message_response_type_none,
    se_message_response_type_int,
    se_message_response_type_values,
    se_message_response_type_ptrs,
    se_message_response_type_text,
    se_message_response_type_other
} t_se_message_response_type;

/*t t_se_message
 */
typedef struct t_se_message
{
    int reason;
    int response;
    const char *text_request;
    t_se_message_response_type response_type;
    union
    {
        int               ints[8];
        t_se_signal_value values[4];
        void               *(ptrs[4]);
        char              text[32];
    } data;
} t_se_message;

/*t t_se_interrogation_handle
 */
typedef struct t_se_interrogation_data *t_se_interrogation_handle;

/*t t_se_engine_monitor_callback_fn
 */
typedef void (*t_se_engine_monitor_callback_fn)( void *handle, void *handle_b );

/*t t_se_engine_simulation_callback_fn
 */
typedef int (*t_se_engine_simulation_callback_fn)( void *handle, void *handle_b );

/*t t_engine_message_level
 */
typedef enum
{
     message_level_none,
     message_level_pass,
     message_level_fail,
     message_level_end
} t_engine_message_level;

/*t t_se_instantiation_fn
 */
typedef t_sl_error_level (*t_se_instantiation_fn)( class c_engine *engine, void *engine_handle );

/*t t_se_worklist_call
 */
typedef enum
{
    se_wl_item_prepreclock,
    se_wl_item_preclock,
    se_wl_item_clock,
    se_wl_item_count // Must be last - indicates the number of elements
} t_se_worklist_call;

/*t t_engine_mutex
 */
typedef enum
{
    engine_mutex_message,
    engine_mutex_log_callback,
    engine_mutex_call_worklist,
    engine_mutex_last_mutex // Must be last...
} t_engine_mutex;

/*t t_engine_callback_fn
 */
typedef t_sl_error_level (*t_engine_callback_fn)( void *handle );

/* t_engine_log_event_callback_fn
 */
typedef t_sl_error_level (*t_engine_log_event_callback_fn)( void *engine_handle, int cycle, struct t_engine_log_interest *log_interest, void *handle, int event_number, struct t_engine_log_event_array_entry *log_event_interest );

/*t t_engine_callback_arg_fn
 */
typedef t_sl_error_level (*t_engine_callback_arg_fn)( void *handle, int pass );

/*t t_engine_callback_argp_fn
 */
typedef t_sl_error_level (*t_engine_callback_argp_fn)( void *handle, void *arg );

/*t t_engine_sim_function_type
 */
typedef enum t_engine_sim_function_type
{
    engine_sim_function_type_posedge_prepreclock,
    engine_sim_function_type_posedge_preclock,
    engine_sim_function_type_posedge_clock,
    engine_sim_function_type_negedge_prepreclock,
    engine_sim_function_type_negedge_preclock,
    engine_sim_function_type_negedge_clock,
} t_engine_sim_function_type;

/*t t_engine_submodule_clock_set_ptr;
 */
typedef struct t_engine_submodule_clock_set *t_engine_submodule_clock_set_ptr;

/*t t_engine_state_desc_type
 */
typedef enum
{
     engine_state_desc_type_none, // used as a terminator in lists
     engine_state_desc_type_bits, // args[0] is size, ptr is int *data
     engine_state_desc_type_array, // args[0] is size of each entry in bits, args[1] is size of array, ptr is int *data (for old int **, use indirected type for all the memory)
     engine_state_desc_type_fsm, // args[0] is the number of bits in the descriptor, arg[1] is the number of states, arg[2] is the encoding (0=>plain, 1=>one-hot, 2=>one-cold), arg_ptr[0] points to a list of char * state names
     engine_state_desc_type_exec_file, // ptr/offset points to an exec_file_data structure, arg_ptr[0] points to a filename
} t_engine_state_desc_type;

/*t t_engine_state_desc_type_mask
 */
typedef enum
{
     engine_state_desc_type_mask_none = 1<<engine_state_desc_type_none,
     engine_state_desc_type_mask_bits = 1<<engine_state_desc_type_bits,
     engine_state_desc_type_mask_array = 1<<engine_state_desc_type_array,
     engine_state_desc_type_mask_fsm = 1<<engine_state_desc_type_fsm,
     engine_state_desc_type_mask_exec_file = 1<<engine_state_desc_type_exec_file,
} t_engine_state_desc_type_mask;

/*t t_engine_interrogate_include_mask
 */
typedef enum
{
     engine_interrogate_include_mask_state = 1,
     engine_interrogate_include_mask_clocks = 2,
     engine_interrogate_include_mask_inputs = 4,
     engine_interrogate_include_mask_outputs = 8,
     engine_interrogate_include_mask_all = 0x0f,
     engine_interrogate_include_mask_submodules = 0x100,
} t_engine_interrogate_include_mask;

/*t t_engine_state_desc
 */
typedef struct t_engine_state_desc
{
     const char *name;
     t_engine_state_desc_type type;
     void *ptr; // NULL if offset is from the instance ptr
     int offset;
     int args[4];
     void *arg_ptr[4];
} t_engine_state_desc;

/*t t_engine_log_event
 */
typedef struct t_engine_log_event
{
    const char *name; // May be duplicated, as a 'read' event may have many instances
    int number_values;
    struct
    {
        const char *value_name;
        int value_offset; // Add to a t_se_signal_value * base_address to get the address of the value of this arg
    } values[1];
} t_engine_log_event;

/*t t_engine_text_value_pair
 */
typedef struct t_engine_text_value_pair
{
    const char *text; // Either event name for a descriptor, or value name
    int value; // Either number of values for a descriptor, or value offset
} t_engine_text_value_pair;

/*a c_engine class
 */
/*c	c_engine
*/
class DLLEXPORT c_engine
{
public:

     /*b Constructors/destructors
      */
     c_engine( c_sl_error *error, const char *name );
     ~c_engine();
     void delete_instances_and_signals( void );

     /*b Internal logic module methods - public as it is called from a static function
      */
     struct t_internal_module_data *create_internal_module_data( void );

     /*b Hardware file and instantiation methods
      */
     t_sl_error_level instantiation_exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb );
     t_sl_error_level read_file( const char *filename );
     t_sl_error_level read_and_interpret_hw_file( const char *filename, t_sl_get_environment_fn env_fn, void *env_handle );
     t_sl_error_level read_and_interpret_py_object( PyObject *describer, t_sl_get_environment_fn env_fn, void *env_handle );
     void free_forced_options( void );
     t_sl_error_level instance_add_forced_option( const char *full_module_name, t_sl_option_list option_list );
     t_sl_error_level instantiate( void *parent_engine_handle, const char *type, const char *name, t_sl_option_list option_list );
     t_sl_error_level add_global_signal( const char *name, int size );
     t_sl_error_level name( const char *filename, int line_number, const char *driver_module_name, const char *driver_signal_name, const char *global_name );
     t_sl_error_level drive( const char *filename, int line_number, const char *driven_module_name, const char *driven_signal_name, const char *global_name );
     t_sl_error_level bit_extract( const char *filename, int line_number, const char *global_bus_name, int start, int size, const char *global_name );
     t_sl_error_level bundle( const char *filename, int line_number, int number_inputs, const char *global_output_bus_name, const char **global_input_bus_names );
     t_sl_error_level data_mux( const char *filename, int line_number, int number_inputs, const char *global_select_name, const char *global_output_bus_name, const char **global_input_bus_names );
     t_sl_error_level assign( const char *filename, int line_number, const char *output_bus, int value, int until_cycle, int after_value );
     t_sl_error_level create_clock_phase( const char *filename, int line_number, const char *output_signal, const char *global_clock_name, int delay, int pattern_length, const char *pattern );
     t_sl_error_level compare( const char *filename, int line_number, const char *input_bus, const char *output_signal, int value );
     t_sl_error_level decode( const char *filename, int line_number, const char *input_bus, const char *output_bus, const char *enable );
     t_sl_error_level generic_logic( const char *filename, int line_number, int number_inputs, const char *type, const char *global_output_bus_name, const char **global_input_bus_names );
     t_sl_error_level create_clock( const char *filename, int line_number, const char *global_clock_name, int delay, int high_cycles, int low_cycles );
     t_sl_error_level create_clock_divide( const char *filename, int line_number, const char *global_clock_name, const char *source_clock_name, int delay, int high_cycles, int low_cycles );
     t_sl_error_level bind_clock( const char *filename, int line_number, const char *module_instance_name, const char *module_signal_name, const char *global_clock_name );
     void module_drive_input( struct t_engine_function *input, t_se_signal_value *value_ptr );
     t_sl_error_level check_connectivity( void );
     t_sl_error_level build_schedule( void );
     char *create_unique_instance_name( void *parent, const char *first_hint, ... ); // Find a unique name within a given module (NULL parent for global)

     /*b Option handling methods - called by modules instances at their instantiation
      */
     int get_option_int( void *handle, const char *keyword, int default_value );
     const char *get_option_string( void *handle, const char *keyword, const char *default_value );
     void *get_option_object( void *handle, const char *keyword );
     t_sl_option_list get_option_list( void *handle );
     t_sl_option_list set_option_list( void *handle, t_sl_option_list option_list );

     /*b Module, signal, clock, state registration methods - called by modules instances at their instantiation
      */
     void register_delete_function( void *engine_handle, void *handle, t_engine_callback_fn delete_fn );
     void register_reset_function( void *engine_handle, void *handle, t_engine_callback_arg_fn reset_fn );
     void register_input_signal( void *engine_handle, const char *name, int size, t_se_signal_value **value_ptr_ptr );
     void register_input_signal( void *engine_handle, const char *name, int size, t_se_signal_value **value_ptr_ptr, int may_be_unconnected );
     void register_output_signal( void *engine_handle, const char *name, int size, t_se_signal_value *value_ptr );
     void register_clock_fns( void *engine_handle, void *handle, t_engine_callback_fn prepreclock_fn, t_engine_callback_fn clock_fn ); // for use with modules that just declare preclock for each clock signal
     void register_clock_fns( void *engine_handle, void *handle, const char *clock_name, t_engine_callback_fn posedge_preclock_fn, t_engine_callback_fn posedge_clock_fn );
     void register_clock_fns( void *engine_handle, void *handle, const char *clock_name, t_engine_callback_fn posedge_preclock_fn, t_engine_callback_fn posedge_clock_fn, t_engine_callback_fn negedge_preclock_fn, t_engine_callback_fn negedge_clock_fn );
     void register_preclock_fns( void *engine_handle, void *handle, const char *clock_name, t_engine_callback_fn posedge_preclock_fn, t_engine_callback_fn negedge_preclock_fn ); // for use with modules that support prepreclock
     void register_prepreclock_fn( void *engine_handle, void *handle, t_engine_callback_fn propagate_fn );
     int register_clock_fn( void *engine_handle, void *handle, const char *clock_name, t_engine_sim_function_type type, t_engine_callback_fn x_clock_fn );
     void register_comb_fn( void *engine_handle, void *handle, t_engine_callback_fn comb_fn );
     void register_propagate_fn( void *engine_handle, void *handle, t_engine_callback_fn propagate_fn );
     void register_message_function( void *engine_handle, void *handle, t_engine_callback_argp_fn message_fn );
     void register_input_used_on_clock( void *engine_handle, const char *name, const char *clock_name, int posedge );
     void register_output_generated_on_clock( void *engine_handle, const char *name, const char *clock_name, int posedge );
     void register_comb_input( void *engine_handle, const char *name );
     void register_comb_output( void *engine_handle, const char *name );
     void register_state_desc( void *engine_handle, int static_desc, t_engine_state_desc *state_desc, void *data, const char *prefix );
     int register_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle );
     int register_handle_exec_file_command( void *efl, struct t_sl_exec_file_cmd_cb *cmd_cb );

     /*b Cycle simulation operation methods
      */
     void bfm_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle, const char *clock, int posedge );
     void bfm_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle );
     int simulation_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle, const char *clock, int posedge ); // handle can be NULL for no inputs/outputs
     int simulation_handle_exec_file_command( struct t_sl_exec_file_cmd_cb *cmd_cb );
     void reset_state( void );
     void write_profile( FILE *f );
     t_sl_error_level step_cycles( int cycles );
     struct t_se_engine_monitor *add_monitor_callback( const char *global_name, t_se_engine_monitor_callback_fn callback, void *handle, void *handle_b );
     struct t_se_engine_simulation_callback *simulation_add_callback( t_se_engine_simulation_callback_fn callback, void *handle, void *handle_b );
     void simulation_free_callback( struct t_se_engine_simulation_callback *scb );
     int simulation_invoke_callbacks( void ); // Invoke all simulation callbacks that have been registered
     void simulation_set_cycle( int n ); // Set the cycle number
     int cycle( void );
     void simulation_assist_prepreclock_instance( void *engine_handle, int posedge );
     void simulation_assist_preclock_instance( void *engine_handle, int posedge, const char *clock_name );
     void simulation_assist_clock_instance( void *engine_handle, int posedge, const char *clock_name );
     void simulation_assist_prepreclock_instance( void *engine_handle, int posedge, const char *clock_name );
     void simulation_assist_preclock_instance( void *engine_handle, int posedge );
     void simulation_assist_clock_instance( void *engine_handle, int posedge );
     void simulation_assist_comb_instance( void *engine_handle);
     void simulation_assist_propagate_instance( void *engine_handle);
     void simulation_assist_reset_instance( void *engine_handle, int pass );
     t_sl_error_level add_message( void *location,
                                 t_sl_error_level error_level,
                                 int error_number,
                                 int function_id,
                                 ... );
     t_sl_error_level add_error( void *location,
                                 t_sl_error_level error_level,
                                 int error_number,
                                 int function_id,
                                 ... );

     /*b Checkpoint/restore methods
      */
     int checkpoint_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle );
     int checkpoint_handle_exec_file_command( struct t_checkpoint_ef_lib *lib, struct t_sl_exec_file_data *exec_file_data, int cmd, int num_args, struct t_sl_exec_file_value *args );
     t_sl_error_level initialize_checkpointing( void );
     struct t_engine_checkpoint_entry *checkpoint_create_descriptor_entry( struct t_engine_module_instance *emi, int num_mem_blocks, int num_state_entries );
     t_sl_error_level checkpoint_initialize_instance_declared_state( struct t_engine_module_instance *emi );
     struct t_engine_checkpoint *checkpoint_find_from_name( const char *name );
     struct t_engine_checkpoint *checkpoint_find_up_to_time( int time );
     t_sl_error_level checkpoint_get_information( const char *name, const char **parent, int *time, const char **next_ckpt );
     void checkpointing_display_information( int level ); // for debug only really
     int checkpoint_size( const char *name, const char *previous ); // If previous, then delta from that; approximate size only (to within about 10%)
     t_sl_error_level checkpoint_add( const char *name, const char *previous ); // If previous, then delta from that
     t_sl_error_level checkpoint_restore( const char *name );
     void *find_checkpoint( const char *name );
     void *delete_checkpoint( void *handle );
     void *delete_all_checkpoints( void );
     t_sl_error_level write_checkpoint_to_file( void *handle, const char *filename, int type );
     t_sl_error_level read_checkpoint_from_file( const char *name, const char *filename );

     /*b Instance and state interrogation methods
      */
     void *enumerate_instances( void *previous_handle, int skip );
     t_sl_error_level enumerate_instances( struct t_sl_cons_list *cl );
     int set_state_handle( void *engine_handle, int n );
     t_engine_state_desc_type get_state_name_and_type( void *engine_handle, const char **module_name, const char **state_prefix, const char **state_name );
     int get_state_value_string( void *engine_handle, char *buffer, int buffer_size );
     t_engine_state_desc_type interrogate_get_internal_fn( t_se_interrogation_handle entity, void **fn );
     int         get_state_value_data_and_sizes( void *engine_handle, int **data, int *sizes );
     const char *get_instance_name( void *engine_handle );
     const char *get_instance_full_name( void *engine_handle );
     const char *get_instance_name( void );
     c_engine   *get_next_instance( void );
     void       *find_module_instance( void *parent, const char *name ); // Find module instance from parent (NULL for global) and name
     void       *find_module_instance( void *parent, const char *name, int length ); // Find module instance from parent (NULL for global) and name/length
     void       *find_module_instance( const char *name ); // Find module instance from global name
     void       *find_module_instance( const char *name, int length ); // Find module instance from global name/length
     int        module_instance_send_message( void *module, t_se_message *message ); // Send a module instance (from handle) a message if the instance supports messages
     void       *find_global( const char *name );
     int        find_state( char *name, int *id );
     t_se_interrogation_handle     interrogation_handle_create( void );
     void                          interrogation_handle_free( t_se_interrogation_handle entity );
     t_se_interrogation_handle     find_entity( const char *string ); // Find a signal, input, output, state, or module_instantiation from name
     void                          check_output_shadows_state( struct t_engine_module_instance *emi, struct t_engine_function *signal );
     int                           interrogate_count_hierarchy( t_se_interrogation_handle entity, t_engine_state_desc_type_mask type_mask, t_engine_interrogate_include_mask include_mask );
     int                           interrogate_enumerate_hierarchy( t_se_interrogation_handle entity, int sub_number, t_engine_state_desc_type_mask type_mask, t_engine_interrogate_include_mask include_mask, t_se_interrogation_handle *sub_entity );
     int                           interrogate_find_entity( t_se_interrogation_handle entity, const char *entity_name, t_engine_state_desc_type_mask type_mask, t_engine_interrogate_include_mask include_mask, t_se_interrogation_handle *sub_entity );
     int                           interrogate_get_entity_strings( t_se_interrogation_handle entity, const char **module_or_global, const char **prefix, const char **tail );
     int                           interrogate_get_entity_path_string( t_se_interrogation_handle entity, char *buffer, int buffer_size );
     const char                    *interrogate_get_entity_clue( t_se_interrogation_handle entity );
     int                           interrogate_get_entity_value_string( t_se_interrogation_handle entity, char *buffer, int buffer_size );
     t_engine_state_desc_type      interrogate_get_data_sizes_and_type( t_se_interrogation_handle entity, t_se_signal_value **data, int *sizes );

     /*b Submodule methods
      */
     void *submodule_get_handle( void *engine_handle, const char *submodule_name );
     void *submodule_get_clock_handle( void *submodule_handle, const char *clock_name ); //   Get a handle for submodule clock calls
     int submodule_clock_has_edge( void *submodule_clock_handle, int posedge ); // Return 1 if the module uses the requested edge clock edge, and therefore should have that edge function invoked
     void submodule_call_reset( void *submodule_handle, int pass );
     void submodule_call_prepreclock( void *submodule_handle ); // Call the submodule's prepreclock function
     void submodule_call_preclock( void *submodule_clock_handle, int posedge ); // Call the submodule's preclock function
     void submodule_call_clock( void *submodule_clock_handle, int posedge ); // Call the submodule's clock function
     void submodule_call_propagate( void *submodule_handle ); // Call the input propagation function of a submodule
     void submodule_call_comb( void *submodule_handle ); // Call the combinatorial function of a submodule
     t_engine_submodule_clock_set_ptr submodule_clock_set_declare( int n ); // Declare a set of functions to call, possibly in parallel
     int submodule_clock_set_entry( t_engine_submodule_clock_set_ptr scs, int n, void *submodule_handle ); // Declare an entry of the ser
     void submodule_clock_set_invoke( t_engine_submodule_clock_set_ptr scs, int *bitmask, t_engine_sim_function_type type ); // Possible parallel calling of multiple functions
     int submodule_input_type( void *submodule_handle, const char *name, int *comb, int *size );
     t_sl_error_level submodule_drive_input( void *submodule_handle, const char *name, t_se_signal_value *signal, int size ); // Drive the input of a submodule with a signal
     t_sl_error_level submodule_drive_input_with_module_input( void *module_handle, const char *module_name, void *submodule_handle, const char *submodule_name ); // Drive the input of a submodule with the input of a module
     t_sl_error_level submodule_drive_input_with_submodule_output( void *submodule_output_handle, const char *submodule_output_name, void *submodule_input_handle, const char *submodule_input_name );
     int submodule_output_type( void *submodule_handle, const char *name, int *comb, int *size );
     t_sl_error_level submodule_output_add_receiver( void *submodule_handle, const char *name, t_se_signal_value **signal, int size ); // Use an output of a submodule to drive a signal
     t_sl_error_level submodule_output_drive_module_output( void *submodule_output_handle, const char *submodule_output_name, void *module_output_handle, const char *module_output_name );
     void submodule_init_clock_worklist( void *engine_handle, int number_of_calls );
     t_sl_error_level submodule_set_clock_worklist_prepreclock( void *engine_handle, int call_number, void *submodule_handle );
     t_sl_error_level submodule_set_clock_worklist_clock( void *engine_handle, void *submodule_handle, int call_number, void *submodule_clock_handle, int posedge );
     t_sl_error_level submodule_call_worklist( void *engine_handle, t_se_worklist_call wl_call, int *guard );
     t_sl_error_level thread_pool_init( void );
     t_sl_error_level thread_pool_delete( void );
     t_sl_error_level thread_pool_add_thread( const char *thread_name );
     t_sl_error_level thread_pool_map_thread_to_module( const char *thread_name, const char *submodule_name );
     int thread_pool_mapped_submodules(t_engine_module_instance *emi );
     struct t_thread_pool_mapping *thread_pool_mapping_find( const char *emi_full_name, int length );
     struct t_thread_pool_mapping *thread_pool_mapping_add( const char *emi_full_name, int length );
     struct t_thread_pool_submodule_mapping *thread_pool_submodule_mapping_add( struct t_thread_pool_mapping *tpm, const char *smi_name, void *thread_ptr );
     struct t_thread_pool_submodule_mapping *thread_pool_submodule_mapping_find( struct t_thread_pool_mapping *tpm, const char *smi_name );
     const char *thread_pool_submodule_mapping( struct t_engine_module_instance *emi, struct t_engine_module_instance *smi );

     /*b Waveform methods
      */
     struct t_waveform_vcd_file *waveform_vcd_file_find( const char *name );
     struct t_waveform_vcd_file *waveform_vcd_file_create( const char *name );
     void waveform_vcd_file_free( struct t_waveform_vcd_file *wvf );
     int  waveform_vcd_file_open( t_waveform_vcd_file *wvf, const char *filename );
     void waveform_vcd_file_close( t_waveform_vcd_file *wvf);
     void waveform_vcd_file_reset( t_waveform_vcd_file *wvf );
     void waveform_vcd_file_pause( t_waveform_vcd_file *wvf, int pause ); // Pause or unpause a VCD wave file
     void waveform_vcd_file_add( t_waveform_vcd_file *wvf, t_se_interrogation_handle entity ); // Add an entity, and if a module just its ports not submodules
     void waveform_vcd_file_add_hierarchy( t_waveform_vcd_file *wvf, t_se_interrogation_handle entity, int max_depth ); // Add an entity, and if a module, all its submodules too, to a depth of 'max_depth'
     void waveform_vcd_file_add_hierarchy( t_waveform_vcd_file *wvf, t_se_interrogation_handle entity ); // Add an entity, and if a module, all its submodules too
     int waveform_vcd_file_count_and_fill_signals( struct t_waveform_vcd_file_entity_list *wvfel, int number_so_far, struct t_waveform_vcd_file_signal_entry *signals );
     void waveform_vcd_file_callback( struct t_waveform_vcd_file *wvf );
     void waveform_vcd_file_enable( struct t_waveform_vcd_file *wvf );
     int waveform_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data );
     int waveform_handle_exec_file_command( struct t_sl_exec_file_data *exec_file_data, int cmd, struct t_sl_exec_file_value *args );

     /*b Coverage methods
      */
     void coverage_stmt_register( void *engine_handle, void *data, int data_size ); // opaque-ish types, as should be called from se_cmodel_assist
     void coverage_code_register( void *engine_handle, void *data, int data_size ); // opaque-ish types, as should be called from se_cmodel_assist
     int coverage_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data );
     void coverage_reset( void );
     void coverage_load( char *filename, char *entity );
     void coverage_save( char *filename, char *entity );
     int coverage_handle_exec_file_command( struct t_sl_exec_file_data *exec_file_data, int cmd, struct t_sl_exec_file_value *args );

     /*b Log methods
      */
     int log_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle );
     int log_handle_exec_file_command( struct t_log_event_ef_lib *lib, struct t_sl_exec_file_data *exec_file_data, int cmd, int num_args, struct t_sl_exec_file_value *args );
     struct t_engine_log_event_array *log_event_array_create( void *engine_handle, int num_events, int total_args );
     struct t_engine_log_interest *log_interest_create( void *engine_handle, int num_events, t_engine_log_event_callback_fn callback_fn, void *handle );
     struct t_engine_log_parser *log_parse_create( void *engine_handle, int num_events, int num_args );
     struct t_engine_log_event_array *log_event_register( void *engine_handle, const char *event_name, t_se_signal_value *value_base, ... );
     struct t_engine_log_event_array *log_event_register_array( void *engine_handle, const t_engine_text_value_pair *descriptor, t_se_signal_value *value_base );
     void log_event_occurred( void *engine_handle, struct t_engine_log_event_array *event_array, int event_number ); // event_number=0 if registered with log_event_register()
     struct t_engine_log_interest *log_interest( void *engine_handle, void *module_instance, const char *event_name, t_engine_log_event_callback_fn callback_fn, void *handle );
     struct t_engine_log_parser *log_parse_setup( void *engine_handle, struct t_engine_log_interest *log_handle, const char *event_name, const char **value_names );
     struct t_engine_log_parser *log_parse_setup( void *engine_handle, struct t_engine_log_interest *log_handle, const char *event_name, int num_args, ... );  // args are const char *value_names
     int log_parse( struct t_engine_log_event_array_entry *event, struct t_engine_log_parser *log_parser, t_se_signal_value **arg_array );
     int log_get_event_interest_desc( struct t_engine_log_interest *log_handle, int n, struct t_engine_log_event_array_entry **event );
     int log_get_event_desc( struct t_engine_log_event_array_entry *event, const char **event_name, t_se_signal_value **value_base, t_engine_text_value_pair **args );

     /*b Public state
      */
     struct t_internal_module_data *module_data; // public as the internal modules chain from this
     c_sl_error *error;
     c_sl_error *message;

     /*b Private methods
      */
private:
     /*b Clock/global creation functions
      */
     struct t_engine_signal *create_global( const char *name, int size );
     struct t_engine_clock *create_clock( const char *name );
     struct t_engine_clock *find_clock( const char *clock_name );

     /*b Mutex functions - in the submodule side as it is used by worklists
      */
     void mutex_claim( t_engine_mutex mutex );
     void mutex_release( t_engine_mutex mutex );

     /*b Private state
      */
     c_engine *next_instance;
     struct t_engine_module_forced_option *module_forced_options;
     struct t_engine_module_instance *module_instance_list;
     struct t_engine_module_instance *toplevel_module_instance_list;
     char *instance_name;

     struct t_engine_signal *global_signal_list;

     struct t_engine_clock *global_clock_list; // Set of all the simulation-driven clocks in the design

     struct t_se_engine_monitor *monitors; // Set of particular signal monitor/callback pairs to be called when a particular signal has changed on any cycle or reset
     struct t_se_engine_simulation_callback *simulation_callbacks; // Set of callbacks to call after signals have changed on any cycle or reset
     t_se_interrogation_handle interrogation_handles; // List of all the interrogation handles that have been mallocked - will be freed on exit (and that is why we chain them)
     struct t_waveform_vcd_file *vcd_files; // List of all the VCD waveform files that are under construction or are open

     struct t_engine_checkpoint *checkpoint_list; // List of checkpoints for the simulation
     struct t_engine_checkpoint *checkpoint_tail; // Tail of list of checkpoints for the simulation

     void *thread_pool; // Used for multithreaded simulations
     struct t_thread_pool_mapping *thread_pool_mapping;

     int cycle_number;

     t_se_signal_value dummy_input_value[16];
};

/*a Global variables
  Many of these are used as we have split out c_engine into many source code files
 */

/*a Functions
 */
extern DLLEXPORT c_engine *engine_from_name( const char *name );
extern DLLEXPORT void se_c_engine_exit( void );
extern DLLEXPORT void se_c_engine_init( void );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

