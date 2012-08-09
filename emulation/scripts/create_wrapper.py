#!/usr/bin/env python

#a Verilog classes
#c c_signal
class c_signal( object ):
    def __init__( self, width, name, signal_type ):
        self.name = name
        self.width = width
        self.signal_type = signal_type
    def verilog( self ):
        if self.width>1:
            return "[%3d:0] %s" % (self.width-1,self.name)
        return "        %s" % (self.name)

#c c_input_signal
class c_input_signal( c_signal ):
    def __init__( self, name, width=1 ):
        c_signal.__init__( self, name=name, width=width, signal_type="input" )

#c c_output_signal
class c_output_signal( c_signal ):
    def __init__( self, name, width=1 ):
        c_signal.__init__( self, name=name, width=width, signal_type="output" )

#c c_clock_signal
class c_clock_signal( c_signal ):
    def __init__( self, name ):
        c_signal.__init__( self, name=name, width=1, signal_type="clock" )

#c c_verilog_instance
class c_verilog_instance( object ):
    """
    module_name is a string
    module_type is a string
    port_map    is a dictionary of signal -> verilog reg/wire/input/output/clock
    """
    def __init__( self, module_name, module_type, port_map={} ):
        self.module_name = module_name
        self.module_type = module_type
        self.port_map = port_map
    def output( self, writer ):
        writer("\t%s %s\n" % (self.module_type, self.module_name) );
        writer("\t\t(\n");
        ports = self.port_map.keys()
        ports.sort( lambda x,y : cmp(x.name, y.name) )
        num_ports = len(ports)
        i = 0
        for p in ports:
            comma = ""
            if i<num_ports-1:comma = ","
            writer("\t\t\t.%s(%s)%s // %s\n"%(p.name,self.port_map[p].name,comma,self.port_map[p].signal_type))
            i = i +1
        writer("\t\t);\n");

#c c_verilog_module
class c_verilog_module( object ):
    """
    string module_type
    list(c_signal)  clocks
    list(c_signal)  inputs
    list(c_signal)  outputs
    list(c_signal)  regs
    list(c_signal)  wires
    dict(name:{type:string port_map:{c_signal:verilog reg/wire/input/output/clock}}) instances

    """
    def __init__( self, module_type ):
        self.module_type = module_type
        self.clocks = []
        self.inputs = []
        self.outputs = []
        self.regs = []
        self.wires = []
        self.instances = {}
        self.code_blocks = []
    def find_reg( self, signal_name ):
        for r in self.regs:
            if r.name == signal_name:
                return r
        return None
    def find_wire( self, signal_name ):
        for r in self.wires:
            if r.name == signal_name:
                return r
        return None
    def add_clock( self, signal ):
        self.clocks.append(signal)
    def add_input( self, signal ):
        self.inputs.append(signal)
    def add_output( self, signal ):
        self.outputs.append(signal)
    def add_reg( self, signal ):
        self.regs.append(signal)
    def add_wire( self, signal ):
        self.wires.append(signal)
    def add_instance( self, verilog_instance ):
        self.instances[verilog_instance.module_name] = verilog_instance
    def add_code_block( self, code_block ):
        self.code_blocks.append( code_block )
    def output( self, writer ):
        writer( "module %s\n" % self.module_type )
        writer( "(\n" )
        for s in self.clocks:
            writer( "\t%s,\n" % s.name )
        for s in self.inputs:
            writer( "\t%s,\n" % s.name )
        for s in self.outputs:
            writer( "\t%s,\n" % s.name )
        writer( ");\n" )
        for s in self.clocks:
            writer( "\tinput %s;\n" % s.verilog() )
        for s in self.inputs:
            writer( "\tinput %s;\n" % s.verilog() )
        for s in self.outputs:
            writer( "\toutput %s;\n" % s.verilog() )
        for s in self.wires:
            writer( "\twire %s;\n" % s.verilog() )
        for s in self.regs:
            writer( "\treg %s;\n" % s.verilog() )
        for i in self.instances:
            self.instances[i].output( writer )
        for c in self.code_blocks:
            c.output( writer )
        writer( "endmodule //%s\n\n\n" % self.module_type )

