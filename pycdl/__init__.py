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

class WireError(PyCDLError):
    """
    Thrown on a wiring mismatch.
    """
    def __init__(self, msg="Wiring error!"):
        self._msg = msg

    def __str__(self):
        return "WireError: " + self._msg

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

    def __eq__(self, other):
        if isinstance(other, bv):
            if other._size != self._size:
                return False
            else:
                return self._val == other._val
        else:
            return self._val == other

    def __ne__(self, other):
        return not self.__eq__(other)

    def __cmp__(self, other):
        return self._val - other        

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
        self._reset_value = None

    def _check_connectivity(self, other):
        if self._size and other._size and self._size != other._size:
            raise WireError("Size mismatch: %s has %d, %s has %d" % (repr(self), self._size, repr(other), other._size))

    def _is_driven_by(self, other):
        if self._driven_by:
            raise WireError("Double-driven signal at %s" % repr(self))
        self._check_connectivity(other)
        self._driven_by = other
        other._drives.append(self)

    def _connect_cdl_signal(self, signal):
        if self._cdl_signal and self._cdl_signal != signal:
            raise WireError("Connecting two CDL signals on %s: %s and %s" % (repr(self), repr(self._cdl_signal), repr(signal)))
        if not self._cdl_signal:
            #print "CONNECT %s to CDL %s" % (repr(self), repr(signal))
            self._cdl_signal = signal
        for i in self._drives:
            i._connect_cdl_signal(signal)

    def _connectivity_upwards(self, index):
        if self._driven_by:
            print "%sDriven by %s" % (" "*index, repr(self._driven_by))
            return self._driven_by._connectivity_upwards(index+2)
        else:
            print "%sROOT AT %s" % (" "*index, repr(self))
            return self

    def _connectivity_downwards(self, index):
        print "%sAt %s, CDL signal %s" % (" "*index, repr(self), repr(self._cdl_signal))
        for i in self._drives:
            print "%sDrives %s" % (" "*index, repr(i))
            i._connectivity_downwards(index+2)
    
    def print_connectivity(self):
        print "Finding connectivity tree for %s:" % repr(self)
        root = self._connectivity_upwards(0)
        print
        root._connectivity_downwards(0)

    def size(self):
        return self._size

    def value(self):
        if self._cdl_signal:
            return bv(self._cdl_signal.value(), self._size)
        else:
            raise WireError("No underlying value for signal %s" % repr(self))

    def drive(self, value, recursive=True):
        if self._cdl_signal:
            self._cdl_signal.drive(value)
        else:
            raise WireError("No underlying signal to drive for %s" % repr(self))

    def reset(self, value):
        self._reset_value = value

class clock(wire):
    def __init__(self, value=0, reset_period=1, period=1, name=None, owner=None):
        self._init_value = value
        self._reset_period = period
        self._period = period
        wire.__init__(self, 1, name, owner)

class wirebundle(_nameable):
    """
    The object that represents a bundle of wires.
    """
    def __init__(self, **kw):
        self._dict = kw
        self._drives = []
        self._driven_by = None
        self._reset_value = None

    def _check_connectivity(self, other):
        # Check that all the signals in the bundle match the one we're
        # connecting up.
        unusedkeys = set(other._dict.keys())
        for i in self._dict:
            if i not in other._dict:
                raise WireError("Wire bundle mismatch in %s, missing other's value %s" % (repr(self), i))
            del unusedkeys[i]
            self._dict[i].check_connectivity(other._dict[i])
        if len(unusedkeys) > 0:
            raise WireError("Wire bundle mismatch in %s, leftover signals" % repr(self))

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
                raise WireError("Unknown wire type %s" % repr(other.__class__))
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
            raise WireError("Double-driven signal at %s" % repr(self))
        self._update_connectivity(other)
        for i in self._dict:
            self._dict[i]._is_driven_by(other._dict[i])
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
                raise WireError("Wire bundle mismatch in %s, missing other's value %s" % (repr(self), i))
            del unusedkeys[i]
            self._dict[i].drive(inbundle._dict[i])
        if len(unusedkeys) > 0:
            raise WireError("Wire bundle mismatch in %s, leftover signals" % repr(self))

    def reset(self, inbundle):
        # Set the reset value for the bundle.
        self._reset_value = inbundle

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

