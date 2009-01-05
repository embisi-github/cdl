<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.language_specification.grammar" );

$page_title = "Cyclicity CDL Language Specification";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 14th 2004" );
page_header($page_title. " - Grammar" );

?>

<?php
function grammar_start( $item )
{
    global $grammar_pos;
    $tag = $item;
    page_subsection( $tag, $item );
    echo "<table><tr><th class=fixed valign=top>$item</th><th valign=top>:=</th>\n";
    $grammar_pos = 1;
}

function grammar_end()
{
    echo "</table>\n";
}

function grammar_item( $text )
{
    global $grammar_pos;
    if ($grammar_pos==2)
    {
        echo "<tr><td></td><th valign=top>|</th>";
    }
    echo "<td class=fixed>$text</td></tr>\n";
    $grammar_pos = 2;
}

function grammar_note( $text )
{
    echo "<p class=footnote>$text</p>\n";
}

?>

<P>CDL is a language that, at the top level, specifies four things:</P>
<OL>
    <LI><P>Type definitions</P>
    <LI><P>Constant declarations</P>
    <LI><P>Module prototypes and timing specifications</P>
    <LI><P>Module definitions</P>
</OL>

<P>The lexical order of the first two of these in a file is
important; constant evaluations and type definitions are performed in
lexical order, so that a type that utilizes a constant value as a bit
width, for example, must not precede the declaration of that constant
value.</P>

<P>However, module definitions may refer to modules that have not
been defined or prototyped; there is no lexical order requirement
between module definitions and prototypes.</P>

<?php page_section( "scoping", "Scoping" ); ?>

<P>Constants have their own scope. Enumerations are explicitly typed,
and so are within their own type&apos;s scope.</P>

<?php

page_section( "building_blocks", "Grammatical building blocks" );

grammar_start("user symbol list");
grammar_item("&lt;user symbol&gt; [ ',' [ &lt;user symbol list&gt; ] ]");
grammar_end();

grammar_start("type specifier");
grammar_item("&lt;user symbol&gt;");
grammar_item("&lt;base type&gt;");
grammar_item("&lt;type specifier&gt; '['&lt;expression&gt; ']'");
grammar_end();

grammar_note("A type specifier of a bit is essentially a bit vector of size 1.");
grammar_note("Strings must not be used in type specifiers; they are for parameter passing only, and their use is not well defined at present.");
grammar_note("A type specifier of an array of bit vectors of size 1 is defined to be a bit vector of the specified size, but arrays of bit vectors larger than 1.");

grammar_start("base type");
grammar_item("signed bit");
grammar_item("unsigned bit");
grammar_item("bit");
grammar_item("integer");
grammar_item("string");
grammar_end();

grammar_start("lvar");
grammar_item("&lt;user symbol&gt;");
grammar_item("&lt;lvar&gt; '.' &lt;user symbol&gt;");
grammar_item("&lt;lvar&gt; '[' &lt;expression&gt;']'");
grammar_item("&lt;lvar&gt; '[' &lt;expression&gt; ';' &lt;expression&gt; ']' ");
grammar_end();

