/*a Copyright
  
  This file 'eh_scripting_python.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <Python.h>
#include "eh_scripting.h"

/*a Statics
 */
/*v no_methods
 */
static PyMethodDef no_methods[] =
{
    {NULL, NULL, 0, NULL}
};

/*a Python code
 */
/*f scripting_init_module
 */
extern void scripting_init_module( const char *script_module_name )
{
     Py_InitModule( (char *)script_module_name, no_methods );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



