/*a Copyright
  
  This file 'c_se_engine__waveform.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "se_engine_function.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*t t_waveform_vcd_file_signal_entry
 */
typedef struct t_waveform_vcd_file_signal_entry
{
     char code[8]; // 7 characters + NULL is more than enough for any VCD file code
     t_se_signal_value *data;
     t_se_signal_value *last_data;
     int bit_size;
     int int_size;
     char *name; // mallocked fully scoped name
} t_waveform_vcd_file_signal_entry;

/*t t_waveform_vcd_file_entity_list
 */
typedef struct t_waveform_vcd_file_entity_list
{
     struct t_waveform_vcd_file_entity_list *next_in_list;
     t_se_interrogation_handle ih;
} t_waveform_vcd_file_entity_list;
    

/*t t_waveform_vcd_file
 */
typedef struct t_waveform_vcd_file
{
    struct t_waveform_vcd_file *next_in_list;
    c_engine *engine;
    char *name;
    char *filename;
    FILE *file;
    int enabled; // Asserted if VCD file output has been started, and the header has been output
    int paused; // Asserted if VCD file output should be paused
    int values_set; // Asserted if the values in the VCD files are set from the last event; if 0, then the last data needs to be set first
    t_waveform_vcd_file_entity_list *entities;
    int number_signals;
    t_waveform_vcd_file_signal_entry *vcd_signals; // Signal contents of all that will be in the VCD file - includes last and current values of signals
    struct t_se_engine_simulation_callback *callback;
} t_waveform_vcd_file;

/*a Static function declarations
 */
static t_sl_exec_file_method_fn ef_method_eval_open;
static t_sl_exec_file_method_fn ef_method_eval_add;
static t_sl_exec_file_method_fn ef_method_eval_add_hierarchy;
static t_sl_exec_file_method_fn ef_method_eval_enable;
static t_sl_exec_file_method_fn ef_method_eval_close;
static t_sl_exec_file_method_fn ef_method_eval_reset;
static t_sl_exec_file_method_fn ef_method_eval_pause;
static t_sl_exec_file_method_fn ef_method_eval_restart;
static t_sl_exec_file_method_fn ef_method_eval_file_size;

/*a Statics
 */
/*v cmd_*
 */
enum
{
    cmd_vcd_file,
};

/*v sim_file_cmds
 */
static t_sl_exec_file_cmd sim_file_cmds[] =
{
     {cmd_vcd_file,        1, "!vcd_file", "s", "vcd_file <name>"},
     SL_EXEC_FILE_CMD_NONE
};

/*v sl_vcd_file_obj_methods - Exec file object methods
 */
static t_sl_exec_file_method sl_vcd_file_object_methods[] =
{
    {"open",      'i',  1, "s", "open(<string filename>) - open a VCD file", ef_method_eval_open, NULL },
    {"add",       'i',  1, "sssssssssssssss",  "add(<name>[,<namel>]*) - add a number of signals", ef_method_eval_add, NULL },
    {"add_hierarchy", 'i',  1, "sssssssssssssss",  "add_hierarchy(<name>[,<namel>]*) - add a number of signals", ef_method_eval_add_hierarchy, NULL },
    {"enable",     0,   0, "",  "enable() - enable the VCD file - issue after all adds are done and when output should start", ef_method_eval_enable, NULL },
    {"close",      0,   0, "",  "close() - close the VCD file", ef_method_eval_close, NULL },
    {"reset",      0,   0, "",  "reset() - reset the VCD file", ef_method_eval_reset, NULL },
    {"pause",      0,   0, "",  "pause() - pause VCD file output", ef_method_eval_pause, NULL },
    {"restart",    0,   0, "",  "restart() - restart VCD file output if paused", ef_method_eval_restart, NULL },
    {"file_size", 'i',  0, "",  "file_size() - return the current size of the VCD file", ef_method_eval_file_size },
    SL_EXEC_FILE_METHOD_NONE
};

/*v dummy_signal_value
 */
static t_se_signal_value dummy_signal_value[16]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

/*a VCD waveform file functions
 */
/*f c_engine::waveform_vcd_file_find
 */
t_waveform_vcd_file *c_engine::waveform_vcd_file_find( const char *name )
{
     t_waveform_vcd_file *wvf;

     for (wvf=vcd_files; wvf; wvf=wvf->next_in_list)
     {
          if (!strcmp( name, wvf->name ))
               return wvf;
     }
     return NULL;
}

