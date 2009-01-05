<?php

include "web_globals.php";

$page_title = "Cyclicity CDL";
include "web_header.php";


page_header( "Cyclicity CDL" );

page_sp();
?>
<a href="cdl.php">Cyclicity CDL</a>
 is a cycle description language for hardware design,
specifically for FPGA or silicon design, where a high level language
is beneficial for the design.
<?php
page_ep();

page_sp();
?>
This project contains a free implementation of the CDL language, which is
capable of building C models and synthesizable verilog from a CDL description.
The tools are released under three licenses most suitable to the code: the parser and
code generators are released under the GPL; the simulation engine is released under the
LGPL (in broad terms bascially because this engine is a library); some other support code is released
as free code without any restrictions.

<br>
Nothing is released under a license more restrictive or prescriptive than the GPL.
<?php
page_ep();

page_header( "Latest updates - Sept 2007" );

page_sp();
?>

<ul>

<li>
The simulation engine now supports a Python/gtk gui structure, with glade for its window descriptions.

<li>
    The simulation engine now supports instantiation from within Cadence NC-Verilog with the VPI; the manner of instantiation seems somewhat esoteric, but it is quite functional (that's EDA languages for you, really, Cadence ones especially). Any cycle model that can be use with the cycle simulation can be used with Verilog, a great advance for test bench creation. It is also quite simple to use golden C models to compare hand-crafted Verilog or VHDL models, running them in parallel.

<li>

The support libraries have a new module sl_hier_mem, which supports hierarchical memories for us in simulation where a very large memory space is required but relatively little memory is actually used. A simple hierarchical page table is created dynamically as the memory is read and written (with optional allocate-on-read, obligatory allocate-on-write :-)), and the page table may be enumerated to ease checkpoint and restore. This type wil be added to the simulation engine in due course.

<li>

The simulation engine supports checkpoint and restore without, as yet, the chance for C models to register particular checkpoint and restore functions. The only data checkpointed and restored, currently, is the state declared to the simulation engine.

</ul>

<?php
page_ep();

page_header( "Releases" );

page_sp();
?>
The first ever release (v0.01) of CDL occurred over the weekend of April 24th 2004.
A second release (v0.02) took place on April 26th 2004; more of a drive for a standard build structure meant some changes required
for releases.

This release is quite functional; the CDL language is mostly implemented,
and simulations can be run with VCD waveform generation, batch simulation,
assertions, code coverage evaluation, hierarchical simulations, and so on.

<br>

Current releases are somewhat light on documentation; this work is underway, as and when it is required.
For example, the tar file in the release does not actually include instructions for building it. But, to get
you over that hurdle, <a href="installation.php">those instructions are here</a>

<?php
page_ep();

page_header( "Release History" );

page_sp();
?>

<table border=1 frame="box">
<tr>
<th>
Date
</th>
<th>
Version
</th>
<th>
Comment
</th>
</tr>

<tr>
<td>26th April 2004
</td>
<td><a href="http://prdownloads.sourceforge.net/cyclicity-cdl/cyclicity-0.02.tgz?download">v0.02</a>
</td>
<td>Improved some makefiles, pulled the batch mode execution harness out to give it a free license,
 made the simulation engine a real dynamic library, almost certainly broke cygwin builds in the process,
 added a synchronous SRAM internal module (untested as yet).
</td>
</tr>

<tr>
<td>24th April 2004
</td>
<td><a href="http://prdownloads.sourceforge.net/cyclicity-cdl/cyclicity-0.01.tgz?download">v0.01</a>
</td>
<td>First release
</td>
</tr>

</table>

<?php
page_ep();

page_header( "Documentation" );

page_sp();
?>

There is some documentation (growing) in <a href="documentation/index.php">the documentation</a> directory...

<?php
page_ep();

include "web_footer.php"; ?>

