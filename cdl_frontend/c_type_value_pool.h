/*a Copyright
  
  This file 'c_type_value_pool.h' copyright Gavin J Stark 2003, 2004
  
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
#ifndef INC_C_TYPE_VALUE_POOL
#define INC_C_TYPE_VALUE_POOL

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"
#include "c_model_descriptor.h"

/*a Types
 */
/*t type_value and type_value_* are in lexical_types.h
 */
/*t t_type_value_pool_entry_type
 */
typedef enum
{
    type_value_pool_entry_type_struct,
    type_value_pool_entry_type_type_def,
    type_value_pool_entry_type_type_array,
    type_value_pool_entry_type_type_reference,
    type_value_pool_entry_type_variable,
    type_value_pool_entry_type_unresolved_variable,
    type_value_pool_entry_type_resolved_variable,
    type_value_pool_entry_type_runtime_variable,
} t_type_value_pool_entry_type;

/*t t_type_value_pool_named_type
 */
typedef struct t_type_value_pool_named_type
{
    t_symbol *symbol;
    t_type_value type_value;
} t_type_value_pool_named_type;

/*t t_type_value_pool_entry_array
 */
typedef struct t_type_value_pool_entry_array
{
    int resolved_size; // -1 for unresolved, 0 or bigger for resolved
    class c_co_expression *unresolved_start; // for run-time array sizes
    class c_co_expression *unresolved_end; // for run-time array sizes (if not a sized array)
    t_type_value type_value;
} t_type_value_pool_entry_array;

/*t t_type_value_bit_vector_data
 */
typedef t_sl_uint64 t_type_value_bit_vector_data;

/*t t_type_value_pool_entry
 */
typedef struct t_type_value_pool_entry
{
    t_type_value next_in_list;
    t_type_value next_in_hash_list;
    t_type_value_pool_entry_type type_value_pool_entry_type;
    int strong_type; // 0 if this is a readily coercible type/variable, 1 if it is not
    // An fsm state is strong, as it is not implicitly coercible for arithmetic, logical or assignment operations
    // An integer is weak, as it is implicitly coercible to any bit array value (in range)

    int hash;

    t_type_value type_reference; // type of a variable
    t_md_type_definition_handle model_type;
    union
    {
        t_type_value_pool_entry_array type_array;
        t_type_value_pool_named_type type_def;
        struct
        {
            t_symbol *symbol;
            int number_elements;
            t_type_value_pool_named_type elements[1];
        } type_structure;
        t_sl_sint64 value_integer; // Integers are signed 64 bit quantities
        t_type_value_bit_vector_data value_bool;
        struct
        {
            int size;
            //int bit_value_index; // actually int offset into the type_value_pool of t_type_value_bit_vector_data bit_value data
            //int bit_mask_index; // actually int offset into the type_value_pool, or -1 for none (mask is zero)
            int mask_offset; // -1 for no mask, else the number of t_type_value_bit_vector_data's to the mask, within the contents.
            t_type_value_bit_vector_data contents[1]; // Contents 
        } value_bit_array; // for bit_array
        struct
        {
            t_type_value_bit_vector_data value;
            t_type_value_bit_vector_data mask;
        } value_bit; // for bit_array_size_1
        struct
        {
            int length;
            char string[1];
        } value_string;
    } data;
} t_type_value_pool_entry;

/*t c_type_value_pool
 */
class c_type_value_pool
{
public:
    /*b Constructor/destructor
     */
    c_type_value_pool( class c_cyclicity *cyclicity );
    ~c_type_value_pool();

    /*b Type comparision/coercion functions
     */
    int compare_types( t_type_value type_value, class c_co_type_specifier *type_specifier );
     
    /*b Type finding and addition functions
     */
    t_type_value find_type( class c_co_type_specifier *type_specifier );
    t_type_value add_type( t_co_scope *types, class c_co_type_specifier *type_specifier ); // 
    t_type_value add_type( t_co_scope *types, t_symbol *symbol, class c_co_type_specifier *type_specifier ); // typedef fred <type>
    t_type_value add_type( t_co_scope *types, t_symbol *symbol, class c_co_type_struct *type_struct ); // struct <list of named type specifiers>
    t_type_value add_type( t_co_scope *types, t_symbol *symbol, t_type_value type_value ); // FSMs, enums - named bit arrays
    t_type_value add_array( t_type_value sub_index, int size ); // Get type_value of an array of 'size' elements of type 'type'
    t_type_value add_structure( t_symbol *symbol, int number_elements );
    t_type_value add_typedef( t_symbol *symbol, t_type_value type_value );
    t_type_value add_bit_array_type( int size ); // Get type_value of an array of 'size' bits
    int compare_types( c_co_type_specifier *cots_a, c_co_type_specifier *cots_b );

