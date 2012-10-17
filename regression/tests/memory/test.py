#!/usr/bin/env python
import sys, os, unittest

cwd = os.getcwd()
print >>sys.stderr,"Using CDL from", os.environ["CYCLICITY_ROOT"]
sys.path.insert( 0, os.environ["CYCLICITY_ROOT"] )
sys.path.insert( 0, cwd+"/../../build/osx" )

import pycdl

class single_port_memory_th(pycdl.th):
    def __init__(self, clocks, inputs, outputs, test_vector_mif):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.sram_rd_0 = inputs["sram_rd_0"]
        self.address_0 = outputs["address_0"]
        self.read_not_write_0 = outputs["read_not_write_0"]
        self.byte_enables_0 = outputs["byte_enables_0"]
        self.write_data_0 = outputs["write_data_0"]
        self.test_vector_mif = test_vector_mif

    def run(self):
        last_rnw_0 = 0
        last_rnw_1 = 0
        failure = 0
        self.test_vectors = pycdl.load_mif(self.test_vector_mif)
        self.read_not_write_0.reset(1)
        self.byte_enables_0.reset(0)
        self.bfm_wait(1)
        tv_addr = 0
        while True:
            rnw_0 = self.test_vectors[tv_addr]
            ben_0 = self.test_vectors[tv_addr+1]
            addr_0 = self.test_vectors[tv_addr+2]
            data_0 = self.test_vectors[tv_addr+3]
            tv_addr += 4
            if rnw_0 > 256:
                break
            self.read_not_write_0.drive(rnw_0)
            self.byte_enables_0.drive(ben_0)
            self.address_0.drive(addr_0)
            self.write_data_0.drive(0xdeadbeef)
            if rnw_0 != 1:
                self.write_data_0.drive(data_0)
            self.bfm_wait(1)
            if last_rnw_0 != 0 and last_data_0 != self.sram_rd_0.value():
                print "Got %x expected %x on read port 0 vector %d" % (self.sram_rd_0.value(), last_data_0, tv_addr/4)
                failure = 1
            last_rnw_0 = rnw_0
            last_data_0 = data_0

        self.read_not_write_0.reset(1)
        self.byte_enables_0.reset(0)
        if failure != 0:
            self.failtest(self.global_cycle(), "**************************************************************************** Test failed")
        else:
            self.passtest(self.global_cycle(), "Test succeeded")

