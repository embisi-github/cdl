#!/usr/bin/env python

import sys, os, unittest
import pycdl

class and_gate_test_harness(pycdl.th):
    def __init__(self, clocks, inputs, outputs):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.and_out = inputs["and_out"]
        self.and_in_0 = outputs["and_in_0"]
        self.and_in_1 = outputs["and_in_1"]
    def run(self):
        self.bfm_wait(1)
        for i in range(2):
            for j in range(2):
                self.and_in_0.drive(i)
                self.and_in_1.drive(j)
                self.bfm_wait(1)
                if self.and_out.value() != i&j:
                    self.failtest(self.global_cycle(), "AND gate produced %d from %d and %d, not %d!" % (self.and_out.value(), i, j, i&j))
                    print "%d:  AND gate WRONGLY produced %d from %d and %d!" % (self.global_cycle(), self.and_out.value(), i, j)
                else:
                    print "%d: AND gate correctly produced %d from %d and %d!" % (self.global_cycle(), self.and_out.value(), i, j)
        self.passtest(self.global_cycle(), "AND gate test succeeded")
        print "%d: AND gate test completed" % self.global_cycle()

class test_basic_hw(pycdl.hw):
    def __init__(self):
        self.clk = pycdl.clock(0, 1, 1)
        self.and_in_0 = pycdl.wire()
        self.and_in_1 = pycdl.wire()
        self.and_out_0 = pycdl.wire()
        self.andgate = pycdl.module("and",
                                    clocks={},
                                    inputs={ "in_value_0": self.and_in_0,
                                             "in_value_1": self.and_in_1 },
                                    outputs={ "out_value": self.and_out_0 })

        self.th = and_gate_test_harness(clocks={"clk": self.clk},
                                        inputs={ "and_out": self.and_out_0 },
                                        outputs={ "and_in_0": self.and_in_0,
                                                  "and_in_1": self.and_in_1 })
        pycdl.hw.__init__(self, self.andgate, self.th, self.clk)

class TestPyCDL(unittest.TestCase):
    def test_basic(self):
        hw = test_basic_hw()
        engine = pycdl.engine(hw)
        engine.reset()
        engine.step(50)
        self.assertTrue(engine.passed())

if __name__ == '__main__':
    unittest.main()
