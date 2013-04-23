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
from c_python_telnetd import c_python_telnet_server

version_info = (1,4,13,"",0)

def version():
    global version_info
    return version_info

def hexversion():
    global version_info
    v = ( (version_info[0]<<24) |
          (version_info[1]<<16) |
          (version_info[2]<<8) |
          (version_info[4]<<0)  )
    return v

if "CDL_BUILD_DIR" in os.environ:
    oldpath = sys.path
    sys.path = [os.environ["CDL_BUILD_DIR"]]
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
    def _give_name(self, name, value):
        if isinstance(value, _nameable) and (not hasattr(value, "_name") or value._name is None):
            value._name = name
        elif isinstance(value, list):
            for i in range(len(value)):
                self._give_name("%s_%d" % (name, i), value[i])
        elif isinstance(value, dict):
            for i in value.keys():
                self._give_name("%s_%s" % (name, str(i)), value[i])

    def __setattr__(self, name, value):
        self._give_name(name, value)
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

    def _getset(self, key, item, isset):
        # Figure out the start and stop.
        if isinstance(key, slice):
            start = key.start
            stop = key.stop
            step = key.step
        else:
            start = key
            stop = key
            step = 1
        if step is not None and step != 1:
            raise IndexError
        if start is None:
            if self._size is None:
                raise IndexError
            start = self._size - 1
        if stop is None:
            stop = 0
        if start < 0:
            if self._size is None:
                raise IndexError
            start = self._size - start
        if stop < 0:
            if self._size is None:
                raise IndexError
            stop = self._size - stop
        if stop > start:
            (start, stop) = (stop, start)
        if start >= 64 or (self._size is not None and start >= self._size):
            raise IndexError

        # OK, now we have a known-sane start and stop.
        unshiftedmask = (1 << (start - stop + 1)) - 1
        shiftedmask = unshiftedmask << stop

        if (isset):
            self._val &= ~shiftedmask
            self._val |= (item & unshiftedmask) << stop
        else:
            return (self._val & shiftedmask) >> stop 

    def __getitem__(self, key):
        return self._getset(key, None, False)
    
    def __setitem__(self, key, item):
        return self._getset(key, item, True)

class bvbundle(object):
    """
    A bundle of bit vectors (or of more bvbundles)
    """
    def __init__(self, indict=None, **kw):
        if indict:
            self.__dict__["_dict"] = indict
            self._dict.update(kw)
        else:
            self.__dict__["_dict"] = kw

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
    def __init__(self, size=1, name=None):
        self._drives = []
        self._driven_by = None
        self._size = size
        self._cdl_signal = None
        self._name = name
        self._reset_value = None

    def _name_list(self, inname):
        return ["%s[%d]" % (inname, self._size)]

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

    def drive(self, value):
        if isinstance(value, bv):
            value = int(value)
        if self._cdl_signal:
            self._cdl_signal.drive(value)
        else:
            raise WireError("No underlying signal to drive for %s" % repr(self))

    def _reset(self, value):
        if self._cdl_signal:
            self._cdl_signal.reset(value)
        else:
            raise WireError("No underlying signal to drive for %s" % repr(self))

    def reset(self, value):
        if self._cdl_signal:
            self._cdl_signal.reset(value)
        self._reset_value = value

    def wait_for_value(self, val, timeout=-1):
        if self._cdl_signal:
            self._cdl_signal.wait_for_value(val, timeout)
        else:
            raise WireError("No underlying value for signal %s" % repr(self))

    def wait_for_change(self, timeout=-1):
        if self._cdl_signal:
            self._cdl_signal.wait_for_change(timeout)
        else:
            raise WireError("No underlying value for signal %s" % repr(self))


class clock(wire):
    def __init__(self, init_delay=0, cycles_high=1, cycles_low=1, name=None):
        self._init_delay = init_delay
        self._cycles_high = cycles_high
        self._cycles_low = cycles_low
        wire.__init__(self, 1, name)

