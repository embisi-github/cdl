/*a Copyright
  
  This file 'c_type_value_pool.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Documentation
  This code maintains a database of types and typed values
  This is used by the cdl frontend to handle structures for typedefs, arrays, and so on, as well as compile time and runtime instances of them
  Each entry in the database is linked into the main list, as well as a hash table for fast indexing
  The entry then has a type - indicating it is an array definition, or a typedef, or a struct, or whatever
  There is then an indication that the type is coercible. This will probably be set to 0 for bit vectors and bits and integers, but 1 for structures and multidimensional arrays. Also, an FSM is strong - and this is an enum
  The entry then has a hash, which is generated based on any subtypes. Basically, however you refer to a type, the hash is the same for that type.
  Then, for variables, there is a type reference, which is a type_value_pool reference.
  An array type has a type and a size (this may be -1 for unresolved, in which case it is run-time - not currently supported)
  A typedef just has a symbol pointer and the type it refers to
  A structure has a number_elements and a list of (symbol, type) pairs for each element
  An integer has just that - an integer value
  A bit_array, which may include a mask, has a size, and then two offsets into the type_value_pool for the actual value. This allows arbitrary length bit_arrays.
  A bit has a value bit and a mask bit
  A string (we support those) has a length and string contents

  Note that type_value_pool references are NOT pointers. The pool may be relocated at any time. In order to reduce space consumption, and reduce pointer dereferences, the structures are allocated out of a pool, and each structure can be of arbitrary size. So type_value_pool references are in fact indexes into the pool, and there are functions supplied to convert them to pointers.

  The type_value_pool supports a wide range of methods.

  t_type_value c_type_value_pool::find_type( class c_co_type_specifier *type_specifier );
  t_type_value c_type_value_pool::add_type( t_co_scope *types, class c_co_type_specifier *type_specifier );
    Find or add a type based on a c_co_type_specifier - this returns the relevant type_value_pool reference.

  t_type_value c_type_value_pool::add_type( t_co_scope *types, t_symbol *symbol, class c_co_type_specifier *type_specifier );
  t_type_value c_type_value_pool::add_type( t_co_scope *types, t_symbol *symbol, t_type_value type_value );
  t_type_value c_type_value_pool::add_bit_array_type( int size );

    Add an instance of a type - a name and a c_co_type_specifier - this is not a full instance


     int c_type_value_pool::is_array( t_type_value type_value );
     int c_type_value_pool::array_size( t_type_value type_value );
     int c_type_value_pool::compare_types( c_co_type_specifier *cots_a, c_co_type_specifier *cots_b );

     t_type_value c_type_value_pool::new_type_value( int size, t_type_value type_value );
     t_type_value c_type_value_pool::new_type_value_type( t_type_value type_value );
     t_type_value c_type_value_pool::new_type_value_integer( int integer );
     t_type_value c_type_value_pool::new_type_value_bit_array( int size, t_sl_uint64 value, t_sl_uint64 mask );
     int c_type_value_pool::new_type_value_integer_or_bits( char *text, int *ofs, t_type_value *type_value );
     t_type_value c_type_value_pool::new_type_value_string( char *text, int length );

     int c_type_value_pool::check_mask_clear( t_type_value type_value );
     int c_type_value_pool::logical_value( t_type_value type_value );
     int c_type_value_pool::integer_value( t_type_value type_value );
     int c_type_value_pool::get_size_of( t_type_value type_value );
     t_type_value c_type_value_pool::get_type( t_type_value type_value );
     int c_type_value_pool::check_resolved( t_type_value type_value );

     int c_type_value_pool::read_value( char *text, int *ofs, t_type_value_pool_entry *entry, int *bit_length, int *has_mask );

     int c_type_value_pool::can_implicitly_coerce( t_type_value type, t_type_value value );
     int c_type_value_pool::can_explicitly_coerce( t_type_value type, t_type_value value );
     t_type_value c_type_value_pool::coerce( c_cyclicity *cyclicity, int implicit, t_type_value type, t_type_value value );

 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sl_debug.h"
#include "lexical_types.h"
#include "cyclicity_grammar.h"
#include "c_lexical_analyzer.h"
#include "c_cyc_object.h"
#include "c_type_value_pool.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"
#include "c_co_type_definition.h"
#include "c_co_expression.h"
#include "c_cyclicity.h"

/*a Defines
 */
#define HASH_TABLE_SIZE (4096)

/*a Constructors and destructors
 */
/*f c_type_value_pool::c_type_value_pool
 */
c_type_value_pool::c_type_value_pool( c_cyclicity *cyclicity )
{

     this->cyclicity = cyclicity;

     pool_size = 0;
     next_pool_index = type_value_error;
     tail_pool_index = type_value_error;
     pool = NULL;
     hash_table = NULL;

     allocate_pool( 14 );
}

/*f c_type_value_pool::~c_type_value_pool
 */
c_type_value_pool::~c_type_value_pool()
{
     free_pool();
}

/*a Type pool access, allocation and growth functions
 */
/*f c_type_value_pool::free_pool
 */
void c_type_value_pool::free_pool( void )
{
     if (hash_table)
          free(hash_table);
     if (pool)
          free(pool);
     pool = NULL;
     hash_table = NULL;
}

/*f c_type_value_pool::allocate_pool
 */
void c_type_value_pool::allocate_pool( int size )
{
     int i;

     free_pool();

     pool_size = size;
     pool = (int *)malloc( sizeof(int) * pool_size );
     next_pool_index = 0;

     hash_table = (t_type_value *)malloc(sizeof(t_type_value) * HASH_TABLE_SIZE );
     next_pool_index = type_value_last_system;

     if ((!pool) || (!hash_table))
     {
          free_pool();
          return;
     }

     for (i=0; i<HASH_TABLE_SIZE; i++)
          hash_table[i] = type_value_error;
}

/*f c_type_value_pool::grow_pool
 */
int c_type_value_pool::grow_pool( int min_size )
{
     int *new_pool;

     while (pool_size<min_size)
     {
          new_pool = (int *)realloc( pool, sizeof(int) * (pool_size+1024) );
          if (!new_pool)
               return 0;
          pool = new_pool;
          pool_size += 1024;
     }
     return 1;
}

/*f c_type_value_pool::get_entry
 */
t_type_value_pool_entry *c_type_value_pool::get_entry( t_type_value type_value )
{
     if (!pool)
          return NULL;
     if (type_value<type_value_last_system)
          return NULL;
     return (t_type_value_pool_entry *) (&pool[ type_value ]);
}

/*f c_type_value_pool::get_index
 */
t_type_value c_type_value_pool::get_index( t_type_value_pool_entry *type_value_pool_entry )
{
     if (!pool)
          return type_value_error;

     return ((int *)(type_value_pool_entry)) - pool;
}

/*a Type value pool object creation
 */
/*f c_type_value_pool::new_type_value_type
  size is size of entry in bytes
  type is the type for the type_reference field - undefined for actual types
 */
t_type_value c_type_value_pool::new_type_value( int size, t_type_value type )
{
     int pool_index;
     int entry_size;
     t_type_value_pool_entry *entry;
 
     SL_DEBUG( sl_debug_level_verbose_info, "size %d pool_size %d pool_index %d", size,pool_size, next_pool_index );

     pool_index = next_pool_index;
     entry_size = (int)((size+sizeof(int)-1)/sizeof(int));
     if (pool_index+entry_size+1 > pool_size)
     {
          if (!grow_pool( next_pool_index+entry_size ))
          {
               return type_value_error;
          }
     }

     if (tail_pool_index!=type_value_error)
     {
          entry = get_entry( tail_pool_index );
          entry->next_in_list = pool_index;
     }
     tail_pool_index = pool_index;
     entry = get_entry( pool_index );
     entry->type_reference = type;
     entry->next_in_list = type_value_error;
     entry->type_value_pool_entry_type = type_value_pool_entry_type_unresolved_variable;
     entry->model_type.type = md_type_definition_handle_type_none;

     next_pool_index += entry_size;
     return pool_index;
}

