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
#if 0
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),__func__,__LINE__ );}
#define WHERE_I_AM_TH {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),this_thread,__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#define WHERE_I_AM_TH {}
#endif
#if 0
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM_STR(s) {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d:%s\n",tp.tv_sec,tp.tv_usec,pthread_self(),__func__,__LINE__,s );}
#define WHERE_I_AM_TH_STR(s) {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%p:%s:%d:%s\n",tp.tv_sec,tp.tv_usec,pthread_self(),this_thread,__func__,__LINE__,s );}
#undef WHERE_I_AM_TH_STR(s)
#define WHERE_I_AM_TH_STR(s, other) {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%4.4d:%3ld.%06d:%6.6lx:%6.6lx:%s:%.*s:%s\n",__LINE__,tp.tv_sec % 1000,tp.tv_usec,(((long)pthread_self())>>12)&0xFFFFFF,((long)other)&0xFFFFFF,(char*)barrier_sync_callback_handle,((int)((unsigned long)pthread_self())>>12) & 31,"                                ",s );}
#else
#define WHERE_I_AM_STR(s) {}
#define WHERE_I_AM_TH_STR(s, other) {}
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
    int  user_state;
    pthread_cond_t cond;
    void *user_data_start; // This should be suitably aligned
} t_sl_pthread_barrier_thread;

/*f sl_pthread_barrier_init
 */
extern int sl_pthread_barrier_init( t_sl_pthread_barrier *barrier )
{
    int rc;
    const char* barrier_sync_callback_handle = "PtBIni";

    WHERE_I_AM;
    barrier->threads = NULL;
    barrier->gc_threads = NULL;
    barrier->state = barrier_state_idle;
    WHERE_I_AM_TH_STR("B init->idle", &barrier->state );
    barrier->restart_loop = 0;
    rc = pthread_mutex_init( &barrier->mutex, NULL );
    if (rc==0) rc=pthread_cond_init( &barrier->cond, NULL );
    WHERE_I_AM_TH_STR("cond+", &barrier->cond );
    WHERE_I_AM;
    return rc;
}

/*f sl_pthread_barrier_thread_add - call with barrier mutex NOT locked
 */
extern t_sl_pthread_barrier_thread_ptr sl_pthread_barrier_thread_add( t_sl_pthread_barrier *barrier, int user_data_size )
{
    t_sl_pthread_barrier_thread *thread;
    thread = (t_sl_pthread_barrier_thread *)malloc(sizeof(t_sl_pthread_barrier_thread)+user_data_size);
    const char* barrier_sync_callback_handle = "PtBAdd";

    WHERE_I_AM_TH_STR( "thread+", thread );
    WHERE_I_AM;
    WHERE_I_AM_STR("thread_add entry");
    WHERE_I_AM_TH_STR( "barrier_lock+A", &barrier->mutex );
    pthread_mutex_lock( &barrier->mutex );
    WHERE_I_AM_TH_STR( "barrier_lock=A", &barrier->mutex );

    WHERE_I_AM;
    thread->next = barrier->threads;
    barrier->threads = thread;
    pthread_cond_init( &thread->cond, NULL );
    WHERE_I_AM_TH_STR( "cond+", &thread->cond );
    WHERE_I_AM;
    if (barrier->state == barrier_state_idle)
    {
        WHERE_I_AM;
        WHERE_I_AM_STR("adding as running");
        thread->state = barrier_thread_state_running;
        WHERE_I_AM_TH_STR("T idle->running", &thread->state );
    }
    else if (barrier->state == barrier_state_gathering)
    {
        WHERE_I_AM;
        WHERE_I_AM_STR("adding as will_be_waiting");
        thread->state = barrier_thread_state_will_be_waiting;
        WHERE_I_AM_TH_STR("T idle->WBW", &thread->state );
        barrier->restart_loop = 1;
    }
    WHERE_I_AM;
    //fprintf(stderr,"Added barrier thread %p\n",thread);
    WHERE_I_AM_TH_STR( "barrier_lock-A", &barrier->mutex );
    pthread_mutex_unlock( &barrier->mutex );
    WHERE_I_AM;
    WHERE_I_AM_STR("thread_add exit");
    return thread;
}