/*f c_engine::waveform_vcd_file_create
 */
t_waveform_vcd_file *c_engine::waveform_vcd_file_create( const char *name )
{
     t_waveform_vcd_file *wvf;

     SL_DEBUG( sl_debug_level_info, "c_engine::waveform_vcd_file_create", "New VCD file handle %s ", name );
     if (waveform_vcd_file_find( name ))
          return NULL;

     wvf = (t_waveform_vcd_file *)malloc(sizeof(t_waveform_vcd_file));
     if (!wvf)
          return NULL;
     wvf->name = sl_str_alloc_copy( name );
     wvf->next_in_list = vcd_files;
     vcd_files = wvf;
     wvf->engine = this;
     wvf->filename = NULL;
     wvf->file = NULL;
     wvf->enabled = 0;
     wvf->values_set = 0;
     wvf->entities = NULL;
     wvf->number_signals = 0;
     wvf->vcd_signals = NULL;
     wvf->callback = NULL;
     wvf->paused = 0;
     return wvf;
}

/*f c_engine::waveform_vcd_file_reset - return t_waveform_vcd_file to state just after create
 */
void c_engine::waveform_vcd_file_reset( t_waveform_vcd_file *wvf )
{
    t_waveform_vcd_file_entity_list *entity, *next_entity;
    if (!wvf)
        return;
    if (wvf->filename)
    {
        free(wvf->filename);
        wvf->filename = NULL;
    }
    if (wvf->file)
    {
        fclose(wvf->file);
        wvf->file = NULL;
    }
    for (entity=wvf->entities; entity; entity=next_entity)
    {
        next_entity = entity->next_in_list;
        interrogation_handle_free( entity->ih );
        free(entity);
    }
    wvf->entities = NULL;
    if (wvf->vcd_signals)
    {
        free(wvf->vcd_signals);
    }
    wvf->number_signals = 0;
    wvf->vcd_signals = NULL;
    if (wvf->callback)
    {
        simulation_free_callback( wvf->callback );
    }
    wvf->callback = NULL;
    wvf->enabled = 0;
    wvf->values_set = 0;
    wvf->paused = 0;
}

/*f c_engine::waveform_vcd_file_free
 */
void c_engine::waveform_vcd_file_free( t_waveform_vcd_file *wvf )
{
     t_waveform_vcd_file **last_wvf;

     if (!wvf)
          return;
     waveform_vcd_file_reset(wvf);
     for (last_wvf = &vcd_files; (*last_wvf) && ((*last_wvf)!=wvf); last_wvf=&((*last_wvf)->next_in_list));
     if ((*last_wvf) == wvf)
     {
          *last_wvf = wvf->next_in_list;
     }
     free(wvf);
}

/*f c_engine::waveform_vcd_file_add
 */
void c_engine::waveform_vcd_file_add( t_waveform_vcd_file *wvf, t_se_interrogation_handle entity )
{
     t_waveform_vcd_file_entity_list *wvfel;

     SL_DEBUG( sl_debug_level_info, "c_engine::waveform_vcd_file_add", "Adding to file %s entity %s", wvf->name, interrogate_get_entity_clue( entity ) );

     wvfel = (t_waveform_vcd_file_entity_list *)malloc(sizeof(t_waveform_vcd_file_entity_list));
     if (!wvfel)
          return;
     wvfel->next_in_list = wvf->entities;
     wvf->entities = wvfel;
     wvfel->ih = entity;
}

/*f c_engine::waveform_vcd_file_add_hierarchy
 */
void c_engine::waveform_vcd_file_add_hierarchy( t_waveform_vcd_file *wvf, t_se_interrogation_handle entity )
{
     int i;

     SL_DEBUG( sl_debug_level_info, "c_engine::waveform_vcd_file_add_hierarchy", "Adding to file %s entity %s", wvf->name, interrogate_get_entity_clue( entity ) );

     waveform_vcd_file_add( wvf, entity );

    for (i=0; ; i++)
    {
        t_se_interrogation_handle sub_module;
        sub_module = NULL;
        if (!interrogate_enumerate_hierarchy( entity, i, engine_state_desc_type_mask_none, engine_interrogate_include_mask_submodules, &sub_module ))
            break;
        waveform_vcd_file_add_hierarchy( wvf, sub_module );
    }
}

