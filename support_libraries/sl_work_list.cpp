/*a Copyright
  
  This file 'sl_work_list.cpp' copyright Gavin J Stark, 2011
  
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include "sl_debug.h"
#include "sl_timer.h"
#include "sl_general.h"
#include "sl_work_list.h"

/*a Defines
 */

/*a Types
 */
/*t t_sl_wl_thread
 */
typedef struct t_sl_wl_thread
{
    struct t_sl_wl_thread *next_in_list;
    pthread_t id;
    pthread_mutex_t mutex_start;
    pthread_cond_t cond_start;
    int state_start;
    pthread_mutex_t mutex_done;
    pthread_cond_t cond_done;
    int state_done;
    int alive;
    int busy;
    t_sl_wl_worklist *worklist; // Work to do
    int *guard_bits; // Whether to do the work (1 bit per item)
    int entry_number; // which function to call per item to execute if it is this thread
    t_sl_timer timer;
    char name[1]; // Must be at the end
} t_sl_wl_thread;

/*t t_sl_wl_thread_pool
 */
typedef struct t_sl_wl_thread_pool
{
    t_sl_wl_thread *threads;
    t_sl_wl_worklist *worklists;
} t_sl_wl_thread_pool;

/*a Thread functions
  */
/*f thread_sync

  We want to sync parent thread with a child
  We want a 'sync' primitive that both can use
  Use a mutex for playing with sync - so we have a critical code section that only one thread can be in
  We have a state variable too controlled by the mutex
  If the state variable is 1, then the other end is waiting and can be signalled
  If the state variable is 0, then the neither end is waiting, so set to 1 and wait

 */
static int thread_sync( t_sl_wl_thread *thread )
{
    int alive;
    pthread_mutex_lock( &thread->mutex_start );
    if (thread->state_start) // other end is waiting
    {
        thread->state_start=0;
        pthread_cond_signal( &thread->cond_start );
    }
    else // other end is not waiting
    {
        thread->state_start=1;
        pthread_cond_wait( &thread->cond_start, &thread->mutex_start );
    }
    alive = thread->alive;
    pthread_mutex_unlock( &thread->mutex_start );
    return alive;
}

/*f thread_sync_kill
 */
static void thread_sync_kill( t_sl_wl_thread *thread )
{
    pthread_mutex_lock( &thread->mutex_start );
    thread->alive=0;
    if (thread->state_start) // other end is waiting
    {
        thread->state_start=0;
        pthread_cond_signal( &thread->cond_start );
    }
    else // other end is not waiting
    {
        thread->state_start=1;
        pthread_cond_wait( &thread->cond_start, &thread->mutex_start );
    }
    printf("Thread %s killed total work time %f us\n", thread->name, SL_TIMER_VALUE_US(thread->timer));
    pthread_mutex_unlock( &thread->mutex_start );
}

/*f do_thread_work
  Do all the work on the worklist for this thread
 */
static void do_thread_work( t_sl_wl_worklist *worklist, int *guard_bits, int entry_number, t_sl_wl_thread *thread )
{
    int i;
    int data_per_item;
    int guard=0;

    if (thread)
    {
        SL_TIMER_ENTRY(thread->timer);
    }       
    //printf("Doing work %p worklist %p\n", pthread_self(), worklist);

    if (!worklist)
        return;

    data_per_item = worklist->data_per_item;

    for (i=0; i<worklist->number_of_items; i++)
    {
        //printf("Worklist item %d, %x, %p\n", i, guard, guard_bits );
        if (guard_bits)
        {
            if ((i % (8*sizeof(int)))==0)
            {
                guard = *guard_bits++;
                //printf("guard now %08x\n", guard );
            }
            if ((guard&1)==0)
            {
                //printf("Skipping\n", i, guard );
                continue;
            }
            guard=guard>>1;
        }

        if (worklist->heads[i].affinity != (void *)thread)
        {
            //printf("Itme for other thread %p %p\n", thread, worklist->heads[i].affinity);
            continue;
        }

        if (worklist->heads[i].guard_ptr)
            if (worklist->heads[i].guard_ptr[0]==0)
                continue;

        t_sl_wl_data *data_ptr = &(worklist->data[i*data_per_item+entry_number]);
        if (data_ptr->callback)
            data_ptr->callback(data_ptr->handle);
    }

    if (thread)
    {
        SL_TIMER_EXIT(thread->timer);
        //printf("Done work %p total time %f\n", pthread_self(), SL_TIMER_VALUE_US(thread->timer));
    }   
    else
    {
        //printf("Done work for thread %p\n", pthread_self() );
    }
}

