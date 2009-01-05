<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.registration" );

$page_title = "Cyclicity Simulation Engine";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title . " - Registration" );

page_section( "overview", "Overview" );
?>

A module type registers with the simulation engine through the C++ API. It basically registers its name and an
instantiation callback (with handle), and then when the module type is instantiated the callback function is invoked.
This callback function can then check the options (parameters) which it is instantiated with, and declare its inputs and outputs.

<p>

The full details on this will be available within the API documentation

<p>

However, as an overview:

<ul>

<li>
A module instance should declare all its inputs and outputs.

<li>
An input should be declared as 'used on a clock' if it is used to effect that clock edge. But this is not yet really necessary.

<li>
An output should be declared as 'changing on clock' if it may change due to the clock edge. If in doubt, say it is. This will cause evaluation of combinatorial functions for other modules which use that signal unnecessarily at worst.

<li>
An input should be declared as 'used combinatorially' if it may effect any of the outputs of the module combinatorially. The one caveat here is reset; it should be assumed for asynchronous reset that the reset signal is always removed just after a rising edge of the clock, and asserted just prior to the rising edge of the clock. Therefore it may be treated as a standard clocked input.

<li>
An output should be declared as 'created combinatorially' if it is effected by any input combinatorially.

<li>
A module instance may then only have one combinatorial function; all its combinatorial outputs are deemed valid at the end of execution of that function, and all its combinatorial inputs must be valid before it will be called.

</ul>

<?php page_section( "submodules", "Submodules" );?>

<p>

For submodule instantiations the scheduling and invocation of combinatorial, preclock and clock functions is the province of the main module instantiation functions. The submodules are, of course, registered with the simulation engine, but are instantiated explicitly by their parent as submodules through the main engine.

The parent module is expected to invoke:

<ul>
<li>
submodule_get_handle( submodule )

<li>
submodule_get_clock_handle( submodule, clockname, &handle )

<li>
if (has_edge(handle, posedge/negedge))
call_submodule_clock( handle, posedge/negedge )

<li>
call_submodule_comb( submodule_handle )

<li>
t_sl_error_level drive_submodule_input( submodule_handle, output, int*, size )

<li>
int *get_submodule_output_driver( submodule_handle, output, int**, size )

</ul>

<?php

include "${toplevel}web_footer.php"; ?>