class wirebundle(_nameable):
    """
    The object that represents a bundle of wires.
    """
    def __init__(self, bundletype=None, name=None, **kw):
        if bundletype:
            # Build the bundle from a dict.
            self._dict = {}
            for i in bundletype:
                if isinstance(bundletype[i], int):
                    self._dict[i] = wire(bundletype[i], name=i)
                else:
                    self._dict[i] = wirebundle(bundletype=bundletype[i], name=i)
        else:
            self._dict = kw
        self._drives = []
        self._driven_by = None
        self._reset_value = None
        self._name = name

    def _name_list(self, inname):
        retval = []
        for i in self._dict:
            subval = self._dict[i]._name_list(i)
            for j in subval:
                retval.append("%s__%s" % (inname, j))
        return retval

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

    def _add_wire(self, wirename, size, name):
        # First see if we need to recurse.
        wirelist = wirename.split("__", 1)
        if len(wirelist) > 1:
            # We need to go down to another level of bundles.
            if wirelist[0] not in self._dict:
                self._dict[wirelist[0]] = wirebundle()
            return self._dict[wirelist[0]]._add_wire(wirelist[1], size, name)
        else:
            if wirelist[0] in self._dict:
                raise WireError("Double add of wire %s to bundle" % wirelist[0])
            self._dict[wirelist[0]] = wire(size, name)
            return self._dict[wirelist[0]]
            
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

    def _reset(self, inbundle):
        # Drive a whole bundle for reset.
        unusedkeys = set(inbundle._dict.keys())
        for i in self._dict:
            if i not in inbundle._dict:
                raise WireError("Wire bundle mismatch in %s, missing other's value %s" % (repr(self), i))
            del unusedkeys[i]
            self._dict[i]._reset(inbundle._dict[i])
        if len(unusedkeys) > 0:
            raise WireError("Wire bundle mismatch in %s, leftover signals" % repr(self))

    def reset(self, inbundle):
        # Set the reset value for the bundle.
        unusedkeys = set(inbundle._dict.keys())
        for i in self._dict:
            if i not in inbundle._dict:
                raise WireError("Wire bundle mismatch in %s, missing other's value %s" % (repr(self), i))
            del unusedkeys[i]
            self._dict[i].reset(inbundle._dict[i])
        if len(unusedkeys) > 0:
            raise WireError("Wire bundle mismatch in %s, leftover signals" % repr(self))
        self._reset_value = inbundle

    def __getattr__(self, attr):
        if attr not in self._dict:
            raise AttributeError
        else:
            return self._dict[attr]

class _clockable(_namegiver, _nameable):
    """
    The base class for all pieces of hardware.
    """
    def __init__(self, *children):
        # Flatten the list of children.
        self._children = list(itertools.chain.from_iterable(children))

    def _clock(self):
        for i in self._children:
            i._clock()

class _instantiable(_clockable):
    """
    A module that can be instantiated -- either a CDL module or a Python
    test harness.
    """
    def _ports_from_ios(self, iolist, cdl_object):
        if not iolist:
            raise WireError("No IOs passed to a module - perhaps the module type could not be found")
        # The I/O list should have three members -- a list of clocks, a list of
        # inputs and a list of outputs.
        # Each of these members contains a list of ports. We'll turn these into
        # wires.
        retlist = []
        for id_type in iolist:
            retdict = {}
            for io in id_type:
                wirelist = io[0].split("__", 1)
                if len(wirelist) > 1:
                    if wirelist[0] not in retdict:
                        retdict[wirelist[0]] = wirebundle()
                    thewire = retdict[wirelist[0]]._add_wire(wirelist[1], io[1], io[0])
                else:
                    retdict[io[0]] = wire(io[1], io[0])
                    thewire = retdict[io[0]]
                #print "Creating port wire: %s size %d, result %s" % (io[0], io[1], repr(thewire))
                if cdl_object and hasattr(cdl_object, io[0]):
                    #print "Connecting CDL signal %s" % repr(getattr(cdl_object, io[0]))
                    thewire._connect_cdl_signal(getattr(cdl_object,io[0]))
                else:
                    #print "No CDL signal!"
                    pass
                    
            retlist.append(retdict)
        return retlist
                