#c c_wrapper
class c_wrapper( object ):
    """
    clocks is list[clock signal] that are clocks for the wrapped module
    inputs is list[input signal] that are inputs for the wrapped module
    outputs is list[output signal] that are outputs for the wrapped module
    """
    def __init__( self, module_type, output_type=None ):
        self.module_type = module_type
        if output_type is None:
            output_type = module_type+"_wrapper"
        self.output_type = output_type
        self.clocks = []
        self.clock_map = {}
        self.inputs = []
        self.outputs = []
        self.verilog = None
        self.prefix = "__wrp_"
    def add_clock( self, signal ):
        self.clocks.append(signal)
    def add_input( self, signal ):
        self.inputs.append(signal)
    def add_output( self, signal ):
        self.outputs.append(signal)
    def output_verilog( self, writer ):
        if self.verilog:
            self.verilog.output(writer)
    def create_verilog( self ):
        self.verilog = c_verilog_module( self.output_type )
        wrp_clk         = c_clock_signal( self.prefix+"clk" )
        wrp_reset_n     = c_input_signal( self.prefix+"reset_n" )
        wrp_cmd         = c_input_signal( self.prefix+"cmd", 3 )
        wrp_address     = c_input_signal( self.prefix+"address", 12 )
        wrp_input_data  = c_input_signal( self.prefix+"input_data", 128 )
        wrp_output_data = c_output_signal( self.prefix+"output_data", 128 )
        wrp_fsm         = c_signal( name=self.prefix+"fsm", signal_type="reg", width=3 )
        wrp_clocks      = c_signal( name=self.prefix+"clocks", signal_type="reg", width=8 )
        self.verilog.add_clock( wrp_clk )
        self.verilog.add_input( wrp_reset_n )
        self.verilog.add_input( wrp_cmd )
        self.verilog.add_input( wrp_address )
        self.verilog.add_input( wrp_input_data )
        self.verilog.add_output( wrp_output_data )
        self.verilog.add_reg( wrp_output_data )
        self.verilog.add_reg( wrp_fsm )
        self.verilog.add_reg( wrp_clocks )
        for s in self.clocks:
            self.verilog.add_reg( s )
        for s in self.inputs:
            self.verilog.add_reg( s )
        for s in self.outputs:
            self.verilog.add_wire( s )
        port_map = {}
        for c in self.clocks:
            port_map[c] = self.verilog.find_reg(c.name)
        for s in self.inputs:
            port_map[s] = self.verilog.find_reg(s.name)
        for s in self.outputs:
            port_map[s] = self.verilog.find_wire(s.name)
        self.verilog.add_instance( c_verilog_instance( "wrapped_instance", self.module_type, port_map ) )

        input_names = []
        for s in self.inputs:
            input_names.append(s.name)
        input_names.sort()
        input_reg_mappings = []
        input_number = 0
        input_bit = 0
        while input_number<len(input_names):
            reg_mapping = []
            while ((input_number<len(input_names)) and len(reg_mapping)<128):
                s = self.verilog.find_reg(input_names[input_number])
                reg_mapping.append( (s, input_bit) )
                input_bit = input_bit+1
                if input_bit>=s.width:
                    input_number = input_number + 1;
                    input_bit = 0
            input_reg_mappings.append(reg_mapping)

        output_names = []
        for s in self.outputs:
            output_names.append(s.name)
        output_names.sort()
        output_reg_mappings = []
        output_number = 0
        output_bit = 0
        while output_number<len(output_names):
            reg_mapping = []
            while ((output_number<len(output_names)) and len(reg_mapping)<128):
                s = self.verilog.find_wire(output_names[output_number])
                reg_mapping.append( (s, output_bit) )
                output_bit = output_bit+1
                if output_bit>=s.width:
                    output_number = output_number + 1;
                    output_bit = 0
            output_reg_mappings.append(reg_mapping)
        
        wrp_fsm_becomes_zero = c_assignment( c_lvar(wrp_fsm), c_expression(constant=0) )
        wrp_fsm_case = c_case( c_expression(lvar=c_lvar(wrp_fsm)),
                               [ c_case_entry( c_expression(constant=0), [ c_assignment( c_lvar(wrp_fsm), c_expression(lvar=c_lvar(wrp_cmd)) ) ] ),
                                 c_case_entry( c_expression(constant=1), [ c_assignment( c_lvar(wrp_fsm), c_expression(constant=0) ) ] ),
                                 c_case_entry( c_expression(constant=2), [ c_assignment( c_lvar(wrp_fsm), c_expression(constant=0) ) ] ),
                                 c_case_entry( c_expression(constant=3), [ c_assignment( c_lvar(wrp_fsm), c_expression(constant=0) ) ] ),
                                 ] )
        wrp_fsm_code = c_if( c_expression( ("!", c_expression(lvar=c_lvar(wrp_reset_n))) ),
                             if_true  = [wrp_fsm_becomes_zero],
                             if_false = [wrp_fsm_case] )
        wrp_fsm_code_block = c_code_block( sensitivity=[c_clk_edge(wrp_clk,posedge=True), 
                                                         c_clk_edge(wrp_reset_n,posedge=False)],
                                            code=[ wrp_fsm_code ] )
        self.verilog.add_code_block( wrp_fsm_code_block )

        wrp_clocks_becomes_zero   = c_assignment( c_lvar(wrp_clocks), c_expression(constant=0) )
        wrp_clocks_becomes_inputs = c_assignment( c_lvar(wrp_clocks), c_expression(lvar=c_lvar(wrp_input_data,bit=0,width=8)) )
        wrp_input_guard = c_if( c_expression( ("==", c_expression(lvar=c_lvar(wrp_fsm)), c_expression(constant=2) ) ),
                                if_true = [wrp_clocks_becomes_inputs] )
        wrp_clocks_code = c_if( c_expression( ("!", c_expression(lvar=c_lvar(wrp_reset_n))) ),
                             if_true  = [wrp_clocks_becomes_zero],
                             if_false = [wrp_input_guard] )
        wrp_clocks_code_block = c_code_block( sensitivity=[c_clk_edge(wrp_clk,posedge=True), 
                                                         c_clk_edge(wrp_reset_n,posedge=False)],
                                            code=[ wrp_clocks_code ] )
        self.verilog.add_code_block( wrp_clocks_code_block )

        wrp_clock_map_code = []
        i=0
        for c in self.clocks:
            wrp_clock_map_code.append( c_assignment( c_lvar(self.verilog.find_reg(c.name)), c_expression(lvar=c_lvar(wrp_clocks,i)) ) )
            i = i + 1
        wrp_clock_map_code_block = c_code_block( sensitivity=[c_any_edge(wrp_clocks)],
                                            code=wrp_clock_map_code )
        self.verilog.add_code_block( wrp_clock_map_code_block )

        wrp_input_code = []
        for i in range(len(input_reg_mappings)):
            wrp_input_assignments = []
            for j in range(len(input_reg_mappings[i])):
                wrp_input_assignment = c_assignment( c_lvar(input_reg_mappings[i][j][0],input_reg_mappings[i][j][1]),
                                                      c_expression(lvar=c_lvar(wrp_input_data,j)) )
                wrp_input_assignments.append(wrp_input_assignment)
            wrp_input_condition = c_if( c_expression( ("==", c_expression(lvar=c_lvar(wrp_address)), c_expression(constant=i) ) ),
                                         if_true = wrp_input_assignments )
            wrp_input_code.append(wrp_input_condition)
        wrp_input_guard = c_if( c_expression( ("==", c_expression(lvar=c_lvar(wrp_fsm)), c_expression(constant=1) ) ),
                                if_true = wrp_input_code )
        wrp_input_code_block = c_code_block( sensitivity=[c_clk_edge(wrp_clk,posedge=True)],
                                             code=[wrp_input_guard] )
        self.verilog.add_code_block( wrp_input_code_block )

        wrp_output_code = []
        for i in range(len(output_reg_mappings)):
            wrp_output_assignments = []
            for j in range(len(output_reg_mappings[i])):
                wrp_output_assignment = c_assignment( c_lvar(wrp_output_data,j),
                                                      c_expression(lvar=c_lvar(output_reg_mappings[i][j][0],output_reg_mappings[i][j][1])) )
                wrp_output_assignments.append(wrp_output_assignment)
            wrp_output_condition = c_if( c_expression( ("==", c_expression(lvar=c_lvar(wrp_address)), c_expression(constant=i) ) ),
                                         if_true = wrp_output_assignments )
            wrp_output_code.append(wrp_output_condition)
        wrp_output_code_block = c_code_block( sensitivity=[c_clk_edge(wrp_clk,posedge=True)],
                                             code=wrp_output_code )
        self.verilog.add_code_block( wrp_output_code_block )