class dual_port_memory_th(pycdl.th):
    def __init__(self, clocks, inputs, outputs, test_vector_mif):
        pycdl.th.__init__(self, clocks, inputs, outputs)
        self.sram_rd_0 = inputs["sram_rd_0"]
        self.sram_rd_1 = inputs["sram_rd_1"]
        self.address_0 = outputs["address_0"]
        self.read_not_write_0 = outputs["read_not_write_0"]
        self.byte_enables_0 = outputs["byte_enables_0"]
        self.write_data_0 = outputs["write_data_0"]
        self.address_1 = outputs["address_1"]
        self.read_not_write_1 = outputs["read_not_write_1"]
        self.byte_enables_1 = outputs["byte_enables_1"]
        self.write_data_1 = outputs["write_data_1"]
        self.test_vector_mif = test_vector_mif

    def read_sram_location( self, address ):
        self._thfile.sim_msg.send_value("sram",8,0,address)
        return self._thfile.sim_msg.get_value(2)

    def write_sram_location( self, address, data ):
        self._thfile.sim_msg.send_value("sram",9,0,address,data)
        return self._thfile.sim_msg.get_value(0)

    def run(self):
        last_rnw_0 = 0
        last_rnw_1 = 0
        failure = 0
        self.test_vectors = pycdl.load_mif(self.test_vector_mif)
        self.read_not_write_0.reset(1)
        self.byte_enables_0.reset(0)
        self.read_not_write_1.reset(1)
        self.byte_enables_1.reset(0)
        self.bfm_wait(1)
        self._thfile.cdlsim_reg.sim_message("sim_msg")
        #self._thfile.cdlsim_chkpnt.checkpoint_init("blob")
        #self._thfile.cdlsim_chkpnt.checkpoint_add("fred","joe")
        #print dir(self)
        #print dir(self._thfile)
        tv_addr = 0
        while True:
            rnw_0 = self.test_vectors[tv_addr]
            ben_0 = self.test_vectors[tv_addr+1]
            addr_0 = self.test_vectors[tv_addr+2]
            data_0 = self.test_vectors[tv_addr+3]
            rnw_1 = self.test_vectors[tv_addr+4]
            ben_1 = self.test_vectors[tv_addr+5]
            addr_1 = self.test_vectors[tv_addr+6]
            data_1 = self.test_vectors[tv_addr+7]
            tv_addr += 8
            if rnw_0 > 256:
                break
            self.read_not_write_0.drive(rnw_0)
            self.read_not_write_1.drive(rnw_1)
            self.byte_enables_0.drive(ben_0)
            self.byte_enables_1.drive(ben_1)
            self.address_0.drive(addr_0)
            self.address_1.drive(addr_1)
            self.write_data_0.drive(0xdeadbeef)
            self.write_data_1.drive(0xdeadbeef)
            if rnw_0 != 1:
                self.write_data_0.drive(data_0)
            if rnw_1 != 1:
                self.write_data_1.drive(data_1)
            self.bfm_wait(1)
            if last_rnw_0 != 0 and last_data_0 != self.sram_rd_0.value():
                print >>sys.stderr, self.global_cycle(),"Got %x expected %x on read port 0 vector %d" % (self.sram_rd_0.value(), last_data_0, tv_addr/8)
                failure = 1
            if last_rnw_1 != 0 and last_data_1 != self.sram_rd_1.value():
                print >>sys.stderr, self.global_cycle(),"Got %x expected %x on read port 1 vector %d" % (self.sram_rd_1.value(), last_data_1, tv_addr/8)
                failure = 1
            last_rnw_0 = rnw_0
            last_data_0 = data_0
            last_rnw_1 = rnw_1
            last_data_1 = data_1

        self.read_not_write_0.reset(1)
        self.read_not_write_1.reset(1)
        self.byte_enables_0.reset(0)
        self.byte_enables_1.reset(0)
        if failure != 0:
            self.failtest(self.global_cycle(), "**************************************************************************** Test failed")
        else:
            self.passtest(self.global_cycle(), "Test succeeded")
        if False:
            for i in range(64):
                print "%3d %016x"%(i,self.read_sram_location(i))
        expected_data = []
        for i in range(64):
            d = (i*0x73261fc)>>12
            d = d & 0xffff
            expected_data.append(d)
            self.write_sram_location(i,d)
        for i in range(64):
            if self.read_sram_location(i)!=expected_data[i]:
                self.failtest(self.global_cycle(), "Misread of sram written by message got %016x expected %016x"%(self.read_sram_location(i),expected_data[i]))


class single_port_memory_srw_hw(pycdl.hw):
    def __init__(self, bits_per_enable, mif_filename, tv_filename):
        print "Regression batch arg mif:%s" % mif_filename
        print "Regression batch arg bits_per_enable:%d" % bits_per_enable
        print "Regression batch arg tv_file:%s" % tv_filename
        print "Running single port SRAM test on 1024x16 %d bits per enable srw sram mif file %s test vector file %s" % (bits_per_enable, mif_filename, tv_filename)

        if bits_per_enable == 8:
            self.byte_enables_0 = pycdl.wire(2)
        else:
            self.byte_enables_0 = pycdl.wire(1)
        self.test_reset = pycdl.wire()
        self.sram_rd_0 = pycdl.wire(16)
        self.address_0 = pycdl.wire(10)
        self.read_not_write_0 = pycdl.wire()
        self.write_data_0 = pycdl.wire(16)
        

        self.system_clock = pycdl.clock(0, 1, 1)

        sram_inputs={ "address": self.address_0,
                      "read_not_write": self.read_not_write_0,
                      "write_data": self.write_data_0 }
        if bits_per_enable != 0:
            sram_inputs["write_enable"] = self.byte_enables_0 
        self.sram = pycdl.module("se_sram_srw",
                                 options={ "filename": mif_filename,
                                           "size": 1024,
                                           "width": 16,
                                           "bits_per_enable": bits_per_enable,
                                           "verbose": 0 },
                                 clocks={ "sram_clock": self.system_clock },
                                 inputs=sram_inputs,
                                 outputs={ "data_out": self.sram_rd_0 })

        self.test_harness_0 = single_port_memory_th(clocks={ "clock": self.system_clock },
                                                    inputs={ "sram_rd_0": self.sram_rd_0 },
                                                    outputs={ "address_0": self.address_0,
                                                              "read_not_write_0": self.read_not_write_0,
                                                              "byte_enables_0": self.byte_enables_0,
                                                              "write_data_0": self.write_data_0},
                                                    test_vector_mif=tv_filename)
        self.rst_seq = pycdl.timed_assign(self.test_reset, 1, 5, 0)
	pycdl.hw.__init__(self, self.sram, self.test_harness_0, self.system_clock, self.rst_seq)
        
