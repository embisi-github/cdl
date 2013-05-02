#!/usr/bin/env python

#a Imports
import os, string
import c_httpd
from xml.dom.minidom import Document
import traceback
import c_logs

#a Globals

#a Log server class
#c c_http_log_server
class c_http_log_server(object):
    #f __init__
    def __init__( self, log_filename, config_filename, config_path, server_address, file_path ):
        self.config_path = config_path
        self.load_config_and_logfile( config_filename=config_filename, log_filename=log_filename )
        self.server = c_httpd.c_http_server( server_address = server_address,
                                             file_path = file_path,
                                             client_callback = self.httpd_callback,
                                             verbose = {}, # "get":99, "response":99 }
                                             )
        pass

    #f serve
    def serve( self ):
        self.server.serve()
        
    #f Create log file and add filters
    def load_config_and_logfile( self, config_filename, log_filename ):
        self.log_filename = log_filename
        self.config_filename = config_filename

        lc = c_logs.c_log_config(config_path=self.config_path)
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
        self.log_file = lf
        return lf

    #f get_query_dict
    def get_query_dict( self, query_dict, k, index=None ):
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
    def xml_get_summary( self, url_dict, query_dict ):
        """
        Get a summary of the log file that matches the provided filters
        """
        doc = Document()
        xml = doc.createElement("log_summary")
        filter_names = self.get_query_dict(query_dict,"filters")
        for k in self.log_file.matching_events( filter_names ):
            event = self.log_file.log_events[k]
            entry = doc.createElement("log_event")
            (module, module_log_number) = k
            entry.setAttribute( "module_alias", event.module_alias )
            entry.setAttribute( "module", module )
            entry.setAttribute( "module_log_number", str(module_log_number) )
            entry.setAttribute( "reason", event.reason )
            entry.setAttribute( "num_args", str(len(event.arguments)) )
            entry.setAttribute( "num_occurrences", str(len(event.occurrences)) )
            xml.appendChild( entry )
            pass
        return ("xml", xml)

    #f xml_get_events
    def xml_get_events( self, url_dict, query_dict ):
        """
        Get all the log events that match the provided filters
        Optionally provide also 'module' and 'module_log_number', to further constrain the returned events
        """

        #b Get arguments from the query
        module            = self.get_query_dict(query_dict,"module",0)
        module_log_number = self.get_query_dict(query_dict,"module_log_number",0)
        start_occurrence  = self.get_query_dict(query_dict,"start",0)
        max_occurrences   = self.get_query_dict(query_dict,"max",0)
        if module_log_number is not None: module_log_number=int(module_log_number)
        filter_names = self.get_query_dict(query_dict,"filters")

        #b Find occurrences of events that match
        occurrences = self.log_file.matching_event_occurrences( module, module_log_number, filter_names, max_occurrences=max_occurrences, start_occurrence=start_occurrence )

        #b Find all argument field names - argument_fields is dict( <arg_name>: None ) for all arg_names in events which have occurrences
        argument_fields = {}
        event_types_added = {}
        for (k,o) in occurrences:
            event = self.log_file.log_events[k]
            if k in event_types_added.keys():
                continue
            event_types_added[k] = {}
            for a in event.arguments:
                if a not in argument_fields.keys():
                    argument_fields[a] = None
                    pass
                pass
            pass

        #b Get sorted list of argument names from argument_fields.keys() sorted, and make argument_fields be dict( <arg_name>:<alphabetical index> )
        argument_field_alphabetical = argument_fields.keys()
        argument_field_alphabetical.sort()
        for i in range(len(argument_field_alphabetical)):
            argument_fields[argument_field_alphabetical[i]] = i
            pass

        #b Make event_types_added be dict( <event_type>:[ <alphabetical index of arg N> (N = 0 .. num args in event_type) ]
        event_types_added = {}
        for (k,o) in occurrences:
            event = self.log_file.log_events[k]
            if k in event_types_added.keys():
                continue
            event_types_added[k] = [0] * len(event.arguments)
            for i in xrange(len(event.arguments)):
                event_types_added[k][i] = argument_fields[event.arguments[i]]
                pass
            pass

        #b Build XML document header
        doc = Document()
        xml = doc.createElement("log_events")

        #b Build XML 'log_arguments' - list of argument names in alphabetical order from argument_fields.keys()
        entry = doc.createElement("log_arguments")
        xml.appendChild( entry )
        for a in argument_field_alphabetical:
            subentry = doc.createElement("log_argument")
            subentry.setAttribute( "number", str(argument_fields[a]) )
            subentry.setAttribute( "name",   str(a) )
            entry.appendChild( subentry )
            pass

        #b Build XML 'log_occurences' - list of occurences
        # arguments per occurrence are comma-separated-strings of values ordered per argument_field_alphabetical
        #   an occurrence with no argument for a particular argument_field_alphabetical name has nothing presented in its field (e.g. ',,')
        entry = doc.createElement("log_occurrences")
        xml.appendChild( entry )
        for (k,o) in occurrences:
            event = self.log_file.log_events[k]
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
                pass
            subentry.setAttribute( "arguments", argument_string )
            entry.appendChild( subentry )
            pass

        #b Return the xml
        return ("xml", xml)

    #f xml_get_filters
    def xml_get_filters( self, url_dict, query_dict ):
        """
        Get all the filters registered with the log file
        """
        doc = Document()
        xml = doc.createElement("log_filters")
        filters = self.log_file.filters.keys()
        filters.sort()
        for f in filters:
            entry = doc.createElement("log_filter")
            entry.setAttribute( "name", f )
            xml.appendChild( entry )
            pass
        return ("xml", xml)

    #f xml_reload_log_file
    def xml_reload_log_file( self, url_dict, query_dict ):
        """
        Reload the configuration - set a new log_file
        """
        self.load_config_and_logfile( config_filename = self.config_filename,
                                      log_filename    = self.log_filename )
        doc = Document()
        xml = doc.createElement("null")
        return ("xml", xml)

    #f httpd_callback
    def httpd_callback( self, server, url_dict, query_dict ):
        print url_dict, query_dict
        query_type = url_dict.path
        if query_type[0]=='/': query_type=query_type[1:]
        xml_get_callbacks = { "get_summary":self.xml_get_summary,
                              "get_events":self.xml_get_events,
                              "get_filters":self.xml_get_filters,
                              "reload_log_file":self.xml_reload_log_file,
                              }
        if query_type in xml_get_callbacks.keys():
            try:
                return xml_get_callbacks[query_type]( url_dict, query_dict )
            except:
                pass
            print traceback.format_exc()
            return ("text",traceback.format_exc())
        return ("text", "Failed to find query type %s"%query_type )