#c c_verilog_stmt
class c_verilog_stmt( object ):
    def __init__( self ):
        pass

#c c_clk_edge
class c_clk_edge( object ):
    def __init__( self, clock_signal, posedge=True ):
        self.clock_signal = clock_signal
        self.posedge = posedge
    def text( self ):
        if self.posedge:
            return "posedge %s"%self.clock_signal.name
        return "negedge %s"%self.clock_signal.name

#c c_any_edge
class c_any_edge( object ):
    def __init__( self, signal ):
        self.signal = signal
    def text( self ):
        return "%s"%self.signal.name

#c c_code_block
class c_code_block( object ):
    def __init__( self, sensitivity=None, code=[] ):
        self.sensitivity = sensitivity
        self.code = code
    def output( self, writer, indent="" ):
        if self.sensitivity is None:
            writer("%sinitial"%indent);
        else:
            writer("%salways @(\n"%indent);
            i=0
            for s in self.sensitivity:
                or_text = ""
                if i<len(self.sensitivity)-1:
                    or_text = " or"
                writer("%s%s%s\n"%(indent+"    ",s.text(),or_text));
                i=i+1
            writer("%s)\n"%indent);
        writer("%sbegin\n"%indent);
        for c in self.code:
            c.output( writer, indent+"    " )
        writer("%send\n"%indent);

