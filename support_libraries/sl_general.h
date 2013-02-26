/*a Copyright
  
  This file 'sl_general.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_GENERAL
#else
#define __INC_SL_GENERAL

/*a Includes
 */
#include "c_sl_error.h"

/*a Defines
 */
#define BITS_TO_INT64S(size) (((size-1)/32)+1)
#define BITS_TO_INTS(size) (((size-1)/32)+1)
#define BITS_TO_BYTES(size) (((size-1)/8)+1)

/*b Visual studio compatibility
 */
// For Windows, which does not support snprintf, remap it
#ifdef _MSC_VER 
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#define __func__ __FUNCTION__
#endif

/*a Types
 */
/*t uint and ints
 */
typedef unsigned long long t_sl_uint64;
typedef unsigned int t_sl_uint32;
typedef unsigned short t_sl_uint16;
typedef unsigned char t_sl_uint8;

typedef signed long long t_sl_sint64;
typedef signed int t_sl_sint32;
typedef signed short t_sl_sint16;
typedef signed char t_sl_sint8;

/*t t_sl_get_environment_fn
 */
typedef const char *(*t_sl_get_environment_fn)( void *handle, const char *name );

/*a External functions
 */
extern int sl_log2( unsigned int value );
extern unsigned int sl_str_hash( const char *string, int length );
extern char *sl_str_alloc_copy( const char *text );
extern char *sl_str_alloc_copy( const char *text, int length );
extern char *sl_str_copy_max( char *dest, const char *src, int max_length );
extern int sl_str_split( const char *src, char **dest, int argc, char **argv, int escape, int quote, int bracket ); // Split string src into args, ptrs in argv array, array length is given by argc; args may be quoted if quoted is true, or bracketed if bracket is true
extern t_sl_error_level sl_allocate_and_read_file( c_sl_error *error, int no_file_okay, const char *filename, char **file_data_ptr, const char *user );
extern t_sl_error_level sl_allocate_and_read_file( c_sl_error *error, const char *filename, char **file_data_ptr, const char *user );
extern void sl_print_bits_hex( char *buffer, int buffer_size, int *data, int bits );
extern int sl_integer_from_token( char *token, int *data );
extern int sl_integer_from_token( char *token, t_sl_uint64 *data );
extern double sl_double_from_token( char *token, double *data );

/*a Python compatibility
 */
#ifdef SL_PYTHON
#include <Python.h>
#else
struct _object;
typedef struct _object PyObject; // To make non-Python builds link
#define Py_INCREF(x) {}
#define Py_XINCREF(x) {}
#define Py_DECREF(x) {}
#define Py_XDECREF(x) {}
#define PyObject_CallMethod(a,...) (NULL)
#endif
/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