#a Server
#c c_cmd_line_server
class c_cmd_line_server( object ):
    #f __init__
    def __init__( self, default_args={} ):
        import copy
        self.args = copy.copy(default_args)
        for (k,v) in {"port":8000,"ip":"","config_path":""}.iteritems():
            if k not in self.args: self.args[k]=v
            pass
        self.parse_args()
        self.create_and_run_server()
        pass

    #f usage
    def usage(self,code):
        import sys
        print >>sys.stderr, """\
    log_server [options] <logfile>
               --cdl=<path>         File path to CDL root - if not provided, uses env[CYCLICITY_ROOT]
               --config=<name>      Config filename describing aliases and filters; default from env[LOGCONFIG]
               --config_path=<name> Colon-separated directories to search for configuration files, after cwd is searched first
               --ip=<name>          IP address to use; if not provided, all IP addresses on the machine can be used
               --port=<port number> Port number to run the server on, default is 8000
    
    The program runs an HTTP server on 127.0.0.1:<port> for analysis of CDL log files
    The config option should be provided, or an environment variable 'LOGCONFIG' set; one or other is required.
    """
        sys.exit(code)

    #f parse_args
    def parse_args( self ):
        import getopt, sys
        getopt_options = ["config=","ip=","port=","cdl=", "config_path="]
        for o in getopt_options:
            if o[-1]=='=':
                if o[:-1] not in self.args:
                    self.args[o[:-1]]=None
                    pass
                pass
            pass
        try:
            (optlist,file_list) = getopt.getopt(sys.argv[1:],'',getopt_options)
        except:
            print >>sys.stderr, "Exception parsing options:", sys.exc_info()[1]
            self.usage(4)
            pass

        for opt,value in optlist:
            if (opt[0:2]=='--') and  (opt[2:] in self.args):
                self.args[opt[2:]] = value
                pass
            else:
                print >>sys.stderr, "Unknown option", opt, "with value", value
                self.usage(4)
                pass
            pass

        #b Defaults if not provided and can use environment
        self.cyclicity_root = self.args["cdl"]
        if self.args["cdl"]==None:
            if "CYCLICITY_ROOT" not in os.environ:
                self.usage(4)
                pass
            self.cyclicity_root = os.environ["CYCLICITY_ROOT"]
            pass

        self.config_filename = self.args["config"]
        if self.args["config"]==None:
            if "LOGCONFIG" not in os.environ:
                self.usage(4)
                pass
            self.config_filename = os.environ["LOGCONFIG"]
            pass

        self.config_path = self.args["config_path"]

        #b Parse config file and  log file initially
        if len(file_list)==0:
            self.usage(4)
            pass
        self.log_filename = file_list[0]

        #b Done
        return

    #f create_and_run_server
    def create_and_run_server( self ):
        #b Create and run server
        server = c_http_log_server( log_filename    = self.log_filename,
                                    config_filename = self.config_filename,
                                    config_path     = self.config_path,
                                    server_address  = (self.args["ip"],
                                                       int(self.args["port"])),
                                    file_path = [ self.cyclicity_root+"/httpd/log",
                                                  self.cyclicity_root+"/httpd/log/js",
                                                  self.cyclicity_root+"/httpd/js"],
                            )
        server.serve()
        return

#a Toplevel
if __name__ == '__main__':
    server = c_cmd_line_server()


