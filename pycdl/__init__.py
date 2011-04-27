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
import itertools, collections

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

class _nameable(object):
    """
    An object that should get named when assigned to a namegiver object.
    """
    pass

class _namegiver(object):
    """
    An object that gives nameable objects a name.
    """
    def __setattr__(self, name, value):
        if isinstance(value, _nameable) and (not hasattr(value, "_name") or value._name is None):
            value._name = name
            value._owner = self
        object.__setattr__(self, name, value)

class PyCDLError(Exception):
    """
    Base class for all errors.
    """
    pass

class WireError(Exception):
    """
    Thrown on a wiring mismatch.
    """
    pass

class bv(object):
    """
    A bit-vector. Has a size and a value.
    """
    def __init__(self, val, size=None):
        self._val = val
        self._size = size
        if size is not None:
            self._val &= (1<<size)-1

    def value(self):
        return self._val

    __int__ = value # this allows us to use a bv where we otherwise use a number

    def size(self):
        return self._size
            
class bvbundle(object):
    """
    A bundle of bit vectors (or of more bvbundles)
    """
    def __init__(self, **kw):
        self._dict = kw

    def __getattr__(self, name):
        if name in self._dict:
            return self._dict[name]
        raise AttributeError

    def __setattr__(self, name, value):
        raise AttributeError # bvbundles are read-only

class wire(_nameable):
    """
    The object that represents a wire.
    """
    def __init__(self, size=1, name=None, owner=None):
        self._drives = []
        self._driven_by = None
        self._size = size
        self._cdl_signal = None
        self._name = name
        self._owner = owner

    def _check_connectivity(self, other):
        if self._size and other._size and self._size != other._size:
            raise WireError # size mismatch

    def _is_driven_by(self, other):
        if self._driven_by:
            raise WireError # double-driving a signal!
        _check_connectivity(self, other)
        self._driven_by = other
        other._drives.append(self)

    def _connect_cdl_signal(self, signal):
        # FIXME, make sure the size matches
        self._cdl_signal = signal

    def size(self):
        return self._size

    def value(self):
        # FIXME, make this a bv
        if self._cdl_signal:
            return self._cdl_signal.value()
        elif self._driven_by:
            return self._driven_by.value()
        else:
            raise WireError # no value to take!

    def drive(self, value):
        driven_something = False
        if self._cdl_signal:
            self._cdl_signal.drive(value)
            driven_something = True
        for i in self._drives:
            try:
                i.drive(value)
                driven_something = True
            except WireError:
                pass
        if not driven_something:
            raise WireError # nothing to drive!

class clock(wire):
    def __init__(self, value=0, reset_period=1, period=1, name=None, owner=None):
        self._init_value = value
        self._reset_period = period
        self._period = period
        # FINISHME!!!
        wire.__init__(self, 1, name, owner)

class wirebundle(_nameable):
    """
    The object that represents a bundle of wires.
    """
    def __init__(self, **kw):
        self._dict = kw
        self._drives = []
        self._driven_by = None

    def _check_connectivity(self, other):
        # Check that all the signals in the bundle match the one we're
        # connecting up.
        unusedkeys = set(other._dict.keys())
        for i in self._dict:
            if i not in other._dict:
                raise WireError # oops, that one's not there!
            del unusedkeys[i]
            self._dict[i].check_connectivity(other._dict[i])
        if len(unusedkeys) > 0:
            raise WireError # oops, there were signals left over!

    def _populate_dict(self, other):
        # Yay! Now we know what signals we have! If we hooked up signals
        # anonymously, we now need to put together our bundle and check it.
        for i in other._dict:
            if isinstance(other._dict[i], wirebundle):
                self._dict[i] = wirebundle()
                self._dict[i]._populate_dict(other._dict[i])
            elif isinstance(other._dict[i], wire):
                self._dict[i] = wire(other._dict[i].size())
            else:
                raise WireError # what on earth is in this thing?
        # If we've already hooked this thing up, we need to propagate our
        # knowledge of what's in there.
        if self._driven_by:
            self._update_connectivity(self._driven_by)
        for i in self._drives:
            i._update_connectivity(self)        

    def _update_connectivity(self, other):
        if len(self._dict) == 0 and len(other._dict) != 0:
            self._populate_dict(other)
        if len(other._dict) == 0 and len(self._dict) != 0:
            other._populate_dict(self)
        if len(self._dict) > 0:
            self._check_connectivity(other)

    def _is_driven_by(self, other):
        if self._driven_by:
            raise WireError # double-driving a signal!
        self._update_connectivity(other)
        for i in self._dict:
            self._dict[i].is_driven_by(other._dict[i])
        self._driven_by = other
        other._drives.append(self)

    def value(self):
        # We want the value. Fair enough. Go dig and get it.
        outdict = {}
        for i in self._dict:
            outdict[i] = self._dict[i].value()
        return bvbundle(outdict)

    def drive(self, inbundle):
        # Drive a whole bundle.
        unusedkeys = set(inbundle._dict.keys())
        for i in self._dict:
            if i not in inbundle._dict:
                raise WireError # oops, that one's not there!
            del unusedkeys[i]
            self._dict[i].drive(inbundle._dict[i])
        if len(unusedkeys) > 0:
            raise WireError # oops, there were signals left over!
        

