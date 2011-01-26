/*a Copyright
  
  This file 'sl_general.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_PTHREAD
#else
#define __INC_SL_PTHREAD

/*a Includes
 */
#include <pthread.h>

/*a Defines
 */

/*a Types
 */
/*t t_sl_ptherad_barrier_state
 */
typedef enum
{
    barrier_state_idle,
    barrier_state_gathering,
} t_sl_pthread_barrier_state;

/*t t_sl_pthread_barrier
 */
typedef struct t_sl_pthread_barrier
{
    struct t_sl_pthread_barrier_thread *threads;
    struct t_sl_pthread_barrier_thread *gc_threads; // Threads to garbage collect when barrier is done with them
    t_sl_pthread_barrier_state state;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int restart_loop;
    void *user_data;
} t_sl_pthread_barrier;

/*t t_sl_pthread_barrier_thread_ptr
 */
typedef struct t_sl_pthread_barrier_thread *t_sl_pthread_barrier_thread_ptr;

/*t t_sl_pthread_barrier_callback_fn
 */
typedef void (*t_sl_pthread_barrier_callback_fn)( t_sl_pthread_barrier *barrier, void *handle );

/*t t_sl_pthread_barrier_thread_iter_fn
*/
typedef void (*t_sl_pthread_barrier_thread_iter_fn)( void *handle, t_sl_pthread_barrier_thread_ptr barrier_thread );

/*a External functions
 */
extern int sl_pthread_barrier_init( t_sl_pthread_barrier *barrier );
extern t_sl_pthread_barrier_thread *sl_pthread_barrier_thread_add( t_sl_pthread_barrier *barrier, int user_data_size );
extern void sl_pthread_barrier_thread_delete( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_ptr thread );
extern void sl_pthread_barrier_thread_set_user_state( t_sl_pthread_barrier_thread_ptr this_thread, int state );
extern int sl_pthread_barrier_thread_get_user_state( t_sl_pthread_barrier_thread_ptr this_thread );
extern void *sl_pthread_barrier_thread_get_user_ptr( t_sl_pthread_barrier_thread_ptr this_thread );
extern void sl_pthread_barrier_thread_iter( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_iter_fn iter_fn, void *handle );
extern void sl_pthread_barrier_wait( t_sl_pthread_barrier *barrier, t_sl_pthread_barrier_thread_ptr this_thread, t_sl_pthread_barrier_callback_fn barrier_sync_callback, void *barrier_sync_callback_handle );

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