/*a Private hash table functions
  The hash table works as follows:
  hash of a named structure is a hash of the (global) pointer to the lex_symbol of the name, which is guaranteed unique for each symbol
  hash of a named reference is a hash of the (global) pointer to the lex_symbol of the name, which is guaranteed unique for each symbol
  hash of a system type is the value of that system type
  hash of an array of a type is a combination of the hash of the type and the size
 */
/*f c_type_value_pool::add_to_hash_table
 */
void c_type_value_pool::add_to_hash_table( int hash, t_type_value index )
{
     t_type_value_pool_entry *entry;

     if (!hash_table)
          return;

     entry = get_entry( index );
     if (hash_table[hash]!=type_value_error)
     {
          entry->next_in_hash_list = hash_table[hash];
          hash_table[hash] = index;
     }
     else
     {
          entry->next_in_hash_list = type_value_error;
          hash_table[hash] = index;
     }
}

/*f c_type_value_pool::get_system_type_index
 */
t_type_value c_type_value_pool::get_system_type_index( t_symbol *symbol )
{
     switch (get_symbol_type(symbol->lex_symbol))
     {
     case TOKEN_TEXT_INTEGER:
          return type_value_integer;
     case TOKEN_TEXT_BIT:
         return add_bit_array_type( 1 );
     case TOKEN_TEXT_STRING:
          return type_value_string;
     }
     return type_value_error;
}

/*f c_type_value_pool::get_array_size
  return -2 if there is no size/first index, or if either index is not an int (even undefined), or if the size <0
 */
int c_type_value_pool::get_array_size( c_co_type_specifier *type_specifier )
{
     int size;

     if ( (!type_specifier->first) ||
          (get_type( type_specifier->first->type_value )!=type_value_integer) )
          return -2;
     size = integer_value( type_specifier->first->type_value );
     if (type_specifier->last)
     {
          if (get_type( type_specifier->last->type_value )!=type_value_integer)
               return -2;
          size += 1-integer_value( type_specifier->last->type_value );
     }
     if (size<0)
          return -2;
     return size;
}

/*f c_type_value_pool::add_to_hash
 */
void c_type_value_pool::add_to_hash( int *hash, int value )
{
     *hash = (((*hash) * 0x4567 + value * 0x7def)>>8) & (HASH_TABLE_SIZE-1);
}

/*f c_type_value_pool::hash_symbol
 */
int c_type_value_pool::hash_symbol( t_symbol *symbol )
{
     int hash, i;
     int *hash_ptr;
     if (!symbol)
          return 0;
     hash_ptr = (int *)&(symbol->lex_symbol);
     hash = 0;
     for (i=0; i<(int)(sizeof(int *)/sizeof(int)); i++)
     {
          add_to_hash( &hash, hash_ptr[i] );
     }
     SL_DEBUG( sl_debug_level_verbose_info, "symbol %p '%s' hash %d", symbol, lex_string_from_terminal( symbol ), hash );
     return hash;
}

/*f c_type_value_pool::hash_array
 */
int c_type_value_pool::hash_array( t_type_value sub_type, int size )
{
     int hash;

     hash = 0;
     hash = hash_type( sub_type );
     add_to_hash( &hash, size );
     SL_DEBUG( sl_debug_level_verbose_info, "sub_type %d size %d hash %d", sub_type, size, hash);
     return hash;
}

/*f c_type_value_pool::hash_type (type_specifier)
    generate a hash dependent on the type of the type_specifier, consistent with the type_value_pool
 */
int c_type_value_pool::hash_type( t_co_scope *types, c_co_type_specifier *type_specifier )
{
     t_type_value index;
     int size;
     int hash;

     if (!type_specifier)
          return 0;
     SL_DEBUG( sl_debug_level_verbose_info, "specifier %p symbol %p sub_specifier %p size %d", type_specifier, type_specifier->symbol, type_specifier->type_specifier, get_array_size(type_specifier) );
     if (type_specifier->symbol)
     {
          index = get_system_type_index( type_specifier->symbol );
          if (index!=type_value_error)
               return index;
          hash = hash_symbol( type_specifier->symbol );
          SL_DEBUG( sl_debug_level_verbose_info, "specifier %p symbol %p hash %d", type_specifier, type_specifier->symbol, hash );
          return hash;
     }
     size = get_array_size( type_specifier );
     if (size>=0)
     {
         hash = hash_type( types, type_specifier->type_specifier );
         SL_DEBUG( sl_debug_level_verbose_info, "specifier %p sub_specifier %p sub_hash %d", type_specifier, type_specifier->type_specifier, hash );
         add_to_hash( &hash, size );
         SL_DEBUG( sl_debug_level_verbose_info, "specifier %p sub_specifier %p size %d hash %d", type_specifier, type_specifier->type_specifier, size, hash );
         return hash;
     }
     fprintf(stderr,"********************************************************************************\nRun-time sized types not supported\n");
     return 0;
}

/*f c_type_value_pool::hash_type (type_definition)
    generate a hash dependent on the symbol of the type_definition.
    Since symbols dont get reallocated, this can be a simple hash of the symbol ptr
 */
int c_type_value_pool::hash_type( t_co_scope *types, c_co_type_definition *type_definition )
{
     return hash_symbol( type_definition->symbol );
}

/*f c_type_value_pool::hash_type (type_value)
 */
int c_type_value_pool::hash_type( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value < type_value_last_system)
          return type_value;

     entry = get_entry( type_value );
     if (!entry)
          return 0;
     return entry->hash;
}

/*a Simple type interrogation functions
 */
/*f c_type_value_pool::derefs_to
 Returns the base type value (after all dereferencing) of a value or a type
 */
t_type_value c_type_value_pool::derefs_to( t_type_value type_value )
{
    t_type_value_pool_entry *entry;

    switch (type_value)
    {
    case type_value_error:
    case type_value_undefined:
    case type_value_bool:
    case type_value_bit_array_size_1:
    case type_value_integer:
    case type_value_string:
        return type_value;
    default:
        break;
    }

    if (!(entry = get_entry( type_value )))
        return type_value_error;

    if (entry->type_value_pool_entry_type == type_value_pool_entry_type_resolved_variable)
        return derefs_to( entry->type_reference );

    if (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_def)
        return derefs_to( entry->data.type_def.type_value );

    return type_value;
}

/*f c_type_value_pool::is_bool
 Returns 1 if the type_value is a boolean, (not a reference), or if it is a value instance of one
 */
int c_type_value_pool::is_bool( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value == type_value_bool)
          return 1;

     if (!(entry = get_entry( type_value )))
          return 0;

     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_resolved_variable)
          return is_bool( entry->type_reference );

     return 0;
}

/*f c_type_value_pool::derefs_to_bool
 Returns 1 if the type_value is a boolean, (not a reference), or if it is a value instance of one
 */
int c_type_value_pool::derefs_to_bool( t_type_value type_value )
{
    return is_bool( derefs_to( type_value ));
}

/*f c_type_value_pool::is_bit
 Returns 1 if the type_value is a bit, (not a reference), or if it is a value instance of one
 */
int c_type_value_pool::is_bit( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value == type_value_bit_array_size_1)
          return 1;

     if (!(entry = get_entry( type_value )))
          return 0;

     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_resolved_variable)
          return is_bit( entry->type_reference );

     return 0;
}

/*f c_type_value_pool::derefs_to_bit
  Return 1 if the type is a bit, or a reference to a bit
 */
int c_type_value_pool::derefs_to_bit( t_type_value type_value )
{
    return is_bit( derefs_to( type_value ));
}

/*f c_type_value_pool::is_integer
 Returns 1 if the type_value is a integer, (not a reference), or if it is a value instance of one
 */
int c_type_value_pool::is_integer( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value == type_value_integer)
          return 1;

     if (!(entry = get_entry( type_value )))
          return 0;

     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_resolved_variable)
          return is_integer( entry->type_reference );

     return 0;
}

/*f c_type_value_pool::derefs_to_integer
  Return 1 if the type is an integer, or a reference to a integer
 */
