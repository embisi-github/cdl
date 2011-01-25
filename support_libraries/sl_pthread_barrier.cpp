/*a Copyright
*/
/*a Constraint compiler source code
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sl_pthread_barrier.h"

/*a Defines
 */
#if 1
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),__func__,__LINE__ );}
#define WHERE_I_AM_TH {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),this_thread,__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#define WHERE_I_AM_TH {}
#endif

/*a pthread barriers
 */
/*t t_sl_pthread_barrier_thread_state
 */
typedef enum
{
    barrier_thread_state_pending_idle,
    barrier_thread_state_will_be_waiting,
    barrier_thread_state_waiting,
    barrier_thread_state_ready,
    barrier_thread_state_running
} t_sl_pthread_barrier_thread_state;

/*t t_sl_pthread_barrier_thread
 */
typedef struct t_sl_pthread_barrier_thread
{
    struct t_sl_pthread_barrier_thread *next;
    t_sl_pthread_barrier_thread_state state;
    void *user_ptr;
    int  user_state;
    pthread_cond_t cond;
} t_sl_pthread_barrier_thread;

/*f sl_pthread_barrier_init
 */
#include <string.h>
extern int sl_pthread_barrier_init( t_sl_pthread_barrier *barrier )
{
    int rc;
    WHERE_I_AM;
    strcpy(barrier->prot,"pro");
    WHERE_I_AM;
    fprintf(stderr,"Barrier %p %s\n",barrier, barrier->prot);
    barrier->threads = NULL;
    barrier->gc_threads = NULL;
    barrier->state = barrier_state_idle;
    barrier->restart_loop = 0;
    rc = pthread_mutex_init( &barrier->mutex, NULL );
    if (rc==0) rc=pthread_cond_init( &barrier->cond, NULL );
    WHERE_I_AM;
    return rc;
}

/*f sl_pthread_barrier_thread_add - call with barrier mutex NOT locked
 */
extern t_sl_pthread_barrier_thread_ptr sl_pthread_barrier_thread_add( t_sl_pthread_barrier *barrier )
{
    t_sl_pthread_barrier_thread *thread;
    thread = (t_sl_pthread_barrier_thread *)malloc(sizeof(t_sl_pthread_barrier_thread));

    WHERE_I_AM;
    fprintf(stderr,"Barrier %p %s\n",barrier, barrier->prot);
    WHERE_I_AM;
    pthread_mutex_lock( &barrier->mutex );

    WHERE_I_AM;
    thread->next = barrier->threads;
    barrier->threads = thread;
    pthread_cond_init( &thread->cond, NULL );
    WHERE_I_AM;
    if (barrier->state == barrier_state_idle)
    {
    WHERE_I_AM;
        thread->state = barrier_thread_state_running;
    }
    else if (barrier->state == barrier_state_gathering)
    {
        WHERE_I_AM;
        thread->state = barrier_thread_state_will_be_waiting;
        barrier->restart_loop = 1;
    }
    WHERE_I_AM;
    fprintf(stderr,"Added barrier thread %p\n",thread);
    pthread_mutex_unlock( &barrier->mutex );
    WHERE_I_AM;
    return thread;
}

/*f sl_pthread_barrier_thread_delete - call with barrier mutex NOT locked
 */
