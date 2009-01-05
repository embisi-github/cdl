<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.support_libraries.sl_hier_mem" );

$page_title = "CDL Support libraries";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "Sept 11th 2007" );
page_header($page_title );

page_section( "overview", "Overview" );
?>

This module defines a set of functions for creating and accessing a hierarchical memory. This is a potentially sparse memory of very large size, which can be read and written at any location and appropriate memory allocated for it at such times. Large unused portions of the memory are not allocated, and so a large address space can be simulated with ease.

<p>

The address space supported is 64-bits. This means that every address supplied to the module is t_sl_uint64.

<?php

page_section( "codedoc", "Header file (including documentation)" );

code_format( "h", "code/sl_hier_mem.h");

page_ep();

include "${toplevel}web_footer.php"; ?>