int c_type_value_pool::derefs_to_integer( t_type_value type_value )
{
    return is_integer( derefs_to( type_value ));
}

/*f c_type_value_pool::is_bit_vector
  Return 1 if the type is a bit array, not a reference to one
 */
int c_type_value_pool::is_bit_vector( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value == type_value_bit_array_size_1)
          return 1;

     if (type_value == type_value_bit_array_any_size)
          return 1;

     if (!(entry = get_entry( type_value )))
          return 0;

     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_resolved_variable)
          return is_bit_vector( entry->type_reference );

     if ( (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_array) && 
          is_bit(entry->data.type_array.type_value) )
          return 1;

     return 0;
}

/*f c_type_value_pool::derefs_to_bit_vector
  Return 1 if the type is a bit array, or a reference to a bit array
 */
int c_type_value_pool::derefs_to_bit_vector( t_type_value type_value )
{
    return is_bit_vector( derefs_to( type_value ));
}

/*f c_type_value_pool::is_array
 */
int c_type_value_pool::is_array( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     entry = get_entry( type_value );

     if (!entry)
          return 0;

     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_array)
     {
          return 1;
     }
     return 0;
}

/*f c_type_value_pool::derefs_to_array
  Return 1 if the type is a bit array, or a reference to a bit array
 */
int c_type_value_pool::derefs_to_array( t_type_value type_value )
{
    return is_array( derefs_to( type_value ));
}

/*f c_type_value_pool::array_type
 */
t_type_value c_type_value_pool::array_type( t_type_value type_value )
{
    t_type_value_pool_entry *entry;

    entry = get_entry( derefs_to(type_value) );
    if (!entry)
        return type_value_error;

    if (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_array)
    {
        return entry->data.type_array.type_value;
    }
    return type_value_error;
}

/*f c_type_value_pool::array_size
 */
int c_type_value_pool::array_size( t_type_value type_value )
{
    t_type_value_pool_entry *entry;

    if (derefs_to_bit( type_value ))
        return 1;

    entry = get_entry( derefs_to(type_value) );
    if (!entry)
        return -2;

    if (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_array)
    {
        return entry->data.type_array.resolved_size;
    }
    return -2;
}

/*f c_type_value_pool::is_structure
 */
int c_type_value_pool::is_structure( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     entry = get_entry( type_value );

     if (!entry)
          return 0;

     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_struct)
     {
          return 1;
     }
     return 0;
}

/*f c_type_value_pool::derefs_to_structure
  Return 1 if the type is a structure, or a reference to a structure
 */
int c_type_value_pool::derefs_to_structure( t_type_value type_value )
{
    return is_structure( derefs_to( type_value ));
}

/*f c_type_value_pool::find_structure_element
 */
int c_type_value_pool::find_structure_element( t_type_value structure, t_symbol *element )
 {
      t_type_value_pool_entry *entry;
      int i;

      entry = get_entry( structure );
      if (!entry)
           return -1;
      if (entry->type_value_pool_entry_type!=type_value_pool_entry_type_struct)
           return -1;
      for (i=0; i<entry->data.type_structure.number_elements; i++)
      {
           if (entry->data.type_structure.elements[i].symbol->lex_symbol == element->lex_symbol)
           {
                return i;
           }
      }
      return -1;
}

/*f c_type_value_pool::get_structure_element_type
  Get the internal type_value of the 'element'th element of a structure, or error if the structure does not exist or the element is out of range
 */
t_type_value c_type_value_pool::get_structure_element_type( t_type_value structure, int element )
{
      t_type_value_pool_entry *entry;
      entry = get_entry( structure );
      if (!entry)
           return type_value_error;
      if (entry->type_value_pool_entry_type!=type_value_pool_entry_type_struct)
           return type_value_error;
      if ((element<0) || (element>=entry->data.type_structure.number_elements))
           return type_value_error;
      return entry->data.type_structure.elements[element].type_value;
}

/*f c_type_value_pool::get_structure_element_number
  Get the number of elements in a structure
 */
int c_type_value_pool::get_structure_element_number( t_type_value structure )
{
    t_type_value_pool_entry *entry;
    entry = get_entry( structure );
    if (!entry)
        return type_value_error;
    if (entry->type_value_pool_entry_type!=type_value_pool_entry_type_struct)
        return type_value_error;
    return entry->data.type_structure.number_elements;
}

/*f c_type_value_pool::get_structure_element_name
  Get the name of the structure element
 */
t_symbol *c_type_value_pool::get_structure_element_name( t_type_value structure, int element )
{
    t_type_value_pool_entry *entry;
    entry = get_entry( derefs_to(structure) );
    if (!entry)
        return NULL;
    if (entry->type_value_pool_entry_type!=type_value_pool_entry_type_struct)
        return NULL;
    if ((element<0) || (element>=entry->data.type_structure.number_elements))
        return NULL;
    return entry->data.type_structure.elements[element].symbol;
}

/*a Type pool find/addition functions
 */
/*f c_type_value_pool::add_array
 */
t_type_value c_type_value_pool::add_array( t_type_value sub_index, int size )
{
     int hash;
     t_type_value index;
     t_type_value_pool_entry *entry;

     SL_DEBUG( sl_debug_level_verbose_info, "subindex %d size %d", sub_index, size );
     if ((sub_index==type_value_bit_array_size_1) && (size==1))
     {
         return type_value_bit_array_size_1;
     }

     hash = hash_array( sub_index, size );
     SL_DEBUG( sl_debug_level_verbose_info, "subindex %d size %d hash %d", sub_index, size, hash );
     if (hash_table[hash]!=type_value_error)
     {
          index = hash_table[hash];
          while (index!=type_value_error)
          {
               entry = get_entry( index );
               if ( (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_array) &&
                    (entry->data.type_array.resolved_size == size) &&
                    (entry->data.type_array.type_value == sub_index) )
                    return index;
               index = entry->next_in_hash_list;
          }
     }

     index = new_type_value( sizeof(t_type_value_pool_entry), type_value_undefined );
     if (index == type_value_error)
          return type_value_error;

     entry = get_entry( index );
     entry->type_value_pool_entry_type = type_value_pool_entry_type_type_array;
     entry->hash = hash;
     entry->data.type_array.resolved_size = size;
     entry->data.type_array.unresolved_start = NULL;
     entry->data.type_array.unresolved_end = NULL;
     entry->data.type_array.type_value = sub_index;

     add_to_hash_table( entry->hash, index );

     SL_DEBUG( sl_debug_level_verbose_info, "subindex %d size %d returns index %d hash %d", sub_index, size, index, hash );
     return index;
}

/*f c_type_value_pool::add_typedef
 */
t_type_value c_type_value_pool::add_typedef( t_symbol *symbol, t_type_value type_value )
{
     int pool_index;
     t_type_value_pool_entry *entry;

     SL_DEBUG( sl_debug_level_verbose_info, "name %s index %d hash %d", lex_string_from_terminal( symbol), type_value, hash_symbol(symbol) );

     pool_index = new_type_value( sizeof(t_type_value_pool_entry), type_value_undefined );
     if (pool_index == type_value_error)
          return type_value_error;

     entry = get_entry( pool_index );
     entry->type_value_pool_entry_type = type_value_pool_entry_type_type_def;
     entry->hash = hash_symbol( symbol );
     entry->data.type_def.symbol = symbol;
     entry->data.type_def.type_value = type_value;

     add_to_hash_table( hash_symbol( symbol ), pool_index );

     return pool_index;
}

/*f c_type_value_pool::add_structure
 */
