<?php

$toplevel = "../../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.simulation_engine.internal_modules" );

$page_title = "Cyclicity Simulation Engine Internal Modules";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 20th 2004" );
page_header($page_title . "" );

page_section( "overview", "Overview" );
?>

<p>

The simulation engine comes with some internal modules that may be instantiated. They are automatically registered, and require
no extra external C code to mainpulate; indeed, a simulation may be created just by using the batch simulation script engine with no
additional modules.

</p>

<p>

The internal modules are:

<table border=1 frame="box">
<tr>
<th><a href="se_test_harness.php">se_test_harness</a></th>
<td>Complex test harness module with run-time script to describe its behaviour</td>
</tr>

<tr>
<th>se_sram_srw</th>
<td>Synchronous SRAM with single port</td>
</tr>

</table>

</p>

<?php

include "${toplevel}web_footer.php"; ?>

