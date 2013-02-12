#!/usr/bin/env python

#a Imports
import os, string
import c_httpd
from xml.dom.minidom import Document
import traceback
import c_logs

#a Globals

#a Http callback
#f get_query_dict
def get_query_dict( query_dict, k, index=None ):
    """Get an element of the query dictionary, with an optional index into the list
    return None for an invalid index, and return an empty list for a query item that is not provided on the get line"""
    qd = []
    if k in query_dict:
        qd = query_dict[k]
        pass
    if index is None:
        return qd
    if (index<0) or (index>=len(qd)):
        return None
    return qd[index]

#f xml_get_summary
def xml_get_summary( server, url_dict, query_dict, log_file ):
    """
    Get a summary of the log file that matches the provided filters
    """
    doc = Document()
    xml = doc.createElement("log_summary")
    filter_names = get_query_dict(query_dict,"filters")
    for k in log_file.matching_events( filter_names ):
        event = log_file.log_events[k]
        entry = doc.createElement("log_event")
        (module, module_log_number) = k
        entry.setAttribute( "module_alias", event.module_alias )
        entry.setAttribute( "module", module )
        entry.setAttribute( "module_log_number", str(module_log_number) )
        entry.setAttribute( "reason", event.reason )
        entry.setAttribute( "num_args", str(len(event.arguments)) )
        entry.setAttribute( "num_occurrences", str(len(event.occurrences)) )
        xml.appendChild( entry )
    return ("xml", xml)

#f xml_get_events
def xml_get_events( server, url_dict, query_dict, log_file ):
    """
    Get all the log events that match the provided filters
    Optionally provide also 'module' and 'module_log_number', to further constrain the returned events
    """
    module            = get_query_dict(query_dict,"module",0)
    module_log_number = get_query_dict(query_dict,"module_log_number",0)
    start_occurrence  = get_query_dict(query_dict,"start",0)
    max_occurrences   = get_query_dict(query_dict,"max",0)
    if module_log_number is not None: module_log_number=int(module_log_number)
    filter_names = get_query_dict(query_dict,"filters")
    occurrences = log_file.matching_event_occurrences( module, module_log_number, filter_names, max_occurrences=max_occurrences, start_occurrence=start_occurrence )
    doc = Document()
    xml = doc.createElement("log_events")
    argument_fields = {}
    event_types_added = {}
    for (k,o) in occurrences:
        event = log_file.log_events[k]
        if k in event_types_added.keys():
            continue
        event_types_added[k] = {}
        for a in event.arguments:
            if a not in argument_fields.keys():
                argument_fields[a] = None
                pass
            pass
        pass
    argument_field_alphabetical = argument_fields.keys()
    argument_field_alphabetical.sort()
    for i in range(len(argument_field_alphabetical)):
        argument_fields[argument_field_alphabetical[i]] = i

    event_types_added = {}
    for (k,o) in occurrences:
        event = log_file.log_events[k]
        if k in event_types_added.keys():
            continue
        event_types_added[k] = [0] * len(event.arguments)
        for i in xrange(len(event.arguments)):
            event_types_added[k][i] = argument_fields[event.arguments[i]]
            pass
        pass

    entry = doc.createElement("log_arguments")
    xml.appendChild( entry )
    for a in argument_field_alphabetical:
        subentry = doc.createElement("log_argument")
        subentry.setAttribute( "number", str(argument_fields[a]) )
        subentry.setAttribute( "name",   str(a) )
        entry.appendChild( subentry )

    entry = doc.createElement("log_occurrences")
    xml.appendChild( entry )
    for (k,o) in occurrences:
        event = log_file.log_events[k]
        (module, module_log_number) = k

        subentry = doc.createElement("log_occurrence")
        subentry.setAttribute( "module"           ,module)
        subentry.setAttribute( "module_log_number",str(module_log_number))
        subentry.setAttribute( "module_alias", event.module_alias )
        subentry.setAttribute( "timestamp", str(o[0]) )
        subentry.setAttribute( "number",    str(o[1]) )
        arguments = ['']*len(argument_field_alphabetical)
        for i in xrange(len(event.arguments)):
            arguments[event_types_added[k][i]] = hex(o[2][i])
            pass
        argument_string=''
        for a in arguments:
            argument_string = argument_string + str(a) + ","
        subentry.setAttribute( "arguments", argument_string )
        entry.appendChild( subentry )

    return ("xml", xml)

