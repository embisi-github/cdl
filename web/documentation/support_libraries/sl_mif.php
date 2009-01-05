<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.support_libraries.sl_mif" );

$page_title = "CDL Support libraries";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "Sept 12th 2007" );
page_header($page_title );

page_section( "overview", "Overview" );
?>

This module defines some functions for reading MIF (memory interchange format) files, particularly for simulation support.

<p>

Some simple functions are supplied for a 32-bit address space with up
to 32-bit data, but a full 64-bit address space is supported through sl_read_mif

<?php

page_section( "codedoc", "Header file (including documentation)" );

code_format( "h", "code/sl_mif.h");

page_ep();

include "${toplevel}web_footer.php"; ?>
