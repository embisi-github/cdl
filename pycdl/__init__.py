#!/usr/bin/env python

#a Copyright
#  
#  This file '__init__' copyright Gavin J Stark 2011
#  
#  This program is free software; you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software
#  Foundation, version 2.0.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
#  for more details.


"""
This file implements PyCDL, which is the human-friendly interface from Python
code to CDL's py_engine interface.
"""

# First, try to import the py_engine that we built. Either something around us
# will have put it in the PYTHONPATH or it will have set the CDL_BUILD_DIR
# environment variable.
import sys, os
if "CDL_BUILD_DIR" in os.environ:
    oldpath = sys.path
    sys.path = [os.environ("CDL_BUILD_DIR")]
    try:
        import py_engine
    except ImportError:
        raise
    finally:
        sys.path = oldpath
else:
    import py_engine

class waves(object):
    """
    The object that controls waves.
    """
    def __init__(self):
        raise NotImplementedError

class wire(object):
    """
    The object that represents a wire.
    """
    def __init__(self):
        raise NotImplementedError

class th(object):
    """
    The object that represents a test harness.
    """
    def __init__(self):
        raise NotImplementedError

class hw(object):
    """
    The object that represents a piece of hardware.
    """
    def __init__(self):
        raise NotImplementedError

class engine(object):
    """
    The actual engine object. Corresponds to py_engine.py_engine().
    """
    def __init__(self, hw):
        """
        Initialize the engine, given some hardware.
        """
        self._engine = py_engine.py_engine()
        self._engine.describe_hw(hw)

    def reset(self):
        """
        Reset the engine.
        """
        self._engine.reset()

    def step(self, cycles=1):
        """
        Step for n cycles.
        """
        self._engine.step(cycles)

    def passtest(self, cycles, msg):
        """
        Mark the test as complete.
        """
        raise NotImplementedError

    def failtest(self, cycles, msg):
        """
        Mark the test as failed.
        """
        raise NotImplementedError

    def passed(self):
        """
        Did the test pass?
        """
        raise NotImplementedError
