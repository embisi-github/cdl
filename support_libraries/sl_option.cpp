/*a Copyright
  
  This file 'sl_option.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "sl_option.h"
#include "sl_debug.h"

/*a Defines
 */

/*a Types
 */
/*t option_type_*
 */
enum
{
    option_type_int,
    option_type_int64,
    option_type_string,
    option_type_object,
};

/*t t_sl_option
 */
typedef struct t_sl_option
{
    struct t_sl_option *next_in_list;
    int type;
    int integer;
    t_sl_uint64 integer64;
    char *string;
    void *object;
    char keyword[1];
} t_sl_option;

/*a Option handling methods - called by modules instances at their instantiation
 */
/*f sl_option_find
 */
static t_sl_option *sl_option_find( t_sl_option *list, const char *keyword, int which )
{
     t_sl_option *opt;

     for (opt=list; opt && (which>=0); opt=opt->next_in_list)
     {
          if (!strcmp( opt->keyword, keyword ))
          {
              //fprintf(stderr,"Found possible option for keyword '%s' which %d, value %d/%s\n", opt->keyword, which, opt->integer, opt->string);
              if (which==0)
                  return opt;
              which--;
          }
     }
     return NULL;
}

/*f sl_option_get_int( list, keyword, which, *value ) - returns 1 for okay, 0 for failure
 */
extern int sl_option_get_int( t_sl_option *list, const char *keyword, int which, int *value )
{
     t_sl_option *opt;
     //fprintf(stderr,"Find int option %s\n", keyword);
     opt = sl_option_find( list, keyword, which );
     if (!opt)
         return 0;
     switch (opt->type)
     {
     case option_type_int:    *value=opt->integer; return 1;
     case option_type_int64:  *value=(int)(opt->integer64); return 1;
     case option_type_string: return (sscanf(opt->string,"%d",value)==1);
     case option_type_object: *value=0; return 1;
     }
     return 0;
}

/*f sl_option_get_int( list, keyword, default value ) - return value
 */
extern int sl_option_get_int( t_sl_option *list, const char *keyword, int default_value )
{
    int result;
    result = default_value;
    sl_option_get_int( list, keyword, 0, &result );
    return result;
}

/*f sl_option_get_int64( list, keyword, which, *value ) - returns 1 for okay, 0 for failure
 */
extern int sl_option_get_int64( t_sl_option *list, const char *keyword, int which, t_sl_uint64 *value )
{
     t_sl_option *opt;
     //fprintf(stderr,"Find int64 option %s\n", keyword);
     opt = sl_option_find( list, keyword, which );
     if (!opt)
         return 0;
     switch (opt->type)
     {
     case option_type_int:    *value=(t_sl_uint64)opt->integer; return 1;
     case option_type_int64:  *value=(t_sl_uint64)(opt->integer64); return 1;
     case option_type_string: return (sscanf(opt->string,"%lld",value)==1);
     case option_type_object: *value=0; return 1;
     }
     return 0;
}

/*f sl_option_get_double( list, keyword, which, *value ) - returns 1 for okay, 0 for failure
 */
extern int sl_option_get_double( t_sl_option *list, const char *keyword, int which, double *value )
{
     t_sl_option *opt;
     opt = sl_option_find( list, keyword, which );
     if (!opt)
         return 0;
     *value=0;
     return 0; // Double is not supported yet
}

/*f sl_option_get_double( list, keyword, default value ) - return value
 */
extern double sl_option_get_double( t_sl_option *list, const char *keyword, double default_value )
{
    double result;
    result = default_value;
    sl_option_get_double( list, keyword, 0, &result );
    return result;
}

/*f sl_option_get_string( list, keyword, which, *value ) - returns 1 for okay, 0 for failure
 */
extern int sl_option_get_string( t_sl_option *list, const char *keyword, int which, const char **value )
{
     t_sl_option *opt;
     opt = sl_option_find( list, keyword, which );
     if (!opt)
         return 0;
     *value = opt->string;
     return 1;
}

/*f sl_option_get_string_with_default( list, keyword, default value ) - return value
 */
extern const char *sl_option_get_string_with_default( t_sl_option *list, const char *keyword, const char *default_value )
{
    const char *result;
    result = default_value;
    sl_option_get_string( list, keyword, 0, &result );
    return result;
}

/*f sl_option_get_string( list, keyword ) - return value or NULL
 */
extern const char *sl_option_get_string( t_sl_option *list, const char *keyword )
{
    const char *result;
    if (!sl_option_get_string( list, keyword, 0, &result ))
        return NULL;
    return result;
}

/*f sl_option_get_string( list, keyword ) - return value or NULL
 */
extern const char *sl_option_get_string( void *list, const char *keyword )
{
    t_sl_option *olist;
    olist = (t_sl_option *)list;
    return sl_option_get_string( olist, keyword );
}

/*f sl_option_get_object( list, keyword ) - return value or NULL
 */
extern void *sl_option_get_object( t_sl_option *list, const char *keyword )
{
    t_sl_option *opt;
    //fprintf(stderr,"Find int option %s\n", keyword);
    opt = sl_option_find( list, keyword, 0 );
    if (!opt)
        return NULL;
    switch (opt->type)
    {
    case option_type_object: return opt->object;
    }
    return NULL;
}

