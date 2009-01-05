/*a Copyright
  
  This file 'c_model_descriptor.h' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_C_MODEL_DESCRIPTOR
#else
#define __INC_C_MODEL_DESCRIPTOR

/*a Includes
 */
#include "c_sl_error.h"
#include "sl_option.h"

/*a Defines
 */
// Note: MD_MAX_SIGNAL_BITS, which is the maximum length of any single signal,
// must be a multiple of bits in t_se_signal_value
#define MD_MAX_SIGNAL_BITS (256)
#define MD_BITS_PER_UINT64 ((int)(sizeof(t_sl_uint64)*8))
#define MD_REFERENCE_ENTRIES_PER_CHUNK (64)
#define MD_OUTPUT_MAX_SIGNAL_ARGS (8)
#define MD_MAX_ERROR_ARGS (4)
#define BE_OPTIONS \
     { "model", required_argument, NULL, option_be_model }, \
     { "cpp", required_argument, NULL, option_be_cpp }, \
     { "verilog", required_argument, NULL, option_be_verilog }, \
     { "vmod", no_argument, NULL, option_be_vmod_mode }, \
     { "v_clkgate_type", required_argument, NULL, option_be_v_clkgate_type }, \
     { "v_clkgate_ports", required_argument, NULL, option_be_v_clkgate_ports }, \
     { "v_comb_suffix", required_argument, NULL, option_be_v_comb_suffix }, \
     { "v_displays", no_argument, NULL, option_be_v_displays }, \
     { "vhdl", required_argument, NULL, option_be_vhdl }, \
     { "include-assertions", no_argument, NULL, option_be_include_assertions }, \
     { "include-coverage", no_argument, NULL, option_be_include_coverage }, \
     { "include-stmt-coverage", no_argument, NULL, option_be_include_stmt_coverage }, \
     { "coverage-desc-file", required_argument, NULL, option_be_coverage_desc_file }, \
     { "remap-module-name", required_argument, NULL, option_be_remap_module_name }, \
     { "remap-instance-type", required_argument, NULL, option_be_remap_instance_type },
#define MD_TYPE_DEFINITION_HANDLE_VALID(a) ( (a).type!=md_type_definition_handle_type_none )

/*a Types
 */
/*t t_md_client_string_type
 */
typedef enum
{
    md_client_string_type_coverage,       // Expect <source filename> <line> <char>
    md_client_string_type_human_readable, // Expect anything for human consumption
} t_md_client_string_type;

/*t t_md_client_string_from_reference_fn
  This is a callback function that produces a textual version of a client reference for output files
  
 */
typedef int (*t_md_client_string_from_reference_fn)( void *handle, void *base_handle, const void *item_handle, int item_reference, char *buffer, int buffer_size, t_md_client_string_type type );

/*t option_be_*
 */
enum
{
    option_be_verilog = option_be_start,
    option_be_vhdl,
    option_be_cpp,
    option_be_model,
    option_be_include_assertions,
    option_be_include_coverage,
    option_be_include_stmt_coverage,
    option_be_coverage_desc_file,
    option_be_remap_module_name,
    option_be_remap_instance_type,
    option_be_vmod_mode,
    option_be_v_clkgate_type,
    option_be_v_clkgate_ports,
    option_be_v_comb_suffix,
    option_be_v_displays
};

/*t t_md_output_fn
  This is a prototype function that is used for output from all of the code output routines
  The handle is as supplied to the code output routine
  The indent is a number from 0 upwards for a level of indentation, or -1 for 'follow on to previous line'
  The format etc is as passed to printf
*/
typedef void (*t_md_output_fn)( void *handle, int indent, const char *format, ... );

/*t md_edge_*
  Enumeration of positive edge and negative edge
*/
enum
{
    md_edge_pos,
    md_edge_neg
};

/*t t_md_client_reference
  A client reference that is supplied by the client of the backend has three items
  A base handle - a void *
  An item handle - a void *
  An item reference integer
  This structure stores these.
  For simple clients these will be NULL and 0.
  For the CDL client, for example, the base_handle is the CDL module reference, the
  item handle is the statement or expression itself, and the item_reference is a handy spare.
*/
typedef struct t_md_client_reference
{
    void *base_handle;
    const void *item_handle;
    int item_reference;
} t_md_client_reference;

/*t t_md_reference_type
  An enumeration used for t_md_reference, i.e. for references to either data, signal or state
*/
typedef enum t_md_reference_type
{
    md_reference_type_state,
    md_reference_type_signal,
    md_reference_type_instance,
    md_reference_type_clock_edge,
    md_reference_type_none
};

/*t t_md_reference
  The structure of a reference - basically an explicitly typed union
*/
typedef struct t_md_reference
{
    t_md_reference_type type;
    union
    {
        void *data;
        struct t_md_state *state;
        struct t_md_signal *signal;
        struct t_md_type_instance *instance;
    } data;
    int edge;
} t_md_reference;

/*t t_md_reference_set
  This structure is used to maintain sets of dependencies for output code ordering
  It is applied to all the state and signals that are generated (internal combinatorial, output combinatorial, state)
  It is not generally useful for the state, as all that code is output after the combinatorials are generated.
  It is also applied to the code blocks (once their statements have been fully analyzed).
  For good design practice (I think...) code blocks should not have mutual dependencies. So we will warn with those.
*/
typedef struct t_md_reference_set
{
    struct t_md_reference_set *next_set;
    int max_entries;
    int number_entries;
    t_md_reference members[1];
} t_md_reference_set;

/*t t_md_reference_iter
  To iterate over a reference set, we provide a function call and this structure which maintains the state of the iterating.
  It is initialized with the set and the entry for the first set and entry 0, and then subsequent calls will change the set
  if required and the entry number.
*/
typedef struct t_md_reference_iter
{
    t_md_reference_set *set;
    int entry;
} t_md_reference_iter;