/*f c_engine::waveform_vcd_file_count_and_fill_signals
  We need to count the number of bit-vectors that we can put into the VCD file
  We then need to fill the structure passed in, with the size, data location, and full name
 */
int c_engine::waveform_vcd_file_count_and_fill_signals( t_waveform_vcd_file_entity_list *wvfel, int number_so_far, t_waveform_vcd_file_signal_entry *signals )
{
     int i, j, k, l;
     t_se_interrogation_handle local_ih;
     char buffer[1024];
     t_se_signal_value *datas[4];
     int sizes[4];

     //fprintf(stderr,"c_engine::waveform_vcd_file_count_and_fill_signals:Called with %p %d %p\n", wvfel, number_so_far, signals );
     if (!wvfel)
          return number_so_far;

     if (0 && !signals)
     {
          number_so_far += interrogate_count_hierarchy( wvfel->ih,
                                                        (t_engine_state_desc_type_mask) (engine_state_desc_type_mask_bits | engine_state_desc_type_mask_memory),
                                                        (t_engine_interrogate_include_mask)(engine_interrogate_include_mask_state | engine_interrogate_include_mask_inputs | engine_interrogate_include_mask_outputs) );
          //fprintf(stderr,"c_engine::waveform_vcd_file_count_and_fill_signals:Counted so far %d\n", number_so_far );
     }
     else
     {
         //fprintf(stderr,"c_engine::waveform_vcd_file_count_and_fill_signals:So far %d %p\n", number_so_far, signals );
          local_ih = NULL;
          for (i=0; ; i++)
          {
               if (interrogate_enumerate_hierarchy( wvfel->ih,
                                                    i,
                                                    (t_engine_state_desc_type_mask) (engine_state_desc_type_mask_bits | engine_state_desc_type_mask_memory),
                                                    (t_engine_interrogate_include_mask)(engine_interrogate_include_mask_state | engine_interrogate_include_mask_inputs | engine_interrogate_include_mask_outputs),
                                                    &local_ih ))
               {
                   t_engine_state_desc_type type;
                   int breakout_to;
                   type = interrogate_get_data_sizes_and_type( local_ih, datas, sizes );
                   breakout_to = 0;
                   if (type==engine_state_desc_type_memory)
                   {
                       breakout_to = sizes[1];
                       if (breakout_to<0) breakout_to=0;
                       if (breakout_to>32) breakout_to=32;
                   }
                   if (signals)
                   {
                       int element;
                       interrogate_get_entity_path_string( local_ih, buffer, sizeof(buffer) );
                       char rename_buffer[1024];
                       //fprintf(stderr,"c_engine::waveform_vcd_file_count_and_fill_signals:Now %d %p %s\n", number_so_far, signals, buffer );
                       for (element=0; (element<breakout_to)||((element==0)&&(breakout_to==0)); element++)
                       {
                           if (breakout_to)
                           {
                               snprintf( rename_buffer, sizeof(rename_buffer), "%s__%d", buffer, element );
                               signals[number_so_far].name = sl_str_alloc_copy( rename_buffer );
                           }
                           else
                           {
                               signals[number_so_far].name = sl_str_alloc_copy( buffer );
                           }
                           signals[number_so_far].bit_size = sizes[0];
                           signals[number_so_far].int_size = (sizes[0]+63)/64;
                           signals[number_so_far].data = datas[0]+element;
                           signals[number_so_far].last_data = (t_se_signal_value *)malloc(sizeof(t_se_signal_value)*signals[number_so_far].int_size);
                           for (k=0; k<signals[number_so_far].int_size; k++)
                           {
                               signals[number_so_far].last_data[k] = 0;
                           }
                           k = 0;
                           l = 52*52*52;
                           while (l>=1)
                           {
                               j = ((number_so_far+1)/l)%52;
                               if ((j>0) || (k>0))
                               {
                                   signals[number_so_far].code[k]=(j<26)?(j+'A'):(j-26+'a');
                                   k++;
                               }
                               l=l/52;
                           }
                           signals[number_so_far].code[k]=0;
                           number_so_far++;
                       }
                   }
                   else
                   {
                       number_so_far += (breakout_to==0)?1:breakout_to;
                   }
               }
               else
               {
                    break;
               }
          }
     }
     return waveform_vcd_file_count_and_fill_signals( wvfel->next_in_list, number_so_far, signals );
}