class th(_instantiable):
    """
    The object that represents a test harness.
    """
    def _flatten_names(self, inobj, inname=None):
        outdict = {}
        if isinstance(inobj, list):
            for i in range(len(inobj)):
                if inname is None:
                    outdict.update(self._flatten_names(inobj[i], "%d" % i))
                else:
                    outdict.update(self._flatten_names(inobj[i], "%s_%d" % (inname, i)))
        elif isinstance(inobj, dict):
            for i in inobj.keys():
                if inname is None:
                    outdict.update(self._flatten_names(inobj[i], "%s" % str(i)))
                else:
                    outdict.update(self._flatten_names(inobj[i], "%s_%s" % (inname, str(i))))
        else:
            outdict = { inname: inobj }
        return outdict

    def __init__(self, clocks, inputs, outputs):
        self._clocks = self._flatten_names(clocks)
        self._inputs = self._flatten_names(inputs)
        self._outputs = self._flatten_names(outputs)
        self._eventanonid = 1

    def sim_message(self):
        self._thfile.cdlsim_reg.sim_message( "sim_message_obj" )
        x = self._thfile.sim_message_obj
        #x = self._thfile.sim_message
        del self._thfile.sim_message_obj
        return x

    def bfm_wait(self, cycles):
        self._thfile.cdlsim_sim.bfm_wait(cycles)

    def spawn(self, boundfn, *args):
        self._thfile.py.pyspawn(boundfn, args)

    def global_cycle(self):
        return self._thfile.cdlsim_sim.global_cycle()

    def passtest(self, code, message):
        return self._thfile.py.pypass(code, message)

    def failtest(self, code, message):
        return self._thfile.py.pyfail(code, message)

    def passed(self):
        return self._thfile.py.pypassed()

    class _event(object):
        """
        The object that controls events, inside a test harness.
        """
        def __init__(self, th):
            th._thfile.sys_event.event("__anonevent%3d" % th._eventanonid)
            self._cdl_obj = getattr(th._thfile, "__anonevent%3d" % th._eventanonid)
            self._th = th
            th._eventanonid += 1
                
        def reset(self):
            self._cdl_obj.reset()

        def fire(self):
            self._cdl_obj.fire()

        def wait(self):
            self._cdl_obj.wait()

    def event(self):
        return th._event(self)

class _internal_th(th):
    """
    A sentinel to show an internal test harness that does not need to be considered
    in pass/fail.
    """
    pass

class _wave_hook(_internal_th):
    """
    A timed assignment, often used for a reset sequence.
    """
    def __init__(self):
        th.__init__(self, clocks={}, inputs={}, outputs={})
    def run(self):
        pass

class timed_assign(_instantiable):
    """
    A timed assignment, often used for a reset sequence.
    """
    def __init__(self, signal, init_value, wait=None, later_value=None):
        if wait is None:
            wait = 1<<31
        if later_value is None:
            later_value = init_value
        self._outputs = {"sig": signal}
        self._ports = self._ports_from_ios([[], [], [["sig", signal._size]]], None)
        self._firstval = init_value
        self._wait = wait
        self._afterval = later_value

class module(_instantiable):
    """
    The object that represents a CDL module.
    """
    def __init__(self, moduletype, clocks={}, inputs={}, outputs={}, options={}, forces={}):
        self._type = moduletype
        self._clocks = clocks
        self._inputs = inputs
        self._outputs = outputs
        self._options = options
        self._forces = forces
    def __repr__(self):
        return "<Module {0}>".format(self._type)

class _thfile(py_engine.exec_file):
    def __init__(self, th):
        self._th = th
        py_engine.exec_file.__init__(self)
    def exec_init(self):
        pass
    def exec_reset(self):
        pass
    def exec_run(self):
        try:
            self._th.run()
        except:
            self._th.failtest(0, "Exception raised!")
            raise