/*t t_md_signal_value
  A signal in the model descriptor is a true binary value, of size from 1 to MD_MAX_SIGNAL_BITS in length
  To do masked signals (i.e. with 'x'es) you need two values - one a value, one a mask. This is used rarely.
*/
typedef struct t_md_signal_value
{
    int width;
    t_sl_uint64 value[MD_MAX_SIGNAL_BITS/(8*sizeof(t_sl_uint64))];
} t_md_signal_value;

/*t t_md_type_definition_handle_type
 */
typedef enum
{
    md_type_definition_handle_type_bit_vector,
    md_type_definition_handle_type_structure,
    md_type_definition_handle_type_none
} t_md_type_definition_handle_type;

/*t t_md_type_definition_handle
 */
typedef struct t_md_type_definition_handle
{
    t_md_type_definition_handle_type type;
    union
    {
        struct t_md_type_definition *type_def;
        int width;
    } data;
} t_md_type_definition_handle;

/*t t_md_type_definition_type
 */
typedef enum
{
    md_type_definition_type_array_of_bit_vectors,
    md_type_definition_type_array_of_types,
    md_type_definition_type_structure,
} t_md_type_definition_type;

/*t t_md_type_definition_data
 */
typedef union
{
    struct
    {
        int name_to_be_freed;
        char *name;
        t_md_type_definition_handle type;
    } element;
    t_md_type_definition_handle type;
} t_md_type_definition_data;

/*t t_md_type_definition
 */
typedef struct t_md_type_definition
{
    struct t_md_type_definition *next_in_list;
    char *name;
    int name_to_be_freed;
    int size;
    t_md_type_definition_type type;
    t_md_type_definition_data data[1];
} t_md_type_definition;

/*t t_md_type_instance_type
 */
typedef enum
{
    md_type_instance_type_bit_vector,
    md_type_instance_type_array_of_bit_vectors,
    md_type_instance_type_structure,
} t_md_type_instance_type;

/*t t_md_type_instance_data
 */
typedef union t_md_type_instance_data
{
    struct
    {
        char *name_ref; // never an ownership link - points to definition
        struct t_md_type_instance *instance;
    } element;
    struct
    {
        int subscript_start; // If >=0, then indicates
        int subscript_end;
        struct t_md_expression *expression;
        union t_md_type_instance_data *next_in_list;
    } reset_value;
} t_md_type_instance_data;

/*t t_md_type_instance
  a structure has:
  type md_type_instance_type_structure
  size = number of elements
  data[] = name_ref of element, instance of element

  a bit vector has:
  type md_type_instance_type_bit_vector
  type_def = bit vector of width 'x'
  size = 0
  data[0] = reset_value (if state)

  an array of bit vectors has:
  type md_type_instance_type_bit_vector
  type_def = bit vector of width 'x'
  size = size of array
  data[0..size-1] = reset_values (if state)

  an array of structures is not... the array is pushed to the bottom
*/
typedef struct t_md_type_instance
{
    struct t_md_type_instance *next_in_module;
    char *name;
    int name_to_be_freed;

    char *output_name; // always mallocked - the full hierarchy name output when this instance is used - only leaf lvars (i.e. a.b.c not a or a.b) have one
    void *output_handle;
    int output_args[MD_OUTPUT_MAX_SIGNAL_ARGS];

    t_md_type_definition_handle type_def; // bit_vector or structure including the width or pointer to the type definition
    int size;
    t_md_type_instance_type type; // bit vector, array of bit vectors, or structure
    t_md_reference reference; // state, input, output or combinatorial this is part of
    t_md_reference_set *dependents; // all direct dependent instances (signals/states that immediately use this, if state, input or combinatorial)
    t_md_reference_set *dependencies; // all direct dependency instances (signals/states that immediately effect this, if state or combinatorial)
    t_md_reference_set *base_dependencies; // all dependency instances, direct and indirect (superset of dependencies), if state or combinatorial
    struct t_md_code_block *code_block; // code block that a state or combinatorial is defined in
    struct t_md_code_block *code_block_last_used_in; // code block that a combinatorial (or state possibly) was last used in
    int number_statements; // Number of statements the instance is assigned to (state or signal), or number of module instances which drive it (could be many if driven partially)
    int derived_combinatorially; // If a net, then indicates whether it is driven from purely a clock or at least partially from combinatorials through a module
    int driven_in_parts; // 1 if the net is driven by many modules, 0 if it is by one module

    t_md_type_instance_data data[1]; // Must be last item of this structure due to malloc mechanism - the elements of a structure
} t_md_type_instance;

/*t t_md_type_instance_iter
 */
typedef struct t_md_type_instance_iter
{
    int number_children;
    t_md_type_instance *children[1];
} t_md_type_instance_iter;

/*t t_md_usage_type
  An enumeration for how a signal may be used - default is RTL, other options are various validation methods
*/
typedef enum
{
    md_usage_type_rtl,
    md_usage_type_assert,
    md_usage_type_cover
} t_md_usage_type;

/*t t_md_signal_type
  An enumeration for the potential types of non-state variables in the models.
*/
typedef enum t_md_signal_type
{
    md_signal_type_clock,
    md_signal_type_input,
    md_signal_type_output,
    md_signal_type_combinatorial,
    md_signal_type_net,
} t_md_signal_type;