/*f worker_thread
  Main loop for a worker thread 
 */
void *worker_thread( void *handle )
{
    t_sl_wl_thread *thread = (t_sl_wl_thread *)handle;
    //printf("Worker thread %p\n", thread);
    while (1)
    {
        //printf("Syncin %p\n", thread);
        if (!thread_sync( thread ))
            break;
        //printf("Woken thread %p\n", thread);
        do_thread_work( thread->worklist, thread->guard_bits, thread->entry_number, thread );
        thread->worklist = NULL;
        thread->busy = 0;
        thread_sync( thread );
        //printf("Loopin %p\n", thread);
    }
    //printf("Worker thread done %p\n", thread);
    pthread_exit(NULL);
}

/*f thread_kill
 */
static t_sl_error_level thread_kill( t_sl_wl_thread *thread )
{
    if (thread->busy)
    {
        fprintf(stderr,"Attempt to terminate a thread that was busy '%s'\n", thread->name);
        return error_level_fatal;
    }

    thread_sync_kill( thread );
    return error_level_okay;
}

/*a Thread pool creation/deletion functions, add/find/delete thread from pool
 */
/*f sl_wl_create_thread_pool
 */
extern t_sl_wl_thread_pool_ptr sl_wl_create_thread_pool( void )
{
    t_sl_wl_thread_pool_ptr thread_pool;

    thread_pool = (t_sl_wl_thread_pool_ptr) malloc(sizeof(t_sl_wl_thread_pool));
    if (!thread_pool)
        return NULL;

    thread_pool->threads = NULL;
    return thread_pool;
}

/*f sl_wl_delete_thread_pool
 */
extern t_sl_error_level sl_wl_delete_thread_pool( t_sl_wl_thread_pool_ptr thread_pool )
{
    if (thread_pool->threads)
        return error_level_fatal;

    free(thread_pool);
    return error_level_okay;
}

/*f thread_add
 */
static t_sl_wl_thread *thread_add( t_sl_wl_thread_pool_ptr thread_pool, const char *thread_name )
{
    t_sl_wl_thread *thread;
    int rc;

    if (!thread_pool)
        return NULL;

    thread = (t_sl_wl_thread *)malloc(sizeof(t_sl_wl_thread)+strlen(thread_name));

    rc = pthread_mutex_init( &thread->mutex_start, NULL );
    if (rc==0) rc=pthread_mutex_init( &thread->mutex_done, NULL );
    if (rc==0) rc=pthread_cond_init( &thread->cond_start, NULL );
    if (rc==0) rc=pthread_cond_init( &thread->cond_done, NULL );
    
    if (rc!=0)
    {
        free(thread);
        return NULL;
    }

    thread->next_in_list = thread_pool->threads;
    thread_pool->threads = thread;
    strcpy( thread->name, thread_name );
    thread->alive=1;
    thread->busy=0;
    thread->worklist = NULL;
    SL_TIMER_INIT( thread->timer );

    return thread;
}

/*f thread_find
 */
static t_sl_wl_thread *thread_find( t_sl_wl_thread_pool_ptr thread_pool, const char *thread_name )
{
    t_sl_wl_thread *thread;
    if (!thread_pool)
        return NULL;
    for (thread=thread_pool->threads; thread; thread=thread->next_in_list)
        if (!strcmp(thread->name,thread_name))
            return thread;

    return NULL;
}

/*f thread_delete
 */
