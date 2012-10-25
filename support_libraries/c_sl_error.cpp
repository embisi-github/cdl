/*a Copyright
  
  This file 'c_sl_error.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "c_sl_error.h"
#include "sl_general.h"

/*a Types
 */
/*t t_error_arg
 */
typedef struct t_error_arg
{
     t_sl_error_arg_type type;
     int needs_to_be_freed;
     union
     {
         t_sl_uint64 uint64;
         t_sl_sint64 sint64;
         t_sl_uint32 uint32;
         t_sl_sint32 sint32;
         int integer;
     } integer;
     char *string;
     void *handle;
} t_error_arg;

/*t t_error
 */
typedef struct t_error
{
     struct t_error *next_in_list;

     t_sl_error_level error_level;
     int error_number;
     int function_id;
     void *location;
     int argc;
     t_error_arg args[1];
} t_error;

/*a Static variables
 */
/*v default_error_messages
 */
static t_sl_error_text default_error_messages[] = {
     C_SL_ERROR_DEFAULT_MESSAGES
};

/*a Constructors and destructors
 */
/*f c_sl_error::c_sl_error ( void )
 */
c_sl_error::c_sl_error( void )
{
     internal_create( 4, 8 );
}

/*f c_sl_error::c_sl_error ( size, args )
 */
c_sl_error::c_sl_error( int private_size, int max_args )
{
     internal_create( private_size, max_args );
}

/*f c_sl_error::internal_create
 */
void c_sl_error::internal_create( int private_size, int max_args )
{
     this->max_error_args = max_args;
     this->private_size = private_size;
     this->worst_error = error_level_okay;
     this->error_list = NULL;
     this->last_error = NULL;
     this->error_message_lists = NULL;
     this->function_message_lists = NULL;

     add_text_list( 1, default_error_messages );
}

/*f c_sl_error::~c_sl_error
 */
c_sl_error::~c_sl_error()
{
     t_error *error, *next_error;
     int i;

     for (error=error_list; error; error=next_error )
     {
          next_error = error->next_in_list;
          for (i=0; i<error->argc; i++)
          {
               if (error->args[i].needs_to_be_freed)
               {
                    if (error->args[i].string)
                         free(error->args[i].string);
                    if (error->args[i].handle)
                         free(error->args[i].handle);
               }
          }
     }
     error_list = NULL;
     last_error = NULL;
     worst_error = error_level_okay;
}

/*a Error handling methods
 */
/*f c_sl_error::reset
 */
void c_sl_error::reset( void )
{
     t_error *error, *next_error;

     //fprintf(stderr,"c_sl_error::reset %p\n", this );

     for (error=error_list; error; error=next_error )
     {
          next_error = error->next_in_list;
          free(error);
     }
     error_list = NULL;
     last_error = NULL;
}

/*f c_sl_error::add_text_list
        add at front so that the default messages are overriden by new messages
        the first argument is 0 for function ids, 1 for error messages
 */
int c_sl_error::add_text_list( int errors, t_sl_error_text *messages )
{
     t_sl_error_text_list *list;
     int i;

     //printf("%p:Add text list %d %p\n", this, errors, messages);
     list = (t_sl_error_text_list *)malloc(sizeof(t_sl_error_text_list));
     if (!list)
          return 0;
     if (errors)
     {
          list->next_in_list = error_message_lists;
          error_message_lists = list;
     }
     else
     {
          list->next_in_list = function_message_lists;
          function_message_lists = list;
     }
     list->messages = messages;
     for (i=0; ; i++)
     {
          if (!messages[i].message)
          {
               break;
          }
     }
     list->number_messages = i;
     //printf("Added %d messages\n",i);
     return 1;
}

/*f c_sl_error::add_error
 */
t_sl_error_level c_sl_error::add_error( void *location, t_sl_error_level error_level, int error_number, int function_id, ... )
{
     va_list ap;
     va_start( ap, function_id );
     t_sl_error_level r = add_error( location, error_level, error_number, function_id, ap );
     va_end( ap );
     return r;
}

/*f c_sl_error::add_error
 */