/*t t_md_signal
  The main type for non-state variables (i.e. inputs, outputs, combs, clocks) in the models
  Chained on a standard linked list
  Contains a name, which may or may no be mallocked (indicated by name_to_be_freed)
  The client reference allows to refer back to the full client structure the variable is derived from
  Two output parameters are kept, so that the output routines can (for example) track dependencies using some store per non-state variable.
  Thereafter it is basically an explicity typed union
*/
typedef struct t_md_signal
{
    struct t_md_signal *next_in_list;

    char *name;
    int name_to_be_freed;
    t_md_client_reference client_ref;
    char *documentation; // always freed if not null

    t_md_signal_type type;
    t_md_usage_type usage_type;

    t_md_type_instance *instance;
    t_md_type_instance_iter *instance_iter;

    union
    {
        struct
        {
            int edges_used[2];
            t_md_reference_set *dependents[2];
            t_md_signal *clock_ref; // for gated clocks
            t_md_signal *gate_signal;  // for gated clocks
            t_md_state  *gate_state;  // for gated clocks
            int gate_level;
        } clock;
        struct
        {
            int levels_used_for_reset[2];
            int used_combinatorially; // 1 if used combinatorially to generate an output
            struct t_md_reference_set *clocks_used_on; // set of clock edges the input is used on
        } input;
        struct
        {
            struct t_md_state *register_ref; // NULL if the output is combinatorial or a net, else points to the state that drives it
            struct t_md_signal *combinatorial_ref; // NULL if the output is state-driven or a net, else points to the combinatorial.
            struct t_md_signal *net_ref; // NULL if the output is state-driven or combinatorial, else points to the net description.
            // one of the above two items should be non-NULL, if the output is driven at all.
            int derived_combinatorially; // 1 if derived combinatorially from inputs to generate an output
            struct t_md_reference_set *clocks_derived_from; // set of clock edges the output is derived from
        } output;
        struct
        {
            struct t_md_signal *output_ref; // if the combinatorial is an output, the reference to that definition
        } combinatorial;
        struct
        {
            struct t_md_signal *output_ref; // if the net is an output, the reference to that definition
        } net;
    } data;

} t_md_signal;

/*t t_md_state
 */
typedef struct t_md_state
{
    struct t_md_state *next_in_list;

    char *name;
    int name_to_be_freed;
    char *documentation; // always freed if not null
    t_md_client_reference client_ref;

    t_md_usage_type usage_type;

    t_md_signal *clock_ref;
    int edge;
    t_md_signal *reset_ref;
    int reset_level;
    t_md_type_instance *instance;
    t_md_type_instance_iter *instance_iter;

    t_md_signal *output_ref;
} t_md_state;

/*t t_md_lvar_data_type
 */
typedef enum t_md_lvar_data_type
{
    md_lvar_data_type_none,
    md_lvar_data_type_integer,
    md_lvar_data_type_expression
} t_md_lvar_data_type;

/*t t_md_lvar_data
 */
typedef struct t_md_lvar_data
{
    t_md_lvar_data_type type;
    union
    {
        t_sl_uint64 integer;
        struct t_md_expression *expression;
    } data;
} t_md_lvar_data;

/*t t_md_lvar_instance_type
 */
typedef enum t_md_lvar_instance_type
{
    md_lvar_instance_type_signal,
    md_lvar_instance_type_state,
} t_md_lvar_instance_type;

/*t t_md_lvar_instance_data
 */
typedef union t_md_lvar_instance_data
{
    struct t_md_signal *signal;
    struct t_md_state *state;
} t_md_lvar_instance_data;

/*t t_md_lvar
  This structure is built for accessing data registers, or portions thereof
  For example, this structure shall be used to refer to something like fred.jim[4].joe.data[4;2]
  You would get a chain of structures:
  lvar 'a' for fred: top_ref=NULL, parent_ref=NULL, child_ref='b', defn=fred, instance=fred, index=none, subscript=none
  lvar 'b' for jim[4]: top_ref='a', parent_ref='a', child_ref='c', defn=fred.jim then fred.jim[], instance=fred.jim, index=int(4), subscript=none
  lvar 'c' for joe: top_ref='a', parent_ref='b', child_ref='d', defn=fred.jim[].joe, instance=fred.jim.joe, index=int(4), subscript=none
  lvar 'd' for data[4;2]: top_ref='a', parent_ref='c', child_ref=NULL, defn=fred.jim[].joe.data, instance=fred.jim.joe.data, index=int(4), subscript_start=int(2), subscript_length=int(4)
*/
typedef struct t_md_lvar
{
    struct t_md_lvar *next_in_module; // fully chained, so we can delete with ease
    struct t_md_lvar *prev_in_module;
    struct t_md_lvar *top_ref;   // ptr to lvar that contains the first reference that this is a chain of
    struct t_md_lvar *parent_ref; // ptr to lvar that contains the previous reference that this is a chain of
    struct t_md_lvar *child_ref; // ptr to lvar that contains the next reference that this is a chain of
    t_md_type_definition_handle type;
    t_md_type_instance *instance;
    t_md_lvar_instance_type instance_type;
    t_md_lvar_instance_data instance_data;
    t_md_lvar_data index;
    t_md_lvar_data subscript_start;
    t_md_lvar_data subscript_length;
    struct
    {
        char *uniquified_name;
    } output;
} t_md_lvar;

/*t t_md_port_lvar
  This structure is built for accessing ports of modules, or portions thereof
  It is similar to an lvar, but it cannot be resolved at instantiation, it has to be resolved at cross referencing
  For example, this structure shall be used to refer to something like fred.john
  You would get a chain of structures:
  lvar 'a' for fred: top_ref=NULL, parent_ref=NULL, child_ref='b', name='fred'
  lvar 'b' for john: top_ref='a', parent_ref='a', child_ref=NULL, name='john'
*/
typedef struct t_md_port_lvar
{
    struct t_md_port_lvar *next_in_module; // fully chained, so we can delete with ease
    struct t_md_port_lvar *prev_in_module;
    struct t_md_port_lvar *top_ref;   // ptr to lvar that contains the first reference that this is a chain of
    struct t_md_port_lvar *parent_ref; // ptr to lvar that contains the previous reference that this is a chain of
    struct t_md_port_lvar *child_ref; // ptr to lvar that contains the next reference that this is a chain of

    char *name; // Name of the port, or element of the port, this refers to
    int name_to_be_freed;

    t_md_type_instance *port_instance; // filled in at cross referencing, only for leaf lvars
//    t_md_lvar_data index;
//    t_md_lvar_data subscript_start;
//    t_md_lvar_data subscript_length;
} t_md_port_lvar;

