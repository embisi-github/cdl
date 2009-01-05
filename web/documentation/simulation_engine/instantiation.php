<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.instantiation" );

$page_title = "Cyclicity Simulation Engine";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title . " - Instantiation" );

page_section( "overview", "Overview" );
?>

<p>

The simulation engine supports instantiation of signals and modules inside a simulate engine class from the class API.
In order that C++ programs do not need to be written for each testbench, access is also supplied through a 
particular set of enhancements to the exec_file language, which is accessible through a single method of the class; the standard
execution harnesses utilize this method for their system instantiations.

</p>

<?php page_section( "exec_file", "Exec file enhancements" ); ?>

The instatiation facilities can be accessed through an exec file if the harness invokes the correct method; the standard
harnesses do this. The exec file has the standard features, plus  the basic exec file language is extended with the commands described below. Note that some of
the internal exec file commands below instantiate some <a href="internal_modules/index.php">internal logic gate modules</a>.

<dl>

<dt><em>module( string type, string instance )
</em></dt>
<dd> Instantiate a module of the given type, with the given instance name, and any options that may have been specified since
the list instantiation.</dd>

<dt><em>option_int( string option_name, integer value )</em></dt>
<dd>Add an integer option to the list of options for the next module instance.</dd>

<dt><em>option_string( string option_name, string value )</em></dt>
<dd>Add a string option to the list of options for the next module instance.</dd>

<dt><em>wire( string global_signal, ... )</em></dt>
<dd>Specify a list of global signals. A signal of the form 'fred[20]' specifies a bus of width<em> 20. Many signals may be declared in a single statement.</dd>

<dt>assign( string driven_signal, integer value [, integer until time [, integer after value]])</em></dt>
<dd>Drive a signal with a particular value, up until a certain time (optionally), when it drives to 0 or another value if specified

<dt>drive( string driven_signal, string driver_signal )</em></dt>
<dd>Specify that a particular signal be driven by another; to specify a module input or output use&lt; instance name.signal name&gt;.

<dt><em>clock( string name, integer delay, integer cycles_high, integer cycles_low )</em></dt>
<dd>Specify a clock; it will start low, and go high after the specified delay, then clock high/low with the given cycles_high and cycles_low</dd>

<dt><em>clock_divide( string name, string source clock, integer delay, integer clock ticks high, integer clock ticks low )</em></dt>
<dd>Specify a clock to be derived from another clock; it will start low, and go high after the specified number of source clock ticks, then clock high/low with the given number of source clock ticks</dd>

<dt><em>clock_phase( string signal name, string source clock, integer delay, integer pattern length, string pattern )</em></dt>
<dd>Drive an output signal with a repeated pattern from a source clock, after an initial delay where the signal is low; the pattern length indicates the cycle repeat length, the pattern indicates with any of 'H', 'h', '*' or 'X' when the output signal should be 1; outside the pattern string the signal is driven low</dd>

<dt><em>mux( string bus_name, string select_signal, string input, ... )</em></dt>
<dd>Multiplex one of many inputs to an output, with a given select signal. The select signal should be the correct width<em> given the number of inputs.</dd>

<dt>decode( string output, string input, optional string enable )</em></dt>
<dd>Instantiate an n-to-m decoder; the output bus should  be m bits wide. If an optional enable signal is supplied then the output bits will be zero if the enable is zero, else
one of the output bits will be one.</dd>

<dt><em>extract( string output, string input, integer start bit, integer width )</em></dt>
<dd>Extract a set of bits from a bus</dd>

<dt><em>logic( and|or|xor|nand|nor|xnor, string output, string input_1, ... )</em></dt>
<dd>Instantiate a logic gate of the specified type with a number of inputs.</dd>

<dt><em>cmp( string output signal, string bus, integer value )</em></dt>
<dd>Combinatorial comparator that drives the output signal low if the bus does not have the value, high if it does</dd>

</dl>

<?php

include "${toplevel}web_footer.php"; ?>