t_type_value c_type_value_pool::add_structure( t_symbol *symbol, int number_elements )
{
     int pool_index;
     t_type_value_pool_entry *entry;
     int i;
 
     SL_DEBUG( sl_debug_level_verbose_info, "number_elements %d", number_elements );

     pool_index = new_type_value( sizeof(t_type_value_pool_entry) + sizeof(t_type_value_pool_named_type)*(number_elements-1), type_value_undefined );
     if (pool_index == type_value_error)
          return type_value_error;

     entry = get_entry( pool_index );
     entry->type_value_pool_entry_type = type_value_pool_entry_type_struct;
     entry->hash = hash_symbol( symbol );
     entry->data.type_structure.symbol = symbol;
     entry->data.type_structure.number_elements = number_elements;
     for (i=0; i<number_elements; i++)
     {
          entry->data.type_structure.elements[i].symbol = NULL;
          entry->data.type_structure.elements[i].type_value = type_value_undefined;
     }

     add_to_hash_table( entry->hash, pool_index );
     SL_DEBUG( sl_debug_level_verbose_info, "hashing '%s' to %d", lex_string_from_terminal(symbol), entry->hash );

     return pool_index;
}

/*f c_type_value_pool::compare_types
    return 1 if equal, 0 otherwise
    Why is this used?
    A user type is the same only if it directly refers to the same system type (int, string, bit)
    A system type is the same only if it is the same
    An array type is the same only if it has the same size and it is an array of the same type
    A structure type is the same only if it ?
 */
int c_type_value_pool::compare_types( t_type_value type_value, c_co_type_specifier *type_specifier )
{
     t_type_value ts_index;
     t_type_value_pool_entry *entry;
     int size;

     SL_DEBUG( sl_debug_level_verbose_info, "index %d type_specifier %p", type_value, type_specifier );

     /*b If a system type or user type reference, look it up, and compare the indices
      */
     if (type_specifier->symbol)
     {
          ts_index = get_system_type_index( type_specifier->symbol );
          return (ts_index == type_value);
     }

     /*b type_specifier must be an array, so check the size and sub_type
      */
     entry = get_entry( type_value );
     if (entry->type_value_pool_entry_type != type_value_pool_entry_type_type_array)
          return 0;
     ts_index = find_type( type_specifier->type_specifier );
     if (ts_index != entry->data.type_array.type_value)
          return 0;
     size = get_array_size( type_specifier );
     if ((size<0) || (size!=entry->data.type_array.resolved_size))
          return 0;
     return 1;
}

/*f c_type_value_pool::find_type
 */
t_type_value c_type_value_pool::find_type( c_co_type_specifier *type_specifier )
{
     int hash;
     t_type_value index;
     t_type_value_pool_entry *entry;

     SL_DEBUG( sl_debug_level_verbose_info, "Find type specifier %p", type_specifier );
     /*b If a system type or user type reference, look it up
      */
     if (type_specifier->symbol)
     {
          index = get_system_type_index( type_specifier->symbol );
          if (index!=type_value_error)
               return index;
          hash = hash_symbol( type_specifier->symbol );
          index = hash_table[hash];

          while (index != type_value_error)
          {
               entry = get_entry( index );
               if ( (entry->type_value_pool_entry_type == type_value_pool_entry_type_type_def) &&
                    (entry->data.type_def.symbol->lex_symbol == type_specifier->symbol->lex_symbol) )
                    return index;
               if ( (entry->type_value_pool_entry_type == type_value_pool_entry_type_struct) && 
                    (entry->data.type_structure.symbol->lex_symbol == type_specifier->symbol->lex_symbol) )
                    return index;
               index = entry->next_in_hash_list;
          }
          SL_DEBUG( sl_debug_level_verbose_info, "Failed to find type specifier %p (%s)", type_specifier, lex_string_from_terminal(type_specifier->symbol) );
          return type_value_error;
     }

     hash = hash_type( NULL, type_specifier );
     index = hash_table[hash];
     while (index!=type_value_error)
     {
          if (compare_types( index, type_specifier))
          {
               return index;
          }
          entry = get_entry( index );
          index = entry->next_in_hash_list;
     }
     SL_DEBUG( sl_debug_level_verbose_info, "Failed to find type specifier %p in hash table (hash %d)", type_specifier, hash );
     return type_value_error;
}

/*f c_type_value_pool::add_type (types, type specifier)
 */
t_type_value c_type_value_pool::add_type( t_co_scope *types, class c_co_type_specifier *type_specifier )
{
     t_type_value index, sub_index;
     int size;

     /*b First try to find it if it is a user or system type
      */
     index = find_type( type_specifier );
     if (index!=type_value_error)
          return index;

     /*b If not found and not an array, then return error, as it should be a user/system type
      */
     if (type_specifier->symbol)
          return type_value_error;

     /*b Okay, we're an array... add our sub-type_specifier, and check expressions are resolved
      */
     size = get_array_size( type_specifier );
     if (size<0)
          return type_value_error;
     sub_index = add_type( types, type_specifier->type_specifier );
     if (sub_index==type_value_error)
          return type_value_error;

     index = add_array( sub_index, size );
     SL_DEBUG( sl_debug_level_verbose_info, "type_specifier %p returns index %d", type_specifier, index );
     return index;
}

/*f c_type_value_pool::add_type (types, symbol, type_specifier) -- a typedef
 */
t_type_value c_type_value_pool::add_type( t_co_scope *types, t_symbol *symbol, class c_co_type_specifier *type_specifier )
{
     t_type_value sub_index;

     sub_index = find_type( type_specifier );
     if (sub_index==type_value_error)
          return type_value_error;

     return add_type( types, symbol, sub_index);
}

/*f c_type_value_pool::add_type (types, symbol, type_struct) -- a named structure of a list of named specifiers
 */
t_type_value c_type_value_pool::add_type( t_co_scope *types, t_symbol *symbol, class c_co_type_struct *type_struct )
{
     int i;
     c_co_type_struct *cots;
     t_type_value type;
     t_type_value_pool_entry *entry;

     SL_DEBUG( sl_debug_level_verbose_info, "structure" );

     for (i=0, cots=type_struct; cots; i++, cots=(c_co_type_struct *)cots->next_in_list );
     
     type = add_structure( symbol, i );
     if (type==type_value_error)
          return type;

     entry = get_entry( type );
     for (i=0, cots=type_struct; cots; i++, cots=(c_co_type_struct *)cots->next_in_list )
     {
          entry->data.type_structure.elements[i].symbol = cots->symbol;
          entry->data.type_structure.elements[i].type_value = find_type( cots->type_specifier );
          SL_DEBUG( sl_debug_level_verbose_info, "Adding element name %s type %d to structure at index %d", lex_string_from_terminal( cots->symbol ), entry->data.type_structure.elements[i].type_value, type );
     }
     return type;
}

/*f c_type_value_pool::add_type (types, symbol, index ) -- a typedef
 */
t_type_value c_type_value_pool::add_type( t_co_scope *types, t_symbol *symbol, t_type_value type_value )
{
     return add_typedef( symbol, type_value );
}

/*f c_type_value_pool::add_bit_array_type ( size )
 */
t_type_value c_type_value_pool::add_bit_array_type( int size )
{
     return add_array( type_value_bit_array_size_1, size );
}

/*f c_type_value_pool::compare_types (type_specifier, type_specifier )
    return 1 if identical
 */
int c_type_value_pool::compare_types( c_co_type_specifier *cots_a, c_co_type_specifier *cots_b )
{
     t_type_value index_a, index_b;

     index_a = find_type( cots_a );
     index_b = find_type( cots_b );
     if ( (index_a==index_b) &&
          (index_a != type_value_error ) )
          return 1;
     return 0;
}

/*a Typed value creation methods
 */
/*f c_type_value_pool::new_type_value_type
 */
t_type_value c_type_value_pool::new_type_value_type( t_type_value type_reference )
{
     t_type_value value;
     t_type_value_pool_entry *entry;

     value = new_type_value( sizeof( t_type_value_pool_entry ), type_value_integer );
     if (value==type_value_error)
          return type_value_error;

     entry = get_entry( value );
     entry->type_reference = type_reference;
     entry->type_value_pool_entry_type = type_value_pool_entry_type_type_reference;

     return value;
}

/*f c_type_value_pool::new_type_value_bool
 */
