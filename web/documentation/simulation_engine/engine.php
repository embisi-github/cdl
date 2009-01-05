<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.waveform" );

$page_title = "Cyclicity Simulation Engine";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title );

page_section( "execution", "Engine execution" );
?>

The simulation harness is invoked from some simple execution harness; in
particular here the code that is included in se_engine.cpp. This is to be moved 
in the near future to the new directory execution_harnesses/batch.cpp, where it 
can be placed under the GPL and dynamically linked to the simulation engine 
code.

<p>

This batch execution engine uses batch files written in the exec file language.
As such is supports the standard exec file language, with some additional
enhancements described below to support instantiation of individual simulations
with arguments, and their execution.

<?php page_section( "exec_file", "Batch simulation exec file enhancements" ); ?>

Batch simulation exec files support the following commands, and also the  <a href="waveform.php">waveform display</a>
 (or rather, saving),  <a
href="coverage.php">code coverage</a> and <a href="simulation.php">signal access</a>
simulation enhancements

</p>

<p>

<dl>
<dt><em>read_hw_file(string filename)</em></dt>
<dd>discard the current instantiations, and read the given hardware file, which
should be an <a href="instantiation.php">instantiation exec_file</a>
</dd>

<dt><em>
reset
</em></dt>
<dd>
reset the current instantiations
</dd>

<dt><em>
step(integer cycles)
</em></dt>
<dd>
step the given number of cycles
</dd>

<dt><em>
setenv(string name, string value)
</em></dt>
<dd>
set an environment option for the hardware instantation file to read; this is
the mechanism used for passing arguments to the instantiation files
</dd>

<dt><em>
display_state
</em></dt>

<dd>
display the state registered by the instantiations; useful for debugging
</dd>

</dl>

<p>

A simulation batch might then look like:

<pre>
setenv "width" 16
setenv "module_name" hierarchy_test_harness
setenv "module_mif_filename" "hierarchy_test_harness.mif"
read_hw_file vector.hwex
reset
step 50
end
</pre>

<p>

This simulation file sets three arguments for the hardware instantiation, then reads a hardware instantiation batch file.
It can then run a simulation (reset, step), before its work is completed.

<p>

The hardware instantiation file should have suitable test harnesses that produce pass and fail messges.

<p>

Additionally waveforms can be generated from a batch file, using the waveform exec file enhancements, for example like this (replacing the reset and step):

<pre>
vcd_file_open hierarchy hierarchy.vcd
vcd_file_add  hierarchy vector_input_0 vector_input_1 vector_output_0 vector_output_1
reset
vcd_file_enable hierarchy
step 50
vcd_file_close hierarchy
</pre>

</p>

<p>

Furthermore, code coverage may be gathered for a simulation, or a module in the simulation,
using the waveform exec file enhancements, for example like this (again replacing the reset and step):

<pre>
coverage_reset
coverage_load "coverage_file_so_far.cov"
reset
step 50
coverage_save "coverage_file_so_far.cov"
</pre>

This will build on a current code coverage file, so incremental code coverage can be performed

</p>

<?php include "${toplevel}web_footer.php"; ?>