/*f sl_pthread_barrier_thread_delete - call with barrier mutex NOT locked
 */
extern void sl_pthread_barrier_thread_delete( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_ptr thread )
{
    t_sl_pthread_barrier_thread **prev_thread_ptr;
    const char* barrier_sync_callback_handle = "PtBDel";

    WHERE_I_AM;
    WHERE_I_AM_STR("thread_delete entry");
    // Note that danger lurks if the barrier is already waiting for this thread - we pull the thread off the barrier here first
    WHERE_I_AM_TH_STR( "barrier_lock+D", &barrier->mutex );
    pthread_mutex_lock( &barrier->mutex );
    WHERE_I_AM_TH_STR( "barrier_lock=D", &barrier->mutex );

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
    //fprintf(stderr,"Deleted barrier thread %p\n",thread);
    WHERE_I_AM_TH_STR( "barrier_lock-D", &barrier->mutex );
    pthread_mutex_unlock( &barrier->mutex );
    WHERE_I_AM;
}

/*f sl_pthread_barrier_thread_set_user_state
 */
extern void sl_pthread_barrier_thread_set_user_state( t_sl_pthread_barrier_thread_ptr this_thread, int state )
{
    this_thread->user_state = state;
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
    return (void *)(&this_thread->user_data_start);
}

/*f sl_pthread_barrier_thread_iter
 */