class single_port_memory_hw(pycdl.hw):
    def __init__(self, bits_per_enable, mif_filename, tv_filename):
        print "Regression batch arg mif:%s" % mif_filename
        print "Regression batch arg bits_per_enable:%d" % bits_per_enable
        print "Regression batch arg tv_file:%s" % tv_filename
        print "Running single port SRAM test on 1024x16 %d bits per enable mrw sram mif file %s test vector file %s" % (bits_per_enable, mif_filename, tv_filename)

        if bits_per_enable == 8:
            self.byte_enables_0 = pycdl.wire(2)
        else:
            self.byte_enables_0 = pycdl.wire(1)
        self.test_reset = pycdl.wire()
        self.sram_rd_0 = pycdl.wire(16)
        self.address_0 = pycdl.wire(10)
        self.read_not_write_0 = pycdl.wire()
        self.write_data_0 = pycdl.wire(16)
        

        self.system_clock = pycdl.clock(0, 1, 1)

        sram_inputs={ "address_0": self.address_0,
                      "read_not_write_0": self.read_not_write_0,
                      "write_data_0": self.write_data_0 }
        if bits_per_enable != 0:
            sram_inputs["write_enable_0"] = self.byte_enables_0 
        self.sram = pycdl.module("se_sram_mrw",
                                 options={ "filename": mif_filename,
                                           "num_ports": 1,
                                           "size": 1024,
                                           "width": 16,
                                           "bits_per_enable": bits_per_enable,
                                           "verbose": 0 },
                                 clocks={ "sram_clock_0": self.system_clock },
                                 inputs=sram_inputs,
                                 outputs={ "data_out_0": self.sram_rd_0 })
        self.test_harness_0 = single_port_memory_th(clocks={ "clock": self.system_clock },
                                                    inputs={ "sram_rd_0": self.sram_rd_0 },
                                                    outputs={ "address_0": self.address_0,
                                                              "read_not_write_0": self.read_not_write_0,
                                                              "byte_enables_0": self.byte_enables_0,
                                                              "write_data_0": self.write_data_0},
                                                    test_vector_mif=tv_filename)
        self.rst_seq = pycdl.timed_assign(self.test_reset, 1, 5, 0)
	pycdl.hw.__init__(self, self.sram, self.test_harness_0, self.system_clock, self.rst_seq)
        
class dual_port_memory_hw(pycdl.hw):
    def __init__(self, bits_per_enable, mif_filename, tv_filename):
        print "Regression batch arg mif:%s" % mif_filename
        print "Regression batch arg bits_per_enable:%d" % bits_per_enable
        print "Regression batch arg tv_file:%s" % tv_filename
        print "Running dual port SRAM test on 1024x16 %d bits per enable mrw sram mif file %s test vector file %s" % (bits_per_enable, mif_filename, tv_filename)

        if bits_per_enable == 8:
            self.byte_enables_0 = pycdl.wire(2)
            self.byte_enables_1 = pycdl.wire(2)
        else:
            self.byte_enables_0 = pycdl.wire(1)
            self.byte_enables_1 = pycdl.wire(1)
        self.test_reset = pycdl.wire()
        self.sram_rd_0 = pycdl.wire(16)
        self.address_0 = pycdl.wire(10)
        self.read_not_write_0 = pycdl.wire()
        self.write_data_0 = pycdl.wire(16)
        self.sram_rd_1 = pycdl.wire(16)
        self.address_1 = pycdl.wire(10)
        self.read_not_write_1 = pycdl.wire()
        self.write_data_1 = pycdl.wire(16)
        

        self.system_clock = pycdl.clock(0, 1, 1)

        sram_inputs={ "address_0": self.address_0,
                      "read_not_write_0": self.read_not_write_0,
                      "write_data_0": self.write_data_0,
                      "address_1": self.address_1,
                      "read_not_write_1": self.read_not_write_1,
                      "write_data_1": self.write_data_1 }
        if bits_per_enable != 0:
            sram_inputs["write_enable_0"] = self.byte_enables_0 
            sram_inputs["write_enable_1"] = self.byte_enables_1 
        self.sram = pycdl.module("se_sram_mrw",
                                 options={ "filename": mif_filename,
                                           "num_ports": 2,
                                           "size": 1024,
                                           "width": 16,
                                           "bits_per_enable": bits_per_enable,
                                           "verbose": 0 },
                                 clocks={ "sram_clock_0": self.system_clock,
                                          "sram_clock_1": self.system_clock },
                                 inputs=sram_inputs,
                                 outputs={ "data_out_0": self.sram_rd_0,
                                           "data_out_1": self.sram_rd_1 })
        self.test_harness_0 = dual_port_memory_th(clocks={ "clock": self.system_clock },
                                                  inputs={ "sram_rd_0": self.sram_rd_0,
                                                           "sram_rd_1": self.sram_rd_1 },
                                                  outputs={ "address_0": self.address_0,
                                                            "read_not_write_0": self.read_not_write_0,
                                                            "byte_enables_0": self.byte_enables_0,
                                                            "write_data_0": self.write_data_0,
                                                            "address_1": self.address_1,
                                                            "read_not_write_1": self.read_not_write_1,
                                                            "byte_enables_1": self.byte_enables_1,
                                                            "write_data_1": self.write_data_1 },
                                                  test_vector_mif=tv_filename)
        self.rst_seq = pycdl.timed_assign(self.test_reset, 1, 5, 0)
	pycdl.hw.__init__(self, self.sram, self.test_harness_0, self.system_clock, self.rst_seq)
        