grammar_note("Lvars specify use of signals or state, or a subset thereof.");
grammar_note("A double subscript seperated with ';' indicates subscripting a bit
vector with a length (the expression prior to the ';') and the lowest
bit number (the expression after the ';'). This supports build-time
type checking, where the length must be a constant to define the type
of the lvar.");

grammar_start("expression");
grammar_item("&lt;integer&gt;");
grammar_item("&lt;lvar&gt;");
grammar_item("sizeof '(' &lt;expression&gt; ')'");
grammar_item("bundle '(' &lt;expression list&gt; ')'");
grammar_item("&lt;expression&gt; '?' &lt;expression&gt;':' &lt;expression&gt;");
grammar_item("&lt;expression&gt; ( '+' | '-' | '*' | '/' | '%' ) &lt;expression&gt;");
grammar_item("&lt;expression&gt; ( '&amp;' | '|' | '^' | '&amp;&amp;' | '||' | '^^' ) &lt;expression&gt;");
grammar_item("&lt;expression&gt; ( '&lt;' | '&lt;=' | '&gt;' | '&gt;=' | '==' | '!=' ) &lt;expression&gt;");
grammar_item("&lt;expression&gt; ( '&lt;&lt;' | '&gt;&gt;' ) &lt;expression&gt;");
grammar_item("( '!' | '-' | '~' ) &lt;expression&gt;");
grammar_item("'(' &lt;expression&gt; ')'");
grammar_end();

grammar_note("Expression operator precedence is as in 'C', with the addition of the '^^' operator which has the same precedence as '||'.");


grammar_start("expression list");
grammar_item("&lt;expression&gt; [ ',' [ &lt;expression list&gt; ] ]");
grammar_end();

page_section( "types", "Types and constants" );

grammar_start("type definition");
grammar_item("typedef &lt;user symbol&gt; &lt;user symbol&gt; ';'");
grammar_item("typedef enum '[' [ &lt;expression&gt; ] ']' '{' &lt;enumerations&gt; '}' &lt;user symbol&gt; ';'");
grammar_item("typedef struct '{' &lt;structure element&gt;* '}' &lt;user symbol&gt; ';'");
grammar_item("typedef [ one_hot | one_cold ] fsm '{' &lt;fsm state&gt;* '}'  &lt;user symbol&gt; ';'");
grammar_end();

grammar_start("enumerations");
grammar_item("&lt;user symbol&gt; [ '=' &lt;expression&gt; ] [ ',' [ &lt;enumerations&gt; ] ]");
grammar_end();

grammar_start("structure element");
grammar_item("&lt;type specifier&gt; &lt;user symbol&gt; [ &lt;documentation string&gt; ] ';'");
grammar_end();

grammar_start("fsm state");
grammar_item("&lt;user symbol&gt; [ '{' &lt;user symbol list&gt; '}' [ '{' &lt;user symbol list&gt; '}' ] ] <br>&nbsp;&nbsp; [ '=' &lt;expression&gt; ] [&lt;documentation string&gt; ] ';'");
grammar_end();

grammar_note("A type reference should only refer to a previously defined type.");
grammar_note("A type enumeration is given a bit width, and can specify precise values for none, some or all of the enumerated elements; if the first is not specified, it is given the value 0; if others are not specified they take the previous value, and add 1.");
grammar_note("Enumerated element names should not be duplicated within the enumerated type; they may duplicate other names, as they will be resolved from context.");
grammar_note("A type structure should not duplicate element names.");
grammar_note("An FSM that is not explicitly identified as one_hot or one_cold will be enumerated. FSM states may specify a set of preceding states (the first symbol list) and also a set of succeeding states (the second symbol list). If any FSM states are assigned values then <I>all</I> states must be assigned values.");
grammar_note("Documentation strings should be utilized; the documentation will be available to GUI tools which interpret the code, or which utilize output from CDL tools.");

grammar_start("constant declaration");
grammar_item("constant &lt;type specifier&gt; &lt;user symbol&gt; = &lt;expression&gt; <br>&nbsp;&nbsp; [ &lt;optional documentation&gt; ] ';'");
grammar_end();

grammar_note("A constant may only be of a bit vector type, sized or unsized; they may not be structures.");

page_section( "module_prototypes", "Module prototypes and timing specifications" );

?>

<P>Module prototypes provide a mechanism for declaring a module&apos;s
ports and the timing of those ports. The module may also be defined
later in the file. A module prototype may also contain details of how
the module may be drawn in a schematic, and its documentation should
describe the overal functionality of the module.</P>

<?php

grammar_start("module prototype");
grammar_item("extern module &lt;user symbol&gt; '(' &lt;port list&gt; ')' [ &lt;documentation&gt; ] '{' &lt;prototype body&gt;* '}'");
grammar_end();

grammar_start("port list");
grammar_item("&lt;port&gt; [ ',' [ &lt;port list&gt; ] ]");
grammar_end();

grammar_start("port");
grammar_item("clock &lt;user symbol&gt; [ &lt;documentation&gt; ]");
grammar_item("( input | output ) &lt;type specifier&gt; &lt;user symbol&gt; [ &lt;documentation&gt; ]");
grammar_item("parameter &lt;type specifier&gt; &lt;user symbol&gt; [ &lt;documentation&gt; ]");
grammar_end();

grammar_start("prototype body");
grammar_item("&lt;timing specification&gt;");
grammar_end("");

grammar_start("timing specification");
grammar_item("timing ( to | from ) [ rising | falling ] &lt;user symbol&gt; &lt;user symbol list&gt; [ &lt;documentation&gt; ] ';'");
grammar_item("timing comb ( input | output ) &lt;user symbol list&gt; [ &lt;documentation&gt; ] ';'");
grammar_end();

echo "<P><i>Schematic symbol specification is not yet defined in the language.</i></P>";

grammar_note("Actual values for timing specification are not yet defined in the language.");
grammar_note("The language does force limitations on module prototypes combinatorial timing; it will be difficult to specify two different times for paths between combinatorial pins. However, for now this is sufficient, and it may enforce good design practice (clocked logic is easier to specify, comprehend, and build, so combinatorial through paths are in general deprecated in modern design practice).");

?>

<P>All types must be determinable at build time, currently. This
means parameters <I>must not</I> be
used to specify the widths of bit vectors, for example; this would
require run-time checks. (Er, not quite true, but parameters in types
are certainly tricky at the moment... but there are workarounds with
constants and module naming)</P>

<?php page_section( "module_definitions", "Module definitions" ); ?>

<P>Module definitions define the contents of a module. This may
include a schematic of the module itself, which is relatable to the
code inside the module due to the structuring of the CDL language;
however, schematics are not in any sense required.</P>

<P>The documentation of the module should describe the design details
of the module; if figures are required for such documentation, they
should be referenced from here.</P>

<?php

grammar_start("module definition");
grammar_item("module &lt;user symbol&gt; '(' &lt;port list&gt; ')' [ &lt;documentation&gt; ] '{' &lt;module body&gt;* '}'");
grammar_end();

grammar_start("module body");
grammar_item("&lt;clock definition&gt;");
grammar_item("&lt;reset definition&gt;");
grammar_item("&lt;clocked variable&gt;");
grammar_item("&lt;comb variable&gt;");
grammar_item("&lt;net variable&gt;");
grammar_item("&lt;labelled code&gt;");
grammar_end();

grammar_start("clock definition");
grammar_item("default &lt;clock specification&gt; [ &lt;documentation&gt; ] ';'");
grammar_end();

grammar_start("reset definition");
grammar_item("default &lt;reset specification&gt; [ &lt;documentation&gt; ] ';'");
grammar_end();

grammar_start("clocked variable");
grammar_item("clocked [ &lt;clock specification&gt; ] [ &lt;reset specification&gt; ] &lt;type specifier&gt; &lt;user symbol&gt; '=' &lt;nested assignment list&gt; [ &lt;documentation&gt; ] ';'");
grammar_end();

grammar_start("comb variable");
grammar_item("comb &lt;type specifier&gt; &lt;user symbol&gt; [ '=' &lt;nested assignment list&gt; ] [ &lt;documentation&gt; ] ';'");
grammar_end();

grammar_start("net variable");
grammar_item("net &lt;type specifier&gt; &lt;user symbol&gt; [ &lt;documentation&gt; ] ';'");
grammar_end();

grammar_start("clock specification");
grammar_item("clock [ rising | falling ] &lt;user symbol&gt;");
grammar_end();

grammar_start("reset specification");
grammar_item("reset [ active_low | active_high ] &lt;user symbol&gt;");
grammar_end();

grammar_start("labelled code");
grammar_item("&lt;user symbol&gt; [ &lt;documentation&gt; ] ':' '{' &lt;statement&gt;* '}'");
grammar_end();

grammar_note("Statements are defined below.");
grammar_note("Schematic specification is not yet defined in the language.");
grammar_note("Clocked variables take the last (lexically) default reset and/or default clock specification, if they are not provided one in the definition.");
grammar_note("Clocked variables must have reset values specified for their whole state.");
grammar_note("Clocked variables are assigned to in labelled code segments using clocked assignment statements.");
grammar_note("Combinatorial variables may have be fully defined on their declaration line, or defined fully within labelled code segments; they may not be partially defined on their declaration line (as this leads to obfuscation).");
grammar_note("Combinatorial varaibles are assigned to in labelled code segments using combinatorial assignment statements. They must be assigned to in all branches of a labelled code segment that assigns to them at all; they may not be assigned to in more than one labelled code segment. (Is this true for structures, where multiple elements may be assigned in different code segments?)");
grammar_note("Net variables must be driven by the outputs of module instantiations; they cannot be assigned to by assignment statements. Each bit must be driven by precisely one module output.");

grammar_start("statement");
grammar_item("&lt;for statement&gt;");
grammar_item("&lt;assignment statement&gt;");
grammar_item("&lt;if statement&gt;");
grammar_item("&lt;switch statement&gt;");
grammar_item("&lt;print statement&gt;");
grammar_item("&lt;assertion statement&gt;");
grammar_item("&lt;sequence definition&gt;");
grammar_item("&lt;instantiation&gt;");
grammar_end();

grammar_start("for statement");
grammar_item("for '(' &lt;user symbol&gt; ';' &lt;expression&gt; ')' [ '(' &lt;expression&gt; ';' &lt;expression&gt; ';' &lt;expression&gt; ')' ] '{' &lt;statement&gt;* '}'");
grammar_end();

grammar_start("assignment statement");
grammar_item("&lt;lvar&gt; ( '=' | '&lt;=' ) &lt;nested assignment&gt; ';'");
grammar_end();

grammar_start("if statement");
grammar_item("if '(' &lt;expression&gt; ')' '{' &lt;statement&gt;* '}' ( elsif '(' &lt;expression&gt; ')' '{' &lt;statement&gt;* '}' )* [ else '{' &lt;statement&gt;* '}' ]");
grammar_end();

grammar_start("switch statement");
grammar_item("( full_switch | part_switch | priority ) '(' &lt;expression&gt; ')' '{' &lt;case entries&gt; '}'");
grammar_end();

grammar_start("case entries");
grammar_item("( default | (case &lt;expression list&gt; ) ) ':' [ '{' &lt;statement&gt;* '}' ]");
grammar_end();

grammar_start("print statement");
grammar_item("print [&lt;clock specification&gt;] [&lt;reset specification&gt;] '(' &lt;string&gt; ')' ';'");
grammar_end();

grammar_start("assert statement");
grammar_item("assert [&lt;clock specification&gt;] [&lt;reset specification&gt;] '(' &lt;expression&gt; ',' &lt;string&gt; ')' ';'");
grammar_item("assert [&lt;clock specification&gt;] [&lt;reset specification&gt;] '(' &lt;expression&gt; ',' &lt;user symbol&gt; ',' &lt;string&gt; ')' ';'");
grammar_end();

grammar_start("sequence definition");
grammar_item("sequence '{' &lt;sequence item&gt;* '}' &lt;user symbol&gt; ';'");
grammar_end();

grammar_start("sequence item");
grammar_item("delay &lt;unsized integer&gt; [ to &lt;unsized integer&gt; ]");
grammar_item("&lt;sequence expression&gt;");
grammar_end();

grammar_start("sequence expression");
grammar_item("&lt;user symbol&gt;");
grammar_item("'(' &lt;expression&gt; ')'");
grammar_item("&lt;sequence_expression&gt; ( '&amp;&amp;' | '||' | '^^' ) &lt;sequence_expression&gt;");
grammar_item("not &lt;sequence expression&gt;");
grammar_end();

grammar_start("instantiation");
grammar_item("&lt;user symbol&gt; &lt;user symbol&gt; [ '[' &lt;expression&lt; ']' ] '(' [&lt;port map list&gt; ] ')' ';'");
grammar_end();

grammar_start("port map list");
grammar_item("&lt;port map&gt; [ ',' [ &lt;port map list&gt; ] ]");
grammar_end();

grammar_start("port map");
grammar_item("&lt;user symbol&gt; '&lt;-' &lt;user symbol&gt;");
grammar_item("&lt;user symbol&gt; '=' &lt;expression&gt;");
grammar_item("&lt;user symbol&gt; '&lt;=' &lt;expression&gt;");
grammar_item("&lt;user symbol&gt; '=&gt;' &lt;lvar&gt;");
grammar_end();

grammar_note("Note that braces are required inside building blocks, and are not optional as they are in C. This removes problems such as the dangling-else, and requires cleaner code; this is especially important for hardware design.");
grammar_note("For statements are compile-time, <I>not run-time</I>. They are supplied to support multiple instantiations of hardware with single statements, where each piece of hardware may be parametrized in some fashion. It encompasses the capability of a 'generate' statement and the more generic 'for' statement in most HDLs; note that as CDL is a hardware design description language, implementing the 'for' statement at compile time mirrors what synthesis must do, so there is no loss of capability.");
grammar_note("Instantiations allow for setting of paramters from expressions, driving of clocks from clock ports, driving of inputs with expressions, and driving of outputs (which must be nets or components of nets).");

include "${toplevel}web_footer.php"; ?>
