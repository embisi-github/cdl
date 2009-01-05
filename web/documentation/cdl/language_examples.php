<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.language_specification.examples" );

$page_title = "Cyclicity CDL Language Specification";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 14th 2004" );
page_header($page_title. " - Language Examples" );

?>

<P>Some example designs are included here,
with notes on their use.</P>

<?php page_section( "simple_examples", "Simple examples" ); ?>

<?php page_subsection( "simple_register", "Simple register" ); ?>

<P>The following code is a simple D flip-flop, with and active high
reset, and reset low value.</P>

<?php code_format( "cdl", "examples/register.cdl"); ?>

<?php page_subsection( "simple_adder", "Simple adder" ); ?>

<P>The following code is a simple arbitrary width adder; the width is
given by a constant.
</P>

<?php code_format( "cdl", "examples/adder.cdl"); ?>

<?php page_subsection( "simple_clocked_adder", "Simple clocked adder" ); ?>

<P>This code uses the simple adder module above, and registers the
result. 
</P>

<?php code_format( "cdl", "examples/clocked_adder.cdl"); ?>

<?php page_section( "cpu_examples", "CPU code examples" ); ?>

<P>This section includes some potential implementations for a CPU
core.</P>
<P>These are more realistic coding examples than those described
previously, and they are included to show the real nature of the
language and the readability that it supplies.</P>
<P>By convention types are all prefixed 't_', constants 'c_'. This is
again for readability; it is not required by the Cyclicity CDL
language.</P>
<br>
<?php page_subsection( "cpu_examples_headers", "Headers" ); ?>

<P>A few header files are required to define the types and such for
the core.</P>

<?php code_format( "cdl", "examples/cpu_headers.cdl"); ?>

<?php page_subsection( "cpu_examples_alu", "ALU" ); ?>

<P>This section supplies an ALU that
performs the operations required by an ALU in a simple CPU core. It
consists of a barrel shifter, an adder with preconditioning logic,
and a result multiplexor with additional logic for the ALU logical
operations.</P>

<?php code_format( "cdl", "examples/cpu_alu.cdl"); ?>

<?php include "${toplevel}web_footer.php"; ?>