/*f waveform_vcd_file_callback_fn
 */
static int waveform_vcd_file_callback_fn( void *handle, void *handle_b )
{
     c_engine *engine;
     engine = (c_engine *)handle;
     engine->waveform_vcd_file_callback( (t_waveform_vcd_file *) handle_b );
     return 0;
}

/*f c_engine::waveform_vcd_file_callback
 */
void c_engine::waveform_vcd_file_callback( struct t_waveform_vcd_file *wvf )
{
     int i, j, k;
     int first;
     if ((!wvf) || (!wvf->enabled) || (!wvf->file) || wvf->paused)
          return;

     first = 1;
     for (i=0; i<wvf->number_signals; i++)
     {
         if (!wvf->vcd_signals[i].data)
         {
             fprintf( stderr, "WAVEFORM ERROR: No data for signal %d '%s' - is it wired up?\n", i, wvf->vcd_signals[i].name );
             wvf->vcd_signals[i].data = dummy_signal_value;
         }
         if (!wvf->values_set)
         {
             k=1;
         }
         else
         {
             for (j=k=0; j<wvf->vcd_signals[i].int_size; j++)
             {
                 if (wvf->vcd_signals[i].data[j] != wvf->vcd_signals[i].last_data[j])
                 {
                     k=1;
                     break;
                 }
             }
         }
         if (k)
         {
             if (first)
             {
                 fprintf( wvf->file, "#%d\n", cycle() );
                 first = 0;
             }
             for (j=0; j<wvf->vcd_signals[i].int_size; j++)
             {
                 {
                     wvf->vcd_signals[i].last_data[j] = wvf->vcd_signals[i].data[j];
                 }
             }
             if (wvf->vcd_signals[i].bit_size==1)
             {
                 fprintf( wvf->file, "%c%s\n", '0'+(int)((wvf->vcd_signals[i].data[0]&1)), wvf->vcd_signals[i].code );
             }
             else
             {
                 fputc( 'b', wvf->file );
                 for (j=wvf->vcd_signals[i].bit_size-1; j>=0; j--)
                 {
                     fputc( '0'+((wvf->vcd_signals[i].data[j/64]>>(j%64))&1), wvf->file );
                 }
                 fprintf( wvf->file, " %s\n", wvf->vcd_signals[i].code );
             }
         }
     }
     wvf->values_set = 1;
}

/*f waveform_vcd_file_compare_signal_names
 */
static int waveform_vcd_file_compare_signal_names( const void *a, const void *b )
{
     t_waveform_vcd_file_signal_entry *sa, *sb;
     sa = (t_waveform_vcd_file_signal_entry *)a;
     sb = (t_waveform_vcd_file_signal_entry *)b;
     if (strchr(sa->name, '.'))
     {
          if (!strchr(sb->name,'.'))
          {
               return 1;
          }
          return strcmp( sa->name, sb->name );
     }
     else if (strchr(sb->name,'.'))
     {
          return -1;
     }
     else
     {
          return strcmp( sa->name, sb->name );
     }
}

/*f c_engine::waveform_vcd_file_enable
  If already enabled, do nothing
  Mark as enabled
  Build monitoring table, assigning code to each signal
  Sort table according to name
  Output header from table, building scopes up appropriately
 */
