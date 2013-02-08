/*a Copyright
  
  This file 'se_py_engine.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include <Python.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sl_debug.h"
#include "sl_cons.h"
#include "sl_indented_file.h"
#include "sl_general.h"
#include "sl_exec_file.h"
#include "model_list.h"
#include "c_se_engine.h"

/*a Defines
 */
#define PY_DEBUG
#undef PY_DEBUG
#if 0
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif

/*a Types
 */
/*t t_py_engine_PyObject
 */
typedef struct {
     PyObject_HEAD
     c_sl_error *error;
     c_engine *engine;
     t_sl_option_list env_options;
} t_py_engine_PyObject;

/*a Declarations
 */
/*b Declarations of engine class methods
 */
static PyObject *py_engine_method_setenv( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_read_hw_file( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_describe_hw( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_read_gui_file( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_reset_errors( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_error_level( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_error( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_step( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_reset( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_cycle( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_write_profile( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_thread_pool_init( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_thread_pool_delete( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_thread_pool_add( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_thread_pool_map_module( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_list_instances( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_find_state( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state_info( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state_value_string( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state_memory( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_set_state( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_set_array_state( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_module_ios( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_checkpoint_init( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_checkpoint_add( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_checkpoint_info( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_checkpoint_restore( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );

/*b Declarations of engine class functions
 */
static void py_engine_dealloc( PyObject *self );
static int py_engine_print( PyObject *self, FILE *f, int unknown );
static PyObject *py_engine_repr( PyObject *self );
static PyObject *py_engine_str( PyObject *self );
static PyObject *py_engine_new( PyObject* self, PyObject* args );
static PyObject *py_engine_debug( PyObject* self, PyObject* args, PyObject *kwds );
static PyObject *py_engine_getattr( PyObject *self, char *name);

/*a Statics for engine class
 */
/*f gui_cmds
 */
static t_sl_indented_file_cmd gui_cmds[] =
{
     {"module", "x", "module <name>", 1},
     {"grid", "xxxxxxx", "grid <x> <y> <colspan> <rowspan> <colweights> <rowweights> <options>", 1},
     {"title", "xxxx", "value <x> <y> <colspan> <rowspan>", 0 },
     {"label", "xxxxx", "value <x> <y> <colspan> <rowspan> <text>", 0 },
     {"value", "xxxxxx", "value <x> <y> <colspan> <rowspan> <width> <statename>", 0 },
     {"memory", "xxxxxxxx", "memory <x> <y> <colspan> <rowspan> <width> <height> <items per line> <statename>", 0 },
     {NULL, NULL, NULL, 0}
};

/*v engine_methods - methods supported by the engine class
 */
static PyMethodDef engine_methods[] =
{
     {"setenv",                    (PyCFunction)py_engine_method_setenv,                  METH_VARARGS|METH_KEYWORDS, "Set environment for hardware file"},
     {"read_hw_file",              (PyCFunction)py_engine_method_read_hw_file,            METH_VARARGS|METH_KEYWORDS, "Read hardware file. Clears environment."},
     {"describe_hw",               (PyCFunction)py_engine_method_describe_hw,             METH_VARARGS|METH_KEYWORDS, "Describe hardware"},
     {"read_gui_file",             (PyCFunction)py_engine_method_read_gui_file,           METH_VARARGS|METH_KEYWORDS, "Read gui file."},
     {"reset_errors",              (PyCFunction)py_engine_method_reset_errors,            METH_VARARGS|METH_KEYWORDS, "Gets the worst case error level." },
     {"get_error_level",           (PyCFunction)py_engine_method_get_error_level,         METH_VARARGS|METH_KEYWORDS, "Gets the worst case error level." },
     {"get_error",                 (PyCFunction)py_engine_method_get_error,               METH_VARARGS|METH_KEYWORDS, "Gets the n'th error." },
     {"reset",                     (PyCFunction)py_engine_method_reset,                   METH_VARARGS|METH_KEYWORDS, "Reset the simulation engine."},
     {"cycle",                     (PyCFunction)py_engine_method_cycle,                   METH_VARARGS|METH_KEYWORDS, "Get the global cycle number of the simulation engine."},
     {"step",                      (PyCFunction)py_engine_method_step,                    METH_VARARGS|METH_KEYWORDS, "Step the simulation engine a number of cycles."},
     {"write_profile",             (PyCFunction)py_engine_method_write_profile,           METH_VARARGS|METH_KEYWORDS, "Write out a sim profile after the sim has run."},
     {"thread_pool_init",          (PyCFunction)py_engine_method_thread_pool_init,        METH_VARARGS|METH_KEYWORDS, "Initialize the thread pool."},
     {"thread_pool_delete",        (PyCFunction)py_engine_method_thread_pool_delete,      METH_VARARGS|METH_KEYWORDS, "Delete the thread pool, killing all threads."},
     {"thread_pool_add",           (PyCFunction)py_engine_method_thread_pool_add,         METH_VARARGS|METH_KEYWORDS, "Add a named thread the thread pool."},
     {"thread_pool_map_module",    (PyCFunction)py_engine_method_thread_pool_map_module,  METH_VARARGS|METH_KEYWORDS, "Map a module to a named thread."},
     {"list_instances",            (PyCFunction)py_engine_method_list_instances,          METH_VARARGS|METH_KEYWORDS, "List toplevel instances."},
     {"find_state",                (PyCFunction)py_engine_method_find_state,              METH_VARARGS|METH_KEYWORDS, "Get information about global or module state."},
     {"get_state_info",            (PyCFunction)py_engine_method_get_state_info,          METH_VARARGS|METH_KEYWORDS, "Get information about global or module state."},
     {"get_state",                 (PyCFunction)py_engine_method_get_state,               METH_VARARGS|METH_KEYWORDS,
      "Get state from simulation engine.\n"
      "\n"
      "@module:     module name\n"
      "@state:      state name or enumerated id\n"
      "@what:       what part of state to get (if memory/vector)\n"
      "\n"
      "Retrieve state contents from simulation engine.  If 'state' is\n"
      "a memory then return list of all values or a selection of\n"
      "values based on 'what'.  When 'what' is an int return a single\n"
      "word in the memory.  When 'what' is a slice object return a\n"
      "list of the indicated words in the memory.\n"
      "\n"
      "Throw ValueError if 'module' or 'state' can not be found."},
     {"get_state_memory",          (PyCFunction)py_engine_method_get_state_memory,        METH_VARARGS|METH_KEYWORDS, "Get string value of global or module state."},
     {"get_module_ios",            (PyCFunction)py_engine_method_get_module_ios,          METH_VARARGS|METH_KEYWORDS, "Get information about global or module state."},
     {"set_state",                 (PyCFunction)py_engine_method_set_state,               METH_VARARGS|METH_KEYWORDS,
      "Set state in simulation engine.\n"
      "\n"
      "@module:     module name\n"
      "@state:      state name or enumerated id\n"
      "@value:      value(s) to set\n"
      "@mask:       mask(s) for value(s) (or None)\n"
      "@what:       what part of state to set (if memory/vector)\n"
      "\n"
      "Set state contents of simulation engine.  The 'mask' bits\n"
      "indicate which bits of the state value to set.  If 'mask' is\n"
      "None then all bits in the state is set.\n"
      "\n"
      "When 'state' indicates a memory 'value' and 'mask' can be\n"
      "lists of values.  If 'mask' is a single value then it is\n"
      "applied to all entries in the 'value' list.\n"
      "\n"
      "The 'what' parameter can be used to indicate which parts of\n"
      "the memory state to modify.  A single int indicates a memory\n"
      "word with a specific index.  A slice object indicates multiple\n"
      "memory entries.\n"
      "\n"
      "Throw ValueError if 'module' or 'state' can not be found."},
     {"get_state_value_string",    (PyCFunction)py_engine_method_get_state_value_string,  METH_VARARGS|METH_KEYWORDS, "Get string value of global or module state."},
     {"set_array_state",           (PyCFunction)py_engine_method_set_array_state,         METH_VARARGS|METH_KEYWORDS,"Set an element of an array of state to a certain value, with optional mask."},
     {"checkpoint_init",           (PyCFunction)py_engine_method_checkpoint_init,         METH_VARARGS|METH_KEYWORDS, "Initialize checkpointing"},
     {"checkpoint_add",            (PyCFunction)py_engine_method_checkpoint_add,          METH_VARARGS|METH_KEYWORDS,  "Add a checkpoint"},
     {"checkpoint_info",           (PyCFunction)py_engine_method_checkpoint_info,         METH_VARARGS|METH_KEYWORDS, "Get information about a checkpoint"},
     {"checkpoint_restore",        (PyCFunction)py_engine_method_checkpoint_restore,      METH_VARARGS|METH_KEYWORDS, "Restore a checkpoint"},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

/*v py_engine_PyTypeObject_frame
 */
static PyTypeObject py_engine_PyTypeObject_frame = {
    PyObject_HEAD_INIT(NULL)
    0,
    "Engine", // for printing
    sizeof( t_py_engine_PyObject ), // basic size
    0, // item size
    py_engine_dealloc, /*tp_dealloc*/
    py_engine_print,   /*tp_print*/
    py_engine_getattr,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    py_engine_repr,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
	0,			/* tp_call - called if the object itself is invoked as a method */
	py_engine_str, /*tp_str */
};

/*a Statics for py_engine module
 */
/*v py_engine_error
 */
//static PyObject *py_engine_error;
static PyObject *result;

/*v py_engine_methods
 */
static PyMethodDef py_engine_methods[] =
{
    {"py_engine", py_engine_new, METH_VARARGS, "Create a new engine object."},
    {"debug", (PyCFunction)py_engine_debug, METH_VARARGS|METH_KEYWORDS, "Enable debugging and set level."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

/*a Python method sl functions
 */
/*f py_engine_method_enter
 */
static const char *where_last;
static void py_engine_method_enter( t_py_engine_PyObject *py_eng, const char *where, PyObject *args )
{
 #ifdef PY_DEBUG
    fprintf( stderr, "Enter '%s'\n", where );
    fflush( stderr );
#endif
    where_last = where;
	result = NULL;
}

/*f py_engine_method_return
 */
static PyObject *py_engine_method_return( t_py_engine_PyObject *py_eng, const char *string )
{
 #ifdef PY_DEBUG
    fprintf( stderr, "Returning from '%s' with '%s'\n", where_last, string?string:"<NULL>" );
    fflush( stderr );
#endif
    if (string)
    {
        PyErr_SetString( PyExc_RuntimeError, string );
        Py_CLEAR(result);
        return NULL;
    }
    if (result)
    {
        PyObject *temp = result;
        result = NULL;
        return temp;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

/*f py_engine_method_error
 */
static PyObject *py_engine_method_error( t_py_engine_PyObject *py_eng )
{
 #ifdef PY_DEBUG
    fprintf( stderr, "Exception in '%s'\n", where_last );
    fflush( stderr );
#endif
    Py_CLEAR(result);
    return NULL;
}

/*f py_engine_method_result_empty_list
 */
static void py_engine_method_result_empty_list( void *handle )
{
        if (result)
            Py_CLEAR( result );
        result = PyList_New( 0 );
}

/*f py_engine_method_result_add_int
 */
static void py_engine_method_result_add_int( void *handle, t_sl_uint64 value )
{
	PyObject *obj;

	if (!result)
	{
		result = PyInt_FromLong( value );
		return;
	}
	if (!PyObject_TypeCheck( result, &PyList_Type) )
	{
		obj = PyList_New( 0 );
		PyList_Append( obj, result );
		result = obj;
	}
	obj = PyInt_FromLong( value );
	PyList_Append( result, obj );
}

/*f py_engine_method_result_add_string
 */
static void py_engine_method_result_add_string( void *handle, const char *string )
{
	PyObject *obj;

	if (!result)
	{
		result = PyString_FromString( string?string:"" );
		return;
	}
	if (!PyObject_TypeCheck( result, &PyList_Type) )
	{
		obj = PyList_New( 0 );
		PyList_Append( obj, result );
		result = obj;
	}
	obj = PyString_FromString( string?string:"" );
	PyList_Append( result, obj );
}

/*f py_engine_method_result_add_sized_string
 */
// static void py_engine_method_result_add_sized_string( void *handle, char *string, int size )
// {
//      PyObject *obj;

//      if (!result)
//      {
//           result = PyString_FromStringAndSize( string, size );
//           return;
//      }
//      if (!PyObject_TypeCheck( result, &PyList_Type) )
//      {
//           obj = PyList_New( 0 );
//           PyList_Append( obj, result );
//           result = obj;
//      }
//      obj = PyString_FromStringAndSize( string, size );
//      PyList_Append( result, obj );
// }

/*f py_engine_method_int64_from_obj
  Return 1 if object is convertable to a t_sl_uint64
  Return 0 if not
 */
static int py_engine_method_int64_from_obj( PyObject *obj, t_sl_uint64 *value )
{
    if (PyInt_Check(obj))
    {
        *value = PyInt_AsSsize_t(obj);
        return 1;
    }
    if (PyLong_Check(obj))
    {
        *value = PyLong_AsUnsignedLongLongMask(obj);
        return 1;
    }
    return 0;
}

/*f py_list_from_cons_list
 */
static PyObject *py_list_from_cons_list( t_sl_cons_list *cl )
{
     PyObject *obj, *sub_obj;
     t_sl_cons *item;

     obj = PyList_New( 0 );
     for (item=cl->first; item; item=item->next_in_list)
     {
          sub_obj = NULL;
          switch (item->type)
          {
          case sl_cons_type_string:
               sub_obj = PyString_FromString( item->data.string );
               break;
          case sl_cons_type_integer:
               sub_obj = PyInt_FromLong( (long) item->data.integer );
               break;
          case sl_cons_type_ptr:
               sub_obj = PyInt_FromLong( (long) item->data.ptr );
               break;
          case sl_cons_type_cons_list:
               sub_obj = py_list_from_cons_list( &item->data.cons_list );
               break;
          default:
               break;
          }
          if (sub_obj)
          {
               PyList_Append( obj, sub_obj );
          }
     }
     return obj;
}

/*f py_engine_method_result_add_cons_list
 */
static void py_engine_method_result_add_cons_list( void *handle, t_sl_cons_list *cl )
{
	PyObject *obj;

	if ( (result) &&
         !PyObject_TypeCheck( result, &PyList_Type) )
	{
		obj = PyList_New( 0 );
		PyList_Append( obj, result );
		result = obj;
	}
	obj = py_list_from_cons_list( cl );
	if (!result)
	{
         result = obj;
	}
    else
    {
         PyList_Append( result, obj );
    }
}

/*a Support functions
 */
/*f find_ih_from_module_and_state_or_id
  module_name is required
  state_name is the name of the state being interrogated - if an 'id' is being supplied, then this is ignored
  id should be the interrogate index for the module for mask ALL or -1 if not known (the common case)
 */
static int find_ih_from_module_and_state_or_id( t_py_engine_PyObject *py_eng, const char *module_name, const char *state_name, int id, t_se_interrogation_handle *sub_ih )
{
    t_se_interrogation_handle ih;

    /*b Check arguments
     */
    if ((id==-1) && (!state_name))
    {
        PyErr_SetString( PyExc_TypeError, "required parameter 'id' or 'state' not provided" );
        return 0;
    }

    /*b Lookup module
      Given module/id/state_name, and ih, find a sub_ih or set an error
    */
    ih = py_eng->engine->find_entity( module_name );
    if (!ih)
    {
        PyErr_SetObject( PyExc_ValueError, PyString_FromFormat("invalid module name: %s", module_name) );
        return 0;
    }

    /*b Find sub_ih from state_name if provided
    */
    *sub_ih = NULL;
    if (state_name)
    {
        py_eng->engine->interrogate_find_entity( ih, state_name, (t_engine_state_desc_type_mask) -1 /* type mask */, engine_interrogate_include_mask_all, sub_ih );
        if (!*sub_ih)
        {
            PyErr_SetObject( PyExc_ValueError, PyString_FromFormat("invalid state: %s", state_name) );
        }
    }
    else
    {
        py_eng->engine->interrogate_enumerate_hierarchy( ih, id, (t_engine_state_desc_type_mask) -1 /* type mask */, engine_interrogate_include_mask_all, sub_ih );
        if (!*sub_ih)
        {
            PyErr_SetObject( PyExc_ValueError, PyString_FromFormat("invalid state: %d", id) );
        }
    }

    /*b Done
     */
    py_eng->engine->interrogation_handle_free( ih );
    return ((*sub_ih)!=NULL);
}

/*f find_data_info_from_module_and_state_or_id
  module_name is required
  state_name is the name of the state being interrogated - if an 'id' is being supplied, then this is ignored
  id should be the interrogate index for the module for mask ALL or -1 if not known (the common case)
 */
static t_engine_state_desc_type find_data_info_from_module_and_state_or_id( t_py_engine_PyObject *py_eng, const char *module_name, const char *state_name, int id, t_se_signal_value **data, int *sizes )
{
    t_engine_state_desc_type state_desc_type;
    t_se_interrogation_handle ih;

    /*b Get handles
     */
    if (!find_ih_from_module_and_state_or_id( py_eng, module_name, state_name, id, &ih ))
    {
        return engine_state_desc_type_none;
    }

    /*b Get data about the state
     */
    state_desc_type = py_eng->engine->interrogate_get_data_sizes_and_type( ih, data, sizes );
    py_eng->engine->interrogation_handle_free( ih );

    /*b Return result
     */
    return state_desc_type;
}

/*a Python-called engine class methods
 */
/*f py_engine_method_setenv
 */
static PyObject *py_engine_method_setenv( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    const char *keyword, *value;

    py_engine_method_enter( py_eng, "setenv", args );

    static const char *kwdlist[] = {"keyword", "value", NULL};
    if (PyArg_ParseTupleAndKeywords( args, kwds, "ss",
                                     (char **)kwdlist,
                                     &keyword, &value))
    {
        py_eng->env_options = sl_option_list( py_eng->env_options, keyword, value );
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_read_hw_file
 */
static PyObject *py_engine_method_read_hw_file( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *filename;
     char fn[] = "filename";
     char *kwdlist[] = { fn, NULL };
     t_sl_error_level error_level, worst_error_level;

     py_engine_method_enter( py_eng, "read_hw_file", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &filename))
     {
          py_eng->engine->delete_instances_and_signals();
          error_level = py_eng->engine->read_and_interpret_hw_file( filename, (t_sl_get_environment_fn)sl_option_get_string, (void *)py_eng->env_options );
          worst_error_level = error_level;
          if ((int)error_level <= (int)error_level_warning)
          {
               error_level = py_eng->engine->check_connectivity();
               worst_error_level = ((int)error_level>(int)worst_error_level)?error_level:worst_error_level;
          }
          if ((int)error_level <= (int)error_level_warning)
          {
               error_level = py_eng->engine->build_schedule();
               worst_error_level = ((int)error_level>(int)worst_error_level)?error_level:worst_error_level;
          }
          py_engine_method_result_add_int( NULL, (int) worst_error_level );
          sl_option_free_list(py_eng->env_options);
          py_eng->env_options = NULL;
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_describe_hw
 */
static PyObject *py_engine_method_describe_hw( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    PyObject *describer;
    char desc[] = "describer";
    char *kwdlist[] = { desc, NULL };
    t_sl_error_level error_level, worst_error_level;

    WHERE_I_AM;
    py_engine_method_enter( py_eng, "describe_hw", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "O", kwdlist, &describer))
    {
        WHERE_I_AM;
        py_eng->engine->delete_instances_and_signals();
        WHERE_I_AM;
        error_level = py_eng->engine->read_and_interpret_py_object( describer, (t_sl_get_environment_fn)sl_option_get_string, (void *)py_eng->env_options );
        WHERE_I_AM;
        worst_error_level = error_level;
        WHERE_I_AM;
        if ((int)error_level <= (int)error_level_warning)
        {
            WHERE_I_AM;
            error_level = py_eng->engine->check_connectivity();
            WHERE_I_AM;
            worst_error_level = ((int)error_level>(int)worst_error_level)?error_level:worst_error_level;
        }
        WHERE_I_AM;
        if ((int)error_level <= (int)error_level_warning)
        {
            WHERE_I_AM;
            error_level = py_eng->engine->build_schedule();
            WHERE_I_AM;
            worst_error_level = ((int)error_level>(int)worst_error_level)?error_level:worst_error_level;
            WHERE_I_AM;
        }
        WHERE_I_AM;
        py_engine_method_result_add_int( NULL, (int) worst_error_level );
        WHERE_I_AM;
        sl_option_free_list(py_eng->env_options);
        WHERE_I_AM;
        py_eng->env_options = NULL;
        WHERE_I_AM;
        return py_engine_method_return( py_eng, NULL );
    }
    WHERE_I_AM;
    return NULL;
}

/*f py_engine_method_read_gui_file
 */
static PyObject *py_engine_method_read_gui_file( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *filename;
     char fn[] = "filename";
     char *kwdlist[] = { fn, NULL };
     t_sl_error_level result;
     struct t_sl_cons_list cl;

     py_engine_method_enter( py_eng, "read_gui_file", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &filename))
     {
          result = sl_allocate_and_read_indented_file( py_eng->error, filename, "GUI", gui_cmds, &cl );
          if (result!=error_level_okay)
          {
               return py_engine_method_return( py_eng, NULL );
          }
          py_engine_method_result_add_cons_list( NULL, &cl );
          sl_cons_free_list( &cl );
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_reset_errors
 */
static PyObject *py_engine_method_reset_errors( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *kwdlist[] = { NULL };

     py_engine_method_enter( py_eng, "reset_errors", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
     {
          py_eng->error->reset();
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_get_error_level
 */
static PyObject *py_engine_method_get_error_level( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *kwdlist[] = { NULL };

     py_engine_method_enter( py_eng, "get_error_level", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
     {
          py_engine_method_result_add_int( NULL, (int) py_eng->error->get_error_level() );
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_get_error
 */
static PyObject *py_engine_method_get_error( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     int n;
     char er[] = "error";
     char *kwdlist[] = { er, NULL };
     char error_buffer[1024];
     void *handle;

     py_engine_method_enter( py_eng, "get_error", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "i", kwdlist, &n ))
     {
          handle = py_eng->error->get_nth_error( n, error_level_okay ); // get nth error of any level
          if (!py_eng->error->generate_error_message( handle, error_buffer, 1024, 1, NULL ))
          {
               return py_engine_method_return( py_eng, NULL );
          }
          py_engine_method_result_add_string( NULL, error_buffer );
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_cycle
 */
static PyObject *py_engine_method_cycle( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *kwdlist[] = { NULL };

     py_engine_method_enter( py_eng, "cycle", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
     {
          py_engine_method_result_add_int( NULL, (int) py_eng->engine->cycle() );
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_reset
 */
static PyObject *py_engine_method_reset( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *kwdlist[] = { NULL };

     py_engine_method_enter( py_eng, "reset", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
     {
          py_eng->engine->reset_state();
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_step
 */
static PyObject *py_engine_method_step( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     int n;
     int show_msg = 0;
     char cy[] = "cycles";
     char msg[] = "msg";
     char *kwdlist[] = { cy, msg, NULL };

     py_engine_method_enter( py_eng, "step", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "i|i", kwdlist, &n, &show_msg ))
     {
          py_eng->engine->step_cycles( n );
          if (show_msg)
          {
              py_eng->engine->message->check_errors_and_reset( stdout, error_level_info, error_level_info );
          }
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_write_profile
 */
static PyObject *py_engine_method_write_profile( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char filename_s[] = "filename";
    char *kwdlist[] = { filename_s, NULL };
    const char *filename = NULL;


    py_engine_method_enter( py_eng, "write_profile", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &filename ))
    {
        int append = (filename[0]=='+');
        FILE *f = fopen(filename+append,append?"a":"w");
        if (f)
        {
            py_eng->engine->write_profile(f);
            fclose(f);
            return py_engine_method_return( py_eng, NULL );
        }
        PyErr_SetString( PyExc_TypeError, "failed to open file to write profile to" );
        return py_engine_method_error( py_eng );
    }
    return NULL;
}

/*f py_engine_method_thread_pool_init
 */
static PyObject *py_engine_method_thread_pool_init( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char *kwdlist[] = { NULL };

    py_engine_method_enter( py_eng, "thread_pool", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
    {
        py_eng->engine->thread_pool_init();
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_thread_pool_add
 */
static PyObject *py_engine_method_thread_pool_add( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char nm[] = "name";
    char *kwdlist[] = { nm, NULL };
    char *name;

    py_engine_method_enter( py_eng, "thread_pool_add", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &name ))
    {
        t_sl_error_level result = py_eng->engine->thread_pool_add_thread( name );
        if (result!=error_level_okay)
        {
            py_engine_method_result_add_int( NULL, 0 );
        }
        py_engine_method_result_add_int( NULL, 1 );
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_thread_pool_map_module
 */
static PyObject *py_engine_method_thread_pool_map_module( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char tnm[] = "thread_name";
    char mnm[] = "module_name";
    char *kwdlist[] = { tnm, mnm, NULL };
    char *thread_name, *module_name;

    py_engine_method_enter( py_eng, "thread_pool_add", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "ss", kwdlist, &thread_name, &module_name ))
    {
        t_sl_error_level result = py_eng->engine->thread_pool_map_thread_to_module( thread_name, module_name );
        if (result!=error_level_okay)
        {
            py_engine_method_result_add_int( NULL, 0 );
            return py_engine_method_return( py_eng, NULL );
        }
        py_engine_method_result_add_int( NULL, 1 );
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_thread_pool_delete
 */
static PyObject *py_engine_method_thread_pool_delete( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char *kwdlist[] = { NULL };

    py_engine_method_enter( py_eng, "thread_pool", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
    {
        py_eng->engine->thread_pool_delete();
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_list_instances
 */
static PyObject *py_engine_method_list_instances( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char *kwdlist[] = { NULL };
     t_sl_error_level error;
     struct t_sl_cons_list cl;

     py_engine_method_enter( py_eng, "list_instances", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
     {
          error = py_eng->engine->enumerate_instances( &cl );
          if (error!=error_level_okay)
          {
               return py_engine_method_return( py_eng, NULL );
          }
          py_engine_method_result_add_cons_list( NULL, &cl );
          sl_cons_free_list( &cl );
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_find_state
 */
static PyObject *py_engine_method_find_state( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char nm[] = "name";
     char *kwdlist[] = { nm, NULL };
     char *name;
     int type, id;

     py_engine_method_enter( py_eng, "find_state", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &name ))
     {
          type = py_eng->engine->find_state( name, &id );
          if ( type==0 )
          {
               return py_engine_method_return( py_eng, NULL );
          }
          py_engine_method_result_add_int( NULL, type );
          py_engine_method_result_add_string( NULL, name );
          if (type==2)
          {
               py_engine_method_result_add_int( NULL, id );
               py_engine_method_result_add_string( NULL, name+strlen(name)+1 );
          }
          return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_get_state_info
 */
static PyObject *py_engine_method_get_state_info( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char md[] = "module";
     char i[] = "id";
     char *kwdlist[] = { md, i, NULL };
     const char *module;
     int id;
     t_engine_state_desc_type type;
     t_sl_uint64 *dummy;
     int sizes[4];

     py_engine_method_enter( py_eng, "get_state_info", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "si", kwdlist, &module, &id ))
     {
         t_se_interrogation_handle sub_ih;
         const char *prefix, *tail;

         if (!find_ih_from_module_and_state_or_id( py_eng, module, NULL, id, &sub_ih ))
         {
             return py_engine_method_error( py_eng );
         }

         if (!py_eng->engine->interrogate_get_entity_strings( sub_ih, &module, &prefix, &tail ))
         {
             return py_engine_method_return( py_eng, NULL );
         }

         sizes[0] = sizes[1] = 0;

         type = py_eng->engine->interrogate_get_data_sizes_and_type( sub_ih, &dummy, sizes );
         py_eng->engine->interrogation_handle_free( sub_ih );
         py_engine_method_result_add_int( NULL, (int) type );
         py_engine_method_result_add_string( NULL, module );
         py_engine_method_result_add_string( NULL, prefix );
         py_engine_method_result_add_string( NULL, tail );
         py_engine_method_result_add_int( NULL, sizes[0] );
         py_engine_method_result_add_int( NULL, sizes[1] );
         return py_engine_method_return( py_eng, NULL );

     }
     return NULL;
}

/*f py_engine_method_get_state
 */
static PyObject *py_engine_method_get_state( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char module_s[] = "module";
    char state_s[] = "state";
    char what_s[] = "what";
    char *kwdlist[] = { module_s, state_s, what_s, NULL };
    const char *module, *state_name = NULL;
    PyObject *state, *what;
    Py_ssize_t start, stop, step, length;
    t_engine_state_desc_type state_desc_type;
    t_se_signal_value *data;
    t_sl_uint64 datamask;
    int sizes[4];
    int idx, err, id = 0;

    py_engine_method_enter( py_eng, "get_state", args );

    what = NULL;
    if (PyArg_ParseTupleAndKeywords( args, kwds, "sO|O", kwdlist, &module, &state, &what ))
    {
        /*b Validate parameters - state is string => state_name, state is int/long -> id, 'what' - just check if it is int/long/slice
         */
        if (PyInt_Check(state))
        {
            id = PyInt_AsLong(state);
        }
        else if (PyString_Check(state))
        {
            state_name = PyString_AsString( state );
        }
        else
        {
            PyErr_SetString( PyExc_TypeError, "argument 2 must be string or int" );
            return py_engine_method_error( py_eng );
        }

        if (what == Py_None)
        {
            what = NULL;
        }
        else if (what && !PyInt_Check(what) && !PySlice_Check(what))
        {
            PyErr_SetString( PyExc_TypeError, "argument 3 must be int or slice" );
            return py_engine_method_error( py_eng );
        }

        /*b Get details of the state from the module name and state name / id
         */
        state_desc_type = find_data_info_from_module_and_state_or_id( py_eng, module, state_name, id, &data, sizes );
        if (state_desc_type == engine_state_desc_type_none)
        {
            return py_engine_method_error( py_eng );
        }

        /*b Retrieve state and produce return value(s)
         */
        if (sizes[0] == 64)
        {
            datamask = ~0ULL;
        }
        else
        {
            datamask = (1ULL << sizes[0]) - 1;
        }

        if (state_desc_type == engine_state_desc_type_bits)
        {
            /* Ignore 'what' parameter if state is a single value */
            py_engine_method_result_add_int( NULL, data[0] & datamask );
            return py_engine_method_return( py_eng, NULL );
        }
        else if (state_desc_type == engine_state_desc_type_array)
        {
            if (!what)
            {
                /* Return whole array */
                for (idx = 0; idx < sizes[1]; idx++)
                {
                    py_engine_method_result_add_int( NULL, data[idx] & datamask );
                }
                return py_engine_method_return( py_eng, NULL );
            }
            else if (PyInt_Check( what ))
            {
                /* Return a single value in array */
                idx = PyInt_AsLong( what );
                if (idx >= sizes[1])
                {
                    PyErr_SetObject( PyExc_IndexError, PyString_FromFormat("state index out of range: %d", idx) );
                    return py_engine_method_error( py_eng );
                }
                py_engine_method_result_add_int( NULL, data[idx] & datamask );
                return py_engine_method_return( py_eng, NULL );
            }
            else
            {
                /* Return slice of array */
                err = PySlice_GetIndicesEx( (PySliceObject *) what, sizes[1], &start, &stop, &step, &length );
                if (err)
                    return py_engine_method_error( py_eng );

                /* Make sure we always return a list type */
                py_engine_method_result_empty_list( NULL );
                for (idx = start; idx < stop; idx += step)
                {
                    py_engine_method_result_add_int( NULL, data[idx] & datamask );
                }
                return py_engine_method_return( py_eng, NULL );
            }
        }
        else
        {
            PyErr_SetString( PyExc_RuntimeError, "state interrogation failed" );
            return py_engine_method_error( py_eng );
        }
     }
     return NULL;
}

/*f py_engine_method_get_state_value_string
 */
static PyObject *py_engine_method_get_state_value_string( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char md[] = "module";
     char id_s[] = "id";
     char state_s[] = "state";
     char *kwdlist[] = { md, id_s, state_s, NULL };
     const char *module, *state_name;
     char buffer[256];
     int id;

     /* The way we handle parameters below is not very intuitive or pythonesque, but legacy :-( */

     py_engine_method_enter( py_eng, "get_state_value_string", args );

     id = -1;
     state_name = NULL;
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s|is", kwdlist, &module, &id, &state_name ))
     {
         t_se_interrogation_handle sub_ih;

        /*b Given module/id/state_name, and ih, find a sub_ih or set an error
         */
         if (!find_ih_from_module_and_state_or_id( py_eng, module, state_name, id, &sub_ih ))
         {
             return py_engine_method_error( py_eng );
         }

         py_eng->engine->interrogate_get_entity_value_string( sub_ih, buffer, sizeof(buffer) );
         py_eng->engine->interrogation_handle_free( sub_ih );
         py_engine_method_result_add_string( NULL, buffer );
         return py_engine_method_return( py_eng, NULL );

     }
     return NULL;
}

/*f py_engine_method_get_state_memory
 */
static PyObject *py_engine_method_get_state_memory( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char md[] = "module";
    char id_s[] = "id";
    char st[] = "start";
    char ln[] = "length";
    char state_s[] = "state";
    char *kwdlist[] = { md, id_s, st, ln, state_s, NULL };
    char *module, *state_name, buffer[256];
    int id, start, length;
    int sizes[4];
    t_se_signal_value *data;
    t_engine_state_desc_type state_desc_type;
    int i;

    /* XXX The way we handle parameters below is not very intuitive or pythonesque */

    py_engine_method_enter( py_eng, "get_state_memory", args );

    id = -1;
    start = -1;
    length = -1;
    state_name = NULL;
    if (PyArg_ParseTupleAndKeywords( args, kwds, "s|iiis", kwdlist, &module, &id, &start, &length, &state_name ))
    {
        /*b Validate parameters
         */
        if (start==-1)
        {
            PyErr_SetString( PyExc_TypeError, "required parameter 'start' (pos 3) not found" );
            return py_engine_method_error( py_eng );
        }

        if (length==-1)
        {
            PyErr_SetString( PyExc_TypeError, "required parameter 'length' (pos 4) not found" );
            return py_engine_method_error( py_eng );
        }

        /*b Get details of the state from the module name and state name / id
         */
        state_desc_type = find_data_info_from_module_and_state_or_id( py_eng, module, state_name, id, &data, sizes );
        if (state_desc_type == engine_state_desc_type_none)
        {
            return py_engine_method_error( py_eng );
        }

        /*b Print state to a buffer and return that
         */
        for (i=0; i<length; i++)
        {
            if ((i+start>=0) || (i+start<sizes[1]))
            {
                sl_print_bits_hex( buffer, sizeof(buffer), ((int *)data)+(i+start)*BITS_TO_INTS(sizes[0]), sizes[0] );
                py_engine_method_result_add_string( NULL, buffer );
            }
        }
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_set_state
 */
static PyObject *py_engine_method_set_state( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char module_s[] = "module";
    char state_s[] = "state";
    char value_s[] = "value";
    char mask_s[] = "mask";
    char what_s[] = "what";
    char *kwdlist[] = { module_s, state_s, value_s, mask_s, what_s, NULL };
    const char *module, *state_name = NULL;
    PyObject *state, *value, *mask, *what;
    Py_ssize_t start, stop, step, length;
    t_engine_state_desc_type state_desc_type;
    t_se_signal_value *data;
    t_sl_uint64 datamask, dataval, maskval = 0;
    int sizes[4];
    int idx, n, err, id = 0;

    py_engine_method_enter( py_eng, "get_state", args );

    mask = NULL;
    what = NULL;
    if (PyArg_ParseTupleAndKeywords( args, kwds, "sOO|OO", kwdlist, &module, &state, &value, &mask, &what ))
    {
        /*b Validate parameters
         */
        if (PyInt_Check( state ))
        {
            id = PyInt_AsLong( state );
        }
        else if (PyString_Check( state ))
        {
            state_name = PyString_AsString( state );
        }
        else
        {
            PyErr_SetString( PyExc_TypeError, "argument 2 must be string or int" );
            return py_engine_method_error( py_eng );
        }

        if (PyList_Check( value ))
        {
            length = PyList_Size( value );
            if (length == 0)
            {
                PyErr_SetString( PyExc_ValueError, "len(value) == 0" );
                return py_engine_method_error( py_eng );
            }

            for (idx = 0; idx < length; idx++)
            {
                PyObject *obj = PyList_GET_ITEM( value, idx );
                t_sl_uint64 dummy;
                if (!py_engine_method_int64_from_obj( obj, &dummy ))
                {
                    PyErr_SetObject( PyExc_ValueError, PyString_FromFormat("value[%d] is not an int", idx) );
                    return py_engine_method_error( py_eng );
                }
            }
        }
        else if (!PyInt_Check( value ) && !PyLong_Check( value ))
        {
            PyErr_SetString( PyExc_TypeError, "argument 3 must be int or list of int" );
            return py_engine_method_error( py_eng );
        }

        if (mask == Py_None || !mask)
        {
            mask = NULL;
            maskval = ~0UL;
        }
        else if (PyList_Check( mask ))
        {
            length = PyList_Size( mask );
            if (length == 0)
            {
                PyErr_SetString( PyExc_ValueError, "len(mask) == 0" );
                return py_engine_method_error( py_eng );
            }
            else if (PyList_Check( value ) && (length < PyList_Size( value )))
            {
                PyErr_SetString( PyExc_ValueError, "len(mask) < len(value)" );
                return py_engine_method_error( py_eng );
            }

            for (idx = 0; idx < length; idx++)
            {
                PyObject *obj = PyList_GET_ITEM( mask, idx );
                t_sl_uint64 dummy;
                if (!py_engine_method_int64_from_obj( obj, &dummy ))
                {
                    PyErr_SetObject( PyExc_ValueError, PyString_FromFormat("mask[%d] is not an int", idx) );
                    return py_engine_method_error( py_eng );
                }
            }
            py_engine_method_int64_from_obj( PyList_GetItem( mask, 0 ), &maskval);
        }
        else if (py_engine_method_int64_from_obj( mask, &maskval))
        {
        }
        else
        {
            PyErr_SetString( PyExc_TypeError, "argument 4 must be int or list of int" );
            return py_engine_method_error( py_eng );
        }

        if (what == Py_None)
        {
            what = NULL;
        }
        else if (what != NULL && !PyInt_Check( what ) && !PySlice_Check( what ))
        {
            PyErr_SetString( PyExc_TypeError, "argument 5 must be int or slice" );
            return py_engine_method_error( py_eng );
        }

        /*b Get details of the state from the module name and state name / id
         */
        state_desc_type = find_data_info_from_module_and_state_or_id( py_eng, module, state_name, id, &data, sizes );
        if (state_desc_type == engine_state_desc_type_none)
        {
            return py_engine_method_error( py_eng );
        }

        /*b Modify state
         */
        if (sizes[0] == 64)
            datamask = ~0ULL;
        else
            datamask = (1ULL << sizes[0]) - 1;

        if (state_desc_type == engine_state_desc_type_bits)
        {
            if (!py_engine_method_int64_from_obj( value, &dataval)) // If we cannot get an int64 from the value, then we must be a list of values that we CAN
            {
                py_engine_method_int64_from_obj( PyList_GetItem( value, 0 ), &dataval);
            }

            data[0] = ((data[0] & ~maskval) | (dataval & maskval)) & datamask;
            return py_engine_method_return( py_eng, NULL );
        }
        else if (state_desc_type == engine_state_desc_type_array)
        {
            if (what == 0)
            {
                /* Modify whole array */
                if (!PyList_Check( value ))
                {
                    PyErr_SetString( PyExc_TypeError, "argument 3 must be list of int to set the state of an array" );
                    return py_engine_method_error( py_eng );
                }
                if (PyList_Size( value ) != sizes[1])
                {
                    PyErr_SetObject( PyExc_IndexError, PyString_FromFormat("size of value list for state array must be %d entries", sizes[1]) );
                    return py_engine_method_error( py_eng );
                }
                for (idx = 0; idx < sizes[1]; idx++)
                {
                    py_engine_method_int64_from_obj( PyList_GetItem( value, idx ), &dataval);
                    if (mask && PyList_Check( mask ))
                    {
                        py_engine_method_int64_from_obj( PyList_GetItem( mask, idx ), &maskval);
                    }
                    data[idx] = ((data[idx] & ~maskval) | (dataval & maskval)) & datamask;
                }
                return py_engine_method_return( py_eng, NULL );
            }
            else if (PyInt_Check( what )) // If what is an int, then what is the index of the array that we need to set with datamask
            {
                /* Modify single entry */
                idx = PyInt_AsLong( what );
                if (idx >= sizes[1])
                {
                    PyErr_SetObject( PyExc_IndexError, PyString_FromFormat("state index out of range: %d", idx) );
                    return py_engine_method_error( py_eng );
                }
                if (!py_engine_method_int64_from_obj( value, &dataval)) // If we cannot get an int64 from the value, then we must be a list of values that we CAN
                {
                    py_engine_method_int64_from_obj( PyList_GetItem( value, 0 ), &dataval);
                }

                data[0] = ((data[idx] & ~maskval) | (dataval & maskval)) & datamask;
                return py_engine_method_return( py_eng, NULL );
            }
            else
            {
                /* Modify slice of array */
                err = PySlice_GetIndicesEx( (PySliceObject *) what, sizes[1], &start, &stop, &step, &length );
                if (err)
                    return py_engine_method_error( py_eng );

                if (PyList_Size( value ) != length)
                {
                    PyErr_SetObject( PyExc_IndexError, PyString_FromFormat("size of value list must be %ld entries", length) );
                    return py_engine_method_error( py_eng );
                }
                for (idx = start, n = 0; idx < stop; idx += step, n++)
                {
                    py_engine_method_int64_from_obj( PyList_GetItem( value, n ), &dataval);
                    if (mask && PyList_Check( mask ))
                    {
                        py_engine_method_int64_from_obj( PyList_GetItem( mask, n ), &maskval);
                    }
                    data[idx] = ((data[idx] & ~maskval) | (dataval & maskval)) & datamask;
                }
                return py_engine_method_return( py_eng, NULL );
            }
        }
        else
        {
            PyErr_SetString( PyExc_RuntimeError, "state interrogation failed" );
            return py_engine_method_error( py_eng );
        }
     }
     return NULL;
}

/*f py_engine_method_set_array_state
 */
static PyObject *py_engine_method_set_array_state( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char *module, *state_name;
    int id, index;
    t_sl_uint64 value, mask;

    /* ESK: The way we handle parameters below is not very intuitive or pythonesque */
    /* ESK: We should be doing proper exception reporting below */
    /* ESK: Needed to put 'id' and 'state' parameter last in parameter list */
    py_engine_method_enter( py_eng, "set_array_state", args );

    mask = ~0;
    id = -1;
    state_name = NULL;
    static const char *kwdlist[] = { "module", "id", "value", "mask", "index", "state", NULL };
    if (PyArg_ParseTupleAndKeywords( args, kwds, "siL|Lis",
                                     (char **)kwdlist,
                                     &module, &id, &value, &mask, &index, &state_name ))
    {
        t_engine_state_desc_type state_desc_type;
        t_se_signal_value *data;
        int sizes[4];

        /*b Get details of the state from the module name and state name / id
         */
        state_desc_type = find_data_info_from_module_and_state_or_id( py_eng, module, state_name, id, &data, sizes );
        if (state_desc_type == engine_state_desc_type_none)
        {
            return py_engine_method_error( py_eng );
        }

        if (state_desc_type != engine_state_desc_type_array)
        {
            return NULL;
        }
        if ((index<0) || (index>=sizes[1]))
        {
            return NULL;
        }
        data[index] = ((data[index] & ~mask) | value) & ((1ULL<<sizes[0])-1);

        if (sizes[0]==64)
        {
            data[index] = ((data[index] & ~mask) | value);
        }
        return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_get_module_ios
 */
static PyObject *py_engine_method_get_module_ios( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char md[] = "module";
     char *kwdlist[] = { md, NULL };
     const char *module;
     t_engine_state_desc_type type;
     t_sl_uint64 *dummy;
     int sizes[4];
     int id;
     t_engine_interrogate_include_mask id_type;

     py_engine_method_enter( py_eng, "get_module_ios", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &module ))
     {
         t_se_interrogation_handle ih, sub_ih;
         struct t_sl_cons_list list, typelist, sublist;
         const char *prefix, *tail;

         ih = py_eng->engine->find_entity( module );
         if (!ih)
         {
             return py_engine_method_return( py_eng, NULL );
         }
         sl_cons_reset_list( &list );
         for (id_type = engine_interrogate_include_mask_clocks; id_type < engine_interrogate_include_mask_all; id_type = (t_engine_interrogate_include_mask)(id_type << 1))
         {
             sl_cons_reset_list( &typelist );
             for (id = 0; ; id++)
             {
                 sub_ih = NULL;
                 py_eng->engine->interrogate_enumerate_hierarchy( ih, id, (t_engine_state_desc_type_mask) -1 /* type mask */, id_type, &sub_ih );
                 if (!sub_ih)
                 {
                     break;
                 }
                 if (!py_eng->engine->interrogate_get_entity_strings( sub_ih, &module, &prefix, &tail ))
                 {
                     break;
                 }
                 type = py_eng->engine->interrogate_get_data_sizes_and_type( sub_ih, &dummy, sizes );
                 py_eng->engine->interrogation_handle_free( sub_ih );
                 sl_cons_reset_list( &sublist );
                 sl_cons_append( &sublist, sl_cons_item( (char*)tail, 1 ));
                 sl_cons_append( &sublist, sl_cons_item( sizes[0] ));
                 sl_cons_append( &typelist, sl_cons_item( &sublist ));
             }
             sl_cons_append( &list, sl_cons_item( &typelist ));
         }
         py_eng->engine->interrogation_handle_free( ih );
         py_engine_method_result_add_cons_list( NULL, &list );
         sl_cons_free_list( &list );
         return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_checkpoint_init
 */
static PyObject *py_engine_method_checkpoint_init( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char *kwdlist[] = { NULL };

    py_engine_method_enter( py_eng, "checkpoint_init", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
    {
        py_eng->engine->initialize_checkpointing();
        //py_eng->engine->checkpointing_display_information(100);
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_checkpoint_info
 */
static PyObject *py_engine_method_checkpoint_info( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char ck[] = "checkpoint";
    char *kwdlist[] = { ck, NULL };
    char *ckpt;
    const char *parent;
    int time;
    const char *next;

    py_engine_method_enter( py_eng, "checkpoint_info", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &ckpt ))
    {
        py_eng->engine->checkpoint_get_information( ckpt, &parent, &time, &next );
        py_engine_method_result_add_string( NULL, parent );
        py_engine_method_result_add_int( NULL, time );
        py_engine_method_result_add_string( NULL, next );
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_checkpoint_add
 */
static PyObject *py_engine_method_checkpoint_add( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char ck[] = "checkpoint";
    char pv[] = "previous";
    char *kwdlist[] = { ck, pv, NULL };
    char *ckpt, *previous;

    py_engine_method_enter( py_eng, "checkpoint_add", args );
    previous = NULL;
    if (PyArg_ParseTupleAndKeywords( args, kwds, "s|s", kwdlist, &ckpt, &previous ))
    {
        py_engine_method_result_add_int( NULL, (int) py_eng->engine->checkpoint_add( ckpt, previous ) );
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*f py_engine_method_checkpoint_restore
 */
static PyObject *py_engine_method_checkpoint_restore( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    char ck[] = "checkpoint";
    char *kwdlist[] = { ck, NULL };
    char *ckpt;

    py_engine_method_enter( py_eng, "checkpoint_restore", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &ckpt ))
    {
        py_engine_method_result_add_int( NULL, (int) py_eng->engine->checkpoint_restore( ckpt ) );
        return py_engine_method_return( py_eng, NULL );
    }
    return NULL;
}

/*a Python-callled engine class functions
 */
/*f py_engine_getattr
 */
static PyObject *py_engine_getattr( PyObject *self, char *name)
{
    return Py_FindMethod( engine_methods, self, name);
}

/*f py_engine_print
 */
static int py_engine_print( PyObject *self, FILE *f, int unknown )
{
    t_py_engine_PyObject *py_eng;
	py_eng = (t_py_engine_PyObject *)self;
//	py_eng->engine->print_debug_info();
	return 0;
}

/*f py_engine_repr
 */
static PyObject *py_engine_repr( PyObject *self )
{
	return Py_BuildValue( "s", "cycle simulation engine" );
}

/*f py_engine_str
 */
static PyObject *py_engine_str( PyObject *self )
{
	return Py_BuildValue( "s", "string representing in human form the engine" );
}

/*f py_engine_new
 */
static PyObject *py_engine_new( PyObject* self, PyObject* args )
{
	 t_py_engine_PyObject *py_eng;

    if (!PyArg_ParseTuple(args,":new"))
	{
        return NULL;
	}

    py_eng = PyObject_New( t_py_engine_PyObject, &py_engine_PyTypeObject_frame );
	py_eng->error = new c_sl_error( 4, 8 ); // GJS
	py_eng->engine = new c_engine( py_eng->error, "default engine" );
    py_eng->env_options = NULL;

    return (PyObject*)py_eng;
}

/*f py_engine_debug
 */
static PyObject *py_engine_debug( PyObject* self, PyObject* args, PyObject *kwds )
{
	 t_py_engine_PyObject *py_eng;
     char lv[] = "level";
     char *kwdlist[] = { lv, NULL };
     int level;

     py_engine_method_enter( py_eng, "debug", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "i", kwdlist, &level ))
     {
          sl_debug_set_level( (t_sl_debug_level)level );
          sl_debug_enable(1);
          return Py_None;
     }
     sl_debug_enable(0);
     return Py_None;
}

/*f py_engine_dealloc
 */
static void py_engine_dealloc( PyObject* self )
{
    PyObject_Del(self);
}

/*a C code for py_engine
 */
extern "C" void initpy_engine( void )
{
    PyObject *m;
    int i;
    se_c_engine_init();
    m = Py_InitModule3("py_engine", py_engine_methods, "Python interface to CDL simulation engine" );

    // This creates the class py_engine.exec_file, defined in sl_exec_file.cpp
    sl_exec_file_python_add_class_object( m );

    for (i=0; model_init_fns[i]; i++)
    {
         model_init_fns[i]();
    }
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

