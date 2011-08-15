#!/usr/bin/env python
import sys, os, unittest
import pycdl

class partial_ports_test_harness(pycdl.th):
    def __init__(self, clocks, inputs, outputs):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.chain_in = outputs["chain_in"]
        self.chain_out = inputs["chain_out"]
        self.cnt = inputs["cnt"]
    def run(self):
        self.chain_in.reset(0)
        toggle = 0
        self.bfm_wait(60)
        
        for i in range(10):
            toggle ^= 1
            self.chain_in.drive(toggle)
            self.bfm_wait(1)
        
        for i in range(1000):
            toggle ^= 1
            self.chain_in.drive(toggle)
            self.bfm_wait(1)
            #print "%d Input %d Toggle %d Count %d" % (self.global_cycle(), self.chain_out.value(), toggle, self.cnt.value())
            if toggle == 0:
                if self.chain_out.value() != 2:
                    print "%d Expected input of 2: Input %d Toggle %d Count %d" % (self.global_cycle(), self.chain_out.value(), toggle, self.cnt.value())
                    self.failtest(self.global_cycle(), "**************************************************************************** Test failed")
            else:
                if self.chain_out.value() != 1:
                    print "%d Expected input of 1: Input %d Toggle %d Count %d" % (self.global_cycle(), self.chain_out.value(), toggle, self.cnt.value())
                    self.failtest(self.global_cycle(), "**************************************************************************** Test failed")
            if self.cnt.value() != 2:
                print "%d Expected count of 2: Input %d Toggle %d Count %d" % (self.global_cycle(), self.chain_out.value(), toggle, self.cnt.value())
                self.failtest(self.global_cycle(), "**************************************************************************** Test failed")
        self.passtest(self.global_cycle(), "Test succeeded")

class partial_ports_hw(pycdl.hw):
    def __init__(self):
        self.rst = pycdl.wire()
        self.wirein = pycdl.wire()
        self.wireout = pycdl.wire(2)
        self.cnt = pycdl.wire(4)
        self.clk = pycdl.clock(0, 1, 1)

        self.pp = pycdl.module("partial_ports", 
                               clocks={ "clk": self.clk }, 
                               inputs={ "rst": self.rst,
                                        "in": self.wirein },
                               outputs={ "out": self.wireout,
                                         "cnt": self.cnt })
        self.th = partial_ports_test_harness(clocks={ "clk": self.clk }, 
                                             inputs={ "chain_out": self.wireout,
                                                      "cnt": self.cnt},
                                             outputs={ "chain_in": self.wirein }
                                             )

        self.rst_seq = pycdl.timed_assign(self.rst, 1, 30, 0)
	pycdl.hw.__init__(self, self.pp, self.th, self.clk, self.rst_seq)

class bundle_width_test_harness(pycdl.th):
    def __init__(self, clocks, inputs, outputs):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.wireout = inputs["out"]
        self.wirein = outputs["in"]
    def run(self):
        value = 0
        print "Regression batch arg Running bundle width test to check for bugs in bundles reported Feb 6th 2008"
        self.wirein.reset(0)
        self.bfm_wait(60)

        for i in range(10):
            value += 1
            self.wirein.drive(value)
            self.bfm_wait(1)

        for i in range(1000):
            value += 1
            self.wirein.drive(value)
            self.bfm_wait(1)

            temp = value - 1
            if self.wireout.value() != temp:
                print "%d Expected input of value+1: got input %d Value %d" % (self.global_cycle(), self.wireout.value(), temp)
                self.failtest(self.global_cycle(), "**************************************************************************** Test failed")

        self.passtest(self.global_cycle(), "Test succeeded")

class bundle_width_hw(pycdl.hw):
    def __init__(self):
        self.rst = pycdl.wire()
        self.wirein = pycdl.wire(16)
        self.wireout = pycdl.wire(16)
        self.clk = pycdl.clock(0, 1, 1)

        self.bw = pycdl.module("bundle_width", 
                               clocks={ "clk": self.clk }, 
                               inputs={ "rst": self.rst,
                                        "in": self.wirein },
                               outputs={ "out": self.wireout })
        self.th = bundle_width_test_harness(clocks={ "clk": self.clk }, 
                                            inputs={ "out": self.wireout },
                                            outputs={ "in": self.wirein }
                                            )

        self.rst_seq = pycdl.timed_assign(self.rst, 1, 30, 0)
	pycdl.hw.__init__(self, self.bw, self.th, self.clk, self.rst_seq)

