#!/usr/bin/env python
#a imports
import sys, os, unittest
import pycdl

#a Classes
#c vector_th
class vector_th(pycdl.th):
    def __init__(self, vectors, clocks, inputs, outputs):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.vectors = vectors
        self.vector_output_0 = inputs["vector_output_0"]
        self.vector_output_1 = inputs["vector_output_1"]
        self.vector_input_0 = outputs["vector_input_0"]
        self.vector_input_1 = outputs["vector_input_1"]
        self.test_passed = False
        self.test_failures = 0

    def run(self):
        cycle=0
        self.bfm_wait(1)
        for (i0,i1,o0,o1) in self.vectors:
            self.vector_input_0.drive(i0)
            self.vector_input_1.drive(i1)
            #if (o0!=self.vector_output_0.value().value()):
            #    self.failure( "Mismatch between output 0 and expected value ({0:x} versus {1:x})".format(self.vector_output_0.value().value(),o0) )
            #if (o1!=self.vector_output_1.value().value()):
            #    self.failure( "Mismatch between output 1 and expected value ({0:x} versus {1:x})".format(self.vector_output_1.value().value(),o1) )
            self.bfm_wait(1)
            cycle=cycle+1
        self.bfm_wait(10)
        self.test_passed = (self.test_failures==0)
        print "Completed run with passed",self.test_passed
        self.bfm_wait(999999)

    def failure(self, reason):
        self.test_failures = self.test_failures+1
        print "{0}:Failure in vector test harness: {1}".format(self.global_cycle(),reason)
    def passed(self):
        if (self.test_passed):
            print "{0}:Passed vector test".format(self.global_cycle())
        return self.test_passed

#c vector_hw
class vector_hw(pycdl.hw):
    def __init__(self, module_name, test_name, test_vectors, width):
        print "Regression batch arg",module_name
        print "Regression batch arg",test_name
        #print "Running vector test on module %s with mif file %s" % (module_name, module_mif_filename)

        system_clock = pycdl.clock(0, 1, 1)

        test_reset = pycdl.wire( 1 )
        vector_input_0 = pycdl.wire( width )
        vector_input_1 = pycdl.wire( width )
        vector_output_0 = pycdl.wire( width )
        vector_output_1 = pycdl.wire( width )

        self.reset_driver = pycdl.timed_assign( signal=test_reset,
                                                init_value = 1,
                                                wait=5,
                                                later_value=0 )

        self.th = vector_th( vectors = test_vectors,
                             clocks = { "clk": system_clock,
                                           },
                                inputs = { "vector_output_0": vector_output_0,
                                            "vector_output_1": vector_output_1,
                                            },
                                outputs = { "vector_input_0": vector_input_0,
                                           "vector_input_1": vector_input_1,
                                           },
                                )

        self.dut = pycdl.module( module_name, 
                                   clocks = { "io_clock":  system_clock,
                                              },
                                   inputs = { "io_reset":       test_reset,
                                              "vector_input_0": vector_input_0,
                                              "vector_input_1": vector_input_1,
                                              },
                                   outputs = { "vector_output_0": vector_output_0,
                                               "vector_output_1": vector_output_1,
                                              },
                                   )

        pycdl.hw.__init__(self, system_clock, self.th, self.dut, self.reset_driver )
    def hw(self):
        return [self.dut, self.th]