t_sl_error_level c_sl_error::add_error( void *location, t_sl_error_level error_level, int error_number, int function_id, va_list ap )
{
     t_error *error;
     int i;
     t_sl_error_arg_type arg_type;

     //fprintf(stderr,"c_sl_error::add error %p (location %p) level %d number %d function %d\n", this, location, error_level, error_number, function_id );

     if ((int)error_level > (int)worst_error)
     {
          worst_error = error_level;
     }

     error = (t_error *)malloc(sizeof(t_error)+sizeof(t_error_arg)*max_error_args);
     if (!error)
          return error_level_fatal;

     error->next_in_list = NULL;
     if (last_error)
     {
          last_error->next_in_list = error;
     }
     else
     {
          error_list = error;
     }
     last_error = error;

     error->error_level = error_level;
     error->error_number = error_number;
     error->function_id = function_id;
     error->location = location;
     error->argc = 0;

     for (i=0; i<max_error_args; i++)
     {
          arg_type = (t_sl_error_arg_type)va_arg( ap, int ); // not t_sl_error_arg_type in the va_arg
          switch (arg_type)
          {
          case error_arg_type_integer:
          case error_arg_type_line_number:
               error->args[i].type = arg_type;
               error->args[i].integer.integer = (int)va_arg( ap, int);
               error->args[i].string = NULL;
               error->args[i].handle = NULL;
               error->args[i].needs_to_be_freed = 0;
               break;
          case error_arg_type_uint32:
          case error_arg_type_sint32:
               error->args[i].type = arg_type;
               error->args[i].integer.sint32 = (t_sl_sint32)va_arg( ap, t_sl_sint32);
               error->args[i].string = NULL;
               error->args[i].handle = NULL;
               error->args[i].needs_to_be_freed = 0;
               break;
          case error_arg_type_uint64:
          case error_arg_type_sint64:
               error->args[i].type = arg_type;
               error->args[i].integer.sint64 = (t_sl_sint64)va_arg( ap, t_sl_sint64);
               error->args[i].string = NULL;
               error->args[i].handle = NULL;
               error->args[i].needs_to_be_freed = 0;
               break;
          case error_arg_type_const_string:
          case error_arg_type_const_filename:
               error->args[i].type = arg_type;
               error->args[i].integer.sint64 = 0;
               error->args[i].string = (char *)va_arg( ap, char *);
               //printf("Const string %p from %s\n",error->args[i].string,error->args[i].string); 
               error->args[i].handle = NULL;
               error->args[i].needs_to_be_freed = 0;
               break;
          case error_arg_type_malloc_filename:
          case error_arg_type_malloc_string:
               error->args[i].type = arg_type;
               error->args[i].integer.sint64 = 0;
               error->args[i].string = sl_str_alloc_copy((char *)va_arg( ap, char *));
               //printf("Allocated string %p\n",error->args[i].string);
               error->args[i].handle = NULL;
               error->args[i].needs_to_be_freed = 1;
               break;
          case error_arg_type_const_private_data:
               error->args[i].type = arg_type;
               error->args[i].integer.sint64 = 0;
               error->args[i].string = NULL;
               error->args[i].handle = (void *)va_arg( ap, void *);
               error->args[i].needs_to_be_freed = 0;
               break;
          case error_arg_type_malloc_private_data:
               error->args[i].type = arg_type;
               error->args[i].integer.sint64 = 0;
               error->args[i].string = NULL;
               error->args[i].handle = malloc( private_size );
               if (error->args[i].handle)
               {
                    memcpy( error->args[i].handle, (void *)va_arg( ap, void *), private_size );
                    error->args[i].needs_to_be_freed = 1;
               }
               break;
          case error_arg_type_none:
               error->args[i].type = arg_type;
               error->args[i].integer.sint64 = 0;
               error->args[i].string = NULL;
               error->args[i].handle = NULL;
               error->args[i].needs_to_be_freed = 0;
               error->argc = i;
               i = max_error_args;
               break;
          }
     }

     return error_level;
}

/*f c_sl_error::add_error_lite
 */
t_sl_error_level c_sl_error::add_error_lite( void *location, t_sl_error_level error_level, const char *format, ... )
{
    char buffer[1024];
    va_list ap;
    va_start( ap, format );
    vsnprintf( buffer, sizeof(buffer), format, ap );
    va_end( ap );
    return add_error( location, error_level, error_number_general_error_s, error_id_sl_lite,
                      error_arg_type_malloc_string, buffer,
                      error_arg_type_const_filename, "<lite error>", 
                      error_arg_type_none );
}

