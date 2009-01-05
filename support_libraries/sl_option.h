/*a Copyright
  
  This file 'sl_option.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_OPTION
#else
#define __INC_SL_OPTION

/*a Includes
 */
#include "sl_general.h"
#include "c_sl_error.h"

/*a Defines
 */
#define SL_OPTIONS \
     { "sl_debug", no_argument, NULL, option_sl_debug }, \
     { "sl_debug_level", required_argument, NULL, option_sl_debug_level }, \
     { "sl_debug_file", required_argument, NULL, option_sl_debug_file }, \

/*a Types
 */
/*t option_*start
 */
enum
{
     option_sl_start = 256,
     option_be_start = 512,
     option_ef_start = 768,
     option_cdl_start = 1024,
     option_se_start = 1280,
};

/*t option_sl_*
 */
enum
{
     option_sl_debug = option_sl_start,
     option_sl_debug_level,
     option_sl_debug_file
};

typedef struct t_sl_option *t_sl_option_list;

/*a External functions
 */
extern int sl_option_get_double( t_sl_option_list list, const char *keyword, int which, double *value );
extern double sl_option_get_double( t_sl_option_list list, const char *keyword, double default_value );
extern int sl_option_get_int( t_sl_option_list list, const char *keyword, int default_value );
extern int sl_option_get_int( t_sl_option_list list, const char *keyword, int which, int *value );
extern int sl_option_get_int64( t_sl_option_list list, const char *keyword, int which, t_sl_uint64 *value );
extern const char *sl_option_get_string_with_default( t_sl_option_list list, const char *keyword, const char *default_value );
extern int sl_option_get_string( t_sl_option_list list, const char *keyword, int which, const char **value );
extern const char *sl_option_get_string( void *list, const char *keyword );
extern const char *sl_option_get_string( t_sl_option_list list, const char *keyword );
extern t_sl_option_list sl_option_list( t_sl_option_list list, const char *keyword, double value );
extern t_sl_option_list sl_option_list( t_sl_option_list list, const char *keyword, int value );
extern t_sl_option_list sl_option_list( t_sl_option_list list, const char *keyword, t_sl_uint64 value );
extern t_sl_option_list sl_option_list( t_sl_option_list list, const char *keyword, const char *string);
extern t_sl_option_list sl_option_list_copy_item( t_sl_option_list item );
extern t_sl_option_list sl_option_list_prepend( t_sl_option_list list, t_sl_option_list item );
extern void sl_option_free_list( t_sl_option_list list );
extern void sl_option_getopt_usage( void );
extern int sl_option_handle_getopt( t_sl_option_list *env_options, int c, const char *optarg );

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