class _hwexfile(py_engine.exec_file):
    def __init__(self, hw):
        self._hw = hw
        self._running = False
        self._instantiated_wires = set()
        self._instantiation_anonid = 0
        py_engine.exec_file.__init__(self)

    def _connect_wire(self, name, wireinst, connectedwires, inputs, ports, assign, firstval, wait, afterval):
        wire_basename = name.split('.')[-1].split('__')[-1]
        if wire_basename not in ports:
            raise WireError("Connecting to undefined port '%s' (from name '%s')" % (wire_basename,name))
        port = ports[wire_basename]
        if not hasattr(port, "_size"):
            raise WireError("Port does not have a _size attribute, please check port %s, connection to wire %s." % (wire_basename, wireinst._name))
        elif wireinst._size != port._size:
            raise WireError("Port size mismatch for port %s, wire is size %d got a port size of %d" % (wire_basename, wireinst._size, port._size))
                # size mismatch!
        if wireinst not in connectedwires:
            wireinst._instantiated_name = wireinst._name
            if wireinst._instantiated_name in self._instantiated_wires:
                wireinst._instantiated_name += "_INST%03d" % self._instantiation_anonid
                self._instantiation_anonid += 1
            self._instantiated_wires.add(wireinst._instantiated_name)
            if isinstance(wireinst, clock):
                self.cdlsim_instantiation.clock(wireinst._name, wireinst._init_delay, wireinst._cycles_high, wireinst._cycles_low)
                #print "CLOCK %s" % wireinst._name
            else:
                self.cdlsim_instantiation.wire("%s[%d]" % (wireinst._instantiated_name, wireinst._size))
                #print "WIRE %s" % wireinst._instantiated_name
            connectedwires.add(wireinst)
        if port not in connectedwires:
            connectedwires.add(port)
        if inputs:
            # This is an input to the module so the wire drives the port.
            self.cdlsim_instantiation.drive(name, wireinst._instantiated_name)
            #print "DRIVE %s %s" % (name, wireinst._instantiated_name)
            #print "Port %s driven by wire %s" % (repr(port), repr(wireinst))
            port._is_driven_by(wireinst)
        else:
            # This is an output from the module, so the port drives the wire.
            if assign is True:
                self.cdlsim_instantiation.assign(wireinst._instantiated_name, firstval, wait, afterval)
                #print "ASSIGN %s %d %d %d" % (wireinst._instantiated_name, firstval, wait, afterval)
            else:
                self.cdlsim_instantiation.drive(wireinst._instantiated_name, name)
                #print "DRIVE %s %s" % (wireinst._instantiated_name, name)
            #print "Wire %s driven by port %s" % (repr(wireinst), repr(port))
            wireinst._is_driven_by(port)

    def _connect_wires(self, name, wiredict, connectedwires, inputs, ports, assign=False, firstval=None, wait=None, afterval=None):
        for i in wiredict:
            if isinstance(wiredict[i], wire):
                self._connect_wire(name+i, wiredict[i], connectedwires, inputs, ports, assign, firstval, wait, afterval)
            elif isinstance(wiredict[i], wirebundle):
                if i not in ports or not isinstance(ports[i], wirebundle):
                    raise WireError("Connecting wire bundle %s to unknown port!" % i)
                self._connect_wires(name+i+"__", wiredict[i]._dict, connectedwires, inputs, ports[i]._dict, assign, firstval, wait, afterval)
            else:
                raise WireError("Connecting unknown wire type %s" % repr(wiredict[i].__class__))

    def _set_up_reset_values(self):
        # And set up the reset values.
        if not hasattr(self, "_connectedwires"):
            return
        for i in self._connectedwires:
            if i._reset_value:
                i._reset(i._reset_value)


    def exec_init(self):
        pass
    def exec_reset(self):
        self._set_up_reset_values()
    def exec_run(self):
        anonid = 1
        connectedwires = set()
        for i in self._hw._children:
            if not hasattr(i, "_name") or i._name is None:
                i._name = "__anon%3d" % anonid
                anonid += 1
            if isinstance(i, module):
                for j in i._forces.keys():
                    (submodule,s,option) = j.rpartition(".")
                    if isinstance(i._forces[j], str):
                        self.cdlsim_instantiation.module_force_option_string(i._name+"."+submodule, option, i._forces[j])
                    elif isinstance(i._forces[j], int):
                        self.cdlsim_instantiation.module_force_option_int(i._name+"."+submodule, option, i._forces[j])
                    else:
                        self.cdlsim_instantiation.module_force_option_object(i._name+"."+submodule, option, i._forces[j])
                for j in i._options:
                    if isinstance(i._options[j], str):
                        self.cdlsim_instantiation.option_string(j, i._options[j])
                    else:
                        self.cdlsim_instantiation.option_int(j, i._options[j])
                #print "MODULE %s %s" % (i._type, i._name)
                self.cdlsim_instantiation.module(i._type, i._name)
                ios = self._hw._engine.get_module_ios(i._name)
                if not ios:
                    raise WireError("Failed to find IOs for module '{0}'; probably failed to instantiate it".format(i._name))
                i._ports = i._ports_from_ios(ios, None)
            elif isinstance(i, th):
                i._thfile = _thfile(i)
                self.cdlsim_instantiation.option_string("clock", " ".join(i._clocks.keys()))
                self.cdlsim_instantiation.option_string("inputs", " ".join([" ".join(i._inputs[x]._name_list(x)) for x in i._inputs]))
                #print "INPUTS %s" % " ".join([" ".join(i._inputs[x]._name_list(x)) for x in i._inputs])
                self.cdlsim_instantiation.option_string("outputs", " ".join([" ".join(i._outputs[x]._name_list(x)) for x in i._outputs]))
                #print "OUTPUTS %s" % " ".join([" ".join(i._outputs[x]._name_list(x)) for x in i._outputs])
                self.cdlsim_instantiation.option_object("object", i._thfile)
                #print "MODULE se_test_harness %s" % i._name
                self.cdlsim_instantiation.module("se_test_harness", i._name)
                i._ports = i._ports_from_ios(self._hw._engine.get_module_ios(i._name), i._thfile)
            elif isinstance(i, clock):
                pass # we'll hook up later
            elif isinstance(i, timed_assign):
                pass # we'll hook up later
            else:
                raise NotImplementedError
            
        for i in self._hw._children:
            if hasattr(i, "_ports") and (not i._ports):
                raise WireError("Bad port structure in connecting module '%s' - perhaps a module could not be found?" % (repr(i)))
            if hasattr(i, "_clocks") and hasattr(i, "_ports"):
                self._connect_wires(i._name+".", i._clocks, connectedwires, inputs=True, ports=i._ports[0])
            if hasattr(i, "_inputs") and hasattr(i, "_ports"):
                self._connect_wires(i._name+".", i._inputs, connectedwires, inputs=True, ports=i._ports[1])
            if hasattr(i, "_outputs") and hasattr(i, "_ports"):
                if isinstance(i, timed_assign):
                    self._connect_wires(i._name+".", i._outputs, connectedwires, inputs=False, ports=i._ports[2], assign=True,
                                        firstval = i._firstval, wait=i._wait, afterval=i._afterval)
                else:
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

        self._connectedwires = connectedwires

        self._set_up_reset_values()

        # Hook up any waves.
        if self._hw._wavesinst:
            self._hw._wavesinst._connect_waves()

        # Say we're in business.
        self._running = True