/*f c_sl_error::get_error_count
 */
int c_sl_error::get_error_count( t_sl_error_level error_level )
{
     int i;
     t_error *error;

     i = 0;
     for (error=error_list; error; error=error->next_in_list)
     {
          if ((int)error->error_level >= (int)error_level)
          {
               i++;
          }
     }
     return i;
}

/*f c_sl_error::get_next_error
 */
void *c_sl_error::get_next_error( void *handle, t_sl_error_level error_level )
{
     int i;
     t_error *error;

     if (!handle)
     {
          error = error_list;
     }
     else
     {
          error = ((t_error *)handle)->next_in_list;
     }
     i = 0;
     for (; error; error=error->next_in_list)
     {
          if ((int)error->error_level >= (int)error_level)
          {
               return (void *)error;
          }
     }
     return NULL;
}

/*f c_sl_error::get_nth_error
 */
void *c_sl_error::get_nth_error( int n, t_sl_error_level error_level )
{
     int i;
     t_error *error;

     error = NULL;
     for (i=0; (i<=n) && !((i>0) && (error==NULL)); i++)
     {
         error = (t_error *)get_next_error( (void *)error, error_level );
     }
     return error;
}

/*f c_sl_error::get_error_level
 */
t_sl_error_level c_sl_error::get_error_level( void )
{
    return worst_error;
}

/*f c_sl_error::generate_error_message
 */
static int copy_to_sized_buffer( char **buffer, int *buffer_size, const char *text, int length )
{
     if (length<0)
          length = strlen(text);
     if (*buffer_size<length+1)
     {
          return 0;
     }
     memcpy( *buffer, text, length );
     *buffer += length;
     *buffer_size -= length;
     (*buffer)[0] = 0;
     return 1;
}

int c_sl_error::generate_error_message( void *handle, char *buffer, int buffer_size, int verbosity, void *callback )
{
     t_error *error;
     t_sl_error_text_list *text_list;
     const char *message;
     const char *function;
     char temp[32];
     int i, j, k;

     if (!handle)
     {
          return 0;
     }

     error = (t_error *)handle;

     /*b Find the text for the error message
      */
     message = NULL;
     for (text_list = error_message_lists; (!message) && text_list; text_list=text_list->next_in_list)
     {
          for (i=0; (!message) && (i<text_list->number_messages); i++)
          {
              //printf("%p:%s:%d:%d:%d\n", this, text_list->messages[i].message, i, error->error_number,text_list->messages[i].number);
               if (error->error_number==text_list->messages[i].number)
               {
                    message = text_list->messages[i].message;
               }
          }
     }
     if (!message)
          message = "Error type %E";

     /*b Find the text for the function id
      */
     function = NULL;
     for (text_list = function_message_lists; (!function) && text_list; text_list=text_list->next_in_list)
     {
          for (i=0; (!function) && (i<text_list->number_messages); i++)
          {
               if (error->error_number==text_list->messages[i].number)
               {
                    function = text_list->messages[i].message;
               }
          }
     }
     if (!function)
          function = "Function %F";

     i=0;
     while (1)
     {
          if (message[i]==0)
          {
               break;
          }
          while (message[i]=='%')
          {
               switch (message[i+1])
               {
               case 's':
                    k = message[i+2]-'0';
                    if ((k<0) || (k>=max_error_args))
                         return 0;
                    if (error->args[k].string)
                    {
                         if (!copy_to_sized_buffer( &buffer, &buffer_size, error->args[k].string, -1))
                              return 0;
                    }
                    else
                    {
                         if (!copy_to_sized_buffer( &buffer, &buffer_size, "<null>", -1))
                              return 0;
                    }
                    i=i+3;
                    break;
               case 'd':
                    k = message[i+2]-'0';
                    if ((k<0) || (k>=max_error_args))
                         return 0;
                    sprintf(temp, "%d",error->args[k].integer.integer); 
                    if (!copy_to_sized_buffer( &buffer, &buffer_size, temp, -1))
                         return 0;
                    i=i+3;
                    break;
               case 'f':
                    for (k=0; k<error->argc; k++)
                         if ( (error->args[k].type == error_arg_type_const_filename) ||
                              (error->args[k].type == error_arg_type_malloc_filename) )
                              break;
                    if ( (k<error->argc) && (error->args[k].string) )
                    {
                         if (!copy_to_sized_buffer( &buffer, &buffer_size, error->args[k].string, -1))
                              return 0;
                    }
                    else
                    {
                         if (!copy_to_sized_buffer( &buffer, &buffer_size, "<unknown file>", -1))
                              return 0;
                    }
                    i=i+2;
                    break;
               case 'l': // line_number, if given, else ?
                    for (k=0; k<error->argc; k++)
                         if (error->args[k].type == error_arg_type_line_number)
                              break;
                    if (k<error->argc)
                    {
                         sprintf( temp, "%d", error->args[k].integer.integer );
                         if (!copy_to_sized_buffer( &buffer, &buffer_size, temp, -1))
                              return 0;
                    }
                    else
                    {
                         if (!copy_to_sized_buffer( &buffer, &buffer_size, "?", -1))
                              return 0;
                    }
                    i=i+2;
                    break;
               case 'E':
                    sprintf( temp, "%d", error->error_number );
                    if (!copy_to_sized_buffer( &buffer, &buffer_size, temp, -1))
                         return 0;
                    i=i+2;
                    break;
               default:
                    return 0;
               }
          }
          for (j=i; message[j] && (message[j]!='%'); j++);
          if (!copy_to_sized_buffer( &buffer, &buffer_size, message+i, j-i))
               return 0;
          i=j;
     }
     
     return 1;
}