/*t t_md_expr_type
 */
typedef enum t_md_expr_type
{
    md_expr_type_value,
    md_expr_type_lvar,
    md_expr_type_cast,
    md_expr_type_bundle,
    md_expr_type_fn,
    md_expr_type_none
};

/*t t_md_expr_fn
  add lsl, lsr, query
*/
typedef enum t_md_expr_fn
{
    md_expr_fn_neg,
    md_expr_fn_add,
    md_expr_fn_sub,
    md_expr_fn_mult,
    md_expr_fn_not,
    md_expr_fn_and,
    md_expr_fn_or,
    md_expr_fn_bic,
    md_expr_fn_xor,
    md_expr_fn_eq,
    md_expr_fn_neq,
    md_expr_fn_ge,
    md_expr_fn_gt,
    md_expr_fn_le,
    md_expr_fn_lt,
    md_expr_fn_logical_not,
    md_expr_fn_logical_and,
    md_expr_fn_logical_or,
    md_expr_fn_query,
    md_expr_fn_none
};

/*t t_md_expression
 */
typedef struct t_md_expression
{
    struct t_md_expression *next_in_list;
    t_md_expr_type type;
    int width;
    union
    {
        struct {
            t_md_signal_value value;
        } value;
        struct t_md_lvar *lvar;
        struct {
            t_md_expr_fn fn;
            struct t_md_expression *args[3];
        } fn;
        struct {
            t_md_expression *expression;
            int bottom;
        } static_subscript;
        struct {
            t_md_expression *expression;
            t_md_expression *bottom; // guaranteed to be a single bit for now - else its a run-time width
        } dynamic_subscript;
        struct {
            int signed_cast;
            t_md_expression *expression;
        } cast;
        struct {
            t_md_expression *a; // top bits
            t_md_expression *b; // bottom bits
        } bundle;
    } data;
    struct t_md_code_block *code_block; // Code block the expression is in - used in VMOD output
    struct t_md_expression *next_in_chain; // in chain of expressions for example for assertions in an expression list
} t_md_expression;

/*t t_md_switch_item_type
  Enumeration for a switch item entry
  It can be dynamic (for a dynamic parallel switch or a dynamic priority list)
  It can be static (for a parallel switch)
  It can be static and masked (for a parallel switch or a static priority list)
  It can be the default (for anything)
*/
typedef enum
{
    md_switch_item_type_dynamic,
    md_switch_item_type_static,
    md_switch_item_type_static_masked,
    md_switch_item_type_default
} t_md_switch_item_type;

/*t t_md_switch_item
  Type for the switch items
  They are owned by the module ownership links
  They are linked also within the statement
*/
typedef struct t_md_switch_item
{
    struct t_md_switch_item *next_in_module;
    struct t_md_switch_item *next_in_list;

    t_md_client_reference client_ref;

    t_md_switch_item_type type;

    union
    {
        t_md_expression *expr;
        struct
        {
            t_md_signal_value value;
            t_md_signal_value mask;
        } value;
    } data;
    struct t_md_statement *statement;
} t_md_switch_item;

/*t t_md_statement_type
  Enumeration of possible statements
*/
typedef enum t_md_statement_type
{
    md_statement_type_if_else,
    md_statement_type_state_assign,
    md_statement_type_comb_assign,
    md_statement_type_parallel_switch, // dynamic switch expression, case expressions (possibly with masks) which MUST NOT overlap
    md_statement_type_priority_list, // dynamic switch expression, case expressions (some with masks) which may overlap
    md_statement_type_print_assert,
    md_statement_type_cover,
    md_statement_type_log,
    md_statement_type_none
} t_md_statement_type;

/*t t_md_statement
 */
typedef struct t_md_statement
{
    struct t_md_statement *next_in_module; // Ownership link, for freeing purposes; all statements are on a single chain
    struct t_md_statement *next_in_code; // Next statement at this level; not an ownership link

    t_md_client_reference client_ref;

    t_md_statement_type type;
    union
    {
        struct {
            t_md_lvar *lvar;
            t_md_expression *expr;
            char *documentation;
        } state_assign;
        struct {
            t_md_lvar *lvar;
            t_md_expression *expr;
            char *documentation;
        } comb_assign;
        struct {
            t_md_expression *expr;
            char *expr_documentation;
            struct t_md_statement *if_true;
            struct t_md_statement *if_false;
        } if_else;
        struct {
            t_md_expression *expr;
            char *expr_documentation;
            int parallel; // 1 if the items may never occur simultaneously (for static nonnmasked items for example, this is 1) - if 0, then the priority is first-to-last of the case statements and a priority if statement should be built.
            int full; // 1 if the items are supposed to cover all possible (occurring/valid) cases of the expression - can be checked for with assertions
            int all_static; // 1 if the items are all static
            int all_unmasked; // 1 if the items are without a mask
            t_md_switch_item *items; // list of items in the switch statement
        } switch_stmt;
        struct { // A print statement has expression NULL, value_list NULL, statement NULL - just a message, in other words; All statements require EITHER clock OR statement
            struct t_md_signal *clock_ref; // Clock edge to print the message on, if this statement is reached then
            int edge;
            struct t_md_expression *expression; // Assertion value (print if FALSE)
            struct t_md_expression *value_list; // Values to match the assertion to
            struct t_md_message *message; // Message to print when this statement is reached on this clock edge
            struct t_md_statement *statement; // Code to run if assertion fails, or if cover statement hits
            int cover_case_entry; // For cover statements that catch individual events
        } print_assert_cover;
        struct {
            struct t_md_signal *clock_ref; // Clock edge to print the message on, if this statement is reached then
            int edge;
            const char *message;
            struct t_md_labelled_expression *arguments;
            int id_within_module; // Used for output, created by md_target_*
            int arg_id_within_module; // Used for output, created by md_target_*
        } log;
    } data;
    t_md_reference_set *dependencies;
    t_md_reference_set *effects;
    struct t_md_code_block *code_block; // Code block the statement is in - used in VMOD output
    int enumeration; // A number starting at zero, which counts the statements in a module - used for statement coverage
    struct
    {
        int unique_id;
    } output;
} t_md_statement;