class _instantiable(_clockable):
    """
    A module that can be instantiated -- either a CDL module or a Python
    test harness.
    """
    def _ports_from_ios(self, iolist, cdl_object):
        # The I/O list should have three members -- a list of clocks, a list of
        # inputs and a list of outputs.
        # Each of these members contains a list of ports. We'll turn these into
        # wires.
        retlist = []
        for id_type in iolist:
            retdict = {}
            for io in id_type:
                retdict[io[0]] = wire(io[1], io[0], self)
                #print "Creating port wire: %s size %d, result %s" % (io[0], io[1], repr(retdict[io[0]]))
                if cdl_object and hasattr(cdl_object, io[0]):
                    retdict[io[0]]._connect_cdl_signal(getattr(cdl_object,io[0]))
            retlist.append(retdict)
        return retlist
                

class th(_instantiable):
    """
    The object that represents a test harness.
    """
    def __init__(self, clocks, inputs, outputs):
        self._clocks = clocks
        self._inputs = inputs
        self._outputs = outputs

    def bfm_wait(self, cycles):
        self._thfile.cdlsim_sim.bfm_wait(cycles)

    def global_cycle(self):
        return self._thfile.cdlsim_sim.global_cycle()

    def passtest(self, code, message):
        return self._thfile.py.pypass(code, message)

    def failtest(self, code, message):
        return self._thfile.py.pyfail(code, message)

    def passed(self):
        return self._thfile.py.pypassed()

class timed_assign(th):
    """
    A timed assignment, often used for a reset sequence.
    """
    def __init__(self, signal, init_value, wait, later_value):
        self._timed_signal = signal
        self._timed_init = init_value
        self._timed_wait = wait
        self._timed_later = later_value
        th.__init__(self, clocks={}, inputs={}, outputs={"sig": self._timed_signal})
    def run(self):
        self._timed_signal.reset(self._timed_init)
        self.bfm_wait(self._timed_wait)
        self._timed_signal.drive(self._timed_later)

class module(_instantiable):
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
        py_engine.exec_file.__init__(self)
    def exec_init(self):
        pass
    def exec_reset(self):
        pass
    def exec_run(self):
        self._th.run()

