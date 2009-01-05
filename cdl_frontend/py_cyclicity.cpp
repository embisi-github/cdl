/*a Copyright
  
  This file 'py_cyclicity.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
/*a Constraint compiler source code
 */

/*a Includes
 */
#include <Python.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>  /* For math functions, cos(), sin(), etc. */
#include <ctype.h>
#include "c_cyclicity.h"
#include "c_cyc_object.h"

/*a Defines
 */
#define PY_DEBUG
#undef PY_DEBUG

/*a Types
 */
/*t t_py_cyclicity_PyObject
 */
typedef struct {
    PyObject_HEAD
	c_cyclicity *cyclicity;
} t_py_cyclicity_PyObject;

/*a Declarations
 */
/*b Declarations of cyclicity class methods
 */
static PyObject *py_cyclicity_method_read_file( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_list_source_files( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_get_file_length( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_read_file_data( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_read_file_line_start_posn( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_read_file_line( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_get_toplevel_handle( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_get_object_handle( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_get_object_data( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );
static PyObject *py_cyclicity_method_get_object_links( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds );

/*b Declarations of cyclicity class functions
 */
static void py_cyclicity_dealloc( PyObject *self );
static int py_cyclicity_print( PyObject *self, FILE *f, int unknown );
static PyObject *py_cyclicity_repr( PyObject *self );
static PyObject *py_cyclicity_str( PyObject *self );
static PyObject *py_cyclicity_new( PyObject* self, PyObject* args );
static PyObject *py_cyclicity_getattr( PyObject *self, char *name);

/*a Statics for cyclicity class
 */
/*v cyclicity_methods - methods supported by the cyclicity class
 */
static PyMethodDef cyclicity_methods[] =
{
    {"read_file",      (PyCFunction)py_cyclicity_method_read_file,       METH_VARARGS|METH_KEYWORDS, "Read file."},
    {"list_source_files",      (PyCFunction)py_cyclicity_method_list_source_files,       METH_VARARGS|METH_KEYWORDS, "List source filenames."},
    {"get_file_length",      (PyCFunction)py_cyclicity_method_get_file_length,       METH_VARARGS|METH_KEYWORDS, "Gets length of file in characters and lines." },
    {"read_file_data",      (PyCFunction)py_cyclicity_method_read_file_data,       METH_VARARGS|METH_KEYWORDS, "Reads file data."},
    {"read_file_line",      (PyCFunction)py_cyclicity_method_read_file_line,       METH_VARARGS|METH_KEYWORDS, "Reads file line."},
    {"read_file_line_start_posn",      (PyCFunction)py_cyclicity_method_read_file_line_start_posn,       METH_VARARGS|METH_KEYWORDS, "Reads file line."},
    {"get_toplevel_handle",      (PyCFunction)py_cyclicity_method_get_toplevel_handle,       METH_VARARGS|METH_KEYWORDS, "Reads file line."},
    {"get_object_handle",      (PyCFunction)py_cyclicity_method_get_object_handle,       METH_VARARGS|METH_KEYWORDS, "Reads file line."},
    {"get_object_data",      (PyCFunction)py_cyclicity_method_get_object_data,       METH_VARARGS|METH_KEYWORDS, "Reads file line."},
    {"get_object_links",      (PyCFunction)py_cyclicity_method_get_object_links,       METH_VARARGS|METH_KEYWORDS, "Reads file line."},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

/*v py_cyclicity_PyTypeObject_frame
 */
static PyTypeObject py_cyclicity_PyTypeObject_frame = {
    PyObject_HEAD_INIT(NULL)
    0,
    "Cyclicity", // for printing
    sizeof( t_py_cyclicity_PyObject ), // basic size
    0, // item size
    py_cyclicity_dealloc, /*tp_dealloc*/
    py_cyclicity_print,   /*tp_print*/
    py_cyclicity_getattr,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    py_cyclicity_repr,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
	0,			/* tp_call - called if the object itself is invoked as a method */
	py_cyclicity_str, /*tp_str */
};

/*a Statics for py_cyclicity
 */
/*v py_cyclicity_error
 */
//static PyObject *py_cyclicity_error;
static PyObject *result;

/*v py_cyclicity_methods
 */
static PyMethodDef py_cyclicity_methods[] =
{
    {"new", py_cyclicity_new, METH_VARARGS, "Create a new cyclicity object."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

/*a Python method support functions
 */
/*f py_cyclicity_method_enter
 */
static char *where_last;
static void py_cyclicity_method_enter( t_py_cyclicity_PyObject *py_cyc, char *where, PyObject *args )
{
 #ifdef PY_DEBUG
    fprintf( stderr, "Enter '%s'\n", where );
    fflush( stderr );
#endif
    where_last = where;
	result = NULL;
}

/*f py_cyclicity_method_return
 */
static PyObject *py_cyclicity_method_return( t_py_cyclicity_PyObject *py_cyc, const char *string )
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

/*f py_cyclicity_method_result_add_double
 */
// static void py_cyclicity_method_result_add_double( void *handle, double d )
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

/*f py_cyclicity_method_result_add_int
 */
static void py_cyclicity_method_result_add_int( void *handle, int i )
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

/*f py_cyclicity_method_result_add_string
 */
static void py_cyclicity_method_result_add_string( void *handle, const char *string )
{
	PyObject *obj;

	if (!result)
	{
		result = PyString_FromString( string );
		return;
	}
	if (!PyObject_TypeCheck( result, &PyList_Type) )
	{
		obj = PyList_New( 0 );
		PyList_Append( obj, result );
		result = obj;
	}
	obj = PyString_FromString( string );
	PyList_Append( result, obj );
}

/*f py_cyclicity_method_result_add_sized_string
 */
static void py_cyclicity_method_result_add_sized_string( void *handle, char *string, int size )
{
	PyObject *obj;

	if (!result)
	{
		result = PyString_FromStringAndSize( string, size );
		return;
	}
	if (!PyObject_TypeCheck( result, &PyList_Type) )
	{
		obj = PyList_New( 0 );
		PyList_Append( obj, result );
		result = obj;
	}
	obj = PyString_FromStringAndSize( string, size );
	PyList_Append( result, obj );
}

/*a Python-called cyclicity class methods
 */
/*f py_cyclicity_method_read_file
 */
static PyObject *py_cyclicity_method_read_file( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     char *filename;
     char *kwdlist[] = {"filename", NULL };

     py_cyclicity_method_enter( py_cyc, "read_file", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &filename))
     {
          py_cyclicity_method_result_add_int( NULL, py_cyc->cyclicity->parse_input_file( filename ));
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_list_source_files
 */
static PyObject *py_cyclicity_method_list_source_files( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int n, i;
     char *kwdlist[] = { NULL };

     py_cyclicity_method_enter( py_cyc, "list_source_files", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist))
     {
          n = py_cyc->cyclicity->get_number_of_files();
          result = PyList_New(0);
          for (i=0; i<n; i++)
          {
               py_cyclicity_method_result_add_string( NULL, py_cyc->cyclicity->get_filename( i ));
          }
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_get_file_length
 */
static PyObject *py_cyclicity_method_get_file_length( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int file_number;
     int size, nlines;
     char *data;
     char *kwdlist[] = {"file_number", NULL };

     py_cyclicity_method_enter( py_cyc, "get_file_length", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "i", kwdlist, &file_number))
     {
          if (!py_cyc->cyclicity->get_file_data( file_number, &size, &nlines, &data ))
          {
               return py_cyclicity_method_return( py_cyc, "Failed to get the data" );
          }
          py_cyclicity_method_result_add_int( NULL, size );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_read_file_data
 */
static PyObject *py_cyclicity_method_read_file_data( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int file_number;
     int start, end;
     int size, nlines;
     char *data;
     char *kwdlist[] = {"file_number", "start", "end", NULL };

     py_cyclicity_method_enter( py_cyc, "read_file_data", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "iii", kwdlist, &file_number, &start, &end))
     {
          if (!py_cyc->cyclicity->get_file_data( file_number, &size, &nlines, &data ))
          {
               return py_cyclicity_method_return( py_cyc, "Failed to get the data" );
          }
          if ((start<0) || (end>size))
          {
               return py_cyclicity_method_return( py_cyc, "Request for data outside the file bounds" );
          }
          py_cyclicity_method_result_add_sized_string( NULL, data+start, (end>start)?end-start:0 );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_read_file_line
 */
static PyObject *py_cyclicity_method_read_file_line( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int file_number, line_number;
     int length;
     char *data;
     char *kwdlist[] = {"file_number", "line_number", NULL };

     py_cyclicity_method_enter( py_cyc, "read_file_line", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "ii", kwdlist, &file_number, &line_number))
     {
          if (!py_cyc->cyclicity->get_line_data( file_number, line_number, &data, &length ))
          {
               return py_cyclicity_method_return( py_cyc, "Failed to get the data" );
          }
          py_cyclicity_method_result_add_sized_string( NULL, data, length );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_read_file_line_start_posn
 */
static PyObject *py_cyclicity_method_read_file_line_start_posn( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int file_number, line_number;
     int length;
     int size, nlines;
     char *data, *file_start;
     char *kwdlist[] = {"file_number", "line_number", NULL };

     py_cyclicity_method_enter( py_cyc, "read_file_line_start_posn", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "ii", kwdlist, &file_number, &line_number))
     {
          if ( (!py_cyc->cyclicity->get_line_data( file_number, line_number, &data, &length )) ||
               (!py_cyc->cyclicity->get_file_data( file_number, &size, &nlines, &file_start )) )
          {
               return py_cyclicity_method_return( py_cyc, "Failed to get the data" );
          }
          py_cyclicity_method_result_add_int( NULL, data-file_start );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_get_toplevel_handle
 */
static PyObject *py_cyclicity_method_get_toplevel_handle( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     char handle[16];
     char *kwdlist[] = { NULL };

     py_cyclicity_method_enter( py_cyc, "get_toplevel_handle", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "", kwdlist ))
     {
          py_cyc->cyclicity->fill_top_handle( handle );
          py_cyclicity_method_result_add_string( NULL, handle );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_get_object_handle
 */
static PyObject *py_cyclicity_method_get_object_handle( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int file_number, file_position;
     char handle[16];
     char *kwdlist[] = { "file_number", "file_position", NULL };

     py_cyclicity_method_enter( py_cyc, "get_object_handle", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "ii", kwdlist, &file_number, &file_position))
     {
          py_cyc->cyclicity->fill_object_handle( handle, file_number, file_position );
          py_cyclicity_method_result_add_string( NULL, handle );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_get_object_data
 */
static PyObject *py_cyclicity_method_get_object_data( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     c_cyc_object *co;
     char *handle;
     char link_handle[32];
     char parent_handle[32];
     const char *textual_type;
     int type;
     char *kwdlist[] = { "handle", NULL };
     t_lex_file_posn *start, *end;
     PyObject *obj;
     int file_number, file_position;

     py_cyclicity_method_enter( py_cyc, "get_object_data", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &handle ))
     {
          co = object_from_handle( handle );
          if (!co)
          {
               return py_cyclicity_method_return( py_cyc, "Bad object handle" );
          }
          co->get_data(  &textual_type, &type, link_handle, parent_handle, &start, &end );
          result = PyList_New(0);
          py_cyclicity_method_result_add_string( NULL, textual_type );
          py_cyclicity_method_result_add_int( NULL, type );
          py_cyclicity_method_result_add_string( NULL, link_handle );
          py_cyclicity_method_result_add_string( NULL, parent_handle );
          obj = PyList_New(0);
          if (!py_cyc->cyclicity->translate_lex_file_posn( start, &file_number, &file_position, 0 ))
          {
               PyList_Append( obj, PyInt_FromLong( (long)-1 ));
               PyList_Append( obj, PyInt_FromLong( (long)-1 ));
          }
          else
          {
               PyList_Append( obj, PyInt_FromLong( (long)file_number ));
               PyList_Append( obj, PyInt_FromLong( (long)file_position ));
          }
          PyList_Append( result, obj );
          obj = PyList_New(0);
          if (!py_cyc->cyclicity->translate_lex_file_posn( end, &file_number, &file_position, 1 ))
          {
               PyList_Append( obj, PyInt_FromLong( (long)-1 ));
               PyList_Append( obj, PyInt_FromLong( (long)-1 ));
          }
          else
          {
               PyList_Append( obj, PyInt_FromLong( (long)file_number ));
               PyList_Append( obj, PyInt_FromLong( (long)file_position ));
          }
          PyList_Append( result, obj );
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*f py_cyclicity_method_get_object_links
 */
static PyObject *py_cyclicity_method_get_object_links( t_py_cyclicity_PyObject *py_cyc, PyObject *args, PyObject *kwds )
{
     int n, i, stage, type;
     c_cyc_object *co;
     const char *text;
     char *handle;
     const char *link_text;
     char link_handle[16];
     char *kwdlist[] = { "handle", NULL };
     PyObject *obj;

     py_cyclicity_method_enter( py_cyc, "get_object_links", args );
     if (PyArg_ParseTupleAndKeywords( args, kwds, "s", kwdlist, &handle ))
     {
          co = object_from_handle(handle);
          if (!co)
          {
               return py_cyclicity_method_return( py_cyc, "Bad object handle" );
          }
          n = co->get_number_of_links();
          result = PyList_New(0);
          for (i=0; i<n; i++)
          {
               co->get_link( i, &text, link_handle, &link_text, &stage, &type );

               obj = PyList_New( 0 );
               PyList_Append( obj, PyString_FromString( text ) );
               PyList_Append( obj, PyString_FromString( link_handle[0]?link_handle:(link_text?link_text:"") ) );
               PyList_Append( obj, PyInt_FromLong( stage ) );
               PyList_Append( obj, PyInt_FromLong( type ) );
               PyList_Append( result, obj );
          }
          return py_cyclicity_method_return( py_cyc, NULL );
     }
     return NULL;
}

/*a Python-callled cyclicity class functions
 */
/*f py_cyclicity_getattr
 */
static PyObject *py_cyclicity_getattr( PyObject *self, char *name)
{
    return Py_FindMethod( cyclicity_methods, self, name);
}

/*f py_cyclicity_print
 */
static int py_cyclicity_print( PyObject *self, FILE *f, int unknown )
{
    t_py_cyclicity_PyObject *py_cyc;
	py_cyc = (t_py_cyclicity_PyObject *)self;
	py_cyc->cyclicity->print_debug_info();
	return 0;
}

/*f py_cyclicity_repr
 */
static PyObject *py_cyclicity_repr( PyObject *self )
{
	return Py_BuildValue( "s", "constraint system" );
}

/*f py_cyclicity_str
 */
static PyObject *py_cyclicity_str( PyObject *self )
{
	return Py_BuildValue( "s", "string representing in human form the constraint system" );
}

/*f py_cyclicity_new
 */
static PyObject *py_cyclicity_new( PyObject* self, PyObject* args )
{
	 t_py_cyclicity_PyObject *py_cyc;

    if (!PyArg_ParseTuple(args,":new"))
	{
        return NULL;
	}

    py_cyc = PyObject_New( t_py_cyclicity_PyObject, &py_cyclicity_PyTypeObject_frame );
	py_cyc->cyclicity = new c_cyclicity();

    return (PyObject*)py_cyc;
}

/*f py_cyclicity_dealloc
 */
static void py_cyclicity_dealloc( PyObject* self )
{
    PyObject_Del(self);
}

/*a C code for py_cyclicity
 */
extern "C" void initpy_cyclicity( void )
{
	PyObject *m;

    m = Py_InitModule("py_cyclicity", py_cyclicity_methods);

}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


