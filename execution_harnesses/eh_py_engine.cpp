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
static PyObject *py_engine_method_list_instances( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_find_state( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state_info( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state_value_string( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_engine_method_get_state_memory( t_py_engine_PyObject *py_cyc, PyObject *args, PyObject *kwds );
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
     {"cycle",                     (PyCFunction)py_engine_method_cycle,                   METH_VARARGS|METH_KEYWORDS, "Reset the simulation engine."},
     {"step",                      (PyCFunction)py_engine_method_step,                    METH_VARARGS|METH_KEYWORDS, "Step the simulation engine a number of cycles."},
     {"list_instances",            (PyCFunction)py_engine_method_list_instances,          METH_VARARGS|METH_KEYWORDS, "List toplevel instances."},
     {"find_state",                (PyCFunction)py_engine_method_find_state,              METH_VARARGS|METH_KEYWORDS, "Get information about global or module state."},
     {"get_state_info",            (PyCFunction)py_engine_method_get_state_info,          METH_VARARGS|METH_KEYWORDS, "Get information about global or module state."},
     {"get_state_value_string",    (PyCFunction)py_engine_method_get_state_value_string,  METH_VARARGS|METH_KEYWORDS, "Get string value of global or module state."},
     {"get_state_memory",          (PyCFunction)py_engine_method_get_state_memory,        METH_VARARGS|METH_KEYWORDS, "Get string value of global or module state."},
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
		return NULL;
	}
	if (result)
		return result;

	Py_INCREF(Py_None);
	return Py_None;
}

/*f py_engine_method_result_add_double
 */
// static void py_engine_method_result_add_double( void *handle, double d )
// {
// 	PyObject *obj;

// 	if (!result)
// 	{
// 		result = PyFloat_FromDouble( d );
// 		return;
// 	}
// 	if (!PyObject_TypeCheck( result, &PyList_Type) )
// 	{
// 		obj = PyList_New( 0 );
// 		PyList_Append( obj, result );
// 		result = obj;
// 	}
// 	obj = PyFloat_FromDouble( d );
// 	PyList_Append( result, obj );
// }

/*f py_engine_method_result_add_int
 */
static void py_engine_method_result_add_int( void *handle, int i )
{
	PyObject *obj;

	if (!result)
	{
		result = PyInt_FromLong( (long) i );
		return;
	}
	if (!PyObject_TypeCheck( result, &PyList_Type) )
	{
		obj = PyList_New( 0 );
		PyList_Append( obj, result );
		result = obj;
	}
	obj = PyInt_FromLong( (long) i );
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

/*a Python-called engine class methods
 */
/*f py_engine_method_setenv
 */
static PyObject *py_engine_method_setenv( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
    const char *keyword, *value;
    char kw[] = "keyword";
    char vl[] = "value";
    char *kwdlist[] = { kw, vl, NULL };

    py_engine_method_enter( py_eng, "setenv", args );
    if (PyArg_ParseTupleAndKeywords( args, kwds, "ss", kwdlist, &keyword, &value))
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
	    py_eng->engine->message->check_errors_and_reset( stdout, error_level_info, error_level_info );
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
         t_se_interrogation_handle ih, sub_ih;
         const char *prefix, *tail;

         ih = py_eng->engine->find_entity( module );
         if (!ih)
         {
             return py_engine_method_return( py_eng, NULL );
         }
         sub_ih = NULL;
         py_eng->engine->interrogate_enumerate_hierarchy( ih, id, (t_engine_state_desc_type_mask) -1 /* type mask */, engine_interrogate_include_mask_all, &sub_ih );
         if (!sub_ih)
         {
             return py_engine_method_return( py_eng, NULL );
         }
         if (!py_eng->engine->interrogate_get_entity_strings( sub_ih, &module, &prefix, &tail ))
         {
             return py_engine_method_return( py_eng, NULL );
         }
         type = py_eng->engine->interrogate_get_data_sizes_and_type( sub_ih, &dummy, sizes );
         py_eng->engine->interrogation_handle_free( sub_ih );
         py_eng->engine->interrogation_handle_free( ih );
         //sizes[0] = 0;
         //sizes[1] = 0;
         py_engine_method_result_add_int( NULL, (int) type );
         py_engine_method_result_add_string( NULL, module );
         py_engine_method_result_add_string( NULL, prefix );
         py_engine_method_result_add_string( NULL, tail );
         py_engine_method_result_add_int( NULL, sizes[0] );
         py_engine_method_result_add_int( NULL, sizes[1] );
         return py_engine_method_return( py_eng, NULL );
             // Old
         //handle = py_eng->engine->find_module_instance( module );
         // if ( (!handle) ||
         //      (!py_eng->engine->set_state_handle( handle, id )) )
         // {
         //      return py_engine_method_return( py_eng, NULL );
         // }
         // type = py_eng->engine->get_state_name_and_type( handle, &module, &prefix, &state_name );
         // py_engine_method_result_add_int( NULL, (int) type );
         // py_engine_method_result_add_string( NULL, module );
         // py_engine_method_result_add_string( NULL, prefix );
         // py_engine_method_result_add_string( NULL, state_name );
         // sizes[0] = 0;
         // sizes[1] = 0;
         // py_eng->engine->get_state_value_data_and_sizes( handle, &dummy, sizes );
         // py_engine_method_result_add_int( NULL, sizes[0] );
         // py_engine_method_result_add_int( NULL, sizes[1] );
         // return py_engine_method_return( py_eng, NULL );
     }
     return NULL;
}

/*f py_engine_method_get_state_value_string
 */
static PyObject *py_engine_method_get_state_value_string( t_py_engine_PyObject *py_eng, PyObject *args, PyObject *kwds )
{
     char md[] = "module";
     char id_s[] = "id";
     char *kwdlist[] = { md, id_s, NULL };
     const char *module;
     char buffer[256];
     int id;

     py_engine_method_enter( py_eng, "get_state_value_string", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "si", kwdlist, &module, &id ))
     {
         t_se_interrogation_handle ih, sub_ih;
         const char *prefix, *tail;

         ih = py_eng->engine->find_entity( module );
         if (!ih)
         {
             return py_engine_method_return( py_eng, NULL );
         }
         sub_ih = NULL;
         py_eng->engine->interrogate_enumerate_hierarchy( ih, id, (t_engine_state_desc_type_mask) -1 /* type mask */, engine_interrogate_include_mask_all, &sub_ih );
         if (!sub_ih)
         {
             return py_engine_method_return( py_eng, NULL );
         }
         if (!py_eng->engine->interrogate_get_entity_strings( sub_ih, &module, &prefix, &tail ))
         {
             return py_engine_method_return( py_eng, NULL );
         }
         py_eng->engine->interrogate_get_entity_value_string( sub_ih, buffer, sizeof(buffer) );
         py_eng->engine->interrogation_handle_free( sub_ih );
         py_eng->engine->interrogation_handle_free( ih );
         py_engine_method_result_add_string( NULL, buffer );
         return py_engine_method_return( py_eng, NULL );

         // handle = py_eng->engine->find_module_instance( module );
         // if ( (!handle) ||
         //      (!py_eng->engine->set_state_handle( handle, id )) )
         // {
         //      return py_engine_method_return( py_eng, NULL );
         // }
         // py_eng->engine->get_state_value_string( handle, buffer, sizeof(buffer) );
         // py_engine_method_result_add_string( NULL, buffer );
         // return py_engine_method_return( py_eng, NULL );
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
     char *kwdlist[] = { md, id_s, st, ln, NULL };
     void *handle;
     char *module, buffer[256];
     int id, start, length;
     int sizes[4], *data;
     int i;

     py_engine_method_enter( py_eng, "get_state_memory", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "siii", kwdlist, &module, &id, &start, &length ))
     {
          handle = py_eng->engine->find_module_instance( module );
          if ( (!handle) ||
               (!py_eng->engine->set_state_handle( handle, id )) ||
               (!py_eng->engine->get_state_value_data_and_sizes( handle, &data, sizes )) )
          {
               return py_engine_method_return( py_eng, NULL );
          }
          if (!data)
          {
               return py_engine_method_return( py_eng, NULL );
          }
          for (i=0; i<length; i++)
          {
               if ((i+start>=0) || (i+start<sizes[1]))
               {
                    sl_print_bits_hex( buffer, sizeof(buffer), data+(i+start)*BITS_TO_INTS(sizes[0]), sizes[0] );
                    py_engine_method_result_add_string( NULL, buffer );
               }
          }
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

