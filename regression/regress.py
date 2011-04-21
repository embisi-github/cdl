#!/usr/bin/env python

#a Copyright
#  
#  This file 'regress' copyright Gavin J Stark 2003, 2004
#  
#  This program is free software; you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software
#  Foundation, version 2.0.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
#  for more details.

import sys, os, unittest

#a Variables
ospath = "linux"
if os.environ["OSTYPE"] == "darwin":
    ospath = "osx"

bin_directory = os.path.join("build", ospath)
test_dirs = [ "simple", "vector", "instantiation", "memory", "event", "bugs", "clock_gate", "." ]
debug_level = 0

#a Find the tests
# We assume that there is a file 'test.py' in each subdirectory, containing
# all the tests. The code below grabs it and imports it.
sys.path = [] + sys.path
test_modules = {}
suite = unittest.TestSuite()
for i in test_dirs:
    sys.path[0] = os.path.abspath(i)
    try:
        test_modules[i] = __import__("test")
        del sys.modules["test"]
    except ImportError:
        pass
    else:
        suite.addTests(unittest.TestLoader().loadTestsFromModule(test_modules[i]))

#a Run tests
unittest.TextTestRunner(verbosity=2).run(suite)