class _hwexfile(py_engine.exec_file):
    def __init__(self, hw, engine):
        self._hw = hw
        self._engine = engine
        py_engine.exec_file.__init__(self)

    def _connect_wire(self, name, wireinst, connectedwires, inputs, ports):
        wire_basename = name.split('.')[-1]
        if wire_basename not in ports:
            raise WireError("Connecting to undefined port %s" % wire_basename)
        port = ports[wire_basename]
        if wireinst._size != port._size:
            raise WireError("Port size mismatch for port %s, expected %d got %d" % (wire_basename, wireinst._size, port._size))
                # size mismatch!
        if wireinst not in connectedwires:
            if isinstance(wireinst, clock):
                self.cdlsim_instantiation.clock(wireinst._name, wireinst._init_value, wireinst._reset_period, wireinst._period)
                #print "CLOCK %s" % wireinst._name
            else:
                self.cdlsim_instantiation.wire("%s[%d]" % (wireinst._name, wireinst._size))
                #print "WIRE %s" % wireinst._name
            connectedwires.add(wireinst)
        if port not in connectedwires:
            connectedwires.add(port)
        if inputs:
            # This is an input to the module so the wire drives the port.
            self.cdlsim_instantiation.drive(name, wireinst._name)
            #print "DRIVE %s %s" % (name, wireinst._name)
            #print "Port %s driven by wire %s" % (repr(port), repr(wireinst))
            port._is_driven_by(wireinst)
        else:
            # This is an output from the module, so the port drives the wire.
            self.cdlsim_instantiation.drive(wireinst._name, name)
            #print "DRIVE %s %s" % (wireinst._name, name)
            #print "Wire %s driven by port %s" % (repr(wireinst), repr(port))
            wireinst._is_driven_by(port)

    def _connect_wires(self, name, wiredict, connectedwires, inputs, ports):
        for i in wiredict:
            if isinstance(wiredict[i], wire):
                self._connect_wire(name+i, wiredict[i], connectedwires, inputs, ports)
            elif isinstance(wiredict[i], wirebundle):
                self._connect_wires(name+i+"__", self.wiredict[i]._dict, connectedwires, inputs, ports)
            else:
                raise WireError("Connecting unknown wire type %s" % repr(wiredict[i].__class__))

    def exec_init(self):
        pass
    def exec_reset(self):
        pass
    def exec_run(self):
        anonid = 1
        connectedwires = set()
        for i in self._hw._children:
            if not hasattr(i, "_name") or i._name is None:
                i._name = "__anon%3d" % anonid
                i._owner = self._hw
                anonid += 1
            if isinstance(i, module):
                self.cdlsim_instantiation.module(i._type, i._name)
                i._ports = i._ports_from_ios(self._engine._engine.get_module_ios(i._name), None)
            elif isinstance(i, th):
                i._thfile = _thfile(i)
                self.cdlsim_instantiation.option_string("clock", " ".join(i._clocks.keys()))
                self.cdlsim_instantiation.option_string("inputs", " ".join(["%s[%d]" % (x, i._inputs[x]._size) for x in i._inputs]))
                self.cdlsim_instantiation.option_string("outputs", " ".join(["%s[%d]" % (x, i._outputs[x]._size) for x in i._outputs]))
                self.cdlsim_instantiation.option_object("object", i._thfile)
                self.cdlsim_instantiation.module("se_test_harness", i._name)
                i._ports = i._ports_from_ios(self._engine._engine.get_module_ios(i._name), i._thfile)
            elif isinstance(i, clock):
                pass # we'll hook up later
            else:
                raise NotImplementedError
        for i in self._hw._children:
            if hasattr(i, "_clocks") and hasattr(i, "_ports"):
                self._connect_wires(i._name+".", i._clocks, connectedwires, inputs=True, ports=i._ports[0])
            if hasattr(i, "_inputs") and hasattr(i, "_ports"):
                self._connect_wires(i._name+".", i._inputs, connectedwires, inputs=True, ports=i._ports[1])
            if hasattr(i, "_outputs") and hasattr(i, "_ports"):
                self._connect_wires(i._name+".", i._outputs, connectedwires, inputs=False, ports=i._ports[2])
        # Now make sure all the CDL signals are hooked up.
        #print "*** Starting CDL signal hookup"
        for i in connectedwires:
            #print "Looking at %s" % repr(i)
            origi = i
            # Find a CDL signal for this tree.
            while not i._cdl_signal and i._driven_by:
                i = i._driven_by
            if i._cdl_signal:                    
                # Find the root of the driving tree.
                sig = i._cdl_signal
                while i._driven_by:
                    i = i._driven_by
                i._connect_cdl_signal(sig)
            #print "Done looking at %s" % repr(origi)

        # And set up the reset values.
        for i in connectedwires:
            if i._reset_value:
                i.drive(i._reset_value)


class hw(_clockable):
    """
    The object that represents a piece of hardware.
    """
    def __init__(self, *children):
        _clockable.__init__(self, children)

    def passed(self):
        for i in self._children:
            if isinstance(i, th) and not isinstance(i, timed_assign):
                if not i.passed():
                    return False
        return True

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
        self._engine.describe_hw(_hwexfile(hw, self))

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