class TestMemory(unittest.TestCase):
    def do_memory_test(self, memory_type, bits_per_enable, mif_filename, tv_filename):
        hw = memory_type(bits_per_enable, mif_filename, tv_filename)
        hw.reset()
        hw.step(1000)
        hw.step(1000)
        self.assertTrue(hw.passed())

    def xtest_simple(self):
        hw = simple_memory_hw()
        hw.reset()
        hw.step(50)
        self.assertTrue(hw.passed())

    # FINISHME/FIXME: Gavin's code has added some sim_message stuff that we don't have in PyCDL.

    # This is the first single-port test: 1024x16, rnw, no additional enables
    def test_1024x16_srw_rnw_no_enables(self):
        self.do_memory_test(single_port_memory_srw_hw, 0, "single_port_memory_in.mif", "single_port_memory.tv")

    # This is the second single-port test: 1024x16, rnw, with write enable
    def test_1024x16_srw_rnw_write_enable(self):
        self.do_memory_test(single_port_memory_srw_hw, 16, "single_port_memory_in.mif", "single_port_memory_1_be.tv")

    # This is the third single-port test: 1024x16, rnw, with individaul byte write enables
    def test_1024x16_srw_rnw_byte_write_enable(self):
        self.do_memory_test(single_port_memory_srw_hw, 8, "single_port_memory_in.mif", "single_port_memory_2_be.tv")

    # This is the first  single multi-port test: 1024x16, rnw, no additional enables
    def test_1024x16_srnw_byte_write_enable(self):
        self.do_memory_test(single_port_memory_hw, 0, "single_port_memory_in.mif", "single_port_memory.tv")

    # This is the second single multi-port test: 1024x16, rnw, with write enable
    def test_1024x16_srnw_write_enable(self):
        self.do_memory_test(single_port_memory_hw, 16, "single_port_memory_in.mif", "single_port_memory_1_be.tv")

    # This is the third single multi-port test: 1024x16, rnw, with individaul byte write enables
    def test_1024x16_srnw_byte_write_enable(self):
        self.do_memory_test(single_port_memory_hw, 8, "single_port_memory_in.mif", "single_port_memory_2_be.tv")

    # This is the first  dual-port test: 1024x16, rnw, no additional enables
    def test_1024x16_drnw_byte_write_enable(self):
        self.do_memory_test(dual_port_memory_hw, 0, "dual_port_memory_in.mif", "dual_port_memory.tv")

    # This is the second dual-port test: 1024x16, rnw, with write enable
    def test_1024x16_drnw_write_enable(self):
        self.do_memory_test(dual_port_memory_hw, 16, "dual_port_memory_in.mif", "dual_port_memory_1_be.tv")

    # This is the third dual-port test: 1024x16, rnw, with individaul byte write enables
    def test_1024x16_drnw_byte_write_enable2(self):
        self.do_memory_test(dual_port_memory_hw, 8, "dual_port_memory_in.mif", "dual_port_memory_2_be.tv")


if __name__ == '__main__':
    unittest.main()