static void thread_delete( t_sl_wl_thread_pool_ptr thread_pool, t_sl_wl_thread *thread )
{
    t_sl_wl_thread **thread_ptr;
    for ( thread_ptr=&(thread_pool->threads); (*thread_ptr); thread_ptr=&((*thread_ptr)->next_in_list) )
    {
        if ((*thread_ptr)==thread)
        {
            *thread_ptr = thread->next_in_list;
            break;
        }
    }

    pthread_cond_destroy( &thread->cond_start );
    pthread_cond_destroy( &thread->cond_done );
}

/*a Thread start/stop functions
 */
/*f sl_wl_spawn_worker_thread
  Spawn a worker thread
 */
extern t_sl_error_level sl_wl_spawn_worker_thread( t_sl_wl_thread_pool_ptr thread_pool, const char *thread_name, t_wl_affinity *affinity )
{
    t_sl_wl_thread *thread;
    thread = thread_add( thread_pool, thread_name );

    if (!thread)
        return error_level_fatal;

    if (pthread_create( &thread->id, NULL, worker_thread, (void *)thread )!=0)
        return error_level_fatal;

    return error_level_okay;
}

/*f sl_wl_find_thread
 */
extern t_sl_wl_thread_ptr sl_wl_find_thread( t_sl_wl_thread_pool_ptr thread_pool, const char *name )
{
    return thread_find( thread_pool, name );
}

/*f sl_wl_thread_name
 */
extern const char *sl_wl_thread_name( t_sl_wl_thread_ptr thread_ptr )
{
    return thread_ptr->name;
}

/*f sl_wl_terminate_worker_thread
  Terminate a worker thread
 */
int sl_wl_terminate_worker_thread( t_sl_wl_thread_pool_ptr thread_pool, const char *thread_name )
{
    t_sl_wl_thread *thread;
    thread = thread_find( thread_pool, thread_name );
    if (!thread)
    {
        fprintf(stderr,"Failed to find thread '%s'\n", thread_name);
        return error_level_fatal;
    }

    thread_kill( thread );
    thread_delete( thread_pool, thread );
    return error_level_okay;
}

/*f sl_wl_terminate_all_worker_threads
  Terminate all worker threads
 */
extern void sl_wl_terminate_all_worker_threads( t_sl_wl_thread_pool_ptr thread_pool )
{
    t_sl_wl_thread *thread;
    while ((thread=thread_pool->threads)!=NULL)
    {
        thread_kill( thread );
        thread_delete( thread_pool, thread );
    }
}

/*a Worklist manipluation functions
 */
/*f sl_wl_add_worklist
  Add a worklist; set each item to default thread affinity
  The name is copied
 */
extern t_sl_wl_worklist *sl_wl_add_worklist( t_sl_wl_thread_pool_ptr thread_pool, const char *name, int number_of_items, int data_per_item )
{
    t_sl_wl_worklist *worklist;
    int i, j;

    worklist = (t_sl_wl_worklist *) malloc( sizeof(t_sl_wl_worklist)+sizeof(t_sl_wl_head)*number_of_items+sizeof(t_sl_wl_data)*number_of_items*data_per_item+strlen(name) );
    worklist->data = (t_sl_wl_data *)(&((char *)worklist)[sizeof(t_sl_wl_worklist)+sizeof(t_sl_wl_head)*number_of_items]);
    worklist->name = (&((char *)worklist)[sizeof(t_sl_wl_worklist)+sizeof(t_sl_wl_head)*number_of_items+sizeof(t_sl_wl_data)*number_of_items*data_per_item]);

    worklist->next_in_list = thread_pool->worklists;
    thread_pool->worklists = worklist;

    worklist->thread_pool = thread_pool;
    worklist->number_of_items = number_of_items;
    worklist->data_per_item = data_per_item;
    strcpy( worklist->name, name );

    for (i=0; i<number_of_items; i++)
    {
        worklist->heads[i].name = NULL;
        worklist->heads[i].subname = NULL;
        worklist->heads[i].guard_ptr = NULL;
        worklist->heads[i].affinity = NULL;
        for (j=0; j<data_per_item; j++)
        {
            worklist->data[i*data_per_item+j].callback=NULL;
            worklist->data[i*data_per_item+j].handle=NULL;
        }
    }
    return worklist;
}

