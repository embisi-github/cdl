#!/usr/bin/env python
import unittest
import py_engine
import sys

class c_tb(py_engine.exec_file):
    def __init__(self):
        py_engine.exec_file.__init__(self,firstname="fred")
    def exec_init(self):
        pass
    def invert_spawn(self, id, chain_to_follow):
        print >>sys.stderr, "Spawned run, id=%d, chain=%s" % (id, str(chain_to_follow))
        while True:
            self.cdlsim_sim.bfm_wait((id+1)*33)
            print >>sys.stderr, "Spawned %d running, BFM cycle %d, global cycle %d, chain value %d" % (id, self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), chain_to_follow.value())

    def exec_run(self):
        print >>sys.stderr, "Testbench run start"
        self.test_reset.reset(1)
        self.cdlsim_sim.bfm_wait(2)
        self.test_reset.drive(0)
        for i in range(8):
            self.py.pyspawn(self.invert_spawn, (i, getattr(self, "invert_chain_%d" % i)))
        for i in range(10):
            print >>sys.stderr, "Testbench run after", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value()
            self.cdlsim_sim.bfm_wait(1)
            self.toggle_0.wait_for_value(0)
        print >>sys.stderr, "Testbench run after", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value()
        self.cdlsim_sim.bfm_wait(1)
        print >>sys.stderr, "Testbench run after", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value()
        self.cdlsim_sim.bfm_wait(1)
        print >>sys.stderr, "Testbench run after", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value()
        self.cdlsim_sim.bfm_wait(1)
        print >>sys.stderr, "Testbench run after", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value()
        self.cdlsim_sim.bfm_wait(1)
        for i in range(1000):
            self.cdlsim_sim.bfm_wait(2)
            print >>sys.stderr, "Testbench run after", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value(), " ***"
        print >>sys.stderr, "Testbench run complete", self.cdlsim_sim.bfm_cycle(), self.cdlsim_sim.global_cycle(), self.toggle_0.value()

testbench = c_tb()