/*a Display methods
*/
/*f c_sl_error::check_errors - to file
 */
int c_sl_error::check_errors( FILE *f, t_sl_error_level error_level_display, t_sl_error_level error_level_count )
{
     char error_buffer[1024];
     void *handle;

     //fprintf(stderr,"c_sl_error::check_errors:%p:display level %d count level %d\n", this, error_level_display, error_level_count );

     if (get_error_count( error_level_display ))
     {
          handle = get_next_error( NULL, error_level_display );
          //fprintf(stderr,"c_sl_error::check_errors:%p:handle %p\n", this, handle );
          for (; handle; handle = get_next_error( handle, error_level_display ))
          {
               generate_error_message( handle, error_buffer, 1024, 1, NULL );
               fprintf( f, "%s\n",error_buffer);
          }
     }
     return (get_error_count( error_level_count )>0);
}

/*f c_sl_error::check_errors_and_reset - to file
 */
int c_sl_error::check_errors_and_reset( FILE *f, t_sl_error_level error_level_display, t_sl_error_level error_level_count )
{
     int result;
     result = check_errors( f, error_level_display, error_level_count );
     reset();
     return result;
}

/*f c_sl_error::check_errors - to buffer
 */
int c_sl_error::check_errors_and_reset( char *str, size_t str_length, t_sl_error_level error_level_display, t_sl_error_level error_level_count, void **handle )
{
    char error_buffer[1024];
    int errors_found;

    //fprintf(stderr,"c_sl_error::check_errors:%p:display level %d count level %d\n", this, error_level_display, error_level_count );
    if (str_length<sizeof(error_buffer))
    {
        fprintf(stderr,"c_sl_error::check_errors:BUG:MUST BE CALLED WITH 'str_length' > size of error buffer, preferably 16x\n");
    }
    errors_found = 0;
    if (get_error_count( error_level_display ))
    {
        //fprintf(stderr,"c_sl_error::check_errors:%p:handle %p\n", this, handle );
        while (str_length>sizeof(error_buffer))
        {
            *handle = get_next_error( *handle, error_level_display );
            if (!*handle)
            {
                break;
            }
            generate_error_message( *handle, error_buffer, sizeof(error_buffer), 1, NULL );
            copy_to_sized_buffer( &str, (int*)&str_length, error_buffer, -1 );
            if (str_length)
            {
                *str++ = '\n';
                *str = '\0';
                str_length--;
                errors_found++;
            }
        }
        if (!*handle)
        {
            reset();
        }
    }
    return errors_found;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