/*t t_md_module_instance_clock_port
 */
typedef struct t_md_module_instance_clock_port
{
    struct t_md_module_instance_clock_port *next_in_list;
    char *port_name; // Name of the port on the module; should match a clock signal on the module definition
    char *clock_name;
    t_md_signal *module_port_signal; // filled in at cross referencing; points to actual signal on module's 'outputs' list
    t_md_signal *local_clock_signal; // filled in at cross referencing; points to actual local clock signal driving instance's clock port
} t_md_module_instance_clock_port;

/*t t_md_module_instance_input_port
 */
typedef struct t_md_module_instance_input_port
{
    struct t_md_module_instance_input_port *next_in_list;
    struct t_md_port_lvar *port_lvar; // Port lvar for the port on the module; should match a single instance of an input signal on the module definition
    struct t_md_expression *expression; // expression to tie the input to
    struct t_md_type_instance *module_port_instance; // filled in at cross referencing; points to actual instance of module's input
} t_md_module_instance_input_port;

/*t t_md_module_instance_output_port
 */
typedef struct t_md_module_instance_output_port
{
    struct t_md_module_instance_output_port *next_in_list;
    struct t_md_port_lvar *port_lvar; // Port lvar for the port on the module; should match a single instance of an output signal on the module definition
    struct t_md_lvar *lvar; // Lvar to drive with the output signal; should have constant indices in the lvars
    struct t_md_type_instance *module_port_instance; // filled in at cross referencing; points to actual instance of module's output
} t_md_module_instance_output_port;

/*t t_md_module_instance
 */
typedef struct t_md_module_instance
{
    struct t_md_module_instance *next_in_list;
    struct t_md_module *module_definition; // Filled in at analysis

    t_md_client_reference client_ref;

    int output_args[MD_OUTPUT_MAX_SIGNAL_ARGS];

    char *name; // Textual name of the instance - should be unique within the module
    int name_to_be_freed;
    char *type; // Textual type of the module - should match the definition
    int type_to_be_freed;
    const char *output_type; // Textual type of the module for output

    t_md_module_instance_clock_port *clocks; // List of clocks - must be name/name pairs
    t_md_module_instance_input_port *inputs; // List of inputs - must be name/expression pairs
    t_md_module_instance_output_port *outputs; // List of outputs - must be name/lvar pairs with constant indices in the lvars

    t_md_reference_set *combinatorial_dependencies; // set of all instances that this instantiation explicitly depends combinatorially on
    t_md_reference_set *dependencies; // set of all instances that this instantiation explicitly depends on
    t_md_reference_set *effects; // set of all instances (for code) that this code block effects

} t_md_module_instance;

/*t t_md_code_block
 */
typedef struct t_md_code_block
{
    struct t_md_code_block *next_in_list;

    char *name;
    int name_to_be_freed;
    char *documentation; // always freed if not null
    t_md_client_reference client_ref;

    struct t_md_module *module;
    t_md_statement *first_statement;
    t_md_statement *last_statement; // used to chain on new statements

    t_md_reference_set *dependencies; // set of all instances that this code block explicitly depends on
    t_md_reference_set *effects; // set of all instances (for code) AND clock edges (for assertions) that this code block effects
} t_md_code_block;

/*t t_md_message
 */
typedef struct t_md_message
{
    struct t_md_message *next_in_list;
    t_md_client_reference client_ref;
    char *text;
    t_md_expression *arguments; // Integer arguments
    int text_to_be_freed;
} t_md_message;

/*t t_md_labelled_expression
 */
typedef struct t_md_labelled_expression
{
    struct t_md_labelled_expression *next_in_list;
    t_md_client_reference client_ref;
    char *text;
    t_md_expression *expression;
    int text_to_be_freed;
    struct t_md_labelled_expression *next_in_chain;
} t_md_labelled_expression;

/*t t_md_module
 */
typedef struct t_md_module
{
    struct t_md_module *next_in_list; // ownership chain, in order declared by the client
    struct t_md_module *next_in_hierarchy; // bottom-up hierarchy, not ownership link, first is the lowest leaf, last is the toplevel

    char *name;
    int name_to_be_freed;
    char *documentation; // always freed if not null
    const char *output_name; // never freed - points in to option, or points to name
    t_md_client_reference client_ref;

    int external; // 1 if the module is not to be analyzed; its input and output dependencies are given explicitly, and no code is expected to be generated 
    int analyzed; // 1 after the module has been successfully analyzed
    int combinatorial_component; // 1 after analysis if any output dependents combinatorially on any input
    t_md_signal *clocks; // list of clocks used in the model; if non-null, then the component is at least partially synchronous
    t_md_signal *inputs;
    t_md_signal *outputs;
    t_md_state *registers; // List of all internal state (possibly tied to outputs) that are assignable by synchronous assignments
    t_md_signal *combinatorials; // List of all internal signals (possibly tied to outputs) that are declared combinatorial; that is, they are assigned by combinatorial assignments
    t_md_signal *nets; // List of all internal signals (possibly tied to outputs) that are declared as nets; that is, they are driven by submodules
    t_md_code_block *code_blocks;
    t_md_code_block *last_code_block; // used to chain on new code blocks
    t_md_module_instance *module_instances;
    t_md_module_instance *last_module_instance; // used to chain on new instances
    t_md_message *messages; // Ownership list of messages used in module
    t_md_labelled_expression *labelled_expressions; // Ownership list of labelled expressions used in module
    t_md_expression *expressions; // used as ownership list, for freeing purposes
    t_md_statement *statements; // used as ownership list, for freeing purposes
    t_md_switch_item *switch_items; // used as ownership list, for freeing purposes
    t_md_type_instance *instances; // used as ownership list, for freeing purposes
    t_md_lvar *lvars; // used as ownership list, for freeing purposes
    t_md_port_lvar *port_lvars; // used as ownership list, for freeing purposes
    t_md_reference_set *module_types_instantiated; // filled in at hierarchy determination time, with module types that this module instantiates
    int last_statement_enumeration;
    int next_cover_case_entry; // For cover case counting
    struct
    {
        int unique_id;
        int total_log_args;
    } output;
} t_md_module;