t_type_value c_type_value_pool::new_type_value_bool( int boolean )
{
     t_type_value type_value;
     t_type_value_pool_entry *entry;

     type_value = new_type_value( sizeof( t_type_value_pool_entry ), type_value_bool );
     if (type_value==type_value_error)
          return type_value_error;

     entry = get_entry( type_value );
     entry->data.value_bool = boolean;
     entry->type_value_pool_entry_type = type_value_pool_entry_type_resolved_variable;

     return type_value;
}

/*f c_type_value_pool::new_type_value_integer
 */
t_type_value c_type_value_pool::new_type_value_integer( t_sl_sint64 integer )
{
     t_type_value type_value;
     t_type_value_pool_entry *entry;

     type_value = new_type_value( sizeof( t_type_value_pool_entry ), type_value_integer );
     if (type_value==type_value_error)
          return type_value_error;

     entry = get_entry( type_value );
     entry->data.value_integer = integer;
     entry->type_value_pool_entry_type = type_value_pool_entry_type_resolved_variable;

     return type_value;
}

/*f c_type_value_pool::new_type_value_bit_array
 */
t_type_value c_type_value_pool::new_type_value_bit_array( int size, t_sl_uint64 value, t_sl_uint64 mask )
{
     t_type_value type_value;
     t_type_value_pool_entry *entry;
     t_type_value type_index;

     SL_DEBUG( sl_debug_level_verbose_info, "Size %d value %d mask %d", size, value, mask );

     if (size==1)
     {
         type_value = new_type_value( sizeof( t_type_value_pool_entry ), type_value_bit_array_size_1 );
          if (type_value==type_value_error)
               return -1;
          entry = get_entry( type_value );
          entry->data.value_bit.value=value&1;
          entry->data.value_bit.mask=0;
          if (mask!=0)
              entry->data.value_bit.mask=mask;
          entry->type_value_pool_entry_type = type_value_pool_entry_type_resolved_variable;
          return type_value;
     }

     type_index = add_bit_array_type( size );
     if (type_index==type_value_error)
          return -1;

     if (mask!=0)
     {
          type_value = new_type_value( sizeof( t_type_value_pool_entry )+sizeof(t_type_value_bit_vector_data), type_index );
     }
     else
     {
          type_value = new_type_value( sizeof( t_type_value_pool_entry ), type_index );
     }
     if (type_value==type_value_error)
          return -1;

     entry = get_entry( type_value );
     entry->data.value_bit_array.size = size;
     entry->data.value_bit_array.contents[0] = (((t_sl_uint64)value)<<(64-size)) >> (64-size);
     if (mask!=0)
     {
         entry->data.value_bit_array.contents[1] = (((t_sl_uint64)mask)<<(64-size)) >> (64-size);
     }
     else
     {
         entry->data.value_bit_array.mask_offset = -1;
     }
     entry->type_value_pool_entry_type = type_value_pool_entry_type_resolved_variable;

     return type_value;
}

/*f c_type_value_pool::new_type_value_integer_or_bits
 */
int c_type_value_pool::new_type_value_integer_or_bits( char *text, int *ofs, t_type_value *type_value )
{
    t_type_value_pool_entry *entry;
    t_type_value bit_array;
    int text_ofs;
    int bit_length;
    int has_mask;
    int read_value_error;
    int i;

    text_ofs = *ofs;
    read_value_error = read_value( text, ofs, NULL, 0, &bit_length, &has_mask );
    if (read_value_error!=0)
        return read_value_error;

    if (bit_length==-1)
    {
        *type_value = new_type_value( sizeof( t_type_value_pool_entry ), type_value_integer );
        if (*type_value==type_value_error)
            return -1;
        entry = get_entry( *type_value );
    }
    else if (bit_length==1)
    {
        *type_value = new_type_value( sizeof( t_type_value_pool_entry ), type_value_bit_array_size_1 );
        if (*type_value==type_value_error)
            return -1;
        entry = get_entry( *type_value );
        entry->data.value_bit.value=0;
        entry->data.value_bit.mask=0;
    }
    else
    {
        bit_array = add_bit_array_type( bit_length );
        // bit_length is number of bits we need
        // i is number of t_type_value_bit_vector_data's needed to store bit_length bits
        i = (bit_length+sizeof(t_type_value_bit_vector_data)*8-1)/sizeof(t_type_value_bit_vector_data)/8;
        SL_DEBUG( sl_debug_level_verbose_info, "Length of bits is %d t_type_value_bit_vector_data's", i );
        if (has_mask)
        {
            *type_value = new_type_value( sizeof( t_type_value_pool_entry )+(2*i-1)*sizeof(t_type_value_bit_vector_data), bit_array );
        }
        else
        {
            *type_value = new_type_value( sizeof( t_type_value_pool_entry )+(i-1)*sizeof(t_type_value_bit_vector_data), bit_array );
        }
        if (*type_value==type_value_error)
            return -1;
        entry = get_entry( *type_value );
        entry->data.value_bit_array.size = bit_length;
        if (has_mask)
        {
            entry->data.value_bit_array.mask_offset = i;
        }
        else
        {
            entry->data.value_bit_array.mask_offset = -1;
        }
    }

    read_value_error = read_value( text, &text_ofs, entry, (bit_length==1), &bit_length, &has_mask );
    entry->type_value_pool_entry_type = type_value_pool_entry_type_resolved_variable;
    return read_value_error;
}

/*f c_type_value_pool::new_type_value_string
 */
t_type_value c_type_value_pool::new_type_value_string( char *text, int length )
{
     t_type_value type_value;
     t_type_value_pool_entry *entry;

     type_value = new_type_value( sizeof( t_type_value_pool_entry )+length, type_value_string );
     if (type_value==type_value_error)
          return type_value_error;

     entry = get_entry( type_value );
     entry->data.value_string.length = length;
     strncpy( entry->data.value_string.string, text, length );
     entry->data.value_string.string[length+1] = 0;
     entry->type_value_pool_entry_type = type_value_pool_entry_type_resolved_variable;

     return type_value;
}

/*a Inspection methods
 */
/*f c_type_value_pool::check_mask_clear
    return 1 for mask clear
 */
int c_type_value_pool::check_mask_clear( t_type_value type_value )
{
     t_type_value_pool_entry *entry;
     SL_DEBUG( sl_debug_level_verbose_info, "Check mask clear %d", type_value );
     entry = get_entry( type_value );
     switch (entry->type_reference)
     {
     case type_value_integer:
     case type_value_bool:
     case type_value_string:
         SL_DEBUG( sl_debug_level_verbose_info, "integer/bool/string" );
          return 1;
     case type_value_bit_array_size_1:
         SL_DEBUG( sl_debug_level_verbose_info, "bit %d", entry->data.value_bit.mask);
          return (entry->data.value_bit.mask == 0);
     default:
         SL_DEBUG( sl_debug_level_verbose_info, "summat else %d",entry->data.value_bit_array.mask_offset );
         return (entry->data.value_bit_array.mask_offset<0);
     }
     return 0;
}

/*f c_type_value_pool::logical_value
 */
int c_type_value_pool::logical_value( t_type_value type_value )
{
     t_type_value_pool_entry *entry;
     int i;

     if (type_value == type_value_error)
          return 0;
     if (type_value == type_value_undefined)
          return 0;

     entry = get_entry( type_value );
     if (!entry)
         return 0;

     if (entry->type_value_pool_entry_type != type_value_pool_entry_type_resolved_variable)
     {
         return 0;
     }

     switch (entry->type_reference)
     {
     case type_value_bool:
         return (entry->data.value_bool);
     case type_value_integer:
         return (entry->data.value_integer!=0);
     case type_value_string:
         return (entry->data.value_string.length>0);
     case type_value_bit_array_size_1:
         return (entry->data.value_bit.value != 0);
     default:
     {
         int size;
         t_type_value_bit_vector_data *bits, *mask;
         int szof_tvbvd = sizeof(t_type_value_bit_vector_data)*8; // Size in bits
         if ( !is_bit_vector(type_value) )
             return 0;
         bit_vector_values( type_value, &size, &bits, &mask );
         for (i=0; i<size; i++)
         {
             if ((bits[(i/szof_tvbvd)]>>(i%szof_tvbvd))&1)
             {
                 return 1;
             }
         }
         return 0;
     }
     }
     return 0;
}

