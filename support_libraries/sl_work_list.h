/*a Copyright
  
  This file 'sl_work_list.h' copyright Gavin J Stark 2011
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_WORK_LIST
#else
#define __INC_SL_WORK_LIST

/*a Includes
 */
#include "sl_general.h"

/*a Defines
 */

/*a Types
 */
/*t t_wl_affinity
 */
typedef struct
{
    int core; // not currently used
} t_wl_affinity;

/*t t_sl_wl_callback_fn
 */
typedef void (*t_sl_wl_callback_fn)( void *handle );

/*t t_sl_wl_thread_pool_ptr
 */
typedef struct t_sl_wl_thread_pool *t_sl_wl_thread_pool_ptr;

/*t t_sl_wl_thread_affinity_ptr
 */
typedef void *t_sl_wl_thread_affinity_ptr;

/*t t_sl_wl_head
 */
typedef struct
{
    t_sl_wl_thread_affinity_ptr affinity;
    const char *name;
    const char *subname;
    int *guard_ptr;
} t_sl_wl_head;

/*t t_sl_wl_data
 */
typedef struct
{
    t_sl_wl_callback_fn callback;
    void *handle;
} t_sl_wl_data;

/*t t_sl_wl_worklist
 */
typedef struct t_sl_wl_worklist
{
    struct t_sl_wl_worklist *next_in_list;
    char *name;
    t_sl_wl_thread_pool_ptr thread_pool;
    int number_of_items;
    int data_per_item; // 2 for two function calls per work item
    t_sl_wl_data *data; // pointer to data for each entry index by (item*data_per_item+entry_in_item)
    t_sl_wl_head heads[1]; // heads for all 'number_of_items' items
} t_sl_wl_worklist;

/*a External functions
 */
/*f sl_wl_create_thread_pool
 */
extern t_sl_wl_thread_pool_ptr sl_wl_create_thread_pool( void );

/*f sl_wl_delete_thread_pool
 */
extern t_sl_error_level sl_wl_delete_thread_pool( t_sl_wl_thread_pool_ptr thread_pool );

/*f sl_wl_spawn_worker_thread
  Spawn a worker thread
 */
extern t_sl_error_level sl_wl_spawn_worker_thread( t_sl_wl_thread_pool_ptr thread_pool, const char *thread_name, t_wl_affinity *affinity );

/*f sl_wl_terminate_worker_thread
  Terminate a worker thread
 */
extern int sl_wl_terminate_worker_thread( t_sl_wl_thread_pool_ptr thread_pool, const char *thread_name );

/*f sl_wl_terminate_all_worker_threads
  Terminate all worker threads
 */
extern void sl_wl_terminate_all_worker_threads( t_sl_wl_thread_pool_ptr thread_pool );

/*f sl_wl_add_worklist
  Add a worklist; set each item to default thread affinity
  The name is copied
 */
extern t_sl_wl_worklist *sl_wl_add_worklist( t_sl_wl_thread_pool_ptr thread_pool, const char *name, int number_of_items, int data_per_item );

/*f sl_wl_find_worklist
  Find a previously-added worklist
 */
extern t_sl_wl_worklist *sl_wl_find_worklist( t_sl_wl_thread_pool_ptr thread_pool, const char *name );

/*f sl_wl_delete_worklist
  Delete a worklist - MUST NOT BE CALLED FROM WITHIN A WORK ITEM
 */
extern void sl_wl_delete_worklist( t_sl_wl_thread_pool_ptr thread_pool, t_sl_wl_worklist *worklist );

/*f sl_wl_set_work_head
  The name and subname ARE NOT COPIED
 */
extern void sl_wl_set_work_head( t_sl_wl_worklist *worklist, int item, const char *name, const char *subname, int *guard_ptr );

/*f sl_wl_set_work_item
 */
extern void sl_wl_set_work_item( t_sl_wl_worklist *worklist, int item, int entry_number, t_sl_wl_callback_fn callback, void *handle );

/*f sl_wl_assign_work_to_thread
 */
extern t_sl_error_level sl_wl_assign_work_to_thread( t_sl_wl_worklist *worklist, int item, const char *thread_name );

/*f sl_wl_execute_worklist
  Execute every item on the worklist, except if the guard bit corresponding to the item is
 */
extern t_sl_error_level sl_wl_execute_worklist( t_sl_wl_worklist *worklist,
                                                int *guard_bits,
                                                int entry_number ); // which of the entry_in_item function to call

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/