/*f sl_wl_find_worklist
  Find a previously-added worklist
 */
extern t_sl_wl_worklist *sl_wl_find_worklist( t_sl_wl_thread_pool_ptr thread_pool, const char *name )
{
    t_sl_wl_worklist *worklist;
    for (worklist = thread_pool->worklists; worklist; worklist=worklist->next_in_list)
        if (!strcpy(worklist->name,name))
            return worklist;
    return NULL;
}

/*f sl_wl_delete_worklist
  Delete a worklist - MUST NOT BE CALLED FROM WITHIN A WORK ITEM
 */
extern void sl_wl_delete_worklist( t_sl_wl_thread_pool_ptr thread_pool, t_sl_wl_worklist *worklist )
{
    t_sl_wl_worklist **worklist_ptr;
    for (worklist_ptr=&thread_pool->worklists; (*worklist_ptr); worklist_ptr=&((*worklist_ptr)->next_in_list))
    {
        if ((*worklist_ptr)==worklist)
        {
            *worklist_ptr = worklist->next_in_list;
            break;
        }
    }
}

/*f sl_wl_set_work_head
 */
extern void sl_wl_set_work_head( t_sl_wl_worklist *worklist, int item, const char *name, const char *subname, int *guard_ptr )
{
    if ((item>=0) && (item<worklist->number_of_items))
    {
        worklist->heads[item].name = name;
        worklist->heads[item].subname = subname;
        worklist->heads[item].guard_ptr = guard_ptr;
    }
}

/*f sl_wl_set_work_item
 */
extern void sl_wl_set_work_item( t_sl_wl_worklist *worklist, int item, int entry_number, t_sl_wl_callback_fn callback, void *handle )
{
    if ((item>=0) && (item<worklist->number_of_items))
    {
        if ((entry_number>=0) && (entry_number<=worklist->data_per_item))
        {
            worklist->data[item*worklist->data_per_item+entry_number].callback = callback;
            worklist->data[item*worklist->data_per_item+entry_number].handle = handle;
        }
    }
}

/*f sl_wl_assign_work_to_thread
 */
extern t_sl_error_level sl_wl_assign_work_to_thread( t_sl_wl_worklist *worklist, int item, const char *thread_name )
{
    t_sl_wl_thread *thread;
    thread = thread_find( worklist->thread_pool, thread_name );
    if (!thread)
        return error_level_serious;

    printf("Assigning work item %d to thread %p\n",item, thread);
    if ((item>=0) && (item<worklist->number_of_items))
    {
        worklist->heads[item].affinity = (void *)thread;
        return error_level_okay;
    }
    return error_level_serious;
}

/*a Work execution functions
 */
/*f sl_wl_execute_worklist
  Execute every item on the worklist, except if the guard bit corresponding to the item is
 */
extern t_sl_error_level sl_wl_execute_worklist( t_sl_wl_worklist *worklist, int *guard_bits, int entry_number )
{
    t_sl_wl_thread *thread;

    // Run through each subthread and check they are idle, and set them to see the available work
    for (thread=worklist->thread_pool->threads; thread; thread=thread->next_in_list)
    {
        if (thread->busy)
        {
            fprintf(stderr,"Major bug - work list thread is busy when it should be idle\n");
            return error_level_fatal;
        }
        thread->busy = 1;
        thread->worklist = worklist;
        thread->guard_bits = guard_bits;
        thread->entry_number = entry_number;
        //printf("Signalling for work %p\n", thread);
        thread_sync( thread );
    }

    // Do the work ourselves for anything not assigned to another thread
    do_thread_work( worklist, guard_bits, entry_number, NULL );

    // Run through each subthread and wait for them to finish
    for (thread=worklist->thread_pool->threads; thread; thread=thread->next_in_list)
    {
        thread_sync( thread );
    }
    
    return error_level_okay;
}