/*f c_type_value_pool::read_value
 return 0 for success
 return 1 for value too large
 return 2 for bad value
 return 3 for more bits given than in the size
 if entry is NULL, then don't store anything... just return the size
 */
int c_type_value_pool::read_value( char *text, int *ofs, t_type_value_pool_entry *entry, int is_bit, int *bit_length, int *has_mask )
{
    int c, eoi, nbits;
    t_sl_sint64 size;
    int mask_zero;
    SL_DEBUG( sl_debug_level_verbose_info, "%p %d %p", text, *ofs, entry );

    /*b Read first integer - could be all we have...
     */
    if (sscanf( text+*ofs, "%lld%n", &size, &eoi )==0)
    {
        SL_DEBUG( sl_debug_level_verbose_info, "reading a value, expecting a sized integer (or just integer), but did not get an integer" );
        *ofs += 1;
        return 2;
    }
    eoi += *ofs;

    /*b Check for sized binary
     */
    if ( (text[eoi]=='b') || (text[eoi]=='B'))
    {
        if ( (size<1) || (size>64) )
        {
            *ofs = eoi+1;
            return 1;
        }
        if (entry) // second pass
        {
            if (is_bit)
            {
                entry->data.value_bit.value=0;
                entry->data.value_bit.mask=0;
            }
            else
            {
                entry->data.value_bit_array.contents[0] = 0;
                if (entry->data.value_bit_array.mask_offset>0)
                {
                    entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] = 0;
                }
            }
        }
        eoi++;
        nbits = 0;
        mask_zero = 1;
        while (1)
        {
            while (text[eoi]=='_')
            {
                eoi++;
            }
            c = text[eoi];
            if ( (c=='0') || (c=='1') )
            {
                nbits++;
                if (nbits>size)
                {
                    *ofs = eoi;
                    return 3;
                }
                if (entry)
                {
                    if (is_bit)
                    {
                        entry->data.value_bit.mask <<= 1;
                        entry->data.value_bit.value <<= 1;
                        entry->data.value_bit.value |= (c=='1');
                    }
                    else
                    {
                        if (entry->data.value_bit_array.mask_offset>0)
                        {
                            entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] <<= 1;
                        }
                        entry->data.value_bit_array.contents[0] <<= 1;
                        entry->data.value_bit_array.contents[0] |= (c=='1');
                    }
                }
                eoi++;
            }
            else if ( (c=='x') || (c=='X') )
            {
                nbits++;
                if (nbits>size)
                {
                    *ofs = eoi;
                    return 3;
                }
                if (entry)
                {
                    if (is_bit)
                    {
                        entry->data.value_bit.mask <<= 1;
                        entry->data.value_bit.mask |= 1;
                        entry->data.value_bit.value <<= 1;
                    }
                    else
                    {
                        if (entry->data.value_bit_array.mask_offset>0)
                        {
                            entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] <<= 1;
                            entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] |= 1;
                        }
                        entry->data.value_bit_array.contents[0] <<= 1;
                    }
                }
                mask_zero = 0;
                eoi++;
            }
            else
            {
                break;
            }
        }
        *has_mask = !mask_zero;
        *bit_length = size;
        *ofs = eoi;
        return 0;
    }

    /*b Check for sized hex
     */
    if ( (text[eoi]=='h') || (text[eoi]=='H'))
    {
        if ( (size<1) || (size>64) )
        {
            *ofs = eoi+1;
            return 1;
        }
        if (entry)
        {
            if (is_bit)
            {
                entry->data.value_bit.value=0;
                entry->data.value_bit.mask=0;
            }
            else
            {
                entry->data.value_bit_array.contents[0] = 0;
                if (entry->data.value_bit_array.mask_offset>0)
                {
                    entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] = 0;
                }
            }
        }
        eoi++;
        nbits = 0;
        mask_zero = 1;
        while (1)
        {
            int hex;
            while (text[eoi]=='_')
            {
                eoi++;
            }
            c = text[eoi];
            hex = -1;
            if ( (c>='0') && (c<='9') )
            {
                hex = (c-'0');
            }
            else if ( (c>='a') && (c<='f') )
            {
                hex = (c-'a')+10;
            }
            else if ( (c>='A') && (c<='F') )
            {
                hex = (c-'A')+10;
            }
            if (hex>=0)
            {
                nbits+=4;
                if (nbits>size+3)
                {
                    *ofs = eoi;
                    return 3;
                }
                if (entry)
                {
                    if (is_bit)
                    {
                        entry->data.value_bit.mask <<= 4;
                        entry->data.value_bit.value <<= 4;
                        entry->data.value_bit.value |= hex;
                    }
                    else
                    {
                        if (entry->data.value_bit_array.mask_offset>0)
                        {
                            entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] <<= 4;
                        }
                        entry->data.value_bit_array.contents[0] <<= 4;
                        entry->data.value_bit_array.contents[0] |= hex;
                    }
                }
                eoi++;
            }
            else if ( (c=='x') || (c=='X') )
            {
                nbits+=4;
                if (nbits>size+3)
                {
                    *ofs = eoi;
                    return 3;
                }
                if (entry)
                {
                    if (is_bit)
                    {
                        entry->data.value_bit.mask <<= 4;
                        entry->data.value_bit.value <<= 4;
                        entry->data.value_bit.mask |= 0x1;
                    }
                    else
                    {
                        if (entry->data.value_bit_array.mask_offset>0)
                        {
                            entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] <<= 4;
                            entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] |= 0xf;
                        }
                        entry->data.value_bit_array.contents[0] <<= 4;
                    }
                }
                mask_zero = 0;
                eoi++;
            }
            else
            {
                break;
            }
        }
        if (entry)
            SL_DEBUG( sl_debug_level_verbose_info, "read hex value <>h... %llx mask %llx", entry->data.value_bit_array.contents[0], mask_zero?0ll:entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] );
        *has_mask = !mask_zero;
        *bit_length = size;
        *ofs = eoi;
        return 0;
    }

    /*b Check for sized hex
     */
    if ( (text[eoi]=='d') || (text[eoi]=='D'))
    {
        if ( (size<1) || (size>64) )
        {
            *ofs = eoi+1;
            return 1;
        }
        if (entry)
        {
            if (is_bit)
            {
                entry->data.value_bit.value=0;
                entry->data.value_bit.mask=0;
            }
            else
            {
                entry->data.value_bit_array.contents[0] = 0;
                if (entry->data.value_bit_array.mask_offset>0)
                {
                    entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] = 0;
                }
            }
        }
        eoi++;
        nbits = 0;
        mask_zero = 1;
        t_sl_uint64 value=0;
        while (1)
        {
            int digit;
            while (text[eoi]=='_')
            {
                eoi++;
            }
            c = text[eoi];
            digit = -1;
            if ( (c>='0') && (c<='9') )
            {
                digit = (c-'0');
            }
            if (digit>=0)
            {
                value = 10*value + digit;
                if (entry)
                {
                    if (is_bit)
                    {
                        entry->data.value_bit.value = value;
                    }
                    else
                    {
                        entry->data.value_bit_array.contents[0] = value;
                    }
                }
                eoi++;
            }
            else
            {
                break;
            }
        }
        if (entry)
            SL_DEBUG( sl_debug_level_verbose_info, "read dec value <>h... %llx mask %llx", entry->data.value_bit_array.contents[0], mask_zero?0ll:entry->data.value_bit_array.contents[entry->data.value_bit_array.mask_offset] );
        *has_mask = 0;
        *bit_length = size;
        *ofs = eoi;
        return 0;
    }

    /*b Check for 0x and 0X
     */
    if ( (text[*ofs]=='0') && (text[1+*ofs]=='x' || text[1+*ofs]=='X') )
    {
        if (sscanf( text+*ofs+2, "%llx%n", &size, &eoi)==0)
        {
            SL_DEBUG( sl_debug_level_verbose_info, "reading a value, expecting a hex number, but did not get it" );
            *ofs += 3;
            return 2;
        }
        if (entry)
        {
            entry->data.value_integer = size;
            SL_DEBUG( sl_debug_level_verbose_info, "read hex value 0x... %llx", entry->data.value_integer );
        }
        *has_mask = 0;
        *bit_length = -1;
        *ofs = *ofs + eoi + 2;
        return 0;
    }
    /*b Else it must just be a decimal
     */
    if (entry)
    {
        entry->data.value_integer = size;
    }
    SL_DEBUG( sl_debug_level_verbose_info, "integer %d", size );
    *has_mask = 0;
    *bit_length = -1;
    *ofs = eoi;
    return 0;
}

