/*a Copyright
  
  This file 'lexical_types.h' copyright Gavin J Stark 2003, 2004
  
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
#ifndef INC_LEXICAL_TYPES
#define INC_LEXICAL_TYPES

/*a Defines
 */
#define SIZED_INT_MAX_BITS 256

/*a Atoms - symbol, string, integer (sized and masked)
 */
/*t t_lex_file_posn
 */
typedef struct t_terminal_entry *t_lex_file_posn;

/*t t_type_value - offset into the pool of a typed_value structure
 */
typedef int t_type_value;

/*t type_value_*
    reserved indices
    undefined is used for types/values of expressions before an attempt has been made to think of resolving them
 */
enum
{
     type_value_undefined = -2,
     type_value_error = -1,
     type_value_bit_array_size_1 = 0,
     type_value_bit_array_any_size,
     type_value_integer,
     type_value_string,
     type_value_bool, // used to refer to items that are used as bools, such as if statement expressions - not a user-accessible type
     type_value_last_system
};

/*t t_symbol
 */
typedef struct t_symbol
{
     struct t_lex_symbol *lex_symbol;
     t_lex_file_posn file_posn;
     class c_cyc_object *user;
} t_symbol;

/*t t_string
 */
typedef struct t_string
{
     struct t_lex_string *lex_string;
     t_lex_file_posn file_posn;
     class c_cyc_object *user;
     char string[1];
} t_string;

/*t t_sized_int
 */
typedef struct t_sized_int
{
     t_lex_file_posn file_posn;
     class c_cyc_object *user;
     t_type_value type_value;
     class c_cyclicity *cyclicity;
} t_sized_int;

/*a End wrapper
 */
#endif




/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