print "*"*160
class hw(_clockable):
    """
    The object that represents a piece of hardware.
    """
    hw_class_data = {"engine":None,"id":0}
    def __init__(self, thread_mapping=None, children=None, *children_list):
        # Hack arguments
        if (children is None) or (type(children)!=list):
            if children is not None:
                children = [thread_mapping, children]
                thread_mapping = None
            elif thread_mapping is not None:
                children.append(thread_mapping)
                thread_mapping = None
            children.extend(children_list)

        if self.hw_class_data["engine"]==None:
            self.hw_class_data["engine"] = py_engine.py_engine()
        self.hw_class_data["id"] = self.hw_class_data["id"]+1
        self._id = self.hw_class_data["id"]

        self._wavesinst = None
        self._wave_hook = _wave_hook()
        children_unpacked = []
        for child in children:
            if (isinstance(child,list)):
                children_unpacked.extend(child)
            else:
                children_unpacked.append(child)
        children_unpacked = tuple(children_unpacked)
        _clockable.__init__(self, children_unpacked + (self._wave_hook,))
        self._engine = self.hw_class_data["engine"]

        if thread_mapping is not None:
            self._engine.thread_pool_init()
            for x in thread_mapping.keys():
                self._engine.thread_pool_add(x)
            for x in thread_mapping.keys():
                for module_name in thread_mapping[x]:
                    r = self._engine.thread_pool_map_module(x,module_name)
                    print "Map returned",r

        self.display_all_errors()
        self._hwex = _hwexfile(self)
        self._engine.describe_hw(self._hwex)
        self.display_all_errors()

    def passed(self):
        for i in self._children:
            if isinstance(i, th) and not isinstance(i, _internal_th):
                if not i.passed():
                    print "Test harness %s not PASSED" % str(th)
                    return False
        print "ALL TEST HARNESSES PASSED"
        return True

    def display_all_errors( self, max=10000 ):
        for i in range(max):
            x = self._engine.get_error(i)
            if x==None:
                break
            print >>sys.stderr, "CDL SIM ERROR %2d %s"%(i+1,x)
        self._engine.reset_errors()

    class _waves(object):
        """
        The object that controls waves, inside a hardware object. It's
        singletonized per hardware object
        """
        def __init__(self, hw):
            self._cdl_obj = None
            self._hw = hw
	    self._waves_enabled = 0
            if hw._hwex and hw._hwex._running:
                self._connect_waves()
                
        def _connect_waves(self):
            self._hw._wave_hook._thfile.cdlsim_wave.vcd_file("_waves%d"%self._hw._id)
            self._cdl_obj = eval("self._hw._wave_hook._thfile._waves%d"%self._hw._id)

        def reset(self):
            self._cdl_obj.reset()

        def open(self, filename):
            self._cdl_obj.open(filename)

        def close(self):
            self._cdl_obj.close()

        def enable(self):
            if not self._waves_enabled:
                self._waves_enabled = 1
                self._cdl_obj.enable()
            else:
                self._cdl_obj.restart()

        def disable(self):
            self._cdl_obj.pause()

        def file_size(self):
            return self._cdl_obj.file_size()

        def _add(self, hierlist):
            for x in hierlist:
                if isinstance(x, list):
                    self._add(x)
                elif isinstance(x, dict):
                    self._add([x[i] for i in x.keys()])
                else:
                    self._cdl_obj.add(x._name)

        def add(self, *hier):
            self._add(hier)

        def _add_hierarchy(self, hierlist):
            for x in hierlist:
                if isinstance(x, list):
                    self._add_hierarchy(x)
                elif isinstance(x, dict):
                    self._add_hierarchy([x[i] for i in x.keys()])
                else:
                    self._cdl_obj.add_hierarchy(x._name)

        def add_hierarchy(self, *hier):
            self._add_hierarchy(hier)

    def waves(self):
        if not self._wavesinst:
            self._wavesinst = hw._waves(self)
        return self._wavesinst

    def reset(self):
        """
        Reset the engine.
        """
        self._engine.reset()

    def step(self, cycles=1):
        """
        Step for n cycles.
        """
        self._engine.step(cycles,1)
        self.display_all_errors()
    def run_console( self, port=8745, locals={} ):
        console_server = c_python_telnet_server( port=port, locals=locals )
        console_server.serve()

def load_mif(filename, length=0, width=64):
    """
    Open a file and read in hex values into an array. Provided mostly for
    compatibility with legacy CDL.
    """
    fd = open(filename, "r")
    retarray = []
    for line in fd:
        if '#' in line or '//' in line:
            # If there's a comment, kill it off.
            line = line.split('#')[0]
            line = line.split('//')[0]
        if len(line) == 0:
            continue
        vals = line.split(" ")
        for val in vals:
            val = val.strip()
            if len(val) == 0:
                continue
            retarray.append(int(val, 16))
    # Finally, zero-pad the list.
    if len(retarray) < length:
        retarray.extend([0 for x in range(length-len(retarray))])
    fd.close()
    return retarray

def save_mif(array, filename, length=0, width=64):
    """
    Write hex values from an array into a file. Provided mostly for
    compatibility with legacy CDL.
    """
    fd = open(filename, "w")
    for val in array:
        print >>fd, "%0*.*x" % (width/4, width/4, val)
    # Finally, zero-pad the list.
    if len(array) < length:
        for i in range(length-len(array)):
            print >>fd, "%0*.*x" % (width/4, width/4, 0)
    fd.close()

