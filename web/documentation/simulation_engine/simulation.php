<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.simulation" );

$page_title = "Cyclicity Simulation Engine";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title . " - Signal Access" );

page_section( "overview", "Overview" );
?>

The simulation engine basically is a framework for cycle simulation, with the ability to
interconnect multiple module instantiations. Additionally the simulation engine provides access to data and signals within those
module instantiations, and within the interconnect. This access is exposed in some
exec files, and that access is describe here.

</p>

<?php page_section( "exec_file", "Exec file enhancements" ); ?>

<p>

The simulation environment has some exec file additions to support interrogation of state accessible to
the simulation engine, and to support events and other inter-thread communication mechanisms when
state changes. The 'state' may be a global signal or data made availabl by a module instance.


<dl>

<dt><em>global_monitor_display( string global, string format, arguments) </em></dt>
<dd>Set up a monitor of some global state, such that when that state changes
the specified string is displayed. The text displayed comed from a standard
exec file printf-like statement, with format and argments. The arguments are evaluated
<i>at the time the string is generated</i>, not when the monitor is created.
</dd>

<dt><em>global_monitor_eventy( string global, event event_name) </em></dt>
<dd>
Set up a monitor of some global state, such that when that state changes the specified event is fired.
The event may be used to trigger some exec file thread to schedule, for example.
</dd>

<dt><em>integer global_cycle() </em></dt>

<dd>This function returns the current cycle number of the simulation, starting from 0 at reset</dd>

<dt><em>integer global_signal_value( string global_signal )</em></dt>

<dd>This function returns the integer value of a signal at the current simulation time</dd>

<dt><em>string global_signal_value_string( string global_signal )</em></dt>

<dd>This function returns a textual form of the value of a signal at the current simulation time.
This may be the name of an FSM state, or an enumeration, or it may be digits or a hexdecimal
represenation of a bus value; it works for any size signal (even beyond 32 bits wide)
</dd>

</dl>

<?php

include "${toplevel}web_footer.php"; ?>