class c_sim(py_engine.exec_file):
    def __init__(self):
        py_engine.exec_file.__init__(self,firstname="fred")
        pass
    def exec_init(self):
        pass
    def exec_reset(self):
        pass
    def exec_run(self):
        self.cdlsim_instantiation.module("tie_high", "tie_high_0")
        self.cdlsim_instantiation.module("tie_both", "tie_both_0")
        self.cdlsim_instantiation.module("toggle",   "toggle_0")
        self.cdlsim_instantiation.module("invert",   "invert_0")
        self.cdlsim_instantiation.module("and",      "and_0")
        self.cdlsim_instantiation.module("invert_chain",      "invert_chain_0")
        self.cdlsim_instantiation.option_string("clock", "system_clock")
        self.cdlsim_instantiation.option_string( "inputs", "high_0 high_1 low_0 toggle_0 invert_0 and_0 invert_chain_0 invert_chain_1 invert_chain_2 invert_chain_3 invert_chain_4 invert_chain_5 invert_chain_6 invert_chain_7")
        self.cdlsim_instantiation.option_string( "outputs", "test_reset and_in_0 and_in_1")
        #self.cdlsim_instantiation.option_string( "filename", "test_harness.th")
        self.cdlsim_instantiation.option_object( "object", testbench )
        self.cdlsim_instantiation.module( "se_test_harness", "test_harness_0")

        self.cdlsim_instantiation.clock( "system_clock", 0, 1, 1 )
        self.cdlsim_instantiation.drive( "toggle_0.io_clock", "system_clock")
        self.cdlsim_instantiation.drive( "test_harness_0.system_clock", "system_clock")

        self.cdlsim_instantiation.wire( "test_reset" )

        self.cdlsim_instantiation.wire( "high_0", "high_1", "low_0", "toggle_0", "invert_0", "and_0" )
        self.cdlsim_instantiation.wire( "invert_chain_0", "invert_chain_1", "invert_chain_2", "invert_chain_3", "invert_chain_4", "invert_chain_5", "invert_chain_6", "invert_chain_7", "and_in_0", "and_in_1" )

        self.cdlsim_instantiation.drive( "high_0", "tie_high_0.value" )
        self.cdlsim_instantiation.drive( "high_1", "tie_both_0.value_high" )
        self.cdlsim_instantiation.drive( "low_0", "tie_both_0.value_low" )
        self.cdlsim_instantiation.drive( "toggle_0", "toggle_0.toggle" )
        self.cdlsim_instantiation.drive( "invert_0", "invert_0.out_value" )

        self.cdlsim_instantiation.drive( "toggle_0.io_reset", "test_reset" )
        self.cdlsim_instantiation.drive( "invert_0.in_value", "toggle_0" )
        self.cdlsim_instantiation.drive( "invert_chain_0.in_value", "toggle_0" )

        self.cdlsim_instantiation.drive( "invert_chain_0", "invert_chain_0.out_value_0" )
        self.cdlsim_instantiation.drive( "invert_chain_1", "invert_chain_0.out_value_1" )
        self.cdlsim_instantiation.drive( "invert_chain_2", "invert_chain_0.out_value_2" )
        self.cdlsim_instantiation.drive( "invert_chain_3", "invert_chain_0.out_value_3" )
        self.cdlsim_instantiation.drive( "invert_chain_4", "invert_chain_0.out_value_4" )
        self.cdlsim_instantiation.drive( "invert_chain_5", "invert_chain_0.out_value_5" )
        self.cdlsim_instantiation.drive( "invert_chain_6", "invert_chain_0.out_value_6" )
        self.cdlsim_instantiation.drive( "invert_chain_7", "invert_chain_0.out_value_7" )

        self.cdlsim_instantiation.drive( "and_0.in_value_0", "and_in_0" )
        self.cdlsim_instantiation.drive( "and_0.in_value_1", "and_in_1" )
        self.cdlsim_instantiation.drive( "and_0", "and_0.out_value" )

        self.cdlsim_instantiation.drive( "test_reset", "test_harness_0.test_reset" )
        self.cdlsim_instantiation.drive( "and_in_0", "test_harness_0.and_in_0" )
        self.cdlsim_instantiation.drive( "and_in_1", "test_harness_0.and_in_1" )
        self.cdlsim_instantiation.drive( "test_harness_0.high_0", "high_0" )
        self.cdlsim_instantiation.drive( "test_harness_0.high_1", "high_1" )
        self.cdlsim_instantiation.drive( "test_harness_0.low_0", "low_0" )
        self.cdlsim_instantiation.drive( "test_harness_0.and_0", "and_0" )
        self.cdlsim_instantiation.drive( "test_harness_0.toggle_0", "toggle_0" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_0", "invert_0" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_0", "invert_chain_0" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_1", "invert_chain_1" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_2", "invert_chain_2" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_3", "invert_chain_3" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_4", "invert_chain_4" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_5", "invert_chain_5" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_6", "invert_chain_6" )
        self.cdlsim_instantiation.drive( "test_harness_0.invert_chain_7", "invert_chain_7" )

class TestPyEngine(unittest.TestCase):
    def test_py_engine(self):
        x = py_engine.py_engine()
        print >>sys.stderr, "Create y as a c_sim"
        y=c_sim()
        print >>sys.stderr, "Created y"
        x.describe_hw(y)
        for i in range(1000):
            n = x.get_error(i)
            if n==None:
                break
            print >>sys.stderr, n
        x.reset()
        print >>sys.stderr, "Stepping 10"
        x.step(10)
        print >>sys.stderr, "Stepping another 10"
        x.step(10)
        print >>sys.stderr, "Stepping last 50"
        x.step(50)
        x.step(5000)

if __name__ == '__main__':
    unittest.main()