/*a c_model_descriptor class
 */
/*t c_model_descriptor
 */
class c_model_descriptor
{
public:
    /*b Constructors/destructors
     */
    c_model_descriptor( t_sl_option_list env_options, c_sl_error *error, t_md_client_string_from_reference_fn string_from_reference_fn, void *client_handle );
    ~c_model_descriptor();

    /*b Inline model/module interrogation functions
     */
    char *get_name( void );
    t_md_module *get_toplevel_module( void );

    /*b Model handling
     */
    int display_references( t_md_output_fn output_fn, void *output_handle  );
    int display_instances( t_md_output_fn output_fn, void *output_handle  );
    int analyze( void );

    /*b Module handling
     */
    t_md_module *module_create( const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *documentation );
    t_md_module *module_create_prototype( const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference );
    t_md_module *module_find( const char *name );
    int module_cross_reference_instantiations( t_md_module *module );
    void push_possible_indices_to_subscripts( t_md_lvar *lvar );
    void push_possible_indices_to_subscripts( t_md_expression *expr );
    void module_analyze_invert_dependency_list( t_md_type_instance_iter *instance_iter, t_md_state *state );
    void module_analyze_dependents( t_md_reference_set **dependents_ptr, void *data, int edge );
    void module_analyze_dependents_of_input( t_md_signal *input );
    void module_analyze_dependents_of_clock_edge( t_md_module *module, t_md_signal *clock, int edge );
    void module_analyze_generate_output_names(  t_md_type_instance_iter *instance_iter );
    int module_analyze( t_md_module *module );
    void module_display_references_instances( t_md_module *module, t_md_output_fn output_fn, void *output_handle, t_md_type_instance_iter *instance_iter, int what );
    int module_display_references( t_md_module *module, t_md_output_fn output_fn, void *output_handle  );
    int module_display_instances( t_md_module *module, t_md_output_fn output_fn, void *output_handle  );
    void modules_free( void );

    /*b Reference set handling
     */
    void reference_set_free( t_md_reference_set **set_ptr );
    void reference_set_add( t_md_reference_set **set_ptr );
    int reference_set_find_member( t_md_reference_set *set, void *data, int edge );
    int reference_internal_add( t_md_reference_set **set_ptr, t_md_reference_type type, void *data, int edge );
    int reference_add( t_md_reference_set **set_ptr, t_md_type_instance *instance );
    int reference_add( t_md_reference_set **set_ptr, t_md_signal *signal );
    int reference_add( t_md_reference_set **set_ptr, t_md_state *state );
    int reference_add( t_md_reference_set **set_ptr, t_md_signal *clock, int edge );
    int reference_add_data( t_md_reference_set **set_ptr, void *data );
    int reference_union_sets( t_md_reference_set *to_add, t_md_reference_set **set_ptr );
    int reference_intersect_sets( t_md_reference_set *to_add, t_md_reference_set **set_ptr );
    void reference_set_iterate_start( t_md_reference_set **set_ptr, t_md_reference_iter *iter );
    t_md_reference *reference_set_iterate( t_md_reference_iter *iter );
    const char *reference_text( t_md_reference *reference );
    int reference_set_includes( t_md_reference_set **set_ptr, t_md_signal *clk, int edge );
    int reference_set_includes( t_md_reference_set **set_ptr, t_md_signal *clk, int edge, t_md_signal *reset, int reset_level );
    int reference_set_includes( t_md_reference_set **set_ptr, t_md_type_instance *instance );
    int reference_set_includes( t_md_reference_set **set_ptr, t_md_reference_type type );

    /*b Type definition handling
     */
    t_md_type_definition_handle type_definition_find( const char *name );
    void type_definition_set_parent( t_md_type_definition_handle child, t_md_type_definition *parent );
    t_md_type_definition *type_definition_create( const char *name, int copy_name, t_md_type_definition_type type, int size, int number_elements );
    t_md_type_definition_handle type_definition_bit_vector_create( int width );
    t_md_type_definition_handle type_definition_array_create( const char *name, int copy_name, int size, t_md_type_definition_handle type );
    t_md_type_definition_handle type_definition_structure_create( const char *name, int copy_name, int number_elements );
    int type_definition_structure_set_element( t_md_type_definition_handle structure, const char *name, int copy_name, int element, t_md_type_definition_handle type );
    void type_definition_display( t_md_output_fn output_fn, void *output_handle  );

    /*b Type instantiation handling (within module)
     */
    t_md_type_instance *type_instance_create( t_md_module *module, t_md_reference *reference, const char *name, const char *sub_name, t_md_type_definition_handle type_def, t_md_type_instance_type type, int size );
    t_md_type_instance *type_instantiate( t_md_module *module, t_md_reference *reference, int array_size, t_md_type_definition_handle type_def, const char *name, const char *sub_name  );
    void type_instance_free( t_md_type_instance *instance );
    int type_instance_count_and_populate_children( t_md_type_instance *instance, t_md_type_instance_iter *iter, int number_children );
    t_md_type_instance_iter *type_instance_iterate_create( t_md_type_instance *instance );
    void type_instance_iterate_free( t_md_type_instance_iter *iter );