void c_engine::waveform_vcd_file_enable( t_waveform_vcd_file *wvf )
{
     int number_signals;
     int i, j;
     char *scope, *new_scope;
     int scope_length, new_scope_length;
     char *ptr;

     SL_DEBUG( sl_debug_level_info, "c_engine::waveform_vcd_file_enable", "Enable vcd handle %s (%d)", wvf->name, wvf->enabled );

     if ((!wvf) || (wvf->enabled))
          return;
     wvf->enabled = 1;
     wvf->values_set = 0;
     wvf->number_signals = 0;

     if (!wvf->file)
          return;

     number_signals = waveform_vcd_file_count_and_fill_signals( wvf->entities, 0, NULL );
     wvf->vcd_signals = (t_waveform_vcd_file_signal_entry *)malloc(sizeof(t_waveform_vcd_file_signal_entry)*number_signals);
     if (!wvf->vcd_signals)
     {
          return;
     }
     for (i=0; i<number_signals; i++)
     {
         strcpy( wvf->vcd_signals[i].code, "_______" ); // Default code for a signal
          wvf->vcd_signals[i].name = NULL;
     }
     wvf->number_signals = waveform_vcd_file_count_and_fill_signals( wvf->entities, 0, wvf->vcd_signals );
     qsort( wvf->vcd_signals, wvf->number_signals, sizeof(t_waveform_vcd_file_signal_entry), waveform_vcd_file_compare_signal_names );
     scope = NULL;
     scope_length = -1; // -1 so that new_scope[scope_length+1] is the first character of the new scope
     for (i=1; i<wvf->number_signals; i++) // remove duplicates
     {
         if (!strcmp(wvf->vcd_signals[i].name, wvf->vcd_signals[i-1].name))
         {
             for (j=i; j+1<wvf->number_signals; j++)
             {
                 wvf->vcd_signals[j] = wvf->vcd_signals[j+1];
             }
             i--;
             wvf->number_signals--;
         }
     }
     for (i=0; i<wvf->number_signals; i++)
     {
          ptr = strrchr(wvf->vcd_signals[i].name, '.');
          if (ptr)
          {
               new_scope = wvf->vcd_signals[i].name;
               new_scope_length = ptr-new_scope;
               ptr++;
          }
          else
          {
               new_scope = NULL;
               new_scope_length = -1; // -1 so that new_scope[scope_length+1] when we get a scope works too
               ptr = wvf->vcd_signals[i].name;
          }
          while (scope) // Go up a scope until scope is the root of new_scope
          {
               if ( new_scope &&
                    !strncmp(scope, new_scope, scope_length) &&
                    (new_scope[scope_length]=='.') )
               {
                    break;
               }
               // Must remove tail of scope, reduce scope_length, go up a scope
               fprintf( wvf->file, "$upscope $end\n" );
               for (j=scope_length-1; j>=0; j--)
               {
                    if (scope[j]=='.')
                    {
                         break;
                    }
               }
               scope_length = j;
               if (j<=0)
               {
                    scope = NULL;
                    scope_length = -1; // -1 so that new_scope[scope_length+1] is the first character of the new scope
               }
          }
          if (new_scope) // Go down scopes from scope_length
          {
               scope = new_scope;
               while (scope_length<new_scope_length)
               {
                    fprintf( wvf->file, "$scope module " );
                    for (j = scope_length+1; new_scope[j]!='.'; j++)
                    {
                         fputc( new_scope[j], wvf->file );
                    }
                    scope_length = j;
                    fprintf( wvf->file, " $end\n" );
               }
          }
          fprintf( wvf->file, "$var wire %d %s %s $end\n", wvf->vcd_signals[i].bit_size, wvf->vcd_signals[i].code, ptr );
     }
     fprintf( wvf->file, "$enddefinitions $end\n" );
     wvf->callback = simulation_add_callback( waveform_vcd_file_callback_fn, (void *)this, (void *)wvf );
}

/*f c_engine::waveform_vcd_file_pause - Pause or unpause VCD file generations
 */
void c_engine::waveform_vcd_file_pause( t_waveform_vcd_file *wvf, int pause )
{
    wvf->paused = pause;
}

/*a Exec file evaluation functions
 */
/*f ef_method_eval_file_size
 */
static t_sl_error_level ef_method_eval_file_size( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    if ( !wvf->file )
        return error_level_fatal;

    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64) ftell(wvf->file) ))
        return error_level_fatal;
    return error_level_okay;
}

/*f ef_method_eval_open
 */
static t_sl_error_level ef_method_eval_open( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    if (wvf->file)
    {
        fclose(wvf->file);
        wvf->file = NULL;
    }
    if (wvf->filename)
    {
        free(wvf->filename);
        wvf->filename = NULL;
    }
    wvf->filename = sl_str_alloc_copy( sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ));
    wvf->file = fopen(wvf->filename, "w" );
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64) (wvf->file!=NULL) ))
        return error_level_fatal;
    return error_level_okay;
}

/*f ef_method_eval_close
 */
static t_sl_error_level ef_method_eval_close( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    if (wvf->file)
    {
        fclose(wvf->file);
        wvf->file = NULL;
    }
    if (wvf->filename)
    {
        free(wvf->filename);
        wvf->filename = NULL;
    }
    return error_level_okay;
}

/*f ef_method_eval_reset
 */
static t_sl_error_level ef_method_eval_reset( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    wvf->engine->waveform_vcd_file_reset( wvf );
    return error_level_okay;
}