/*f c_type_value_pool::integer_value
 */
t_sl_sint64 c_type_value_pool::integer_value( t_type_value type_value )
{
     t_type_value_pool_entry *entry;
     int size;
     t_type_value_bit_vector_data *bits, *mask;

     if (type_value==type_value_error)
     {
         SL_DEBUG( sl_debug_level_high_warning, "called with a bad type_value 'type_value_error'" );
          return 0;
     }
     entry = get_entry( type_value );
     if ((!entry) || (entry->type_value_pool_entry_type != type_value_pool_entry_type_resolved_variable))
     {
          SL_DEBUG( sl_debug_level_high_warning, "called with a non-resolved value" );
          return 0;
     }

     if (derefs_to_bit_vector(type_value))
     {
         if (bit_vector_values( type_value, &size, &bits, &mask ))
         {
             return (t_sl_sint64)(bits[0]);
         }
     }

     if (entry->type_reference == type_value_integer)
     {
          return entry->data.value_integer;
     }
     SL_DEBUG( sl_debug_level_high_warning, "called with a non-integer type_value %d/%d", type_value, entry->type_reference );
     return 0;
}

/*f c_type_value_pool::bit_vector_values
 */
int c_type_value_pool::bit_vector_values( t_type_value type_value, int *size, t_type_value_bit_vector_data **bits, t_type_value_bit_vector_data **mask )
{
    t_type_value_pool_entry *entry;

    if (type_value==type_value_error)
    {
        SL_DEBUG( sl_debug_level_high_warning, "called with a bad type_value" );
        return 0;
    }

    entry = get_entry( type_value );
    if (!entry)
        return 0;

    if (entry->type_value_pool_entry_type != type_value_pool_entry_type_resolved_variable)
    {
        SL_DEBUG( sl_debug_level_high_warning, "called with a non-resolved value" );
        return 0;
    }

    switch (entry->type_reference)
    {
    case type_value_integer:
        *bits = (t_type_value_bit_vector_data *)&entry->data.value_integer;
        *mask = NULL;
        *size = sizeof(t_sl_uint64)*8;
        return 1;
    case type_value_bool:
        *bits = &entry->data.value_bool;
        *mask = NULL;
        *size = 1;
        return 1;
    case type_value_bit_array_size_1:
        *bits = &entry->data.value_bit.value;
        *mask = &entry->data.value_bit.mask;
        *size = 1;
        return 1;
    default:
        *bits = entry->data.value_bit_array.contents;
        *mask = NULL;
        if (entry->data.value_bit_array.mask_offset>0)
        {
            *mask = entry->data.value_bit_array.contents + entry->data.value_bit_array.mask_offset;
        }
        *size = entry->data.value_bit_array.size;
        break;
    }
    return 1;
}

/*f c_type_value_pool::get_size_of
 */
int c_type_value_pool::get_size_of( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value==type_value_error)
     {
          SL_DEBUG( sl_debug_level_high_warning, "called with a bad type_value" );
          return 0;
     }
     // Catch unusual values - type_value_bit will come from arrays, though
     switch (type_value)
     {
     case type_value_bit_array_size_1:
          return 1;
     case type_value_integer:
          return sizeof(t_sl_sint64)*8;
     case type_value_error:
          return -1;
     case type_value_undefined:
          return -1;
     }
     entry = get_entry( type_value );
     switch (entry->type_value_pool_entry_type)
     {
     case type_value_pool_entry_type_struct:
          fprintf(stderr,"c_type_value_pool::get_size_of:Size of type structure not done yet\n");
          return 0;
     case type_value_pool_entry_type_type_def:
         return get_size_of( entry->data.type_def.type_value );
     case type_value_pool_entry_type_type_reference:
          fprintf(stderr,"c_type_value_pool::get_size_of:Size of type reference not done yet\n");
          return 0;
     case type_value_pool_entry_type_type_array:
          return (entry->data.type_array.resolved_size) * get_size_of(entry->data.type_array.type_value);
     default:
          break;
     }
     /*b If not picked up yet then it is a variable, and so size the type reference
      */
     switch (entry->type_reference)
     {
     case type_value_string:
          return entry->data.value_string.length;
     case type_value_integer:
     {
         int i;
         t_sl_uint64 v;
         v = (t_sl_uint64)(entry->data.value_integer);
         for (i=0; v; i++, v>>=1);
         return i;
     }
     default:
          return get_size_of( entry->type_reference );
     }
     return 0;
}

/*f c_type_value_pool::get_type
 */
t_type_value c_type_value_pool::get_type( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value==type_value_error)
     {
          return type_value_error;
     }
     if (type_value==type_value_undefined)
     {
          return type_value_error;
     }
     
     entry = get_entry( type_value );
     return entry->type_reference;
}

/*f c_type_value_pool::check_resolved
 */
int c_type_value_pool::check_resolved( t_type_value type_value )
{
     t_type_value_pool_entry *entry;

     if (type_value==type_value_error)
     {
          return 0;
     }
     if (type_value==type_value_undefined)
     {
          return 0;
     }
     
     entry = get_entry( type_value );
     if (entry->type_value_pool_entry_type == type_value_pool_entry_type_resolved_variable)
          return 1;
     return 0;
}

/*a Backend interaction methods
 */
/*f c_type_value_pool::get_model_type
 */
t_md_type_definition_handle c_type_value_pool::get_model_type( c_model_descriptor *model, t_type_value type_value )
{
     t_md_type_definition_handle result;
     t_type_value_pool_entry *entry;

     if (type_value==type_value_error)
     {
          SL_DEBUG( sl_debug_level_high_warning, "called with a bad type_value" );
          return result;
     }
     result.type = md_type_definition_handle_type_none;
     switch (type_value)
     {
     case type_value_bit_array_size_1:
          return model->type_definition_bit_vector_create( 1 );
     case type_value_integer:
          return model->type_definition_bit_vector_create( sizeof(t_sl_uint64)*8 );
     case type_value_error:
     case type_value_undefined:
          return result;
     }
     entry = get_entry( type_value );
     switch (entry->type_value_pool_entry_type)
     {
     case type_value_pool_entry_type_struct:
     case type_value_pool_entry_type_type_def:
     case type_value_pool_entry_type_type_reference:
     case type_value_pool_entry_type_type_array:
          return entry->model_type;
     default:
          break;
     }
     return result;
}

/*f c_type_value_pool::build_type_definitions
 */