    /*b Lvar handling (within module)
     */
    t_md_lvar *lvar_create( t_md_module *module, t_md_lvar *parent, t_md_type_definition_handle type ); // Create an lvar, from a parent lvar, of the given type
    t_md_lvar *lvar_duplicate( t_md_module *module, t_md_lvar *lvar ); // Copy the lvar (from its root downwards)
    t_md_lvar *lvar_reference( t_md_module *module, t_md_lvar *chain, const char *name ); // Create a new lvar reference that is a sub-lvar of the given chain - if no chain, then start from the state and signals
    t_md_lvar *lvar_index( t_md_module *module, t_md_lvar *lvar, t_md_lvar_data *data );
    t_md_lvar *lvar_index( t_md_module *module, t_md_lvar *lvar, int index );
    t_md_lvar *lvar_index( t_md_module *module, t_md_lvar *lvar, t_md_expression *index );
    t_md_lvar *lvar_bit_select( t_md_module *module, t_md_lvar *lvar, t_md_lvar_data *data );
    t_md_lvar *lvar_bit_select( t_md_module *module, t_md_lvar *lvar, int bit_select );
    t_md_lvar *lvar_bit_select( t_md_module *module, t_md_lvar *lvar, t_md_expression *bit_select );
    t_md_lvar *lvar_bit_range_select( t_md_module *module, t_md_lvar *lvar, t_md_expression *bit_select, int length );
    t_md_lvar *lvar_from_string( t_md_module *module, t_md_lvar *parent, const char *string );
    int lvar_is_terminal( t_md_lvar *lvar );
    int lvar_width( t_md_lvar *lvar );
    void lvar_free( t_md_module *module, t_md_lvar *lvar ); // Completely remove an lvar from the module, going to its root to do so
    void lvars_free( t_md_module *module );

    /*b Port lvar handling (within module)
     */
    t_md_port_lvar *port_lvar_create( t_md_module *module, t_md_port_lvar *parent );
    t_md_port_lvar *port_lvar_duplicate( t_md_module *module, t_md_port_lvar *port_lvar );
    t_md_port_lvar *port_lvar_reference( t_md_module *module, t_md_port_lvar *chain, const char *name );
    t_md_type_instance *port_lvar_resolve( t_md_port_lvar *port_lvar, t_md_signal *signals );
    void port_lvar_free( t_md_module *module, t_md_port_lvar *port_lvar ); // Completely remove an port_lvar from the module, going to its root to do so
    void port_lvars_free( t_md_module *module );

    /*b Signal handling (within module)
      Clocks, inputs and outputs can be added to a protoype; others cannot
      All can be added to a module definition
      Inputs for a prototype can be made to be required by a clock edge, or be combinatorial
      Outputs for a prototype can be made to be generated combinatorially, or off any number of clock edges
     */
    t_md_signal *signal_internal_create( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int array_size, t_md_type_definition_handle type, t_md_signal **list, t_md_signal_type signal_type, t_md_usage_type usage_type );
    t_md_signal *signal_find( const char *name, t_md_signal *list );
    int signal_add_clock( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference );
    int signal_gate_clock( t_md_module *module, const char *name, const char *clock_to_gate, const char *gate, int gate_level, void *client_base_handle, const void *client_item_handle, int client_item_reference );
    int signal_add_input( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width );
    int signal_add_input( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type );
    int signal_add_output( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width );
    int signal_add_output( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type );
    int signal_add_combinatorial( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width, t_md_usage_type usage_type );
    int signal_add_combinatorial( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type, t_md_usage_type usage_type );
    int signal_add_net( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width );
    int signal_add_net( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type );
    int signal_document( t_md_module *module, const char *name, const char *documentation );
    int signal_input_used_combinatorially( t_md_module *module, const char *name );
    int signal_input_used_on_clock( t_md_module *module, const char *name, const char *clock_name, int edge );
    int signal_output_derived_combinatorially( t_md_module *module, const char *name );
    int signal_output_generated_from_clock( t_md_module *module, const char *name, const char *clock_name, int edge );
    void signals_free( t_md_signal *list );

    /*b State handling (within module)
     */
    t_md_state *state_create( t_md_module *module, const char *state_name, int copy_state_name, t_md_client_reference *client_ref, const char *clock_name, int edge, const char *reset_name, int reset_level, t_md_signal *clocks, t_md_signal *signals, t_md_state **list );
    t_md_state *state_find( const char *name, t_md_state *list );
    void state_add_reset_value( t_md_type_instance_data *reset_value_id, int subscript_start, int subscript_end, t_md_expression * expression );
    t_md_state *state_internal_add( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int array_size, t_md_type_definition_handle type, const char *clock, int edge, const char *reset, int level, t_md_usage_type usage_type );
    int state_add( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width, t_md_usage_type usage_type, const char *clock, int edge, const char *reset, int level, t_md_signal_value *reset_value );
    int state_add( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int array_size, t_md_type_definition_handle type, t_md_usage_type usage_type, const char *clock, int edge, const char *reset, int level );
    int state_reset( t_md_module *module, const char *name, t_md_signal_value *reset_value );
    int state_reset( t_md_module *module, t_md_lvar *lvar, t_md_expression *reset_value );
    int state_document( t_md_module *module, const char *name, const char *documentation );
    void states_free( t_md_state *list );

    /*b Expressions
     */
    t_md_expression *expression_create( t_md_module *module, t_md_expr_type type );
    t_md_expression *expression_chain( t_md_expression *chain, t_md_expression *new_element );
    t_md_expression *expression( t_md_module *module, int width, t_md_signal_value *value );
    t_md_expression *expression( t_md_module *module, const char *reference);
    t_md_expression *expression( t_md_module *module, t_md_expression *expression, int top, int bottom);
    t_md_expression *expression_subscript( t_md_module *module, t_md_expression *expression, t_md_expression *bottom );
    t_md_expression *expression_cast( t_md_module *module, int to_width, int signed_cast, t_md_expression *expression );
    t_md_expression *expression_bundle( t_md_module *module, t_md_expression *a, t_md_expression *b );
    t_md_expression *expression( t_md_module *module, t_md_lvar *lvar );
    t_md_expression *expression( t_md_module *module, t_md_expr_fn fn, t_md_expression *args_0, t_md_expression *args_1, t_md_expression *args_2 );
    int expression_find_dependencies( t_md_module *module, t_md_code_block *code_block, t_md_reference_set **set_ptr, t_md_usage_type usage_type, t_md_expression *expr );
    void expression_check_liveness( t_md_module *module, t_md_code_block *code_block, t_md_expression *expr, t_md_reference_set *parent_makes_live, t_md_reference_set *being_made_live );
    void expressions_free( t_md_expression *list );