extern void sl_pthread_barrier_thread_delete( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_ptr thread )
{
    t_sl_pthread_barrier_thread **prev_thread_ptr;

    WHERE_I_AM;
    fprintf(stderr,"Barrier %p %s\n",barrier, barrier->prot);
    WHERE_I_AM;
    // Note that danger lurks if the barrier is already waiting for this thread - we pull the thread off the barrier here first
    pthread_mutex_lock( &barrier->mutex );

    WHERE_I_AM;
    prev_thread_ptr = &barrier->threads;
    while ((*prev_thread_ptr)!=thread)
        prev_thread_ptr = &((*prev_thread_ptr)->next);

    WHERE_I_AM;
    *prev_thread_ptr = thread->next;

    thread->next = barrier->gc_threads;
    barrier->gc_threads = thread;
    // Only fuss with the barrier if it is waiting for this thread
    if (thread->state==barrier_thread_state_waiting)
    {
    WHERE_I_AM;
        barrier->restart_loop = 1; // ... and the danger is resolved by kicking the barrier-seeking thread to start again in its thread loop, which this thread is no longer on
        pthread_cond_signal( &thread->cond );
    }
    WHERE_I_AM;
    fprintf(stderr,"Deleted barrier thread %p\n",thread);
    pthread_mutex_unlock( &barrier->mutex );
    WHERE_I_AM;
}

/*f sl_pthread_barrier_thread_set_user_state
 */
extern void sl_pthread_barrier_thread_set_user_state( t_sl_pthread_barrier_thread_ptr this_thread, int state, void *ptr )
{
    this_thread->user_state = state;
    this_thread->user_ptr   = ptr;
}

/*f sl_pthread_barrier_thread_get_user_state
 */
extern int sl_pthread_barrier_thread_get_user_state( t_sl_pthread_barrier_thread_ptr this_thread )
{
    return this_thread->user_state;
}

/*f sl_pthread_barrier_thread_get_user_ptr
 */
extern void *sl_pthread_barrier_thread_get_user_ptr( t_sl_pthread_barrier_thread_ptr this_thread )
{
    return this_thread->user_ptr;
}

/*f sl_pthread_barrier_thread_iter
 */
extern void sl_pthread_barrier_thread_iter( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_iter_fn iter_fn, void *handle )
{
    WHERE_I_AM;
    fprintf(stderr,"Barrier %p %s\n",barrier, barrier->prot);
    /*b Claim mutex
     */
    WHERE_I_AM;
    pthread_mutex_lock( &barrier->mutex );

    /*b Iterate over threads
     */
    for (t_sl_pthread_barrier_thread *thread=barrier->threads; thread; thread=thread->next)
    {
        iter_fn( handle, thread );
    }

    /*b Release mutex
     */
    WHERE_I_AM;
    pthread_mutex_unlock( &barrier->mutex );
}

/*f sl_pthread_barrier_wait
 */
