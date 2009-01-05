/*a Copyright
  
  This file 'se_engine_function.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Outline
  Models register with this base engine when in cycle simulation mode
  They register by first declaring themselves, and getting a handle in return
  They are then instantiated by this base engine, at which point:
    they may attach themselves to a clock, and specify a reset, preclock, clock and combinatorial functions
    they may register input and output signals
    they may specify which signals depend/are generated on what clocks or combinatorially
    they may declare state that may be interrogated

  The base engine can then have global buses/signals instantiated, and this wires things up
  Inputs to modules are implemented by setting an 'int **' in the module to point to an 'int *' which is an output of another module.
  Inputs, outputs and global signals all have a bit length (size) which must be specified.

 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_timer.h"
#include "se_engine_function.h"

/*a Signal reference function add
 */
/*f se_engine_signal_reference_add
 */
extern void se_engine_signal_reference_add( t_engine_signal_reference **ref_list_ptr, t_engine_signal *signal )
{
     t_engine_signal_reference *efr;

     efr = (t_engine_signal_reference *)malloc(sizeof(t_engine_signal_reference));
     efr->next_in_list = *ref_list_ptr;
     *ref_list_ptr = efr;
     efr->signal = signal;
}

/*a Engine function external functions
 */
/*f se_engine_function_free_functions
 */
extern void se_engine_function_free_functions( t_engine_function *list )
{
     t_engine_function *efn, *next_efn;

     for (efn = list; efn; efn=next_efn )
     {
          next_efn = efn->next_in_list;
          if (efn->name)
          {
               free(efn->name);
          }
          free( efn );
     }
}

/*f se_engine_function_add_function
 */
extern t_engine_function *se_engine_function_add_function( t_engine_module_instance *emi, t_engine_function **efn_list, const char *name )
{
     t_engine_function *efn;

     efn = (t_engine_function *)malloc(sizeof(t_engine_function));
     efn->next_in_list = *efn_list;
     *efn_list = efn;
     efn->module_instance = emi;
     efn->name = name?sl_str_alloc_copy(name) : NULL;
     return efn;
}

/*f se_engine_function_find_function
 */
extern t_engine_function *se_engine_function_find_function( t_engine_function *efn_list, const char *name )
{
     t_engine_function *efn;

     for (efn=efn_list; efn; efn=efn->next_in_list)
     {
          if ((efn->name) && (!strcmp(efn->name,name)))
          {
               return efn;
          }
     }
     return NULL;
}

/*a Engine function reference external functions
 */
/*f se_engine_function_references_free
 */
extern void se_engine_function_references_free( t_engine_function_reference *list )
{
     t_engine_function_reference *efr, *next_efr;

     for (efr = list; efr; efr=next_efr )
     {
          next_efr = efr->next_in_list;
          free( efr );
     }
}

/*f se_engine_function_references_add
 */
extern void se_engine_function_references_add( t_engine_function_reference **ref_list_ptr, t_engine_function *efn )
{
     t_engine_function_reference *efr;

     efr = (t_engine_function_reference *)malloc(sizeof(t_engine_function_reference));
     efr->next_in_list = *ref_list_ptr;
     *ref_list_ptr = efr;
     efr->signal = efn;
}

/*a Engine function reference external functions
 */
/*f se_engine_function_call_add
 */
extern t_engine_function_list *se_engine_function_call_add( t_engine_function_list **list_ptr, void *handle, t_engine_callback_fn callback_fn )
{
     t_engine_function_list *efl, **efl_prev;

     for (efl_prev = list_ptr; (*efl_prev); efl_prev = &((*efl_prev)->next_in_list) );

     efl = (t_engine_function_list *)malloc(sizeof(t_engine_function_list));
     efl->next_in_list = NULL;
     efl->signal = NULL;
     efl->handle = handle;
     efl->callback_fn = callback_fn;
     efl->invocation_count = 0;
     SL_TIMER_INIT(efl->timer);
     *efl_prev = efl;
     return efl;
}

/*f se_engine_function_call_add
 */
extern void se_engine_function_call_add( t_engine_function_list **list_ptr, t_engine_function_reference *efr, t_engine_callback_fn callback_fn )
{
     t_engine_function_list *efl;

     efl = se_engine_function_call_add( list_ptr, efr->signal->handle, callback_fn );
     efl->signal = efr->signal;
}

/*f se_engine_function_call_add
 */
extern t_engine_function_list *se_engine_function_call_add( t_engine_function_list **list_ptr, void *handle, t_engine_callback_arg_fn callback_arg_fn )
{
     return se_engine_function_call_add( list_ptr, handle, (t_engine_callback_fn) callback_arg_fn );
}

/*f se_engine_function_call_invoke_all
 */
extern void se_engine_function_call_invoke_all( t_engine_function_list *list )
{
    t_engine_function_list *efl;
    for (efl=list; efl; efl=efl->next_in_list)
    {
        SL_TIMER_ENTRY(efl->timer);
        efl->invocation_count++;
        (efl->callback_fn)( efl->handle );
        SL_TIMER_EXIT(efl->timer);
    }
}

/*f se_engine_function_call_invoke_all_arg
 */
extern void se_engine_function_call_invoke_all_arg( t_engine_function_list *list, int arg )
{
    t_engine_function_list *efl;
    for (efl=list; efl; efl=efl->next_in_list)
    {
        SL_TIMER_ENTRY(efl->timer);
        efl->invocation_count++;
        ((t_engine_callback_arg_fn)(efl->callback_fn))( efl->handle, arg );
        SL_TIMER_EXIT(efl->timer);
    }
}

/*f se_engine_function_call_display_stats_all
 */
extern void se_engine_function_call_display_stats_all( FILE *f, t_engine_function_list *list )
{
    t_engine_function_list *efl;
    for (efl=list; efl; efl=efl->next_in_list)
    {
        fprintf( f, "EFL module %s clock %s invoked %d times for a total time of %lf usecs\n", efl->signal->module_instance->full_name, efl->signal->name, efl->invocation_count, SL_TIMER_VALUE_US(efl->timer));
    }
}

/*f se_engine_function_call_free
 */
extern void se_engine_function_call_free( t_engine_function_list *list_ptr )
{
     t_engine_function_list *efl, *next_efl;

     for (efl = list_ptr; efl; efl=next_efl )
     {
          next_efl = efl->next_in_list;
          free( efl );
     }
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

