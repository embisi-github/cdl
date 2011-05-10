#!/usr/bin/env python
import sys, os, unittest
import pycdl

class simple_event_test_harness(pycdl.th):
    def __init__(self, clocks, inputs, outputs):
        pycdl.th.__init__(self, clocks, inputs, outputs)
    def run(self):
        self.e1 = self.event()
        self.e2 = self.event()
        print "Regression batch arg simple event test - *SL_EXEC_FILE TEST*"
        self.bfm_wait(1)
        self.spawn(self.second_thread)
        print "First thread start"
        self.e1.reset()
        self.e1.fire()
        self.e2.wait()
        print "First thread continues"
        self.passtest(0, "Test succeeded")
    def second_thread(self):
        print "Second thread start"
        self.e2.reset()
        self.e1.wait()
        print "Second thread continues"
        self.e2.fire()

class simple_event_hw(pycdl.hw):
    def __init__(self):
        self.system_clock = pycdl.clock(0, 1, 1)
        self.th = simple_event_test_harness(clocks={"clk": self.system_clock}, 
                                            inputs={},
                                            outputs={}
                                            )

	pycdl.hw.__init__(self, self.th, self.system_clock)

class TestEvent(unittest.TestCase):
    def test_simple_event(self):
        hw = simple_event_hw()
        hw.reset()
        hw.step(50)
        self.assertTrue(hw.passed())

    def finishme_test_all_event(self):
        hw = all_event_hw()
        waves = hw.waves()
        waves.reset()
        waves.open("event.vcd")
        waves.add_hierarchy(hw.th)
        waves.enable()
        hw.reset()
        hw.step(500)
        self.assertTrue(hw.passed())

if __name__ == '__main__':
    unittest.main()
