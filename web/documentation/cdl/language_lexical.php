<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.language_specification.lexical" );

$page_title = "Cyclicity CDL Language Specification";
include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 14th 2004" );
page_header($page_title. " - Lexical Analysis" );

?>

<P>There are a few basic lexical features of CDL</P>
<UL>
    <LI><P>Single line comments start with '//'; all text to a newline
    following a '//' token will not be used in further analysis.</P>
    <LI><P>Multiline comments start with '/*' and end with '*/'; all
    text between a '/*' token and the next '*/' token will not be used
    in further analysis.</P>
    <LI><P>White space is ignored</P>
</UL>
<P>Files, after comments are ignored, are broken lexically into
tokens. Tokens may then be numbers, strings, symbols, reserved
keywords, or user symbols.</P>
<UL>
    <LI><P>Strings are of the form "text of the string", i.e. data
    enclosed in pairs of quotation marks. Note that comments will NOT be
    stripped within a string; also, the backslash character may be used
    to quote a quotation mark within a string such as: "string with a
    \" quotation \" in it".</P>
    <LI><P>Symbols are non-alphanumerics, either singly or paired. The
    complete list is: , . ~ &amp; | ^ ! * + - / % &amp;&amp; || ^^ =&gt;
    &lt;- = == != &lt; &gt; &lt;= &gt;= ( ) ; : 
    </P>
    <LI><P>Numbers always start with a digit, and they may be sized or
    unsized numbers. Sized numbers are of the form
    '&lt;n&gt;&lt;b|B|h|H&gt;&lt;value&gt;'; the initial 'n' gives the
    size in the number of bits of the number, the base is then
    presented, then the value is given. The value may contain '_'s,
    which will be ignored. The value may also contain 'x' or 'X'
    characters, which indicate the number has an associated mask, so
    that masked comparisons may be expressed in the language. The
    following, then, are valid numbers: '123', '871232', '0',
    '16b1111_0000_11111_0000', '8HaF', '6b10xx01'.</P>
    <LI><P>Reserved keywords are all lower case, and are keywords that
    are used within the language. The complete set is: constant struct
    fsm one_hot one_cold schematic symbol port line fill oval option
    preclock register assert include typedef string bit integer enum
    extern module input output parameter timing to from bundle default
    clock rising falling reset active_low active_high clocked comb net
    for if elsif else full_switch part_switch priority case break sizeof
    print assert</P>
    <LI><P>User symbols start with alphabetical characters and contain
    alphanumerics plus '_'.</P>
</UL>
<P>File inclusion is supported through use of the 'include' token.
After lexical analysis of a file, if an 'include' token is found at
any point immediately followed by a string, then at that point in
file another files will be included, and the file shall be derived
from the string.</P>


<?php include "${toplevel}web_footer.php"; ?>
