<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.waveform" );

$page_title = "Cyclicity Simulation Engine";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title . " - Code coverage" );

page_section( "overview", "Overview" );
?>

Simulation engine modules may supply support for code coverage estimations; any module generated using CDL,
for example, can have code coverage support with appropriate options to the CDL compiler. The simulation
engine supplies support for accessing the code coverage information, and supporting a common access method for it.

<p>

The modules themselves generate the code coverage data, and are responsible for having a mechanism for indicating
the actual correspondence with some code of each element; the simulation engine is not responsible for the data, just
for keeping it in a common format and providing access to it.

</p>

<?php page_section( "exec_file", "Exec file enhancements" ); ?>

The following exec file enhancements are provided for code coverage management:

<dl>
<dt><em>coverage_reset</em></dt>

<dd>Reset any code coverage metrics within the simulation</dd>

<dt><em>coverage_load( string filename, optional string module )</em></dt>

<dd>Load a code coverage file. This can be used for incremental code coverage determination. The top level module that should be handled may be provided; otherwise all modules will have the code coverage loaded.</dd>

<dt><em>coverage_save( string filename, optional string module )</em></dt>

<dd>
Save a code coverage file.
Either all the top level modules, or just the specified module, will have their code coverage saved in the specified file.
</dd>

</dl>

<?php page_section( "file_format", "Code coverage file format" ); ?>

<p>

The code coverage files are textual. They consist of sections, one per module instance, each section of which starts with a line of the form:

</p>

<pre>
&lt;instance type&gt; &lt;instance name&gt; &lt;number of coverage elements&gt;
</pre>

<p>

The sections indicate the code coverage of each module instance: each section heading is then followed by the specified number of lines, each line
containing an integer which should be the number of occurrences of a coverage element.

</p>

<p>

A sample code coverage file may then be:

</p>

<pre>
and and_gate_1 4
 10
 20
 5
 0
and and_gate_2 4
 0
 15
 10
 10
</pre>

<p>

This file may indicate (depending on the coding of the and gate, and the meaning that particular coding supplies to the code coverage
for the simulation engine) that one AND gate has its two inputs exercised as 00 ten times, 01 twenty times, 10 five times, and 11 never. The second
AND gate, then, would have its inputs exercised as 00 never, 01 fifteen times, 10 ten times, and 11 ten times.

<br>
Of course, the actual meaning depends on the code implementing the  AND gate, and particularly how that code calculates its code coverage
that it reports to the simulation engine.

</p>

<?php

include "${toplevel}web_footer.php"; ?>

