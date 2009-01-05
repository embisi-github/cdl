<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine" );

$page_title = "Cyclicity CDL Documentation";
include "${toplevel}/web_header.php";


page_header( "Cyclicity Simulation Engine" );

page_sp();
?>

<p>
The simulation engine is a cycle simulation framework that supports
instantiation of cycle-based modules, their interconnection, and specification
of clocks for the simulation. Furthermore, it supports access to state and signal values,
through simulation files and to waveform files, and it also supports code coverage
and other enhancements to simulations.

</p>

<p>

The simulation engine is instantiated as a C++ class; that class instance may then be
used to build a simulation which may be interrogated at will. A sample harness is supplied
as a batch simulation engine.

</p>

<?php page_header( "License" ); ?>

<p>

The simulation engine is supplied as a library under the LGPL version 2.0.

</p>

<p>

The LGPL is used as it is less restrictive of user code than the GPL. It is expected
that proprietary code will need to be linked to the simulation engine as modules for instantiation.
If the simulation engine were released under the more prescriptive GPL then the source code for those modules would have to be made
available under the GPL also; for many this potential users this would be an insurmountable roadblock.
using the LGPL means that the simulation engine may be dynamically linked proprietary modules without
placing any onus on the modules code.

</p>

<p>

Most other elements of the Cyclicity CDL suite are released under the GPL. The different licensing is
vital, and must be maintained in the future.

</p>

<?php page_header( "Features" ); ?>

<p>

The simulation engine is, crudely, a framework for cycle simulations. Global
clocks are declared to the engine, with appropriate phase and cycle time
information; modules are instantiated; their outputs are connected to module
inputs; then the engine basically supports running a simulation, causing
module functions to be invoked based on the wiring of modules and the
specified clocks.

</p>

<p>

So in basic terms the simulation engine is a simple cycle simulation engine.

</p>

<p>

However, the simulation engine has more features than just the absolute basics.
Firstly, the above functions of the simulation engine functions can be invoked from within an
exec file (which is a scripting language supplied by the support libraries). Secondly, modules
are registered to the simulation engine to allow them to be instantiated. Thirdly, at instantiation modules
may declare to the simulation engine their inputs, outputs, states, code coverage details, and appropriate clocking
and combinatorial callback functions, and it also supports hierarchical module instantiation. Fourthly, the simulation engine statically determines
a schedule for calling instantiated modules callback functions, based on the phasing and cycle times of the clocks. Finally,
the simulation engine has support for waveforms, coverage, and such like, through standard forms, within the module
instantiation process.

</p>

<p>

Further to all this, the simulation engine includes some internal modules, which are also documented here; the most important of which is an arbitrary
test harness module, which utilizes a run time script to define its behaviour.

</p>

<p>

These feature sets are discussed in more detail in other places:

<table border=1 frame="box">
<tr>
<th><a href="engine.php">Engine</a></th>
<td>An overview of the simulation engine</td>
</tr>

<tr>
<th><a href="instantiation.php">Instantiation</a></th>
<td>Details on module instantiation, clocks, and wiring</td>
</tr>

<tr>
<th><a href="coverage.php">Coverage</a></th>
<td>Details on code coverage support in the simulation engine</td>
</tr>

<tr>
<th><a href="simulation.php">Simulation</a></th>
<td>Details on simulation setup and execution</td>
</tr>

<tr>
<th><a href="waveform.php">Waveforms</a></th>
<td>Details on how VCD waveform files can be generated</td>
</tr>

<tr>
<th><a href="internal_modules/index.php">Internal modules</a></th>
<td>Full documentation of the simulation cycle modules included in the engine, including a test harness and memories</td>
</tr>

</table>

</p>

<?php page_header( "API" ); ?>

<?php page_header( "Documentation" ); ?>

<?php
page_ep();

include "${toplevel}web_footer.php"; ?>