#c c_case_entry
class c_case_entry( c_verilog_stmt ):
    def __init__( self, expression, code ):
        self.expression = expression
        self.code = code
    def output( self, writer, indent="" ):
        writer("%s%s:\n"%(indent,self.expression.text()))
        writer("%sbegin\n"%indent);
        for c in self.code:
            c.output( writer, indent+"    " )
        writer("%send\n"%indent);

#c c_case
class c_case( c_verilog_stmt ):
    def __init__( self, expression, case_entries ):
        self.expression = expression
        self.case_entries = case_entries
    def output( self, writer, indent="" ):
        writer("%scase (%s)\n"%(indent,self.expression.text()))
        for c in self.case_entries:
            c.output( writer, indent )
        writer("%sendcase //%s\n"%(indent,self.expression.text()))

#c c_if
class c_if( c_verilog_stmt ):
    def __init__( self, expression, if_true=None, if_false=None ):
        self.expression = expression
        self.if_true = if_true
        self.if_false = if_false
    def output( self, writer, indent="" ):
        writer("%sif (%s)\n"%(indent,self.expression.text()))
        if self.if_true:
            writer("%sbegin\n"%indent);
            for c in self.if_true:
                c.output( writer, indent+"    " )
            writer("%send\n"%indent);
        if self.if_false:
            writer("%selse\n"%indent);
            writer("%sbegin\n"%indent);
            for c in self.if_false:
                c.output( writer, indent+"    " )
            writer("%send\n"%indent);

#c c_lvar
class c_lvar( object ):
    def __init__( self, signal, bit=None, width=None ):
        self.signal = signal
        if ((self.signal.width<2) and (bit==0) and (width<2)):
            bit = None
            width = None
        self.bit = bit
        self.width = width
    def text( self ):
        r = self.signal.name
        if self.width is not None:
            return "%s[%d:%d]"%(r,self.width-1+self.bit,self.bit)
        if self.bit is not None:
            return "%s[%d]"%(r,self.bit)
        return r

