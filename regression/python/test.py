#!/usr/bin/env python
#a Imports
import sys, os, unittest
cwd = os.getcwd()
sys.path.insert( 0, os.environ["CYCLICITY_ROOT"] )
sys.path.insert( 0, cwd+"/../build/osx" )
import pycdl

#a Tests
#c TestVector
class TestVector(unittest.TestCase):
    #f do_test_with_waves
    def do_test_with_waves(self, hw, vcd_file, waves_hierarchy, num_cycles):
        waves = hw.waves()
        #print waves
        waves.reset()
        waves.open(vcd_file)
        waves.add_hierarchy(waves_hierarchy)
        waves.enable()
        hw.reset()
        hw.step(num_cycles)
        print "Stepped",num_cycles
        self.assertTrue(hw.passed())

    #f test_toggle
    def test_toggle(self):
        test_vectors = [ (0, 0, 0, 0),
                    (0, 0, 0, 0),
                    (0, 0, 0, 0),
                    (0x0001, 0x2, 0x0000, 0000),
                    (0xffff, 0x0, 0x0000, 0000),
                    (0x0001, 0x4, 0x0001, 0000),
                    (0x0001, 0x4, 0x0001, 0000),
                    (0x0001, 0x4, 0x0002, 0000),
                    (0x1000, 0x4, 0x0003, 0000),
                    (0x0004, 0x8, 0x0004, 0000),
                    (0xffff, 0xc, 0x1004, 0000),
                    (0x0001, 0x0, 0x1000, 0000),
                    (0x0001, 0x0, 0x1000, 0000)
                    ]
        toggle_test_vectors = [ (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000) ]
        import vector
        hw = vector.vector_hw( module_name="vector_toggle__width_16", test_name="vector_op_1.mif", test_vectors=toggle_test_vectors, width=16 )
        self.do_test_with_waves(hw, "vector.vcd", hw.hw(), 1000)


    #f test_reverse__width_8
    def test_reverse__width_8(self):
        reverse_test_vectors = [ (0, 0, 0, 0),
                                 (0x0, 0x0, 0x0, 0x0),
                                 (0x0, 0x0, 0x0, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x55, 0x0, 0x00, 0x0),
                                 (0x05, 0x0, 0x00, 0x0),
                                 (0xa0, 0x0, 0xaa, 0x0),
                                 (0xfe, 0x0, 0xa0, 0x0),
                                 (0x01, 0x0, 0x05, 0x0),
                                 (0x02, 0x0, 0x7f, 0x0),
                                 (0x04, 0x0, 0x80, 0x0),
                                 (0x08, 0x0, 0x40, 0x0),
                                 (0x10, 0x0, 0x20, 0x0),
                                 (0x20, 0x0, 0x10, 0x0),
                                 (0x40, 0x0, 0x08, 0x0),
                                 (0x80, 0x0, 0x04, 0x0),
                                 (0x00, 0x0, 0x02, 0x0),
                                 (0x00, 0x0, 0x01, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                             ]
        import vector
        hw = vector.vector_hw( module_name="vector_reverse__width_8", test_name="vector_reverse__width_8.mif", test_vectors=reverse_test_vectors, width=8 )
        self.do_test_with_waves(hw, "vector_reverse__width_8.vcd", hw.hw(), 1000)


    #f test_mult_by_11__width_8
    def test_mult_by_11__width_8(self):
        mult_by_11_test_vectors = [ (0, 0, 0, 0),
                                    (0x0, 0x0, 0x0, 0x0),
                                    (0x0, 0x0, 0x0, 0x0),
                                    (0x0, 0x0, 0x0, 0x0),
                                    (0x00, 0x0, 0x00, 0x0),
                                    (0x01, 0x0, 0x00, 0x0),
                                    (0x02, 0x0, 0x00, 0x0),
                                    (0x03, 0x0, 11, 0x0),
                                    (0x04, 0x0, 22, 0x0),
                                    (0x05, 0x0, 33, 0x0),
                                    (0x06, 0x0, 44, 0x0),
                                    (0x07, 0x0, 55, 0x0),
                                    (0x08, 0x0, 66, 0x0),
                                    (0x09, 0x0, 77, 0x0),
                                    (0x0a, 0x0, 88, 0x0),
                                    (0x0b, 0x0, 99, 0x0),
                                    (0x0c, 0x0, 110, 0x0),
                                    (0x0d, 0x0, 121, 0x0),
                                    (0x0e, 0x0, 132, 0x0),
                                    (0x0f, 0x0, 143, 0x0),
                                    (0x10, 0x0, 154, 0x0),
                                    (0x11, 0x0, 165, 0x0),
                                    (0x12, 0x0, 176, 0x0),
                                    (0x0, 0x0, 187, 0x0),
                                    (0x0, 0x0, 198, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                                 (0x00, 0x0, 0x00, 0x0),
                             ]
        import vector
        hw = vector.vector_hw( module_name="vector_mult_by_11__width_8", test_name="vector_mult_by_11__width_8.mif", test_vectors=mult_by_11_test_vectors, width=8 )
        self.do_test_with_waves(hw, "vector_mult_by_11__width_8.vcd", hw.hw(), 1000)


    #f test_sum__width_4
    def test_sum__width_4(self):
        sum_test_vectors = [ (0, 0, 0, 0),
                             (0x0, 0x0, 0x0, 0x0),
                             (0x0, 0x0, 0x0, 0x0),
                             (0x0, 0x1, 0x0, 0x0),
                             (0x0, 0x2, 0x0, 0x0),
                             (0x0, 0x3, 0x1, 0x0),
                             (0x0, 0x4, 0x2, 0x0),
                             (0x0, 0x5, 0x3, 0x0),
                             (0x0, 0x6, 0x4, 0x0),
                             (0x0, 0x7, 0x5, 0x0),
                             (0x0, 0x8, 0x6, 0x0),
                             (0x0, 0x9, 0x7, 0x0),
                             (0x0, 0xa, 0x8, 0x0),
                             (0x0, 0xb, 0x9, 0x0),
                             (0x0, 0xc, 0xa, 0x0),
                             (0x0, 0xd, 0xb, 0x0),
                             (0x0, 0xe, 0xc, 0x0),
                             (0x0, 0xf, 0xd, 0x0),
                             (0x0, 0x0, 0xe, 0x0),
                             (0x0, 0x1, 0xf, 0x0),
                             (0x0, 0x2, 0x0, 0x0),
                             (0x0, 0x3, 0x1, 0x0),
                             ]
        import vector
        hw = vector.vector_hw( module_name="vector_sum__width_4", test_name="vector_sum__width_4.mif", test_vectors=sum_test_vectors, width=4 )
        self.do_test_with_waves(hw, "vector_sum__width_4.vcd", hw.hw(), 1000)


    #f test_sum__width_64
    def test_sum__width_64(self):
        import random
        r = random.Random()
        r.seed("BananaFlumpWomble")
        sum_test_vectors = [ (0, 0, 0, 0),(0, 0, 0, 0),(0, 0, 0, 0) ]
        history = [(0,0,0,0), (0,0,0,0)]
        for i in range(50):
            in0 = (r.randint(0,0xffff)<<48) | (r.randint(0,0xffff)<<32) | (r.randint(0,0xffff)<<16) | (r.randint(0,0xffff)<<0)
            in1 = (r.randint(0,0xffff)<<48) | (r.randint(0,0xffff)<<32) | (r.randint(0,0xffff)<<16) | (r.randint(0,0xffff)<<0)
            sum_test_vectors.append( (in0, in1, history[0][2], history[0][3]) )
            history.pop(0)
            history.append( (0,0,in0+in1,0) )
        sum_test_vectors.extend(history)
        import vector
        hw = vector.vector_hw( module_name="vector_sum__width_64", test_name="vector_sum__width_64.mif", test_vectors=sum_test_vectors, width=64 )
        self.do_test_with_waves(hw, "vector_sum__width_64.vcd", hw.hw(), 1000)


    #f test_toggle__width_16
    def test_toggle__width_16(self):
        toggle_test_vectors = [ (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000),
                        (0x0000,0x0000,0x0000,0xffff),
                        (0x0000,0x0000,0xffff,0x0000) ]
        import vector
        hw = vector.vector_hw( module_name="vector_toggle__width_16", test_name="vector_toggle__width_16.mif", test_vectors=toggle_test_vectors, width=16 )
        self.do_test_with_waves(hw, "vector_toggle__width_16.vcd", hw.hw(), 1000)

    #f test_toggle__width_18
    def test_toggle__width_18(self):
        toggle_test_vectors = [ (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000),
                        (0x0000,0x0000,0x0000,0x3ffff),
                        (0x0000,0x0000,0x3ffff,0x0000) ]
        import vector
        hw = vector.vector_hw( module_name="vector_toggle__width_18", test_name="vector_toggle__width__width_18.mif", test_vectors=toggle_test_vectors, width=18 )
        self.do_test_with_waves(hw, "vector_toggle__width__18.vcd", hw.hw(), 1000)

#a Toplevel
if __name__ == '__main__':
    unittest.main()
