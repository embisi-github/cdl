<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.language_specification.details" );

$page_title = "Cyclicity CDL Language Specification";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 14th 2004" );
page_header($page_title. " - Details" );

?>

<P>This section contains more focussed points on particular aspects
of the language, their semantics, and their purpose and use.</P>

<?php page_section( "types", "Types" ); ?>

<P>This section discusses issues to do with types in the language</P>

<?php page_subsection( "supported_types", "Supported types" ); ?>

<P>The language supports some basic types, and then supports derived
types also. The basic types are described in the next section; the
derived types are examined here in overview.</P>
<P>The derived types may be one of:</P>
<OL>
    <LI><P>an enumeration of a sized bit vector</P>
    <LI><P>a finite state machine (with defined encoding, or unspecified
    encoding, or specified as one hot, one cold)</P>
    <LI><P>a structure consisting of one or more named elements, each of
    which can be any defined type</P>
    <LI><P>an array of types</P>
    <LI><P>a type reference</P>
</OL>
<P>Enumerations require a size of bit vector to be specified, and
then items of that enumeration will all have that bit vector size.</P>
<P>Finite state machine types are again sized bit vectors, basically.
They are defined as a set of states, whose assignments to bit vector
values may be given explicitly. Alternatively they may be assigned
bit vector values automatically, and this may be done in a simple
incrementing fashion, or one hot or one cold. Furthermore, each state
can have documentation with it, which should be used to document the
design of the state machine, and each state may also define potential
'predecessor' states and 'successor' states, which the language
implementers may check statically or dynamically for correctness.</P>
<P>Structures are very similar to C structures. They contain a number
of elements, each of which is assigned a name, and each of which may
be of any defined type. Structures are very important inside CDL;
they are a very good mechanism for writing code that is clear in
intent.</P>
<P>Arrays of types are supported as in C, with one vital exception.
Multidimensional arrays are not supported. The basic type of the bit
vector is not an array as such, and so arrays of bit vectors are
supported. But no array may resolve down to an array of array of bit
vectors.</P>
<P>Type references are transparent type references provided for code
clarity only; they are not tightly defining. For example, one could
define 'short' to be a bit vector of size 16, and declare a
combinatorial variable to be of type 'short'. This will actually make
that variable a bit vector of size 16 internally; the type reference
is just transparent.</P>

<?php page_subsection( "basic_types", "Basic types: sized and unsized bit vectors and strings" ); ?>

<P>The language supports three basic types for expressions:</P>
<OL>
    <LI><P>Sized bit vectors; these are effectively arrays of bits of
    length 1 upwards.</P>
    <LI><P>Unsized bit vectors; these are arrays of bits of a
    compile-defined maximum length, which can be implicity converted to
    any size of sized bit vector.</P>
    <LI><P>Strings.</P>
</OL>
<P>Strings are hardly used as yet.</P>
<P>The fundamental difference between unsized and sized bit vectors
is in casting: sized bit vectors cannot be implicitly cast to be a
bit vector of another size, that cast would have to be done
explicitly by bundling or bit extraction; unsized bit vectors may be
implicitly cast to a bit vector of any size.</P>
<P>As an explicit example: 5+8, 3+4h2, and 6b110011+6h0f are valid
expressions, as the types all match or may be implicitly cast. But
4h2+3h5 is not a valid expression, as the types of the two operands
are both sized bit vectors, but of different size.</P>

<?php page_subsection( "enumerations", "Enumerations" ); ?>

<P>Enumerations are a type, and unlike in C they are <I>only</I><SPAN STYLE="font-style: normal">
available as a type. This means that a signal may be given an
enumeration type, and it may then only be assigned values within that
enumeration type.</SPAN></P>
<P STYLE="font-style: normal">As an example, if there is an
enumeration:</P>

<pre class=fixed>
typedef enum [2] { one=1, two=2 } small;
</pre>

<P>And if there is a combinatorial variable:</P>

<pre class=fixed>
comb small my_small;
</pre>

<P>Then the only two valid assignments to <I>my_small</I>
are <I>one</I> and <I>two</I></P>

<pre class=fixed>
my_small = one; <I>or</I>
my_small = two;</P>
</pre>

<P>To implement this the language imposes
type scoping on every expression; see the discussion of expressions
below.</P>

<?php page_section( "expressions", "Expressions" ); ?>

<P>
This section discusses issues to do
with expressions in the language
</P>

<?php page_subsection( "casting", "Casting" ); ?>

<P STYLE="font-style: normal">Implicit casting is performed to
convert the type of an expression to that requried by its context.
Each expression in the language has a type context which can be
determined from the position of the expression in the code.</P>
<P STYLE="font-style: normal">Explicit casting is not currently
possible; it will be, when a syntax for that has been derived.</P>

<?php page_section( "lvars", "Lvars" ); ?>

<P STYLE="font-style: normal">This section discusses issues to do
with lvars in the language</P>

<?php page_subsection( "lvars_subscripting", "Subscripting" ); ?>

<P>When a bit vector is subscripted in Cyclicity CDL it must produce
another bit vector of a compile-time size. This means that when
subscripting is used to generate a sized bit vector, the size must be
a compile time constant. So the following is valid:</P>

<pre class=fixed>
foo &lt;= bar[ 5; jim ]; // This is valid
</pre>

<P>whereas the following is not</P>

<pre class=fixed>
comb bit[2] joe;
foo &lt;= bar[ joe; 0 ]; // This is NEVER valid
</pre>

<?php page_subsection( "lvar_bundles", "Bundles" ); ?>

<P>Unlike in Verilog, where a bundle may
be used as an lvar, lvars in Cyclicity CDL cannot be bundles. Where a
bundle would be used in Verilog an additional combinatorial value may
be required. For example, in verilog:</P>

<pre class=fixed>
reg carry, result[30], a[31], b[31];
{carry, result} = a+b;
</pre>

<P>would require something like:</P>

<pre class=fixed>
comb bit[31] a_b_sum;
a_b_sum = a+b;
carry   = a_b_sum[30];
result  = a_b_sum[30;0];
</pre>

<P>This makes the language simpler to
interpret from a compiler perspective, but it can also improve the
comprehensibility of code; in the CDL code it is much easier to see
that the signals are of the correct width, and what the code is
actually doing.</P>

<?php include "${toplevel}web_footer.php"; ?>
