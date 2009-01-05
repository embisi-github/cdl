/*a Copyright
  
  This file 'se_external_module.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "se_external_module.h"

/*a Defines
 */

/*a Types
 */
/*t t_engine_module
 */
typedef struct t_engine_module
{
     struct t_engine_module *next_module;
     char *name;
     void *handle;
     t_se_instantiation_fn instantiation_fn;
} t_engine_module;

/*a Statics
 */
/*v registered_module_list
 */
static t_engine_module *registered_module_list;

/*a External functions
 */
/*f se_external_module_find
 */
extern void *se_external_module_find( const char *name )
{
     t_engine_module *em;

     SL_DEBUG(sl_debug_level_verbose_info, "se_external_module_find", "Looking for module type '%s'", name ) ;

     for (em=registered_module_list; em; em=em->next_module)
     {
          if (!strcmp(em->name, name))
          {
               return (void *)em;
          }
     }
     SL_DEBUG(sl_debug_level_verbose_info, "se_external_module_find", "Failed to find module type '%s'", name ) ;
     return NULL;
}

/*f se_external_module_name(handle)
 */
extern char *se_external_module_name( void *handle )
{
     t_engine_module *em;
     em = (t_engine_module *)handle;
     return em->name;
}

/*f se_external_module_name(int)
 */
extern char *se_external_module_name( int n )
{
     t_engine_module *em;

     for (em=registered_module_list; (n>0) && em; n--,em=em->next_module);
     if (em)
         return em->name;
     return NULL;
}

/*f se_external_module_register
  register a module, checking the version number of the registration call first.
  return a handle for use with future instantiation calls, or NULL for failure.
 */
extern void se_external_module_register( int registration_version, const char *name, t_se_instantiation_fn instantiation_fn )
{
     t_engine_module *em;

     SL_DEBUG(sl_debug_level_info, "se_external_module_register", "Registering module type '%s'", name ) ;

     if (registration_version!=1)
     {
          return;
     }

     if (se_external_module_find(name))
     {
          return;
     }

     em = (t_engine_module *)malloc(sizeof(t_engine_module));
     em->next_module = registered_module_list;
     registered_module_list = em;
     em->name = sl_str_alloc_copy( name );

     em->instantiation_fn = instantiation_fn;

     return;
}

/*f se_external_module_instantiate
 */
extern t_sl_error_level se_external_module_instantiate( void *handle, c_engine *engine, void *instantiation_handle )
{
     t_engine_module *em;

     em = (t_engine_module *)handle;
     return em->instantiation_fn( engine, instantiation_handle );
}

/*f se_external_module_deregister
  deregister a module
 */
extern int se_external_module_deregister( const char *name )
{
     t_engine_module *em, *last_em;

     SL_DEBUG(sl_debug_level_info, "engine_deregister_module", "Deregistering module type '%s'", name ) ;

     last_em = NULL;
     for (em=registered_module_list; em; em=em->next_module)
     {
          if (!strcmp(em->name, name))
          {
               break;
          }
          last_em =em;
     }
     if (!em)
     {
          return 0;
     }

     if (!last_em)
     {
          registered_module_list = em->next_module;
     }
     else
     {
          last_em->next_module = em->next_module;
     }
     free(em->name);
     free(em);
     return 1;
}

/*f se_external_module_deregister_all
 */
extern void se_external_module_deregister_all( void )
{
     while (registered_module_list)
     {
          se_external_module_deregister( registered_module_list->name );
     }
}

/*f se_external_module_init
 */
extern void se_external_module_init( void )
{
     registered_module_list = NULL;
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

