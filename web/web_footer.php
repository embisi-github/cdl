<?php

/*a End the main column
 */

page_ep();

echo "</table></td>\n";

/*a Vertical bar between the columns (actually another column
 */
?>
<td width="1" bgcolor="black">
<?php insert_blank(1,1); ?>
</td>

<?php

/*a Navigation column
 */

echo "<td width=$nav_width bgcolor=\"$nav_background\" valign=top align=center >";
include "${toplevel}web_navigation.php";
echo "</td>\n";

/*a Now the bottom section; line, and copyright/license
 */
?>
</tr>

<tr>
<td colspan="3" bgcolor="#000000">
<?php insert_blank(1,1); ?>
</td>
</tr>

<tr>
<td colspan="3" <?php echo "\"bgcolor=$footer_background\""; ?>
<p class=footer>
This web page may be copied freely without any obligations
</td>
</tr>

</table>
</body>
</html>