/*f ef_method_eval_pause
 */
static t_sl_error_level ef_method_eval_pause( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    wvf->engine->waveform_vcd_file_pause( wvf, 1 );
    return error_level_okay;
}

/*f ef_method_eval_restart
 */
static t_sl_error_level ef_method_eval_restart( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    wvf->engine->waveform_vcd_file_pause( wvf, 0 );
    return error_level_okay;
}

/*f ef_method_eval_enable
 */
static t_sl_error_level ef_method_eval_enable( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;

    wvf = (t_waveform_vcd_file *)object_desc->handle;
    wvf->engine->waveform_vcd_file_enable( wvf );
    return error_level_okay;
}

/*f ef_method_eval_add
 */
static t_sl_error_level ef_method_eval_add( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;
    wvf = (t_waveform_vcd_file *)object_desc->handle;
    int i;
    int okay;
    t_se_interrogation_handle entity;

    okay = 1;
    for (i=0; i<cmd_cb->num_args; i++)
    {
        entity = wvf->engine->find_entity( sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, i ) );
        if (!entity)
        {
            fprintf(stderr, "NNE: entity '%s' not found in adding to VCD file\n", sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, i ) );
            okay = 0;
        }
        else
        {
            wvf->engine->waveform_vcd_file_add( wvf, entity );
        }
    }
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64) (okay) ))
        return error_level_fatal;
    return error_level_okay;
}

/*f ef_method_eval_add_hierarchy
 */
static t_sl_error_level ef_method_eval_add_hierarchy( t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, t_sl_exec_file_object_desc *object_desc, t_sl_exec_file_method *method )
{
    t_waveform_vcd_file *wvf;
    wvf = (t_waveform_vcd_file *)object_desc->handle;
    int i;
    int okay;
    t_se_interrogation_handle entity;

    okay = 1;
    for (i=0; i<cmd_cb->num_args; i++)
    {
        entity = wvf->engine->find_entity( sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, i ) );
        if (!entity)
        {
            fprintf(stderr, "NNE: entity '%s' not found in adding to VCD file\n", sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, i ) );
            okay = 0;
        }
        else
        {
            wvf->engine->waveform_vcd_file_add_hierarchy( wvf, entity );
        }
    }
    if (!sl_exec_file_eval_fn_set_result( cmd_cb->file_data, (t_sl_uint64) (okay) ))
        return error_level_fatal;
    return error_level_okay;
}

/*a Cycle simulation exec_file enhancement functions and methods
 */
/*f static exec_file_cmd_handler
 */
static t_sl_error_level exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    c_engine *engine;
    engine = (c_engine *)handle;
    if (!engine->waveform_handle_exec_file_command( cmd_cb->file_data, cmd_cb->cmd, cmd_cb->args ))
        return error_level_serious;
    return error_level_okay;
}

/*f c_engine::waveform_add_exec_file_enhancements
 */
int c_engine::waveform_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;
    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "cdlsim.wave";
    lib_desc.handle = (void *) this;
    lib_desc.cmd_handler = exec_file_cmd_handler;
    lib_desc.file_cmds = sim_file_cmds;
    lib_desc.file_fns = NULL;
    return sl_exec_file_add_library( file_data, &lib_desc );
}

/*f c_engine::waveform_handle_exec_file_command
  returns 0 if it did not handle
  returns 1 if it handled it
 */
int c_engine::waveform_handle_exec_file_command( struct t_sl_exec_file_data *exec_file_data, int cmd, struct t_sl_exec_file_value *args )
{
    t_waveform_vcd_file *wvf;
    t_sl_exec_file_object_desc object_desc;

    switch (cmd)
    {
    case cmd_vcd_file:
        wvf = waveform_vcd_file_create( sl_exec_file_eval_fn_get_argument_string( exec_file_data, args, 0 ));
        if (!wvf)
        {
            fprintf(stderr, "NNE: wavefrom failed to add VCD file\n" );
            return 0;
        }
        memset(&object_desc,0,sizeof(object_desc));
        object_desc.version = sl_ef_object_version_checkpoint_restore;
        object_desc.name = wvf->name;
        object_desc.handle = (void *)wvf;
        object_desc.methods = sl_vcd_file_object_methods;
        sl_exec_file_add_object_instance( exec_file_data, &object_desc );
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