class check_64bits_test_harness(pycdl.th):
    def __init__(self, clocks, inputs, outputs):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.sum_out = inputs["sum_out"]
        self.selected_bit = inputs["selected_bit"]
        self.selected_bus = inputs["selected_bus"]
        self.input_bus = outputs["input_bus"]
        self.sum_enable = outputs["sum_enable"]
        self.select = outputs["select"]
    def run(self):
        failures = 0
        self.input_bus.reset(0)
        self.sum_enable.reset(0)
        self.select.reset(0)
        print "Regression batch arg Running check_64bits test to check for bugs in 64 bit handling (reset value, bit select, sub-bus select, bit assign, sub-bus assign reported Feb 18th 2008"
        self.bfm_wait(30)

        if self.sum_out.value() != 0x76543210fedcba98:
            print "%d:Reset value of sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0x76543210fedcba98)
            failures += 1

        # Clear bits 0 thru 15
        self.input_bus.drive(0)
        self.sum_enable.drive(1)
        self.select.drive(0)
        self.bfm_wait(2)
        if self.sum_out.value() != 0x76543210fedc0000:
            print "%d:Set bottom 16 bits of sum_out failed - got %x expected %x"% (self.global_cycle(), self.sum_out.value(), 0x76543210fedc0000)
            failures += 1

        # Clear bit 63 and 31 thru 15
        self.input_bus.drive(0)
        self.sum_enable.drive(1)
        self.select.drive(63)
        self.bfm_wait(1)
        self.sum_enable.drive(0)
        self.bfm_wait(1)
        if self.sum_out.value() != 0x7654321080000000:
            print "%d:Clear bit 63 and 31 thru 15 of sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0x7654321080000000)
            failures += 1

        # Clear bit 62
        self.input_bus.drive(0)
        self.sum_enable.drive(1)
        self.select.drive(62)
        self.bfm_wait(1)
        self.sum_enable.drive(0)
        self.bfm_wait(1)
        if self.sum_out.value() != 0x3654321080000000:
            print "%d:Clear bit 62 of sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0x3654321080000000)
            failures += 1

        # Clear bit 31
        self.input_bus.drive(0)
        self.sum_enable.drive(1)
        self.select.drive(31)
        self.bfm_wait(1)
        self.sum_enable.drive(0)
        self.bfm_wait(1)
        if self.sum_out.value() != 0x3654321000000000:
            print "%d:Clear bit 31 of sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0x3654321000000000)
            failures += 1

        # Clear bits 0-63
        for i in range(64):
            self.input_bus.drive(0)
            self.sum_enable.drive(1)
            self.select.drive(i)
            self.bfm_wait(1)
        self.sum_enable.drive(0)
        self.bfm_wait(1)
        if self.sum_out.value() != 0:
            print "%d:Cleared all bits of sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0)
            failures += 1

        # Add 0x123456789abcdef0
        self.sum_enable.drive(1)
        self.input_bus.drive(0x123456789abcdef0)
        self.select.drive(0)
        self.bfm_wait(1)
        self.sum_enable.drive(0)
        self.bfm_wait(1)
        if self.sum_out.value() != 0x123456789abcdef0:
            print "%d:Adding 64-bits to sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0x123456789abcdef0)
            failures += 1
        self.sum_enable.drive(1)
        self.bfm_wait(15)
        self.sum_enable.drive(0)
        self.bfm_wait(1)
        if self.sum_out.value() != 0x23456789abcdef00:
            print "%d:Adding 64-bits 16 times to sum_out failed - got %x expected %x" % (self.global_cycle(), self.sum_out.value(), 0x23456789abcdef00)
            failures += 1

        if failures != 0:
            self.failtest(self.global_cycle(), "**************************************************************************** Test failed")
        else:
            self.passtest(self.global_cycle(), "Test succeeded")

class check_64bits_hw(pycdl.hw):
    def __init__(self):
        self.rst = pycdl.wire()
        self.input_bus = pycdl.wire(64)
        self.sum_enable = pycdl.wire()
        self.select = pycdl.wire(6)
        self.sum_out = pycdl.wire(64)
        self.selected_bit = pycdl.wire()
        self.selected_bus = pycdl.wire(32)
        self.clk = pycdl.clock(0, 1, 1)

        self.check_64bits = pycdl.module("check_64bits", 
                                         clocks={ "clk": self.clk }, 
                                         inputs={ "rst": self.rst,
                                                  "input_bus": self.input_bus,
                                                  "sum_enable": self.sum_enable,
                                                  "select": self.select },
                                         outputs={ "sum_out": self.sum_out,
                                                   "selected_bit": self.selected_bit,
                                                   "selected_bus": self.selected_bus })
        self.th = check_64bits_test_harness(clocks={ "clk": self.clk }, 
                                            inputs={ "sum_out": self.sum_out,
                                                     "selected_bit": self.selected_bit,
                                                     "selected_bus": self.selected_bus },
                                            outputs={ "input_bus": self.input_bus,
                                                      "sum_enable": self.sum_enable,
                                                      "select": self.select }
                                            )

        self.rst_seq = pycdl.timed_assign(self.rst, 1, 30, 0)
	pycdl.hw.__init__(self, self.check_64bits, self.th, self.clk, self.rst_seq)



class TestBugs(unittest.TestCase):
    def test_partial_ports(self):
        hw = partial_ports_hw()
        waves = hw.waves()
        waves.reset()
        waves.open("pp.vcd")
        waves.add_hierarchy(hw.pp, hw.th)
        waves.enable()
        hw.reset()
        hw.step(5000)
        self.assertTrue(hw.passed())

    def test_bundle_width(self):
        hw = bundle_width_hw()
        waves = hw.waves()
        waves.reset()
        waves.open("bw.vcd")
        waves.add_hierarchy(hw.bw, hw.th)
        waves.enable()
        hw.reset()
        hw.step(5000)
        self.assertTrue(hw.passed())

    def test_check_64bits(self):
        hw = check_64bits_hw()
        waves = hw.waves()
        waves.reset()
        waves.open("check_64bits.vcd")
        waves.add_hierarchy(hw.check_64bits, hw.th)
        waves.enable()
        hw.reset()
        hw.step(5000)
        self.assertTrue(hw.passed())

if __name__ == '__main__':
    unittest.main()