    /*b Type interrogation functions
     */
    t_type_value derefs_to( t_type_value type_value );   // Returns the type_value that a type or value dereferences to - the base structure, array or bool/bit vector/integer
    int is_bool( t_type_value type_value );              // Returns 1 if the type_value is a boolean, (not a reference), or if it is a value instance of one
    int derefs_to_bool( t_type_value type_value );       // Returns 1 if the type_value is a boolean or dereferences down to one, or if it is a value instance of one
    int is_bit( t_type_value type_value );               // Returns 1 if the type_value is a bit vector of size 1, (not a reference), or if it is a value instance of one
    int derefs_to_bit( t_type_value type_value );        // Returns 1 if the type_value is a bit vector of size 1 or dereferences down to one (e.g. type reference), or if it is a value instance of one
    int is_integer( t_type_value type_value );           // Returns 1 if the type_value is an integer type directly, not a reference, or if it is a value instance of one
    int derefs_to_integer( t_type_value type_value );    // Returns 1 if the type_value is an integer type or dereferences down to one (e.g. type reference), or if it is a value instance of one
    int is_bit_vector( t_type_value type_value );        // Returns 1 if the type_value is a bit vector type (not a reference), or if it is a value instance of one
    int derefs_to_bit_vector( t_type_value type_value ); // Returns 1 if the type_value is a bit vector type or dereferences down to one (e.g. enum, FSM, type reference), or if it is a value instance of one
    int is_array( t_type_value type_value );
    int derefs_to_array( t_type_value type_value );
    t_type_value array_type( t_type_value type_value );
    int array_size( t_type_value type_value );
    int is_structure( t_type_value type_value );
    int derefs_to_structure( t_type_value type_value );
    int find_structure_element( t_type_value structure, t_symbol *element );
    t_type_value get_structure_element_type( t_type_value structure, int element );
    int get_structure_element_number( t_type_value structure );
    t_symbol *get_structure_element_name( t_type_value structure, int element );

    /*b Value creation functions
     */
    t_type_value new_type_value( int size, t_type_value type_value );
    t_type_value new_type_value_type( t_type_value type_value );
    t_type_value new_type_value_bool( int boolean );
    t_type_value new_type_value_integer( t_sl_sint64 integer );
    t_type_value new_type_value_bit_array( int size, t_sl_uint64 value, t_sl_uint64 mask );
    int new_type_value_integer_or_bits( char *text, int *ofs, t_type_value *type_value ); // Create value in the type pool from text and and offset
    t_type_value new_type_value_string( char *text, int length );

    /*b Value interrogation functions
     */
    int check_mask_clear( t_type_value type_value );
    int logical_value( t_type_value type_value );
    t_sl_sint64 integer_value( t_type_value type_value );
    int bit_vector_values( t_type_value type_value, int *size, t_type_value_bit_vector_data **bits, t_type_value_bit_vector_data **mask );
    int get_size_of( t_type_value type_value );
    t_type_value get_type( t_type_value type_value );
    int check_resolved( t_type_value type_value );

    /*b Value support functions
     */
    int read_value( char *text, int *ofs, t_type_value_pool_entry *entry, int is_bit, int *bit_length, int *has_mask );

    /*b Assignment and coercion functions
     */
    int can_implicitly_coerce( t_type_value type, t_type_value value );
    int can_explicitly_coerce( t_type_value type, t_type_value value );
    t_type_value coerce( c_cyclicity *cyclicity, int implicit, t_type_value type, t_type_value value );

    /*b Backend interaction methods
     */
    t_md_type_definition_handle get_model_type( c_model_descriptor *model, t_type_value type_value );
    void build_type_definitions( c_model_descriptor *model );

    /*b Debug methods
     */
    char *display( char *buffer, int buffer_size, t_type_value type_value );
    void display( void );

private:
    /*b Pool memory management functions
     */
    void free_pool( void );
    void allocate_pool( int size );
    int grow_pool( int min_size );

    /*b Entry/index conversion methods
     */
    t_type_value_pool_entry *get_entry( t_type_value type_value );
    t_type_value get_index( t_type_value_pool_entry *type_value_pool_entry );

    /*b Lexical/cyc_object support methods
     */
    t_type_value get_system_type_index( t_symbol *symbol );
    int get_array_size( c_co_type_specifier *type_specifier );

    /*b Hash table management
     */
    void add_to_hash_table( int hash, t_type_value index );
    void add_to_hash( int *hash, int value );
    int hash_symbol( t_symbol *symbol );
    int hash_array( t_type_value sub_type, int size );
    int hash_type( t_co_scope *types, c_co_type_specifier *type_specifier );
    int hash_type( t_co_scope *types, c_co_type_definition *type_definition );
    int hash_type( t_type_value type_value );

    /*b Variables
     */
    class c_cyclicity *cyclicity;

    int pool_size;
    int tail_pool_index;
    int next_pool_index;
    int *pool;
    t_type_value *hash_table;
};

/*a Wrapper end
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


