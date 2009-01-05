<?php

/*a Skip the top of the page
 */
insert_blank( $nav_width, 10 );

/*a Insert global navigation area
 */
nav_link_start("Main links");
nav_link_add( "index.php", "Home" );
nav_link_add( "http://sourceforge.net/projects/cyclicity-cdl", "SF Project" );
nav_link_add( "documentation/index.php", "Documentation" );
nav_link_add( "cdl.php", "CDL" );
nav_link_add( "documentation/cdl/language_examples.php", "Samples" );
nav_link_add( "history.php", "History" );
nav_link_add( "installation.php", "Installation" );
nav_link_add( "documentation/support_libraries/index.php", "Support libraries" );
nav_link_end();

echo "<br>\n";

/*a Insert SourceForge logo
 */
?>

<p>
<a href="http://sourceforge.net/">
<img src="http://sourceforge.net/sflogo.php?group_id=1081843&amp;type=1" width="88" height="31" border="0" alt="SourceForge.net Logo"/>
</a>
</p>

<?php

/*a Insert any local navigation if required
 */
function add_sitemap_tag( $posn, $tag, $current_marker )
{
    global $site_map;
    if ($tag=="")
    {
        $max = $site_map["count"][0];
    }
    else
    {
        $max = $site_map["nsublinks"][$tag];
    }
    for ($i=1; $i<=$max; $i++)
    {
        $elt_tag = $tag.".".$i;
        $marker = $current_marker.".".$site_map["markers"][$elt_tag];
        nav_link_add( $site_map["hrefs"][$elt_tag], $site_map["texts"][$elt_tag] );
        if ($site_map["nsublinks"][$elt_tag]>0)
        {
            $mlen = strlen($marker);
            if ( (substr($posn, 0, $mlen)==$marker) &&
                 ( ($posn==$marker) || ($posn[$mlen]==".")) )
            {
                nav_link_push();
                add_sitemap_tag( $posn, $elt_tag, $marker );
                nav_link_pop();
            }
        }
    }
}
if ($site_depth==0)
{
    $site_position = ".top";
}
nav_link_start_hierarchy("Site map");
add_sitemap_tag( $site_position, "", "" );
nav_link_end();
