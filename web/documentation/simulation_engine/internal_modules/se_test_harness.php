<?php

$toplevel = "../../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.internal_modules.se_test_harness" );

$page_title = "Cyclicity Simulation Engine Internal Modules";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title . " - se_test_harness" );

page_section( "overview", "Overview" );
?>

The test harness is a module with arbitrary inputs and outputs and arbitrary functionality that can be instantiated as other modules. It is,
in fact, just a predefined module meeting the standard API as any other C++ module does.

<p>

The inputs and outputs are specified at instantiation time by string options to the module:
the option string 'inputs' gives a string list of the inputs for the module;
the option string 'outputs' gives a string list of the outputs for the module.

<p>

The module is always clocked, and has a single clock. The name of this is specified with a string option 'clock' at instantiation time.

<p>

The functionality of the module comes from an 'exec_file' whose name is passed in to the instantiation with the string option 'filename'. This exec_file
supports the standard functions, plus those described in the section below.

<p>

All the options (the above named ones and any others given to the instance) are available inside the exec_file, so additional arguments may be passed to
the functionality section of the module by using these options. For example, many of the regression tests pass a string to 
their test harness that contains the name of the particular test to run, and that test data is gathered from a memory which can be preloaded with
a MIF file derived from that test name.

<?php page_section( "exec_file", "Exec file enhancements" ); ?>

The test harness when instantiated executes an exec file, which has the standard features plus the enhancements here. It also
has access to the <a href="../waveform.php">waveform</a>, <a
href="../coverage.php">code coverage</a> and <a href="../simulation.php">signal access</a>

<p>
The exec_file enhancements it itself supplies basically are a small set of commands to control its inputs and outputs, and to do time control.

<dl>
<dt><em>reset( string output signal name, integer value )</em></dt>
<dd>
Immediately set an output to a value; does not wait for the next clock edge. This should be used for reset signals, for example. Most everything else should be clocked (i.e use the drive command).
<dd>
</dd>

<dt><em>drive( string output signal name, integer value )</em></dt>
<dd>
On the next clock edge set the output signal name to the given value
</dd>

<dt><em>wait( integer number of cycles )</em></dt>
<dd>Wait the given number of cycles
</dd>

<dt><em>wait_until_value( string input name, integer value )</em></dt>
<dd>
Wait until the input signal hits the given value; this signal is tested just prior to every clock edge.
</dd>

<dt><em>wait_until_change( string input name )</em></dt>

<dd>
Wait until the input signal changes; this signal is tested just prior to every clock edge.
</dd>

<dt><em>integer input( string input name )</em></dt>
<dd>Get the current integer value of an input
</dd>
</dl>

<?php include "${toplevel}web_footer.php"; ?>

