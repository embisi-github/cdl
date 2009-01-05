<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.language_specification.discussion" );

$page_title = "Cyclicity CDL Language Specification";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 14th 2004" );
page_header($page_title. " - Discussion" );

?>

<P>This section moves on from the detailed grammar description to
talk in more human terms about the concepts and capabilities of the
elements of the language.</P>

<?php page_section( "types", "Types" ); ?>

<P>This section looks at Cyclicity CDL's types from a more human
perspective.</P>
<P>CDL types are a tool for abstracting from simple bit vectors or
collections of bit vectors to a higher level. They are a basic tenet
of all modern languages, and certainly an assist even for hardware
description languages; VHDL and SystemVerilog support them, for
example.</P>
<P>The types in CDL have a range of abilities and restrictions:</P>
<OL>
    <LI><P>CDL's types are defined to be fully known at compile-time, to
    allow for compile-time type checking.</P>
    <LI><P>CDL's types are basically driven from some very simple basic
    types, and CDL does not support the plethora of types that, for
    example, SystemVerilog does; CDL's basic types are focussed on
    producing working hardware descriptions, not on testbench code.</P>
    <LI><P>CDL supports structures, which are collections of other
    types. These allow, for example, a single variable to contain a set
    of flags and data, without requiring individual variables for each.</P>
    <LI><P>CDL supports enumerations; these 
    </P>
</OL>

<?php include "${toplevel}web_footer.php"; ?>