class _clockable(_namegiver, _nameable):
    """
    The base class for all pieces of hardware.
    """
    def __init__(self, children):
        # Flatten the list of children.
        self._children = list(itertools.chain.from_iterable([children]))

    def _clock(self):
        for i in self._children:
            i._clock()

class th(_clockable):
    """
    The object that represents a test harness.
    """
    def __init__(self, clocks, inputs, outputs):
        self._clocks = clocks
        self._inputs = inputs
        self._outputs = outputs

class module(_clockable):
    """
    The object that represents a CDL module.
    """
    def __init__(self, moduletype, clocks, inputs, outputs):
        self._type = moduletype
        self._clocks = clocks
        self._inputs = inputs
        self._outputs = outputs

class _thfile(py_engine.exec_file):
    def __init__(self, th):
        self._th = th
        print "self=%s, self._th=%s, doing _thfile.__init__" % (repr(self), repr(self._th))
        print "self.__dict__=%s" % repr(self.__dict__)
        py_engine.exec_file.__init__(self)
        print "After init, self.__dict__=%s" % repr(self.__dict__)
    def exec_init(self):
        print "At exec_init, self.__dict__=%s" % repr(self.__dict__)
        pass
    def exec_reset(self):
        print "At exec_reset, self.__dict__=%s" % repr(self.__dict__)
        pass
    def exec_run(self):
        print "self=%s, doing _thfile.exec_run" % repr(self)
        print "self.__dict__=%s" % repr(self.__dict__)
        self._th.run()

class _hwexfile(py_engine.exec_file):
    def __init__(self, hw):
        self._hw = hw
        py_engine.exec_file.__init__(self)

    def _connect_wire(self, name, wireinst, connectedwires, inputs):
        if wireinst._owner is self._hw:
            # This is a top-level wire.
            if wireinst not in connectedwires:
                self.cdlsim_instantiation.wire(name)
                connectedwires.append(wireinst)
                wireinst._cdl_signal = getattr(self, name)
            return name
        if inputs:
            # This is an input to the module so we look for a named driver.
            if wireinst._driven_by:
                drivername = _connect_wire(self, wireinst._driven_by._name, wireinst._driven_by, connectedwires, True)
                if wireinst not in connectedwires:
                    self.cdlsim_instantiation.drive(name, drivername)
                    connectedwires.append(wireinst)
                return drivername
            else:
                raise WireError # unconnected input
        else:
            # This is an output to the module so hopefully something named is
            # driven by it.
            if wireinst not in connectedwires:
                for i in wireinst._drives:
                    drivenname = _connect_wire(self, i._name, i, connectedwires, False)
                    self.cdlsim_instantiation.drive(drivenname, name)
                connectedwires.append(wireinst)
            return name

    def _connect_wires(self, name, wiredict, connectedwires, inputs):
        for i in wiredict:
            if isinstance(wiredict[i], wire):
                self._connect_wire(name+i, wiredict[i], connectedwires, inputs)
            elif isinstance(wiredict[i], wirebundle):
                if wiredict[i] in connectedwires:
                    continue
                self._connect_wires(name+i+"__", self.wiredict[i]._dict, connectedwires, inputs)
            else:
                raise WireError # what the blank is this?

    def exec_init(self):
        pass
    def exec_reset(self):
        pass
    def exec_run(self):
        anonid = 1
        for i in self._hw._children:
            if not hasattr(i, "_name") or i._name is None:
                i._name = "__anon%3d" % anonid
                i._owner = self._hw
                anonid += 1
            if isinstance(i, module):
                self.cdlsim_instantiation.module(i._type, i._name)
            elif isinstance(i, th):
                self.cdlsim_instantiation.option_string("clock", " ".join(i._clocks.keys()))
                self.cdlsim_instantiation.option_string("inputs", " ".join(i._inputs.keys()))
                self.cdlsim_instantiation.option_string("outputs", " ".join(i._outputs.keys()))
                self.cdlsim_instantiation.option_object("object", _thfile(i))
                self.cdlsim_instantiation.module("se_test_harness", i._name)
            else:
                raise NotImplementedError
        connectedwires = set()
        for i in self._hw._children:
            self._connect_wires(self, i._name+".", i._clocks, connectedwires, inputs=True)
            self._connect_wires(self, i._name+".", i._inputs, connectedwires, inputs=True)
            self._connect_wires(self, i._name+".", i._outputs, connectedwires, inputs=False)
                    
                    

        # FINISHME, hook up the wires (somehow!)

class hw(_clockable):
    """
    The object that represents a piece of hardware.
    """
    def __init__(self, *children):
        _clockable.__init__(self, children)

    # FINISHME

class waves(object):
    """
    The object that controls waves.
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
        self._engine.describe_hw(_hwexfile(hw))

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
