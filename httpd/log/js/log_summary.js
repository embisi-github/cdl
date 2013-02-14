/*a Static variables
 */
var ajax_reqs = new Array(10);
ajax_reqs[0] = new Ajax();
ajax_reqs[1] = new Ajax();
ajax_reqs[2] = new Ajax();
ajax_reqs[3] = new Ajax();

var url_root = "/";
var datasets = {};
var chart_args = {}

/*a Initialization
 */
/*f rxed_log_filters
 */
filters_to_apply = []
function set_filter(name, value)
{
    var i;
    if (value.checked)
    {
        if (filters_to_apply.indexOf(name)>=0)
            return;
        filters_to_apply.push(name);
    }
    else
    {
        i = filters_to_apply.indexOf(name);
        if (i<0) return;
        filters_to_apply.splice(i,1);
    }
    get_summary();
}
function rxed_cmd(xmldoc)
{
    ajax_progress( 3, 1 );
    init_summary();
}
function rxed_log_filters(xmldoc)
{
    ajax_progress( 2, 1 );
    var table;
    var log_filters=xmldoc.documentElement.childNodes;
    var i;
    table = "<form><table>";
    table = table + "<tr>";
    table = table + "<td colspan=1><input type='button' name='reload' onclick='reload_log_file();' value='Reload'/></td>";
    table = table + "<td colspan=4><input type='button' name='populate' onclick='fill_log_events("+'"",""'+");' value='Populate Occurrences'/></td>";
    table = table + "</tr>";
    table = table + "<tr>";
    for (i=0; i<log_filters.length; i++)
    {
        if (((i%5)==0) && (i>0))
        {
            table = table + "</tr><tr>";
        }
        var name         = log_filters[i].getAttribute("name");
        table = table + "<td><input type='checkbox' onclick='set_filter("+'"'+name+'"'+",filter__"+name+");' name='filter__"+name+"'>"+name+"</input></td>";
    }
    while ((i==0) || ((i%5)!=0))
    {
        table = table + "<td/>";
        i++;
    }
    
    table = table + "</tr>";
    table = table + "</table></form>";
    document.getElementById('log_filters').innerHTML = table;
}

/*f rxed_log_events
 */
function rxed_log_events(xmldoc)
{
    ajax_progress( 1, 1 );
    var table;
    var log_arguments=xmldoc.documentElement.childNodes[0].childNodes;
    var log_occurrences=xmldoc.documentElement.childNodes[1].childNodes;
    var i, j;
    table = "<form><table>";
    table = table + "<tr>";
    table = table + "<th>Time</th>";
    table = table + "<th>Mod</th>";
    table = table + "<th>Entry</th>";
    table = table + "<th>Number</th>";
    table = table + "<th colspan="+log_arguments.length+">Arguments</th>";
    table = table + "</tr>";
    table = table + "<tr>";
    table = table + "<th></th>";
    table = table + "<th></th>";
    table = table + "<th></th>";
    table = table + "<th></th>";
    for (i=0; i<log_arguments.length; i++)
    {
        table = table + "<th>"+log_arguments[i].getAttribute("name")+"</th>";
    }
    table = table + "</tr>";
    for (i=0; i<log_occurrences.length; i++)
    {
        var module            = log_occurrences[i].getAttribute("module_alias");
        var module_log_number = log_occurrences[i].getAttribute("module_log_number");
        var timestamp         = log_occurrences[i].getAttribute("timestamp");
        var occurrence_number = log_occurrences[i].getAttribute("number");
        var arguments         = log_occurrences[i].getAttribute("arguments");
        table = table + "<tr>";
        table = table + "<td>"+timestamp+"</td>";
        table = table + "<td>"+module+"</td>";
        table = table + "<td>"+module_log_number+"</td>";
        table = table + "<td>"+occurrence_number+"</td>";
        arguments_list = arguments.split(",");
        for (j=0; j<arguments_list.length-1; j++)
        {
            table = table + "<td>"+arguments_list[j]+"</td>";
        }
        table = table + "</tr>";
    }
    table = table + "</table></form>";
    document.getElementById('log_events').innerHTML = table;
}

/*f rxed_log_summary
 */
function rxed_log_summary(xmldoc)
{
    ajax_progress( 0, 1 );
    var table;
    var log_events=xmldoc.documentElement.childNodes;
    var i;
    table = "<form><table>";
    table = table + "<tr>";
    table = table + "<th>Module</th>";
    table = table + "<th>Log entry</th>";
    table = table + "<th>Occurrences</th>";
    table = table + "<th>Reason</th>";
    table = table + "<th>Button</th>";
    table = table + "</tr>";
    for (i=0; i<log_events.length; i++)
    {
        var module       = log_events[i].getAttribute("module");
        var module_alias = log_events[i].getAttribute("module_alias");
        var module_log_number  = log_events[i].getAttribute("module_log_number");
        var num_occurrences  = log_events[i].getAttribute("num_occurrences");
        var reason           = log_events[i].getAttribute("reason");
        table = table + "<tr>";
        table = table + "<td>"+module_alias+"</td>";
        table = table + "<td>"+module_log_number+"</td>";
        table = table + "<td>"+num_occurrences+"</td>";
        table = table + "<td>"+reason+"</td>";
        table = table + "<td><input type='button' name='populate' onclick='fill_log_events("+'"'+module+'"'+","+module_log_number+");' value='Populate Occurrences'/></td>";
        table = table + "</tr>";
    }
    table = table + "</table></form>";
    document.getElementById('log_summary').innerHTML = table;
}

/*f doXML
 */
function ajax_progress( n, done )
{
    d = document.getElementById('ajax_in_progress_'+n);
    if (d)
    {
        if (done)
        {
            d.innerHTML = "<font color=black>O</font>";
        }
        else
        {
            d.innerHTML = "<font color=red>X</font>";
        }
    }
}
function doXML( n, url, callback )
{
    ajax_progress( n, 0 );
    ajax_reqs[n].doGet( url, callback, 'xml' );
}

/*f get_summary
 */
function get_summary()
{
    var i;
    request = "get_summary?x=";
    r = "&"
    for (i=0; i<filters_to_apply.length; i++)
    {
        request = request+r+"filters="+filters_to_apply[i]
        r = "&";
    }
    doXML( 0, request, rxed_log_summary);
}

/*f init_summary
 */
function init_summary()
{
    get_summary();
    doXML( 2, 'get_filters?x=', rxed_log_filters);
}

/*f reload_log_file
 */
function reload_log_file()
{
    doXML( 3, 'reload_log_file?x=', rxed_cmd);
}

/*f fill_log_events
 */
function fill_log_events(module,module_log_number)
{
    var i;
    request = "get_events?max=10000";
    r = "&"
    for (i=0; i<filters_to_apply.length; i++)
    {
        request = request+r+"filters="+filters_to_apply[i]
        r = "&";
    }
    if (module !=='')
    {
        request = request+r+"module="+module;
    }
    if (module_log_number !== "")
    {
        request = request+r+"module_log_number="+module_log_number;
    }
    doXML( 1, request, rxed_log_events );
}