extern void sl_pthread_barrier_wait( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_ptr this_thread, t_sl_pthread_barrier_callback_fn barrier_sync_callback, void *barrier_sync_callback_handle )
{
    // A Barrier for a set of threads requires a component per thread
    // The barrier has states idle, gathering, ready
    // The barrier includes a single mutex
    // A thread in the barrier has states 'pending idle', 'will-be-waiting', 'waiting', 'ready', 'running'
    // A thread in the barrier has a condition
    // A thread hits the barrier;
    //  1) It claims the mutex
    //  2a) 2a.1) The barrier is idle, so no other threads are at the barrier (other than those in 'pending idle')
    //      2a.2) Set barrier as gathering
    //      2a.3) For all other threads: if pending-idle, mark as ready, else mark as will-be-waiting
    //      2a.4) For all threads
    //            2a.4.1) If thread is 'will-be-waiting' then mark as waiting, condition wait for it
    //      2a.5) All threads must now be in 'ready' - they must be condition waiting on the barrier condition
    //      2a.6) Mark barrier as ready
    //      2a.7) For all threads, mark as running
    //      2a.7) Broadcast condition signal to the barrier condition
    //      2a.8) Mark barrier as idle
    //  2b) 2b.1) The barrier is gathering, so one of the other threads is at the barrier first
    //      2b.2) This thread should be in barrier type 'will-be-waiting' or 'waiting'
    //      2b.3a) This thread in 'waiting' - i.e. main thread is waiting for us to notify on the condition
    //            2b.3a.2) Condition notify the thread condition
    //      2b.4) Mark thread as 'ready'
    //      2b.5) Condition wait on the barrier condition
    //  2c) 2c.1) The barrier is ready - this thread has rushed around to the barrier while the last barrier thread is yet to reach 'idle'
    //      2c.2) Mark thread in barrier as 'pending idle'
    //      2c.3) Condition wait on the barrier condition
    //  3) Release mutex
    WHERE_I_AM;
    fprintf(stderr,"Barrier %p %s\n",barrier, barrier->prot);

    /*b Claim mutex
     */
    WHERE_I_AM_TH;
    pthread_mutex_lock( &barrier->mutex );

    /*b Garbage collect dead threads
     */
    for (t_sl_pthread_barrier_thread *thread=barrier->gc_threads; thread; thread=thread->next)
    {
        pthread_cond_destroy( &thread->cond );
        free(thread);
    }
    barrier->gc_threads = NULL;

    /*b Handle barrier state
     */
    switch (barrier->state)
    {
    case barrier_state_idle:
    WHERE_I_AM_TH;
        barrier->state = barrier_state_gathering;
        for (t_sl_pthread_barrier_thread *thread=barrier->threads; thread; thread=thread->next)
            if (thread->state==barrier_thread_state_pending_idle) // Thread must have run around to its barrier before the last 'master' thread reached idle
                thread->state=barrier_thread_state_ready;
            else
                thread->state=barrier_thread_state_will_be_waiting; // Thread has not hit barrier yet (must be running) (as if it had it would be pending_idle or the barrier would not be idle)
    WHERE_I_AM_TH;
        this_thread->state = barrier_thread_state_ready;
        WHERE_I_AM_TH;
        {
            t_sl_pthread_barrier_thread *thread;
            thread = barrier->threads;
            while (thread)
            {
                WHERE_I_AM_TH;
                fprintf(stderr,"Thread %p this_thread %p\n", thread, this_thread);
                if (thread->state==barrier_thread_state_will_be_waiting) // Thread still not hit barrier
                {
                    WHERE_I_AM_TH;
                    thread->state=barrier_thread_state_waiting; // When it hits the barrier make it notify us
                    pthread_cond_wait( &thread->cond, &barrier->mutex ); // When this returns we have the mutex, and the thread must be in 'ready' state - or it is finalizing and freed
                }
                // If thread disappeared or thread was added it will have set barrier->restart_loop
                if (barrier->restart_loop)
                {
                    barrier->restart_loop = 0;
                    thread = barrier->threads;
                }
                else
                {
                    thread = thread->next;
                }
            }
        }
        // All threads except this one must now be ready, waiting for the barrier condition to be signaled
    WHERE_I_AM_TH;
        for (t_sl_pthread_barrier_thread *thread=barrier->threads; thread; thread=thread->next)
            thread->state = barrier_thread_state_running;
        barrier->state = barrier_state_idle;
    WHERE_I_AM_TH;
        if (barrier_sync_callback)
            barrier_sync_callback( barrier, barrier_sync_callback_handle );
    WHERE_I_AM_TH;
        pthread_cond_broadcast( &barrier->cond );
    WHERE_I_AM_TH;
        break;
    case barrier_state_gathering: // Another thread is gathering - it should have kicked us to 'will_be_waiting' or 'waiting'
    WHERE_I_AM_TH;
        if (this_thread->state==barrier_thread_state_waiting)
        {
    WHERE_I_AM_TH;
            pthread_cond_signal( &this_thread->cond );
        }
    WHERE_I_AM_TH;
        this_thread->state = barrier_thread_state_ready;
        pthread_cond_wait( &barrier->cond, &barrier->mutex ); // When this returns the barrier state must now be idle and all threads are running
    WHERE_I_AM_TH;
        break;
    }

    /*b Release mutex
     */
    WHERE_I_AM_TH;
    pthread_mutex_unlock( &barrier->mutex );
    WHERE_I_AM_TH;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

