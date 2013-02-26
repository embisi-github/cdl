/*a Copyright
  
  This file 'sl_general.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sl_general.h"

/*a Defines
 */

/*a Types
 */

/*a Static variables
 */

/*a Integer functions
 */
/*f sl_log2
 */
extern int sl_log2( unsigned int value )
{
    int i;
    for (i=0; (value>>i)!=0; i++);
    return i-1;
}

/*a String functions
 */
/*f sl_str_alloc_copy (string)
 */
extern char *sl_str_alloc_copy( const char *text )
{
     char *result;
     if (!text)
          return NULL;
     result = (char *)malloc(strlen(text)+1);
     if (result)
     {
          strcpy( result, text );
     }
     return result;
}

/*f sl_str_alloc_copy (string, length )
 */
extern char *sl_str_alloc_copy( const char *text, int length )
{
     char *result;
     if (!text)
          return NULL;
     result = (char *)malloc(length+1);
     if (result)
     {
          strncpy( result, text, length );
     }
     result[length]=0;
     return result;
}

/*f sl_str_copy_max
 */
extern char *sl_str_copy_max( char *dest, const char *src, int max_length )
{
     int i;
     for (i=0; (src[i]) && (i<max_length-1); i++)
     {
          dest[i] = src[i];
     }
     dest[i] = 0;
     return dest;
}

/*f sl_allocate_and_read_file
 */
extern t_sl_error_level sl_allocate_and_read_file( c_sl_error *error,
                                                        const char *filename,
                                                        char **file_data_ptr,
                                                        const char *user )
{
    return sl_allocate_and_read_file( error, 0, filename, file_data_ptr, user );
}

/*f sl_allocate_and_read_file
 */
extern t_sl_error_level sl_allocate_and_read_file( c_sl_error *error,
                                                   int no_file_okay,
                                                   const char *filename,
                                                   char **file_data_ptr,
                                                   const char *user )
{
     FILE *f;
     int length;

     if (*file_data_ptr)
     {
          free(*file_data_ptr);
          *file_data_ptr = NULL;
     }
     if (!filename)
     {
          return error->add_error( (void *)user, error_level_serious, error_number_general_empty_filename, error_id_sl_general_sl_allocate_and_read_file,
                                   error_arg_type_malloc_filename, user,
                                   error_arg_type_none );
     }
     f = fopen( filename, "r" );
     if (!f)
     {
         if (no_file_okay)
             return error_level_okay;

         return error->add_error( (void *)user, error_level_serious, error_number_general_bad_filename, error_id_sl_general_sl_allocate_and_read_file,
                                  error_arg_type_malloc_string, filename,
                                  error_arg_type_malloc_filename, user,
                                  error_arg_type_none );
     }
     fseek( f, 0,SEEK_END );
     length = ftell(f);
     fseek( f, 0, SEEK_SET );
     (*file_data_ptr) = (char *)malloc(length+1);
     if (!(*file_data_ptr))
     {
          return error->add_error( (void *)user, error_level_fatal, error_number_general_malloc_failed, error_id_sl_general_sl_allocate_and_read_file,
                                   error_arg_type_malloc_string, filename,
                                   error_arg_type_malloc_filename, user,
                                   error_arg_type_none );
     }
     (*file_data_ptr)[0] = 0;

     length = fread( *file_data_ptr, 1, length, f );
     (*file_data_ptr)[length] = 0;

     fclose(f);
     return error_level_okay;
}

/*f sl_print_bits_hex
 */
extern void sl_print_bits_hex( char *buffer, int buffer_size, int *data, int bits )
{
     int i, j, k;
     char fmt[16], *buf_ptr;

     i = bits;
     j = (bits-1)/32;
     buf_ptr = buffer;
     while (j>=0)
     {
          if (i%32!=0)
          {
               sprintf( fmt, "%%0%dx", ((i%32)+3)/4 );
               k = ((int *)data)[j];
               k &= ~(0xffffffff<<(i%32));
               snprintf( buf_ptr, buffer_size - (buf_ptr-buffer), fmt, k );
               buffer[buffer_size-1]=0;
          }
          else
          {
               snprintf( buf_ptr, buffer_size - (buf_ptr-buffer), "%08x", ((int *)data)[j] );
               buffer[buffer_size-1]=0;
          }
          buf_ptr = buf_ptr+strlen(buf_ptr);
          j--;
          i = (i-1)&~31;
     }
     buffer[buffer_size-1]=0;
}

/*f sl_integer_from_token - t_sl_uint64
 */
extern int sl_integer_from_token( char *token, t_sl_uint64 *data )
{
     int i;

     if (sscanf( token, "0x%llx", data)==1)
     {
          return 1;
     }
     else if (!strncmp( token, "0b", 2))
     {
          *data = 0;
          for (i=2; ; i++)
          {
               if (token[i]=='0')
               {
                    (*data)<<=1;
               }
               else if (token[i]=='1')
               {
                    (*data)<<=1;
                    (*data)|=1;
               }
               else if (token[i]=='_')
               {
               }
               else
               {
                    break;
               }
          }
          return 1;
     }
     else if (sscanf( token, "%lld", data)==1)
     {
          return 1;
     }
     return 0;
}

/*f sl_integer_from_token
 */
