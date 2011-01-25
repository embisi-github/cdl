/*a Copyright
*/
/*a Constraint compiler source code
 */

/*a Includes
 */
#include <Python.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <pthread.h>
#include "sl_pthread_barrier.h"

/*a Defines
 */
#if 0
#define WHERE_I_AM {fprintf(stderr,"%s:%d\n",__func__,__LINE__ );}
#define WHERE_I_AM_TH {fprintf(stderr,"%p:%s:%d\n",this_thread,__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#define WHERE_I_AM_TH {}
#endif

/*a Types
 */
/*t t_py_object
 */
typedef struct t_py_object
{
    PyObject_HEAD
} t_py_object;

/*t t_cb_struct_callback_fn
 */
typedef void (*t_cb_struct_callback_fn)( struct t_cb_struct *cb_struct );

/*t t_cb_struct
 */
typedef struct t_cb_struct 
{
    struct t_start_new_thread_sync *start_new_thread_sync;
    t_cb_struct_callback_fn callback_fn;
    PyObject *callback;
    PyInterpreterState* py_interp;
} t_cb_struct;

/*t t_start_new_thread_sync
 */
typedef struct t_start_new_thread_sync
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int spawner_state; // 0 at init, 1 if waiting for condition, 2 if done, 3 if spawner got past sync point first - only change when mutex is claimed
    t_cb_struct *cb_struct;
} t_start_new_thread_sync;

/*a Declarations
 */
/*b Declarations of engine class methods
 */