#f xml_get_filters
def xml_get_filters( server, url_dict, query_dict, log_file ):
    """
    Get all the filters registered with the log file
    """
    doc = Document()
    xml = doc.createElement("log_filters")
    filters = log_file.filters.keys()
    filters.sort()
    for f in filters:
        entry = doc.createElement("log_filter")
        entry.setAttribute( "name", f )
        xml.appendChild( entry )
    return ("xml", xml)

#f xml_reload_log_file
def xml_reload_log_file( server, url_dict, query_dict, log_file ):
    """
    Reload the configuration - set a new log_file
    """
    server.log_file = load_config_and_logfile( config_filename = server.config_filename,
                                               log_filename    = server.log_filename )
    doc = Document()
    xml = doc.createElement("null")
    return ("xml", xml)

#f httpd_callback
def httpd_callback( server, url_dict, query_dict ):
    print url_dict, query_dict
    query_type = url_dict.path
    if query_type[0]=='/': query_type=query_type[1:]
    xml_get_callbacks = { "get_summary":xml_get_summary,
                          "get_events":xml_get_events,
                          "get_filters":xml_get_filters,
                          "reload_log_file":xml_reload_log_file,
                          }
    if query_type in xml_get_callbacks.keys():
        try:
            return xml_get_callbacks[query_type]( server, url_dict, query_dict, server.log_file )
        except:
            pass
        print traceback.format_exc()
        return ("text",traceback.format_exc())
    return ("text", "Failed to find query type %s"%query_type )

#a Server
#b Create log file and add filters
def load_config_and_logfile( config_filename, log_filename ):
    lc = c_logs.c_log_config()
    lc.load_file( config_filename )

    lf = c_logs.c_log_file( module_aliases = lc.module_aliases() )
    lf.open( filename=log_filename )
    for (name,field_dict_list) in lc.filters().iteritems():
        f = c_logs.c_log_filter()
        for field_dict in field_dict_list:
            f.add_match( field_dict )
            pass
        lf.add_filter( name, f )
        pass
    return lf

#f usage
def usage(code):
    print >>sys.stderr, """\
log_server [options] <logfile>
           --cdl=<path>         File path to CDL root - if not provided, uses env[CYCLICITY_ROOT]
           --config=<name>      Config filename describing aliases and filters; default from env[LOGCONFIG]
           --ip=<name>          IP address to use; if not provided, all IP addresses on the machine can be used
           --port=<port number> Port number to run the server on, default is 8000

The program runs an HTTP server on 127.0.0.1:<port> for analysis of CDL log files
The config option should be provided, or an environment variable 'LOGCONFIG' set; one or other is required.
"""
    sys.exit(code)

#b Handle arguments
import getopt, sys
getopt_options = ["config=","ip=","port=","cdl="]
args = {"port":8000,"ip":""}
for o in getopt_options:
    if o[-1]=='=':
        if o[:-1] not in args:
            args[o[:-1]]=None
try:
    (optlist,file_list) = getopt.getopt(sys.argv[1:],'',getopt_options)
except:
    print >>sys.stderr, "Exception parsing options:", sys.exc_info()[1]
    usage(4)

for opt,value in optlist:
    if (opt[0:2]=='--') and  (opt[2:] in args):
        args[opt[2:]] = value
    else:
        print >>sys.stderr, "Unknown option", opt, "with value", value
        usage(4)

#b Defaults if not provided and can use environment
if args["cdl"]==None:
    if "CYCLICITY_ROOT" not in os.environ:
        usage(4)
        pass
    cyclicity_root = os.environ["CYCLICITY_ROOT"]
else:
    cyclicity_root = args["cdl"]

if args["config"]==None:
    if "LOGCONFIG" not in os.environ:
        usage(4)
        pass
    config_filename = os.environ["LOGCONFIG"]
else:
    config_filename = args["config"]

#b Parse config file and  log file initially
if len(file_list)==0:
    usage(4)
    pass

log_filename = file_list[0]
lf = load_config_and_logfile( config_filename=config_filename, log_filename=log_filename )

#b Create and run server
server = c_httpd.c_http_server( server_address=(args["ip"],args["port"]),
                                file_path = [cyclicity_root+"/httpd/log", cyclicity_root+"/httpd/log/js", cyclicity_root+"/httpd/js"],
                                client_callback = httpd_callback,
                                verbose = {}, # "get":99, "response":99 }
                                )
server.log_file = lf
server.log_filename = log_filename
server.config_filename = config_filename
server.serve()
