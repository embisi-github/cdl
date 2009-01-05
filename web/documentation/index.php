<?php

$toplevel = "../";
include "${toplevel}/web_globals.php";
site_set_location( "doc" );
$page_title = "Cyclicity CDL Documentation";
include "${toplevel}/web_header.php";


page_header( "Cyclicity CDL Documentation" );

page_sp();
?>

These documentation pages cover the Cyclicity CDL project, the CDL language, the compiler (frontends and backends), the support library, the simulation engine, and the execution harnesses.

<p>

Initially the documentation is a little sparse. It will fill out over time

<p>

<dl>
<dt><a href="cdl/language_specification.php">Cyclicity CDL specification</a>
</dt>
<dd>
    CDL is the cycle description language that the Cyclicity suite utilizes as its top level design language. It is a high level design language for hardware description, with an emphasis on enforcing good design practice, readability, and verifiability. The languages is specified here. The CDL compiler has backends for C models for use in the simulation engine and for Verilog RTL, for synthesis with FPGA or ASIC tools or to run with Verilog simulators. There are a few known limitations of the current compiler (particularly the C model backend does not support arithmetic on buses wider than 32 bits, and reset values of arrays of types is under reexamination) but it is fine for most any use.
<p>
    The unspecified/unimplemented feature list includes time-based assertions, which are a very good verification vehicle, and multithreaded operation. This work is underway.
</dd>
</dl>


<dl>
<dt><a href="support_libraries/index.php">Support library</a>
</dt>
<dd>

The support library is released entirely free of any GPL or LGPL constraints. It has a number of very useful modules that are used throughout C models and the simulation engine.
</dd>
</dl>


<dl>
<dt><a href="simulation_engine/index.php">Simulation engine</a>
</dt>
<dd>
The simulation engine is a cycle-based simulation engine for C++ models. It can be coupled to a number of different harnesses, for batch simulation or GUI simulation. It links dynamically to C++ models of hardware modules, which may be instantiated by script files or by other harnesses. It simulates such instantiations of modules, and provides mechanisms for accessing their state, code coverage estimates, error and informational messages, and so on. It will in the future support checkpointing and restarting, such that simulations may be run through millions of cycles and then halted on error; the state, say, ten thousand cycles previously can have been checkpointed, and simulations restarted from that point to save much simulation time.
</dd>
</dl>

<?php
page_ep();

include "${toplevel}web_footer.php"; ?>