static PyObject *py_engine_method_spawn( t_py_object *py_object, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_tick( t_py_object *py_object, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_finalize( t_py_object *py_object, PyObject *args, PyObject *kwds );

/*b Declarations of engine class functions
 */

/*a Statics for engine class
 */
/*v py_methods - methods supported by the engine class
 */
static PyMethodDef py_methods[] =
{
    {"spawn",                    (PyCFunction)py_engine_method_spawn,                  METH_VARARGS|METH_KEYWORDS, "Set environment for hardware file"},
    {"tick",                     (PyCFunction)py_engine_method_tick,                  METH_VARARGS|METH_KEYWORDS, "Set environment for hardware file"},
    {"finalize",                 (PyCFunction)py_engine_method_finalize,              METH_VARARGS|METH_KEYWORDS, "Set environment for hardware file"},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

/*a Statics
 */
static t_sl_pthread_barrier barrier;
static t_sl_pthread_barrier_thread_ptr main_barrier_thread;
static int cycle_number;
static int die_at_next_preclock;
static void *start_new_thread_callback( void *handle )
{
    t_start_new_thread_sync *start_new_thread_sync = (t_start_new_thread_sync *)handle;
    //fprintf(stderr,"New thread callback %p\n",handle);
    start_new_thread_sync->cb_struct->callback_fn( start_new_thread_sync->cb_struct );
    return handle;
}
static void start_new_thread_started( t_cb_struct *cb_struct )
{
    t_start_new_thread_sync *start_new_thread_sync=cb_struct->start_new_thread_sync;
    //fprintf(stderr,"client, spawner_state %d\n",start_new_thread_sync->spawner_state);
    pthread_mutex_lock( &start_new_thread_sync->mutex );
    //fprintf(stderr,"client, spawner_state %d\n",start_new_thread_sync->spawner_state);
    if (start_new_thread_sync->spawner_state==1) // Waiting on signal
    {
        //fprintf(stderr,"client, spawner_state %d\n",start_new_thread_sync->spawner_state);
        start_new_thread_sync->spawner_state = 4;
        pthread_cond_signal( &start_new_thread_sync->cond );
        //fprintf(stderr,"client, spawner_state %d\n",start_new_thread_sync->spawner_state);
    }
    else
    {
        //fprintf(stderr,"client, spawner_state %d\n",start_new_thread_sync->spawner_state);
        start_new_thread_sync->spawner_state = 3;
        //fprintf(stderr,"client, spawner_state %d\n",start_new_thread_sync->spawner_state);
    }
    pthread_mutex_unlock( &start_new_thread_sync->mutex );
}
static void *start_new_thread( t_cb_struct *cb_struct )
{
    pthread_t th;
    int status;
    static t_start_new_thread_sync start_new_thread_sync;
    int        rc=pthread_mutex_init( &start_new_thread_sync.mutex, NULL );
    if (rc==0) rc=pthread_cond_init( &start_new_thread_sync.cond, NULL );
    //fprintf(stderr,"Created mutex and cond %d\n",rc);
    start_new_thread_sync.cb_struct = cb_struct;
    start_new_thread_sync.spawner_state = 0;
    cb_struct->start_new_thread_sync = &start_new_thread_sync;

    pthread_mutex_lock( &start_new_thread_sync.mutex );

    status = pthread_create(&th,
                            (pthread_attr_t*)NULL,
                            (void* (*)(void *))start_new_thread_callback,
                            (void *)&start_new_thread_sync );

    //fprintf(stderr,"Created new thread %d\n",status);
    if (status != 0)
        return NULL;

    pthread_detach(th);
    if (start_new_thread_sync.spawner_state==0)
    {
        //fprintf(stderr,"server, spawner_state %d\n",start_new_thread_sync.spawner_state);
        start_new_thread_sync.spawner_state = 1;
        //fprintf(stderr,"server, spawner_state %d\n",start_new_thread_sync.spawner_state);
        Py_BEGIN_ALLOW_THREADS;
        pthread_cond_wait( &start_new_thread_sync.cond, &start_new_thread_sync.mutex );
        //fprintf(stderr,"server, spawner_state %d\n",start_new_thread_sync.spawner_state);
        start_new_thread_sync.spawner_state = 5;
        //fprintf(stderr,"server, spawner_state %d\n",start_new_thread_sync.spawner_state);
        Py_END_ALLOW_THREADS;
    }
    else
    {
        //fprintf(stderr,"server, spawner_state %d\n",start_new_thread_sync.spawner_state);
        start_new_thread_sync.spawner_state=2;
    }
    //fprintf(stderr,"server, spawner_state %d\n",start_new_thread_sync.spawner_state);
    pthread_mutex_unlock( &start_new_thread_sync.mutex );
    pthread_mutex_destroy( &start_new_thread_sync.mutex );
    pthread_cond_destroy( &start_new_thread_sync.cond );
    return th;
}

/*a Python-called engine class methods
 */
/*f barrier_preclock
 */
static void barrier_preclock( t_sl_pthread_barrier *barrier, void *handle )
{
    const char *text = (const char *)handle;
    //fprintf(stderr,"preclock cycle %d (%s) %d\n",cycle_number,text,die_at_next_preclock);
}

/*f barrier_finalize
 */
static void barrier_finalize( t_sl_pthread_barrier *barrier, void *handle )
{
    const char *text = (const char *)handle;
    //fprintf(stderr,"finalize cycle %d (%s)\n",cycle_number,text);
    cycle_number = ((cycle_number/100)+1)*100;
}

/*f barrier_clock
 */
static void barrier_clock( t_sl_pthread_barrier *barrier, void *handle )
{
    const char *text = (const char *)handle;
    fprintf(stderr,"clock cycle %d (%s)\n",cycle_number,text);
    cycle_number++;
}

/*f py_engine_method_spawn
 */
static void call_object_in_thread( PyThreadState *py_thread, PyObject *callback )
{
    PyObject *py_obj;
    PyEval_AcquireThread(py_thread);
    py_obj = PyObject_CallObject( callback, NULL );
    if (py_obj) Py_DECREF(py_obj);
    PyEval_ReleaseThread(py_thread);
}
static void spawn_thread_cb( t_cb_struct *cb_struct )
{
    PyObject *callback = cb_struct->callback;

    PyInterpreterState* py_interp;
    PyThreadState* py_thread;
    t_sl_pthread_barrier_thread_ptr barrier_thread;
    int running;
    int loop_count;

    //fprintf(stderr,"spawn_thread_cb %p\n",cb_struct);

    barrier_thread = sl_pthread_barrier_thread_add( &barrier );

    //fprintf(stderr,"Callback thread %p\n",callback);

    PyEval_AcquireLock();
    py_interp = cb_struct->py_interp;
    //fprintf(stderr,"interp %p\n",py_interp);

    py_thread = PyThreadState_New(py_interp);
    PyThreadState_Clear(py_thread);
    //fprintf(stderr,"new thread %p\n",py_thread);
    PyEval_ReleaseLock();

    start_new_thread_started( cb_struct );

    // Wait for message from parent thread
    running = 1;
    loop_count = 0;
    while (running)
    {
        sl_pthread_barrier_wait( &barrier, barrier_thread, barrier_preclock, (void *)"secondary thread" ); // Wait for all inputs to have been copied etc
        if (die_at_next_preclock)
        {
            sl_pthread_barrier_wait( &barrier, barrier_thread, barrier_finalize, (void *)"secondary thread" ); // Wait for all inputs to have been copied etc
            running=0;
            break;
        }
        //fprintf(stderr,"callback cycle %d (%p)\n",cycle_number,py_thread);
        call_object_in_thread( py_thread, callback ); // Process inputs, prepare new outputs
        loop_count++;
        sl_pthread_barrier_wait( &barrier, barrier_thread, barrier_clock, (void *)"secondary thread" );
    }
    //fprintf(stderr,"Thread exiting %p after %d loops\n",py_thread,loop_count);
    sl_pthread_barrier_thread_delete( &barrier, barrier_thread );
    PyEval_AcquireLock();
    PyThreadState_Clear(py_thread);
    PyThreadState_Delete(py_thread);
    Py_DECREF(callback);
    PyEval_ReleaseLock();
    pthread_exit((void *)cb_struct);
}

/*f py_engine_method_spawn
 */
static PyObject *py_engine_method_spawn( t_py_object *py_eng, PyObject *args, PyObject *kwds )
{
    PyObject *callback;
    char kw[] = "callback";
    char *kwdlist[] = { kw, NULL };
    t_cb_struct *cb_struct;

    PyEval_InitThreads();
    PyThreadState* py_thread;
    PyInterpreterState* py_interp;
    py_thread = PyThreadState_Get();
    //fprintf(stderr,"py_thread %p\n",py_thread);
    py_interp = py_thread->interp;
    //fprintf(stderr,"py_interp %p\n",py_interp);

    if (PyArg_ParseTupleAndKeywords( args, kwds, "O", kwdlist, &callback))
    {
        cb_struct = (t_cb_struct *)malloc(sizeof(t_cb_struct));
        cb_struct->callback_fn = spawn_thread_cb;
        cb_struct->py_interp = py_interp;
        cb_struct->callback = callback;
        Py_INCREF(callback);
        if (start_new_thread( cb_struct )==NULL)
        {
            Py_DECREF(callback);
            free(cb_struct);
            return NULL;
        }
        Py_RETURN_NONE;
    }
    return NULL;
}

/*f py_engine_method_tick
 */
static PyObject *py_engine_method_tick( t_py_object *py_eng, PyObject *args, PyObject *kwds )
{
    Py_BEGIN_ALLOW_THREADS;
    sl_pthread_barrier_wait( &barrier, main_barrier_thread, barrier_preclock, (void *)"main thread" );
    sl_pthread_barrier_wait( &barrier, main_barrier_thread, barrier_clock, (void *)"main thread" );
    Py_END_ALLOW_THREADS;
    Py_RETURN_NONE;
}

/*f py_engine_method_finalize
 */
static PyObject *py_engine_method_finalize( t_py_object *py_eng, PyObject *args, PyObject *kwds )
{
    Py_BEGIN_ALLOW_THREADS;
    die_at_next_preclock = 1;
    sl_pthread_barrier_wait( &barrier, main_barrier_thread, barrier_preclock, (void *)"main thread prefinalize" );
    sl_pthread_barrier_wait( &barrier, main_barrier_thread, barrier_finalize, (void *)"main thread" );
    die_at_next_preclock = 0;
    Py_END_ALLOW_THREADS;
    Py_RETURN_NONE;
}

/*a C code for py_engine
 */
extern "C" void initblob( void )
{
	PyObject *m;
    m = Py_InitModule3("blob", py_methods, "Python interface to CDL simulation engine" );
    sl_pthread_barrier_init( &barrier );
    die_at_next_preclock = 0;
    cycle_number = 0;
    main_barrier_thread = sl_pthread_barrier_thread_add( &barrier );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

