<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.waveform" );

$page_title = "Cyclicity Simulation Engine";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title. " - Waveforms" );

?>

Simulation engine modules supply information on their state and internal signals to
the simulation engine. This is done in a standard fashion, and this data can be accessed by the
simulation engine in many ways. For example, it can be used to generate a graphical front-end
for the simulation engine to provide customizable access to data and signals within a module. It
may also be used to generate waveform files, particularly in VCD file format, through the C++ API. This access is also 
provided through simulation engine exec file enhancements, which should are available from the internal 'se_test_harenss' module and the batch simulation exec files.


<p>

The modules themselves supply the data which may be visible to a waveform file; the
simulation engine purely provides access to it.

</p>

<h1>Exec file enhancements</h1>

<p>

The simulation engine supplies the following exec file enhancements to provide access to the VCD waveform files:

</p>

<dl>

<dt>vcd_file_open( string handle, string filename )</dt>

<dd>Open a VCD waveform file with a given handle (for future reference) and a given filename. The filename should refer to a (potential) file that is writable.
</dd>

<dt>vcd_file_close( string handle )</dt>

<dd>Close a VCD waveform file handle; this cleanly finishes the VCD file attached to the handle.
</dd>

<dt>vcd_file_add( string handle, string signal or state, ... )</dt>

<dd>Add a signal or other described state to the specified VCD handle; the signal or state specifiied may be a global signal name, a state or signal within
a module that a module exports, or it may be a module name in which case all the signals and states of that module
are added to the VCD file.</dd>

<dt>vcd_file_enable( string handle )</dt>

<dd>Outputs the header for the specified VCD file, and enables its further output. This should be issued once after all the VCD file signal
additions have been performed, before a simulation is stepped.
</dd>

<dt>integer vcd_file_size( string handle )</dt>
<dd>Returns the size of the specified VCD file</dd>

</dl>

<p>

So a simple example of the use of these commmands would be:

</p>

<pre>
vcd_file_open my_handle "signals.vcd"
vcd_file_add my_handle input_0 input_1
vcd_file_add my_handle output_2
vcd_file_add my_handle module_1
vcd_file_enable my_handle
reset
step 50
vcd_file_close my_handle
</pre>

<?php include "${toplevel}web_footer.php"; ?>