extern void sl_pthread_barrier_thread_iter( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_iter_fn iter_fn, void *handle )
{
    const char* barrier_sync_callback_handle = "PtBItr";

    /*b Claim mutex
     */
    WHERE_I_AM;
    WHERE_I_AM_TH_STR( "barrier_lock+I", &barrier->mutex );
    pthread_mutex_lock( &barrier->mutex );
    WHERE_I_AM_TH_STR( "barrier_lock=I", &barrier->mutex );

    /*b Iterate over threads
     */
    for (t_sl_pthread_barrier_thread *thread=barrier->threads; thread; thread=thread->next)
    {
        iter_fn( handle, thread );
    }

    /*b Release mutex
     */
    WHERE_I_AM;
    WHERE_I_AM_TH_STR( "barrier_lock-I", &barrier->mutex );
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

    /*b Claim mutex
     */
    WHERE_I_AM_TH;
    WHERE_I_AM_TH_STR( "barrier_wait+", this_thread );
    WHERE_I_AM_TH_STR( "barrier_lock+", &barrier->mutex );
    pthread_mutex_lock( &barrier->mutex );
    WHERE_I_AM_TH_STR( "barrier_lock=", &barrier->mutex );

    /*b Garbage collect dead threads
     */
    for (t_sl_pthread_barrier_thread *thread=barrier->gc_threads; thread; )
    {
        t_sl_pthread_barrier_thread *next_thread=thread->next;
        WHERE_I_AM_TH_STR("cond-", &thread->cond );
        pthread_cond_destroy( &thread->cond );
        WHERE_I_AM_TH_STR("thread-", thread );
        free(thread);
        thread = next_thread;
    }
    barrier->gc_threads = NULL;

    /*b Handle barrier state
     */
    switch (barrier->state)
    {
    case barrier_state_idle:
        WHERE_I_AM_TH;
        WHERE_I_AM_TH_STR("gathering+", 0);
        barrier->state = barrier_state_gathering;
        WHERE_I_AM_TH_STR("B idle->gather", &barrier->state );
        for (t_sl_pthread_barrier_thread *thread=barrier->threads; thread; thread=thread->next)
            if (thread->state==barrier_thread_state_pending_idle) // Thread must have run around to its barrier before the last 'master' thread reached idle
            {
                thread->state=barrier_thread_state_ready;
                WHERE_I_AM_TH_STR("TO p_idle->ready", &thread->state );
            }
            else
            {
                thread->state=barrier_thread_state_will_be_waiting; // Thread has not hit barrier yet (must be running) (as if it had it would be pending_idle or the barrier would not be idle)
                WHERE_I_AM_TH_STR("TO unk->WBW", &thread->state );
            }
        WHERE_I_AM_TH;
        this_thread->state = barrier_thread_state_ready;
        WHERE_I_AM_TH_STR("T unk->ready", &this_thread->state );
        WHERE_I_AM_TH;
        {
            t_sl_pthread_barrier_thread *thread;
            thread = barrier->threads;
            while (thread)
            {
                WHERE_I_AM_TH;
                //fprintf(stderr,"Thread %p this_thread %p\n", thread, this_thread);
                if (thread->state==barrier_thread_state_will_be_waiting) // Thread still not hit barrier
                {
                    WHERE_I_AM_TH;
                    thread->state=barrier_thread_state_waiting; // When it hits the barrier make it notify us
                    WHERE_I_AM_TH_STR( "TO WBW->wait", &thread->state );
                    WHERE_I_AM_TH_STR( "waitother+", thread );
                    WHERE_I_AM_TH_STR( "waitothercond+", &thread->cond );
                    WHERE_I_AM_TH_STR( "barrier_lock-W", &barrier->mutex );
                    pthread_cond_wait( &thread->cond, &barrier->mutex ); // When this returns we have the mutex, and the thread must be in 'ready' state - or it is finalizing and freed
                    WHERE_I_AM_TH_STR( "barrier_lock=+", &barrier->mutex );
                    WHERE_I_AM_TH_STR( "waitothercond-", &thread->cond );
                    WHERE_I_AM_TH_STR( "waitother-", thread );
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
        {
            thread->state = barrier_thread_state_running;
            WHERE_I_AM_TH_STR("TO unk->running", &thread->state );
        }
        WHERE_I_AM_TH_STR("B unk->idle", &barrier->state );
        barrier->state = barrier_state_idle;
        WHERE_I_AM_TH_STR("gathering-", 0);
        WHERE_I_AM_TH;
        if (barrier_sync_callback)
        {
            WHERE_I_AM_TH_STR("callback+", 0);
            barrier_sync_callback( barrier, barrier_sync_callback_handle );
            WHERE_I_AM_TH_STR("callback-", 0);
        }
        WHERE_I_AM_TH;
        WHERE_I_AM_TH_STR("broadcast+", 0);
        pthread_cond_broadcast( &barrier->cond );
        WHERE_I_AM_TH_STR("broadcast-", 0);
        WHERE_I_AM_TH;
        break;
    case barrier_state_gathering: // Another thread is gathering - it should have kicked us to 'will_be_waiting' or 'waiting'
        WHERE_I_AM_TH;
        WHERE_I_AM_TH_STR("waitgathering", this_thread->state);
        if (this_thread->state==barrier_thread_state_waiting)
        {
            WHERE_I_AM_TH;
            WHERE_I_AM_TH_STR("signalother+", &this_thread->cond );
            pthread_cond_signal( &this_thread->cond );
            WHERE_I_AM_TH_STR("signalother-", &this_thread->cond );
        }
        WHERE_I_AM_TH;
        WHERE_I_AM_TH_STR("T unk->ready", &this_thread->state );
        this_thread->state = barrier_thread_state_ready;
        WHERE_I_AM_TH_STR("waitforgather+", &barrier->cond );
        WHERE_I_AM_TH_STR( "barrier_lock-W", &barrier->mutex );
        pthread_cond_wait( &barrier->cond, &barrier->mutex ); // When this returns the barrier state must now be idle and all threads are running
        WHERE_I_AM_TH_STR( "barrier_lock=+", &barrier->mutex );
        WHERE_I_AM_TH_STR("waitforgather-", &barrier->cond );
        WHERE_I_AM_TH;
        break;
    }

    /*b Release mutex
     */
    WHERE_I_AM_TH;
    WHERE_I_AM_TH_STR( "barrier_lock-", &barrier->mutex );
    pthread_mutex_unlock( &barrier->mutex );
    WHERE_I_AM_TH_STR("barrier_wait-", this_thread );
    WHERE_I_AM_TH;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