    /*b Switch items
     */
    t_md_switch_item *switch_item_create( t_md_module *module, t_md_client_reference *client_ref, t_md_switch_item_type type );
    t_md_switch_item *switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_signal_value *value, t_md_statement *statement );
    t_md_switch_item *switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_signal_value *value, t_md_signal_value *mask, t_md_statement *statement );
    t_md_switch_item *switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_expression *expr, t_md_statement *statement );
    t_md_switch_item *switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_statement *statement );
    t_md_switch_item *switch_item_chain( t_md_switch_item *list, t_md_switch_item *item );

    /*b Statements
     */
    t_md_statement *statement_create( t_md_module *module, t_md_client_reference *client_ref, t_md_statement_type type );
    t_md_statement *statement_create_state_assignment( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_lvar *lvar, t_md_expression *expr, const char *documentation );
    t_md_statement *statement_create_combinatorial_assignment( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_lvar *lvar, t_md_expression *expr, const char *documentation );
    t_md_statement *statement_create_if_else( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_expression *expr, t_md_statement *if_true, t_md_statement *if_false, const char *expr_documentation );
    t_md_statement *statement_create_static_switch( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, int parallel, int full, t_md_expression *expr, t_md_switch_item *items, const char *expr_documentation );
    t_md_statement *statement_create_assert_cover( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *clock_name, int edge, t_md_usage_type usage_type, t_md_expression *expr, t_md_expression *value_list, t_md_message *message, t_md_statement *stmt );
    t_md_statement *statement_create_log( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *clock_name, int edge, const char *message, t_md_labelled_expression *values );
    t_md_statement *statement_add_to_chain( t_md_statement *first_statement, t_md_statement *new_statement );
    void statement_add_effects( t_md_statement *chain, t_md_reference_set **effects_set_ptr );
    int statement_analyze( t_md_code_block *code_block, t_md_usage_type usage_type, t_md_statement *statement, t_md_reference_set **predicate_ptr );
    void statement_analyze_liveness( t_md_code_block *code_block, t_md_usage_type usage_type, t_md_statement *statement, t_md_reference_set *parent_makes_live, t_md_reference_set **makes_live );
    void statements_free( t_md_statement *list );

    /*b Code_Blocks
     */
    t_md_code_block *code_block_create( t_md_module *module, const char *name, int copy_name, t_md_client_reference *client_ref, const char *documentation );
    t_md_code_block *code_block_find( const char *name, t_md_code_block *list );
    int code_block( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *documentation );
    int code_block_add_statement( t_md_module *module, const char *code_block_name, t_md_statement *statement );
    int code_block_analyze( t_md_code_block *code_block, t_md_usage_type usage_type );
    void code_blocks_free( t_md_code_block *list );

    /*b Module instances and their ports
     */
    t_md_module_instance *module_instance_create( t_md_module *module, const char *type, int copy_type, const char *name, int copy_name, t_md_client_reference *client_ref );
    t_md_module_instance *module_instance_find( const char *name, t_md_module_instance *list );
    int module_instance( t_md_module *module, const char *name, int copy_name, const char *type, int copy_type, void *client_base_handle, const void *client_item_handle, int client_item_reference );
    int module_instance_add_clock( t_md_module *module, const char *instance_name, const char *port_name, const char *clock_name );
    int module_instance_add_input( t_md_module *module, const char *instance_name, t_md_port_lvar *port_lvar, t_md_expression *expression );
    int module_instance_add_output( t_md_module *module, const char *instance_name, t_md_port_lvar *port_lvar, t_md_lvar *lvar );
    int module_instance_cross_reference( t_md_module *module, t_md_module_instance *module_instance );
    void module_instance_analyze( t_md_module *module, t_md_module_instance *module_instance );

    /*b Messages
     */
    t_md_message *message_create( t_md_module *module, const char *text, int copy_text, t_md_client_reference *client_ref );
    t_md_message *message_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *text, int copy_text );
    t_md_message *message_add_argument( t_md_message *message, t_md_expression *expression );
    void messages_free( t_md_message *list );

    /*b Labelled expressions - text + expression
     */
    t_md_labelled_expression *labelled_expression_create( t_md_module *module, const char *text, int copy_text, t_md_client_reference *client_ref, t_md_expression *expression );
    t_md_labelled_expression *labelled_expression_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *text, int copy_text, t_md_expression *expression );
    void labelled_expression_append( t_md_labelled_expression **labelled_expression_list, t_md_labelled_expression *labelled_expression );
    void labelled_expressions_free( t_md_labelled_expression *list );

    /*b Output generation methods
     */
    void output_coverage_map( const char *filename );
    void generate_output( t_sl_option_list env_options );

    /*b Public data structures
     */
    class c_sl_error *error;
    t_md_module *module_list; // Ownership chain of modules

    t_md_client_string_from_reference_fn client_string_from_reference_fn;
    void *client_handle;

    /*b Private data structures
     */
private:
    char *name;
    t_md_module *module_hierarchy_list; // List in order of leaf first, toplevel last; not ownership links
    t_md_type_definition *types;
    int statement_enumerator; // number starting at zero, used to enumerate the statements during analysis
};

/*a External variables
 */
extern t_md_signal_value md_signal_value_zero;
extern t_md_signal_value md_signal_value_one;

/*a External functions
 */
extern void be_getopt_usage( void );
extern int be_handle_getopt( t_sl_option_list *env_options, int c, const char *optarg );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