/*f sl_option_list (object)
 */
extern t_sl_option *sl_option_list( t_sl_option *list, const char *keyword, void *object )
{
     t_sl_option *opt;
     opt = (t_sl_option *)malloc(sizeof(t_sl_option)+strlen(keyword)+1);
     opt->next_in_list = list;
     opt->type = option_type_object;
     strcpy( opt->keyword, keyword );
     Py_INCREF(object);
     opt->object = object;
     return opt;
}

/*f sl_option_list (integer)
 */
extern t_sl_option *sl_option_list( t_sl_option *list, const char *keyword, int value )
{
     t_sl_option *opt;
     opt = (t_sl_option *)malloc(sizeof(t_sl_option)+strlen(keyword)+32);
     opt->next_in_list = list;
     opt->type = option_type_int;
     strcpy( opt->keyword, keyword );
     opt->integer = value;
     opt->integer64 = (t_sl_uint64)value;
     opt->string = opt->keyword+strlen(keyword)+1;
     snprintf( opt->string, 32, "%d", value );
     //fprintf(stderr,"Created int option %s value %d/%s\n", opt->keyword, opt->integer, opt->string);
     return opt;
}

/*f sl_option_list (integer64)
 */
extern t_sl_option *sl_option_list( t_sl_option *list, const char *keyword, t_sl_uint64 value )
{
     t_sl_option *opt;
     opt = (t_sl_option *)malloc(sizeof(t_sl_option)+strlen(keyword)+32);
     opt->next_in_list = list;
     opt->type = option_type_int64;
     strcpy( opt->keyword, keyword );
     opt->integer = (int)value;
     opt->integer64 = value;
     opt->string = opt->keyword+strlen(keyword)+1;
     snprintf( opt->string, 32, "%lld", value );
     //fprintf(stderr,"Created int64 option %s value %lld/%s\n", opt->keyword, opt->integer64, opt->string);
     return opt;
}

/*f sl_option_list (string)
 */
extern t_sl_option *sl_option_list( t_sl_option *list, const char *keyword, const char *string)
{
     t_sl_option *opt;
     opt = (t_sl_option *)malloc(sizeof(t_sl_option)+strlen(keyword)+strlen(string)+1);
     opt->next_in_list = list;
     opt->type = option_type_string;
     strcpy( opt->keyword, keyword );
     opt->string = opt->keyword+strlen(keyword)+1;
     strcpy( opt->string, string );
     //fprintf(stderr,"Created string option %s value %s\n", opt->keyword, opt->string);
     return opt;
}

/*f sl_option_list_copy_item 
 */
extern t_sl_option *sl_option_list_copy_item( t_sl_option *item )
{
    int size;
    t_sl_option *new_item;

    if (!item) return NULL;
    size = sizeof(t_sl_option)+strlen(item->keyword)+strlen(item->string)+1;
    new_item = (t_sl_option *)malloc(size);
    if (!new_item) return NULL;
    new_item->next_in_list = NULL;
    new_item->type = item->type;
    strcpy( new_item->keyword, item->keyword );
    new_item->string = new_item->keyword+strlen(new_item->keyword)+1;
    strcpy( new_item->string, item->string );
    switch (item->type)
    {
    case option_type_string:
        break;
    case option_type_int:
        new_item->integer = item->integer;
        break;
    case option_type_int64:
        new_item->integer64 = item->integer64;
        break;
    }
    return new_item;
}

/*f sl_option_list_prepend - prepend item to list by copying item
 */
extern t_sl_option *sl_option_list_prepend( t_sl_option *list, t_sl_option *item )
{
    item = sl_option_list_copy_item( item );
    if (!item)
        return list;

    if (list)
    {
        item->next_in_list = list;
    }
    return item;
}

/*f sl_option_free_list
 */
extern void sl_option_free_list( t_sl_option *list )
{
     t_sl_option *next_item;

     while (list)
     {
          next_item = list->next_in_list;
          if (list->type == option_type_object)
              Py_DECREF(list->object);
          free(list);
          list = next_item;
     }
}

/*a External functions for use with getopt
 */
/*f sl_option_getopt_usage
 */
extern void sl_option_getopt_usage( void )
{
     printf( "\t--sl_debug \t\tEnable debugging\n");
     printf( "\t--sl_debug_level <level> \tSet level of debugging information and enable\n\t\t\t\t debugging\n");
     printf( "\t--sl_debug_file <file> \tSet file to output debug to (else stdout)\n");
}

/*f sl_option_handle_getopt
 */
extern int sl_option_handle_getopt( t_sl_option_list *env_options, int c, const char *optarg )
{
     int i;
     FILE *debug_file;
     switch (c)
     {
     case option_sl_debug_level:
          if (sscanf(optarg,"%d", &i )==1)
               sl_debug_set_level( (t_sl_debug_level) i );
          sl_debug_enable(1);
          return 1;
     case option_sl_debug_file:
          debug_file = fopen(optarg,"w");
          if (debug_file)
               sl_debug_set_file( debug_file );
          return 1;
     case option_sl_debug:
          sl_debug_enable(1);
         return 1;
     }
     return 0;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