extern int sl_integer_from_token( char *token, int *data )
{
     int i;

     if (sscanf( token, "0x%x", data)==1)
     {
          return 1;
     }
     else if (!strncmp( token, "0b", 2))
     {
          *data = 0;
          for (i=2; ; i++)
          {
               if (token[i]=='0')
               {
                    (*data)<<=1;
               }
               else if (token[i]=='1')
               {
                    (*data)<<=1;
                    (*data)|=1;
               }
               else if (token[i]=='_')
               {
               }
               else
               {
                    break;
               }
          }
          return 1;
     }
     else if (sscanf( token, "%d", data)==1)
     {
          return 1;
     }
     return 0;
}

/*f sl_double_from_token
 */
extern double sl_double_from_token( char *token, double *data )
{
     if (sscanf( token, "%lf", data)==1)
     {
          return 1;
     }
     return 0;
}


/*f sl_str_split
 Split string src into args, ptrs in argv array, array length is given by argc
    *dest will be mallocked to be at least as long as src, and MUST BE FREED
    args may be quoted if quoted is true, or bracketed if bracket is true
 */
extern int sl_str_split( const char *src, char **dest, int argc, char **argv, int escape, int quote, int bracket )
{
    int i, j, arg;
    int in_quote, in_bracket, escaped, in_arg;

    if (!dest)
    {
        return 0;
    }
    if (!src)
    {
        *dest = NULL;
        return 0;
    }
    for (i=0; (src[i]) && (isspace(src[i])); i++);
    *dest = (char *)(malloc(strlen(src)+1));

    in_quote = 0;
    in_bracket = 0;
    escaped = 0;
    in_arg = 0;
    j=0;
    arg = 0;
    while (src[i])
    {
        if ((src[i]=='\\') && escape && !escaped)
        {
            escaped = 1;
            i++;
            continue;
        }
        if (!escaped)
        {
            if (src[i]==' ')
            {
                if (!in_quote && !in_bracket)
                {
                    if (in_arg)
                    {
                        (*dest)[j++] = 0;
                        i++;
                        in_arg = 0;
                        continue;
                    }
                    else
                    {
                        i++;
                        continue;
                    }
                }
            }
            if ((src[i]=='"') && (quote)) // Start of quote or end of quote
            {
                if (in_bracket && !in_quote) // if in bracket and not in quote then mark as in quote, and we will exit at the next quote
                {
                    (*dest)[j++] = src[i++];
                    in_quote = 1;
                    continue;
                }
                else if (in_bracket && in_quote) // if in bracket and in quote then mark as not in quote
                {
                    (*dest)[j++] = src[i++];
                    in_quote = 0;
                    continue;
                }
                else if (!in_quote) // Not in bracket, so must be a top level quote. If in arg, then don't mark as a quote; this is a bit of a user error. If not in arg, then mark as in_quote and skip the character and start arg
                {
                    if (!in_arg)
                    {
                        i++;
                        in_quote = 1;
                        in_arg = 1;
                        argv[arg] = (*dest)+j;
                        arg++;
                        if (arg>=argc) return argc;
                        continue;
                    }
                }
                else // Not in bracket but in quote - must be end of arg. End it cleanly.
                {
                    (*dest)[j++] = 0;
                    i++;
                    in_arg = 0;
                    in_quote = 0;
                    continue;
                }
            }
            if ((src[i]=='(') && (bracket)) // Start of bracket
            {
                if (!in_quote) // If not in quote then it matters; if in quote, we will treat it as a normal character
                {
                    if (!in_bracket) // Not in bracket (or quote), so if in arg then don't mark as bracket; this is a bit of user error. If no in arg then mark as in_bracket and start arg.
                    {
                        if (!in_arg)
                        {
                            in_arg = 1;
                            argv[arg] = (*dest)+j;
                            arg++;
                            if (arg>=argc) return argc;
                            (*dest)[j++] = src[i++];
                            in_bracket = 1;
                            continue;
                        }
                    }
                    else // In bracket, so just copy across and increment bracket count
                    {
                        (*dest)[j++] = src[i++];
                        in_bracket++;
                        continue;
                    }
                }
            }
            if ((src[i]==')') && (bracket)) // End of bracket
            {
                if (!in_quote) // If not in quote then it matters; if in quote, we will treat it as a normal character
                {
                    if (!in_bracket) // Not in bracket (or quote); user error really, and we can treat it as a normal character
                    {
                    }
                    else // In bracket (must be in_arg), so copy across and decrement bracket count
                    {
                        (*dest)[j++] = src[i++];
                        in_bracket--;
                        continue;
                    }
                }
            }
        }
        // Handle the character as part of an argument, with no special features. Copy it, and if not in an argument then start an argument
        if (!in_arg)
        {
            in_arg = 1;
            argv[arg] = (*dest)+j;
            arg++;
            if (arg>=argc) return argc;
        }
        (*dest)[j++] = src[i++];
    }
    (*dest)[j] = 0;
    return arg;
}

/*f sl_str_hash
 */
extern unsigned int sl_str_hash( const char *str, int length )
{
    unsigned int hash=5381; // This is from djb2
    char c=0;
    for (; ((c=str[0])!=0) && (length!=0); str++, length--) // Note, length!=0 allows length=-1 in as a 'until end of string'
    {
        hash = ((hash<<5)+hash)+c;
    }
    return hash;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