#c c_expression
class c_expression( object ):
    def __init__( self, expression=None, constant=None, lvar=None ):
        self.lvar = lvar
        self.constant = constant
        self.expression = expression
    def text( self ):
        if self.constant is not None:
            return str(self.constant)
        if self.lvar is not None:
            return self.lvar.text()
        if self.expression[0] in ["!"]:
            return "%s(%s)"%(self.expression[0],self.expression[1].text())
        if self.expression[0] in ["=="]:
            return "((%s)%s(%s))"%(self.expression[1].text(),self.expression[0],self.expression[2].text())
        return "expression"

#c c_assignment
class c_assignment( c_verilog_stmt ):
    def __init__( self, lvar, expression ):
        self.lvar = lvar
        self.expression = expression
    def output( self, writer, indent="" ):
        writer( indent+"%s <= %s;\n" % (self.lvar.text(), self.expression.text()) )

#a Top level
#f main
def main():
    import sys

    w = c_wrapper( "local_scratch" )
    w.add_clock(c_clock_signal("xpb_clock"))
    w.add_clock(c_clock_signal("clk"))
    w.add_clock(c_clock_signal("eb_clk"))

    w.add_input(c_input_signal("cluster_status",width=1+15))
    w.add_input(c_input_signal("target_port",width=1+1))
    w.add_input(c_input_signal("target_island",width=1+5))
    w.add_input(c_input_signal("performance_analyzer_trigger_in"))
    w.add_input(c_input_signal("performance_analyzer_bus__low",width=1+31))
    w.add_input(c_input_signal("performance_analyzer_bus__mid",width=1+31))
    w.add_input(c_input_signal("performance_analyzer_bus__high",width=1+31))
    w.add_input(c_input_signal("assertions_in__enable"))
    w.add_input(c_input_signal("assertions_in__asserts_fired",width=1+31))
    w.add_input(c_input_signal("assertions_in__fsm_busy",width=1+31))
    w.add_input(c_input_signal("timer_control__restart"))
    w.add_input(c_input_signal("timer_control__enable"))
    w.add_input(c_input_signal("event_bus_provider_id__island_id",width=1+5))
    w.add_input(c_input_signal("event_bus_provider_id__sub_id",width=1+1))
    w.add_input(c_input_signal("event_bus_slot_in__empty"))
    w.add_input(c_input_signal("event_bus_slot_in__provider__island_id",width=1+5))
    w.add_input(c_input_signal("event_bus_slot_in__provider__sub_id",width=1+1))
    w.add_input(c_input_signal("event_bus_slot_in__data__source",width=1+11))
    w.add_input(c_input_signal("event_bus_slot_in__data__event",width=1+3))
    w.add_input(c_input_signal("eb_reset_n"))
    w.add_input(c_input_signal("xpb_select_cluster",width=1+7))
    w.add_input(c_input_signal("xpb_from_master__clock_enable"))
    w.add_input(c_input_signal("xpb_from_master__control",width=1+2))
    w.add_input(c_input_signal("xpb_from_master__data",width=1+15))
    w.add_input(c_input_signal("dsf_cpp_push_data_taken_b"))
    w.add_input(c_input_signal("dsf_cpp_push_data_taken_a"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__metadata__valid"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__metadata__destination",width=1+5))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__data_is_pull"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__data_master_or_target_port",width=1+3))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__data_or_target_ref",width=1+13))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__signal_master",width=1+7))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__signal_ref_or_cycle",width=1+6))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__last"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__data",width=1+63))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__data_error",width=1+1))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__data_valid",width=1+1))
    w.add_input(c_input_signal("dsf_cpp_pull_data_b__payload__no_split"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__metadata__valid"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__metadata__destination",width=1+5))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__data_is_pull"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__data_master_or_target_port",width=1+3))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__data_or_target_ref",width=1+13))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__signal_master",width=1+7))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__signal_ref_or_cycle",width=1+6))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__last"))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__data",width=1+63))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__data_error",width=1+1))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__data_valid",width=1+1))
    w.add_input(c_input_signal("dsf_cpp_pull_data_a__payload__no_split"))
    w.add_input(c_input_signal("dsf_cpp_pull_id_taken"))
    w.add_input(c_input_signal("dsf_cpp_command__metadata__valid"))
    w.add_input(c_input_signal("dsf_cpp_command__metadata__destination",width=1+5))
    w.add_input(c_input_signal("dsf_cpp_command__payload__target",width=1+3))
    w.add_input(c_input_signal("dsf_cpp_command__payload__action",width=1+4))
    w.add_input(c_input_signal("dsf_cpp_command__payload__token",width=1+1))
    w.add_input(c_input_signal("dsf_cpp_command__payload__length",width=1+4))
    w.add_input(c_input_signal("dsf_cpp_command__payload__address",width=1+39))
    w.add_input(c_input_signal("dsf_cpp_command__payload__byte_mask",width=1+7))
    w.add_input(c_input_signal("dsf_cpp_command__payload__data_master_island",width=1+5))
    w.add_input(c_input_signal("dsf_cpp_command__payload__data_master",width=1+3))
    w.add_input(c_input_signal("dsf_cpp_command__payload__data_ref",width=1+13))
    w.add_input(c_input_signal("dsf_cpp_command__payload__signal_master",width=1+7))
    w.add_input(c_input_signal("dsf_cpp_command__payload__signal_ref",width=1+6))
    w.add_input(c_input_signal("xpb_hard_reset_n"))
    w.add_input(c_input_signal("xpb_reset_n"))
    w.add_input(c_input_signal("global_reset_n"))

    w.add_output(c_output_signal("performance_analyzer_trigger_out"))
    w.add_output(c_output_signal("ring_full_status", width=1+15))
    w.add_output(c_output_signal("ring_empty_status", width=1+15))
    w.add_output(c_output_signal("event_bus_slot_out__empty"))
    w.add_output(c_output_signal("event_bus_slot_out__provider__island_id", width=1+5))
    w.add_output(c_output_signal("event_bus_slot_out__provider__sub_id", width=1+1))
    w.add_output(c_output_signal("event_bus_slot_out__data__source", width=1+11))
    w.add_output(c_output_signal("event_bus_slot_out__data__event", width=1+3))
    w.add_output(c_output_signal("xpb_to_master__response"))
    w.add_output(c_output_signal("xpb_to_master__data", width=1+15))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__metadata__valid"))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__metadata__destination", width=1+5))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__data_is_pull"))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__data_master_or_target_port", width=1+3))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__data_or_target_ref", width=1+13))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__signal_master", width=1+7))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__signal_ref_or_cycle", width=1+6))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__last"))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__data", width=1+63))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__data_error", width=1+1))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__data_valid", width=1+1))
    w.add_output(c_output_signal("dsf_cpp_push_data_b__payload__no_split"))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__metadata__valid"))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__metadata__destination", width=1+5))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__data_is_pull"))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__data_master_or_target_port", width=1+3))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__data_or_target_ref", width=1+13))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__signal_master", width=1+7))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__signal_ref_or_cycle", width=1+6))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__last"))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__data", width=1+63))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__data_error", width=1+1))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__data_valid", width=1+1))
    w.add_output(c_output_signal("dsf_cpp_push_data_a__payload__no_split"))
    w.add_output(c_output_signal("dsf_cpp_pull_data_taken_b"))
    w.add_output(c_output_signal("dsf_cpp_pull_data_taken_a"))
    w.add_output(c_output_signal("dsf_cpp_pull_id__metadata__valid"))
    w.add_output(c_output_signal("dsf_cpp_pull_id__metadata__destination", width=1+5))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__target_island", width=1+5))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__target_port", width=1+1))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__target_ref", width=1+13))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__data_master", width=1+3))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__data_ref", width=1+13))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__signal_master", width=1+7))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__signal_ref", width=1+6))
    w.add_output(c_output_signal("dsf_cpp_pull_id__payload__length", width=1+4))
    w.add_output(c_output_signal("dsf_cpp_command_taken"))

    w.create_verilog()
    w.output_verilog(sys.stdout.write)

#f Invoke main if called as program
if __name__ == "__main__":
    main()