void c_type_value_pool::build_type_definitions( c_model_descriptor *model )
{
     t_type_value index;
     t_type_value_pool_entry *entry;
     t_md_type_definition_handle sub_model_type;
     int i;

     SL_DEBUG( sl_debug_level_info, "model %p", model );
     if (tail_pool_index==type_value_error)
     {
         SL_DEBUG( sl_debug_level_info, "Invalid pool" );
         return;
     }

     index = type_value_last_system;
     while (index != type_value_error)
     {
          entry = get_entry( index );
          //printf("Type value pool entry index %d at %p\n", index, entry );
          switch (entry->type_value_pool_entry_type)
          {
          case type_value_pool_entry_type_struct:
               entry->model_type = model->type_definition_structure_create( lex_string_from_terminal(entry->data.type_structure.symbol), 1, entry->data.type_structure.number_elements );
               if (MD_TYPE_DEFINITION_HANDLE_VALID(entry->model_type))
               {
                    for (i=0; i<entry->data.type_structure.number_elements; i++)
                    {
                         sub_model_type = get_model_type( model, entry->data.type_structure.elements[i].type_value );
                         if (MD_TYPE_DEFINITION_HANDLE_VALID(sub_model_type))
                         {
                              model->type_definition_structure_set_element( entry->model_type, lex_string_from_terminal(entry->data.type_structure.elements[i].symbol), 1, i, sub_model_type );
                         }
                    }
               }
               break;
          case type_value_pool_entry_type_type_def:
               entry->model_type = get_model_type( model, entry->data.type_def.type_value );
               break;
          case type_value_pool_entry_type_type_array:
               sub_model_type = get_model_type( model, entry->data.type_array.type_value );
               if (MD_TYPE_DEFINITION_HANDLE_VALID( sub_model_type) && (entry->data.type_array.resolved_size>=0))
               {
                    entry->model_type = model->type_definition_array_create( "__anon__array", 0, entry->data.type_array.resolved_size, sub_model_type );
               }
               break;
          case type_value_pool_entry_type_type_reference:
              //printf("\ttype_value_pool_entry_type_type_reference:\n");
               break;
          case type_value_pool_entry_type_variable:
               //printf("\ttype_value_pool_entry_type_variable:\n");
               break;
          case type_value_pool_entry_type_unresolved_variable:
               printf("\ttype_value_pool_entry_type_unresolved_variable:\n");
               break;
          case type_value_pool_entry_type_resolved_variable:
               //printf("\ttype_value_pool_entry_type_resolved_variable\n");
               break;
          case type_value_pool_entry_type_runtime_variable:
              //printf("\ttype_value_pool_entry_type_runtime_variable:\n");
               break;
          }
          switch (entry->type_reference)
          {
          case type_value_undefined:
              //printf("\ttype type_value_undefined\n");
               break;
          case type_value_error:
              //printf("\ttype type_value_error\n");
               break;
          case type_value_bit_array_size_1:
              //printf("\ttype type_value_bit_array_size_1\n");
              //printf("\tvalue %d mask %d\n", entry->data.value_bit.value, entry->data.value_bit.mask );
               break;
          case type_value_integer:
               //printf("\ttype type_value_integer\n");
               //printf("\tvalue 0x%08x\n", entry->data.value_integer );
               break;
          case type_value_string:
              //printf("\ttype type_value_string\n");
              //printf("\tlength %d value %s\n", entry->data.value_string.length, entry->data.value_string.string );
               break;
          default:
              //printf("\ttype index %d\n", entry->type_reference );
               break;
          }
          index=entry->next_in_list;
     }
}

/*a Debug methods
 */
char *c_type_value_pool::display( char *buffer, int buffer_size, t_type_value type_value )
{
    switch (type_value)
    {
    case type_value_undefined:
        snprintf( buffer, buffer_size, "<type undefined>" );
        break;
    case type_value_error:
        snprintf( buffer, buffer_size, "<type error>" );
        break;
    case type_value_bit_array_size_1:
        snprintf( buffer, buffer_size, "bit" );
        break;
    case type_value_bit_array_any_size:
        snprintf( buffer, buffer_size, "bit[]" );
        break;
    case type_value_integer:
        snprintf( buffer, buffer_size, "integer" );
        break;
    case type_value_string:
        snprintf( buffer, buffer_size, "string" );
        break;
    case type_value_bool:
        snprintf( buffer, buffer_size, "bool" );
        break;
    default:
        t_type_value_pool_entry *entry;
        entry = get_entry( type_value );
        switch (entry->type_value_pool_entry_type)
        {
        case type_value_pool_entry_type_struct:
            snprintf( buffer, buffer_size, "struct %s", lex_string_from_terminal(entry->data.type_structure.symbol) );
            break;
        case type_value_pool_entry_type_type_def:
            snprintf( buffer, buffer_size, "%s", lex_string_from_terminal(entry->data.type_def.symbol) );
            break;
        case type_value_pool_entry_type_type_array:
            display( buffer, buffer_size, entry->data.type_array.type_value );
            snprintf( buffer+strlen(buffer), buffer_size-strlen(buffer), "[%d]", entry->data.type_array.resolved_size );
            break;
        case type_value_pool_entry_type_type_reference:
            snprintf( buffer, buffer_size, "<type reference>");
            break;
        case type_value_pool_entry_type_variable:
            snprintf( buffer, buffer_size, "<variable>");
            break;
        case type_value_pool_entry_type_unresolved_variable:
            snprintf( buffer, buffer_size, "<unresolved variable>");
            break;
        case type_value_pool_entry_type_resolved_variable:
            display( buffer, buffer_size, entry->type_reference );
            if (derefs_to_bit_vector(type_value))
            {
                int size;
                t_type_value_bit_vector_data *bits, *mask;
                if (bit_vector_values( type_value, &size, &bits, &mask ))
                {
                    snprintf( buffer+strlen(buffer), buffer_size-strlen(buffer), " %llx mask %llx", bits?bits[0]:0, mask?mask[0]:0xdeaddead );
                }
            }
            else if (entry->type_reference==type_value_integer)
            {
                snprintf( buffer+strlen(buffer), buffer_size-strlen(buffer), "%lld", entry->data.value_integer);
            }
            else
            {
                snprintf( buffer+strlen(buffer), buffer_size-strlen(buffer), "<resolved variable>");
            }
            break;
        case type_value_pool_entry_type_runtime_variable:
            snprintf( buffer, buffer_size, "<runtime variable>");
            break;
          }
        break;
    }
    return buffer;
}
void c_type_value_pool::display( void )
{
     t_type_value index;
     t_type_value_pool_entry *entry;

     printf("******************************\n");
     printf("Type value pool display\n");

     if (tail_pool_index==type_value_error)
     {
          printf("Type value pool empty\n");
          return;
     }

     index = type_value_last_system;
     while (index != type_value_error)
     {
          entry = get_entry( index );
          printf("Type value pool entry index %d at %p\n", index, entry );
          switch (entry->type_value_pool_entry_type)
          {
          case type_value_pool_entry_type_struct:
               printf("\ttype_value_pool_entry_type_struct:\n");
               break;
          case type_value_pool_entry_type_type_def:
               printf("\ttype_value_pool_entry_type_type_def:\n");
               break;
          case type_value_pool_entry_type_type_array:
               printf("\ttype_value_pool_entry_type_type_array resolved size %d of type %d:\n", entry->data.type_array.resolved_size, entry->data.type_array.type_value);
               break;
          case type_value_pool_entry_type_type_reference:
               printf("\ttype_value_pool_entry_type_type_reference:\n");
               break;
          case type_value_pool_entry_type_variable:
               printf("\ttype_value_pool_entry_type_variable:\n");
               break;
          case type_value_pool_entry_type_unresolved_variable:
               printf("\ttype_value_pool_entry_type_unresolved_variable:\n");
               break;
          case type_value_pool_entry_type_resolved_variable:
               printf("\ttype_value_pool_entry_type_resolved_variable\n");
               break;
          case type_value_pool_entry_type_runtime_variable:
               printf("\ttype_value_pool_entry_type_runtime_variable:\n");
               break;
          }
          switch (entry->type_reference)
          {
          case type_value_undefined:
               printf("\ttype type_value_undefined\n");
               break;
          case type_value_error:
               printf("\ttype type_value_error\n");
               break;
          case type_value_bit_array_size_1:
               printf("\ttype type_value_bit_array_size_1\n");
               printf("\tvalue %lld mask %lld\n", entry->data.value_bit.value, entry->data.value_bit.mask );
               break;
          case type_value_integer:
               printf("\ttype type_value_integer\n");
               printf("\tvalue 0x%08llx\n", entry->data.value_integer );
               break;
          case type_value_string:
               printf("\ttype type_value_string\n");
               printf("\tlength %d value %s\n", entry->data.value_string.length, entry->data.value_string.string );
               break;
          default:
               printf("\ttype index %d\n", entry->type_reference );
               break;
          }
          index=entry->next_in_list;
     }
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

