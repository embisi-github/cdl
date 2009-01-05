<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.language_specification" );
$page_title = "Cyclicity CDL Language Specification";

include "${toplevel}/web_header.php";

page_identifier( "author", "Gavin J Stark" );
page_identifier( "version", "v0.01" );
page_identifier( "date", "April 14th 2004" );
page_header( $page_title );

?>

<?php page_section( "contents", "Contents" ); ?>

<p>
These web pages specify the Cyliclty cycle design language (CDL).
</p>

<p>
<table width=100%>
<?php echo "<tr><th align=center>".page_link("","")."</th></tr>";?>
<?php echo "<tr><th align=center>".page_link("language_lexical.php","Lexical analysis")."</th></tr>";?>
<?php echo "<tr><th align=center>".page_link("language_grammar.php","Grammar")."</th></tr>";?>
<?php echo "<tr><th align=center>".page_link("language_examples.php","Language examples")."</th></tr>";?>
<?php echo "<tr><th align=center>".page_link("language_details.php","Details")."</th></tr>";?>
<?php echo "<tr><th align=center>".page_link("language_discussion.php","Discussion")."</th></tr>";?>
</table>
</p>

<?php include "${toplevel}web_footer.php"; ?>

