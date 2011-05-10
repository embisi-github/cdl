#!/usr/bin/env python
import sys, os, unittest
import pycdl

class vector_test_harness(pycdl.th):
    def __init__(self, clocks, inputs, outputs, vectors_filename):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.vector_output_0 = inputs["vector_output_0"]
        self.vector_output_1 = inputs["vector_output_1"]
        self.test_reset = outputs["test_reset"]
        self.vector_input_0 = outputs["vector_input_0"]
        self.vector_input_1 = outputs["vector_input_1"]
        self.vectors_filename = vectors_filename

    def test_values(self, vector_number):
        print "Cycle %d vector number %d, testing values" % (self.global_cycle(), vector_number)
        self.vector_input_0.drive(self.test_vectors[vector_number*4])
        self.vector_input_1.drive(self.test_vectors[vector_number*4+1])
        tv_0 = self.test_vectors[vector_number*4+2]
        tv_1 = self.test_vectors[vector_number*4+3]
        print "   info: expected %x %x got %x %x" % (tv_0, tv_1, self.vector_output_0.value(), self.vector_output_1.value())
        if tv_0 != self.vector_output_0.value() or tv_1 != self.vector_output_1.value():
            print "Test failed, vector number %d" % vector_number
            self.failtest(vector_number,  "**************************************************************************** Test failed")
        
    def run(self):
        self.test_vectors = pycdl.load_mif(self.vectors_filename, 2048, 64)
        self.test_reset.reset(1)
        self.bfm_wait(1)
        self.test_values(0)
        self.bfm_wait(1)
        self.test_values(1)
        self.test_reset.drive(0)
        self.bfm_wait(1)
        self.test_values(2)
        for i in range(10):
            self.bfm_wait(1)
            self.test_values(i+3)
        self.passtest(self.global_cycle(), "Test succeeded")

class vector_hw(pycdl.hw):
    def __init__(self, width, module_name, module_mif_filename):
        print "Running vector test on module %s with mif file %s" % (module_name, module_mif_filename)

        self.test_reset = pycdl.wire()
        self.vector_input_0 = pycdl.wire(width)
        self.vector_input_1 = pycdl.wire(width)
        self.vector_output_0 = pycdl.wire(width)
        self.vector_output_1 = pycdl.wire(width)
        self.system_clock = pycdl.clock(0, 1, 1)

        self.dut_0 = pycdl.module(module_name, 
                                  clocks={ "io_clock": self.system_clock }, 
                                  inputs={ "io_reset": self.test_reset,
                                           "vector_input_0": self.vector_input_0,
                                           "vector_input_1": self.vector_input_1 },
                                  outputs={ "vector_output_0": self.vector_output_0,
                                            "vector_output_1": self.vector_output_1 })
        self.test_harness_0 = vector_test_harness(clocks={ "clock": self.system_clock }, 
                                                  inputs={ "vector_output_0": self.vector_output_0,
                                                           "vector_output_1": self.vector_output_1 },
                                                  outputs={ "test_reset": self.test_reset,
                                                            "vector_input_0": self.vector_input_0,
                                                            "vector_input_1": self.vector_input_1 },
                                                  vectors_filename=module_mif_filename)
	pycdl.hw.__init__(self, self.dut_0, self.test_harness_0, self.system_clock)



class TestVector(unittest.TestCase):
    def do_vector_test(self, width, module_name, module_mif_filename):
        hw = vector_hw(width, module_name, module_mif_filename)
        waves = hw.waves()
        waves.open(module_name+".vcd")
        waves.add_hierarchy(hw.dut_0)
        waves.enable()
        hw.reset()
        hw.step(50)
        self.assertTrue(hw.passed())

    def test_toggle_16(self):
        self.do_vector_test(16, "vector_toggle__width_16", "vector_toggle__width_16.mif")

    def test_toggle_18(self):
        self.do_vector_test(18, "vector_toggle__width_18", "vector_toggle__width_18.mif")

    def test_add_4(self):
        self.do_vector_test(4, "vector_add__width_4", "vector_add__width_4.mif")

    def test_mult_11_8(self):
        self.do_vector_test(8, "vector_mult_by_11__width_8", "vector_mult_by_11__width_8.mif")

    def test_reverse_8(self):
        self.do_vector_test(8, "vector_reverse__width_8", "vector_reverse__width_8.mif")

    # Run vector_nest with the vector_reverse file; the functionality is identical
    def test_nest_8(self):
        self.do_vector_test(8, "vector_nest__width_8", "vector_reverse__width_8.mif")

    def test_sum_4(self):
        self.do_vector_test(4, "vector_sum__width_4", "vector_sum__width_4.mif")

    def test_sum_2_4(self):
        self.do_vector_test(4, "vector_sum_2__width_4", "vector_sum_2__width_4.mif")

    def test_op_1_16(self):
        self.do_vector_test(16, "vector_op_1", "vector_op_1.mif")

    def test_op_2_16(self):
        self.do_vector_test(16, "vector_op_2", "vector_op_2.mif")



if __name__ == '__main__':
    unittest.main()
