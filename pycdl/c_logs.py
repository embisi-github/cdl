#!/usr/bin/env python

#a Imports
import string, copy, re
import csv
import ConfigParser
import traceback
import os.path

#a Log filter classes
#c c_log_filter_match_base
class c_log_filter_match_base( object ):
    def __init__( self, args ):
        for (k,v) in args.iteritems():
            setattr( self, k, copy.copy(v) )
    def field_match( self, field_data, possibilities ):
        return False

#c c_log_filter_match_str_eq
class c_log_filter_match_str_eq( c_log_filter_match_base ):
    def field_match( self, field_data ):
        strings = []
        if hasattr(self,"strings"): strings=self.strings
        if hasattr(self,"string"):  strings=[self.string]
        matches = field_data in strings
        if self.invert: matches = not matches
        return matches

#c c_log_filter_match_str_regexp
class c_log_filter_match_str_regexp( c_log_filter_match_base ):
    def field_match( self, field_data ):
        matches = False
        if hasattr(self,"search"):
            matches = (re.search(self.search, field_data)!=None)
        if hasattr(self,"match"):
            matches = (re.match(self.search, field_data)!=None)
        if self.invert: matches = not matches
        return matches

#c c_log_filter_match_int_eq
class c_log_filter_match_int_eq( c_log_filter_match_base ):
    def field_match( self, field_data ):
        if self.mask is not None:
            matches = ((field_data&self.mask)==(self.match&self.mask))
            pass
        else:
            matches = (field_data==self.match)
            pass
        if self.invert: matches = not matches
        return matches

#c c_log_filter_match_int_range
class c_log_filter_match_int_range( c_log_filter_match_base ):
    def field_match( self, field_data ):
        matches = True
        if (field_data<self.min): matches=False
        if (field_data>self.max): matches=False
        if self.invert: matches = not matches
        return matches

#c c_log_filter
class c_log_filter( object ):
    """ Apply a compare to fields of an event of event occurrence
    A field match for a filter is a logical OR of a list of <field> mask (== | != ) match
    The filter passes if every field that the filter applies has a field match
    Unspecified fields are assumed to match
    """
    #f __init__
    def __init__( self ):
        self.fields = {}
        pass

    #f add_match
    def add_match( self, field_match_dict ):
        field = field_match_dict["field"]
        filter_match_class = None
        if field not in self.fields.keys():
            self.fields[field] = []
            pass
        if field_match_dict["type"] in ["streq"]:
            field_match = copy.copy(field_match_dict)
            filter_match_class = c_log_filter_match_str_eq
            pass
        elif field_match_dict["type"] in ["regexp"]:
            field_match = copy.copy(field_match_dict)
            filter_match_class = c_log_filter_match_str_regexp
            pass
        elif field_match_dict["type"] in ["intmatch", "intmaskmatch"]:
            field_match = copy.copy(field_match_dict)
            if "mask" not in field_match.keys(): field_match["mask"]=None
            filter_match_class = c_log_filter_match_int_eq
            pass
        elif field_match_dict["type"] in ["intrange"]:
            field_match = copy.copy(field_match_dict)
            filter_match_class = c_log_filter_match_int_range
            pass
        if "invert" not in field_match.keys(): field_match["invert"]=False
        if filter_match_class is not None:
            self.fields[field].append( filter_match_class(field_match) )
        return

    #f apply_filter_to_field
    def apply_filter_to_field( self, field, data_options ):
        for filter_match in self.fields[field]:
            for d in data_options:
                if filter_match.field_match( d ):
                    return True
                pass
            pass
        return False

    #f apply_to_event
    def apply_to_event( self, module, module_alias, log_number, reason, argument_names ):
        if "module" in self.fields.keys():
            if not self.apply_filter_to_field( "module", [module, module_alias] ):
                return False
            pass
        if "reason" in self.fields.keys():
            if not self.apply_filter_to_field( "reason", [reason] ):
                return False
            pass
        for field_name in self.fields.keys():
            if field_name in ["module", "reason"]:
                continue
            if field_name not in argument_names:
                return False
            pass
        return True

    #f compile_for_event
    def compile_for_event( self, module, module_alias, log_number, reason, argument_names ):
        """
        Returns None if all event occurrences fail the filter
        Else return a function that can be called with an occurrence that will return True/False as to whether to include the occurrence
        """
        compilation = []
        if "module" in self.fields.keys():
            if not self.apply_filter_to_field( "module", [module, module_alias] ):
                return None
            pass
        if "reason" in self.fields.keys():
            if not self.apply_filter_to_field( "reason", [reason] ):
                return None
            pass
        for field_name in self.fields.keys():
            if field_name in ["module", "reason"]:
                continue
            if field_name not in argument_names:
                return None
            compilation.append( self.check_occurrence_fn(field_name, argument_names) )
        return compilation

    #f check_occurrence_fn
    def check_occurrence_fn( self, field_name, argument_names ):
        def check_fn( occurrence, self=self, field_name=field_name, arg_index=argument_names.index(field_name) ):
            arguments = occurrence[2]
            value = arguments[arg_index]
            return self.apply_filter_to_field( field_name, [value] )
        return check_fn

#a Log event classes
#c c_log_event
class c_log_event( object ):
    #f __init__
    def __init__( self, log_file, module, module_log_number, module_log_text_reason, num_args, arguments ):
        self.log_file = log_file
        self.module = module
        self.module_log_number = module_log_number
        self.reason = module_log_text_reason
        self.num_args = num_args
        self.arguments = arguments
        self.module_alias = self.find_module_alias()
        self.alphabetical_key = "%s.%04d"%(self.module_alias, self.module_log_number)
        self.occurrences = []
        
    #f __repr__
    def __repr__( self ):
        result = ""
        result = "log_event( " +str( {"module":self.module,
                                      "alias":self.module_alias,
                                      "reason":self.reason,
                                      "arguments":self.arguments,
                                      "num_occurrences":len(self.occurrences)} ) + ")"
        return result

    #f find_module_alias
    def find_module_alias( self ):
        module_alias = None
        m = self.module
        sm = []
        while len(m)>0:
            if m in self.log_file.module_aliases.keys():
                module_alias = self.log_file.module_aliases[m]
                break
            ms = m.rsplit(".",1)
            if (len(ms)<2):
                break
            new_sm = [ms[1]]
            new_sm.extend(sm)
            sm = new_sm
            m = ms[0]
            pass
        if module_alias is None:
            return self.module
        elif len(sm)==0:
            return module_alias
        return module_alias+"."+string.join(sm,".")

    #f add_occurrence
    def add_occurrence( self, timestamp, number, arguments ):
        self.occurrences.append( (timestamp, number, arguments) )

    #f filter_event
    def filter_event( self, filter ):
        return filter.apply_to_event( module = self.module,
                                      module_alias=self.module_alias,
                                      log_number=self.module_log_number,
                                      reason=self.reason,
                                      argument_names=self.arguments )

#a Log transaction tracing classes
#c c_traced_operation
class c_traced_operation( object ):
    log_entries = []
    #f __init__
    def __init__( self, **kwargs ):
        self.log_entry_mappings = {}
        pass

    #f log_strings
    def log_strings( self ):
        s = []
        for log_name in self.log_entries:
            s.append( log_name )
            pass
        return s

    #f log_entry_event_mapping
    def log_entry_event_mapping( self, event_tag, event ):
        reason = event.reason
        event_args = event.arguments
        if reason in self.log_entries:
            self.log_entry_mappings[ reason ] = []
            self.log_entry_mappings[ event_tag ] = reason
            log_args = self.log_entries[reason][0]
            for arg in log_args:
                if arg in event_args: # It had better be...
                    self.log_entry_mappings[ reason ].append( event_args.index(arg) )
                    pass
                else:
                    raise Exception, "Failed to find blah"
            pass
        return

    #f log_entry_event_occurrence
    def log_entry_event_occurrence( self, event_tag, cycle, number, args, occurrence ):
        reason = self.log_entry_mappings[event_tag]
        callback = self.log_entries[reason][1]
        extracted_args = []
        for i in self.log_entry_mappings[reason]:
            extracted_args.append( args[i] )
            pass
        callback( reason, cycle, number, extracted_args, occurrence )
        return

    #f log_analyze_file
    def log_analyze_file( self, log_filename, max_occurrences=10000 ):
        lf = c_log_file()
        lf.open(log_filename)
        filter = c_log_filter()
        filter.add_match( {"field":"reason", "type":"streq", "strings":self.log_strings()} )
        lf.add_filter( filter_name="blah", filter=filter )
        for e in lf.matching_events( ["blah"] ):
            event = lf.log_events[e]
            self.log_entry_event_mapping( e, event )
            pass
        
        for oc in lf.matching_event_occurrences( filter_name_list=["blah"], max_occurrences=max_occurrences ):
            self.log_entry_event_occurrence( oc[0], oc[1][0], oc[1][1], oc[1][2], oc )
            pass

        return

#c c_traced_operation_transaction
class c_traced_operation_transaction( c_traced_operation ):
    #f __init__
    def __init__( self, transaction_start, transaction_end, **kwargs ):
        c_traced_operation.__init__(self, **kwargs )
        self.transaction_start = transaction_start
        self.transaction_end   = transaction_end
        self.log_entries = { transaction_start[0]:  (transaction_start[1:],self.log_callback),
                             transaction_end[0]:    (transaction_end[1:],self.log_callback), }
        self.transactions_in_progress = {}
        self.transaction_id = 0
        pass

    #f log_callback
    def log_callback( self, reason, cycle, number, extracted_args, occurrence ):
        transaction_key = tuple(extracted_args)
        if reason == self.transaction_start[0]:
            if transaction_key in self.transactions_in_progress:
                print "Unexpected %s - transaction %s already in progress"%(self.transaction_start[0], str(transaction_key))
            else:
                self.transactions_in_progress[transaction_key] = (self.transaction_id, cycle)
                self.transaction_id += 1
                pass
            pass
        else:
            if transaction_key not in self.transactions_in_progress:
                print "Unexpected %s - transaction %s NOT in progress"%(self.transaction_end[0], str(transaction_key))
            else:
                (id, start_cycle) = self.transactions_in_progress[transaction_key]
                self.transaction_completed( tag=self.transaction_id, start_cycle=start_cycle, end_cycle=cycle )
                del self.transactions_in_progress[transaction_key]
                pass
            pass
        return

#a Log file classes
#c c_log_file
class c_log_file( object ):
    #f __init__
    def __init__( self, module_aliases={} ):
        self.reset()
        self.module_aliases = module_aliases

    #f reset
    def reset( self ):
        self.filename = None
        self.log_events = {}
        self.filters = {}

    #f update
    def update( self ):
        self.log_events_keys_alphabetical = self.log_events.keys()
        self.log_events_keys_alphabetical.sort( key=lambda x:self.log_events[x].alphabetical_key )
        return

    #f add_filter
    def add_filter( self, filter_name, filter ):
        self.filters[filter_name] = filter
        return

    #f matching_events
    def matching_events( self, filter_name_list ):
        matching_event_keys = []
        for k in self.log_events_keys_alphabetical:
            event = self.log_events[k]
            filter_allowed = True
            if len(filter_name_list)>0:
                for f in filter_name_list:
                    if not event.filter_event( self.filters[f] ):
                        filter_allowed = False
                        break
                    pass
                pass
            if filter_allowed:
                matching_event_keys.append(k)
                pass
            pass
        return matching_event_keys

    #f matching_event_occurrences
    def matching_event_occurrences( self, module=None, module_log_number=None, filter_name_list=[], max_occurrences=None, start_occurrence=0 ):
        if start_occurrence is None: start_occurrence=0
        if max_occurrences is None: max_occurrences=1000
        occurrences = []
        num = 0
        for k in self.log_events_keys_alphabetical:
            if module is not None:
                if k[0]!=module:
                    continue
                pass
            if module_log_number is not None:
                if k[1]!=module_log_number:
                    continue
                pass
            log_event = self.log_events[k]
            compiled_filters = {}
            for f in filter_name_list:
                cf = self.filters[f].compile_for_event( module=k[0],
                                                        module_alias=log_event.module_alias,
                                                        log_number=k[1],
                                                        reason=log_event.reason,
                                                        argument_names=log_event.arguments)
                if cf is None:
                    compiled_filters = None
                    break
                compiled_filters[f] = cf
                pass
            if compiled_filters is None:
                continue
            for o in self.log_events[k].occurrences:
                accept_occurrence = True
                for (f,cf_list) in compiled_filters.iteritems():
                    filter_passed = False
                    if len(cf_list)==0: filter_passed=True
                    for cf in cf_list:
                        if cf(occurrence=o):
                            filter_passed = True
                            break
                        pass
                    if not filter_passed:
                        accept_occurrence = False
                        break
                    pass
                if accept_occurrence:
                    occurrences.append( (k, o) )
                    pass
                pass
            pass
        occurrences.sort( key=lambda x:x[1][0] )
        if ( (start_occurrence<len(occurrences)) and (start_occurrence>0) ):
            occurrences=occurrences[start_occurrence:]
            pass
        if max_occurrences<len(occurrences):
            occurrences=occurrences[:max_occurrences]
        return occurrences

    #f add_csv_event_type
    def add_csv_event_type( self, row ):
        if len(row)<5: return
        module = row[2]
        module_log_type = row[3].split("=")
        module_log_number      = int(module_log_type[0])
        module_log_text_reason = module_log_type[1][1:-1]
        num_args = row[4]
        arguments = row[5:]
        self.log_events[ (module,module_log_number) ] = c_log_event( self, module, module_log_number, module_log_text_reason, num_args, arguments )

    #f add_csv_event_occurrence
    def add_csv_event_occurrence( self, row ):
        if len(row)<5: return
        timestamp = int(row[0])
        occurrence_number = int(row[1])
        module = row[2]
        module_log_number = int(row[3])
        num_args = int(row[4])
        try:
            arguments = []
            for i in range(num_args):
                arguments.append(int(row[5+i],16))
                pass
            pass
        except ValueError:
            arguments = None
            pass
        if arguments is None:
            return
        k = (module,module_log_number)
        if k not in self.log_events.keys():
            return
        self.log_events[k].add_occurrence( timestamp = timestamp,
                                           number    = occurrence_number,
                                           arguments = arguments )

    #f open
    def open( self, filename ):
        f = open(filename)
        if f is None:
            return None
        self.filename = filename
        csv_row_iterator = csv.reader( f, dialect=csv.excel )
        for row in csv_row_iterator:
            if row[0][0] == '#':
                try:
                    self.add_csv_event_type( row )
                    pass
                except:
                    print "Failed to add log file event header row",row
                    print traceback.format_exc()
                    pass
            else:
                try:
                    self.add_csv_event_occurrence( row )
                    pass
                except:
                    print "Failed to add log file event row",row
                    print traceback.format_exc()
                    pass
            pass
        f.close()
        self.update()
        return True


#a Log file config loading
#c c_log_config
class c_log_config( object ):
    #f __init__
    def __init__( self, config_path=None ):
        self.config_path = config_path
        self.reset()
        pass
    #f reset
    def reset(self):
        self.config_filename = None
        self.config_files = {}
        self.config_sections = {}
        self.results = {"module_aliases":{}, "filters":{} }
        return

    #f load_file
    def load_file( self, config_filename ):
        self.reset()
        self.filename = config_filename
        if not self.read_config_files( config_filename ):
            self.reset()
            return False
        rc = self.parse_config()
        print self
        return rc

    #f __repr__
    def __repr__( self ):
        result = ""
        result += """Log config\n"""
        result += """  config files:\n"""
        for x in self.config_files:
            result += """    """+x+":"+str(self.config_files[x])+"\n"
            pass
        result += """  config sections:\n"""
        for x in self.config_sections:
            result += """    """+x+":"+str(self.config_sections[x])+"\n"
            pass
        result += """  module_aliases:\n"""
        for x in self.results["module_aliases"]:
            result += """    """+x+":"+str(self.results["module_aliases"][x])+"\n"
            pass
        result += """  filters:\n"""
        for x in self.results["filters"]:
            result += """    """+x+":"+str(self.results["filters"][x])+"\n"
            pass
        return result

    #f file_on_path
    def file_on_path( self, filename, path ):
        for p in path.split(":"):
            pf = os.path.join(p,filename)
            if os.path.exists( pf ):
                return pf
            pass
        return None

    #f open_config_file
    def open_config_file( self, filename ):
        try:
            f = open(filename)
        except:
            path_filename = self.file_on_path( filename, self.config_path )
            if path_filename is None:
                raise
            f = open(path_filename)
            pass
        return f
    #f read_config_files
    def read_config_files( self, filename, id="top" ):
        """
        Read and parse a config file; if it has been parsed before, then assume it is already done
        Return True if okay, False if not
        """
        if filename in self.config_files: return True
        config_file = self.open_config_file(filename)
        config = ConfigParser.RawConfigParser()
        config.readfp( config_file )
        config_file.close()
        self.config_files[ filename ] = config
        if config.has_section("include"):
            include_files = config.items("include")
            for (include_id,include_filename) in include_files:
                self.read_config_files( id=include_id, filename=eval(include_filename) )
            pass
        for s in config.sections():
            self.config_sections[id+":"+s] = (filename, config)
            pass
        return True

    #f parse_config
    def parse_config( self ):
        config_data    = { "alias":"", "path":"" }
        return self.read_config( "top", "main", config_data )

    #f config_from_id_and_section
    def config_from_id_and_section( self, id, section ):
        if ':' in section:
            id      = section[:section.index(':')]
            section = section[section.index(':')+1:]
            pass

        full_section=id+":"+section
        if full_section not in self.config_sections:
            raise Exception, "Failed to find section '%s' in log file configuration"%(full_section)

        return (id,section,self.config_sections[full_section][1])

    #f read_config_filter
    def read_config_filter( self, id, section, config_data ):
        (id,section,config) = self.config_from_id_and_section( id, section )

        if config.has_option(section,"filters"):
            for f in eval(config.get(section, "filters")):
                if not self.read_config_filter( id, f, config_data ):
                    return False
                pass
            pass

        if config.has_option(section,"filter"):
            filter = eval(config.get(section, "filter"))
            self.results["filters"][section] = filter
            pass
        return True
    
    #f read_config
    def read_config( self, id, section, config_data ):
        (id,section,config) = self.config_from_id_and_section( id, section )

        config_data = copy.copy(config_data)
        if config.has_option(section,"root"):
            config_data["path"] = config_data["path"]+config.get(section, "root")+"."
            pass

        if config.has_option(section,"components"):
            components = eval(config.get(section, "components"))
            for (subsection,instance_data_list) in components.iteritems():
                for instance_data in instance_data_list:
                    alias = config_data["alias"]
                    if "alias" in instance_data.keys(): alias = config_data["alias"]+instance_data["alias"]+"."
                    path = config_data["path"]
                    if "path" in instance_data.keys(): path = config_data["path"]+instance_data["path"]+"."
                    instance_config_data = copy.copy(config_data)
                    instance_config_data["alias"] = alias
                    instance_config_data["path"] = path
                    if not self.read_config( id, subsection, instance_config_data ):
                        return False
                    pass
                pass
            pass

        if config.has_option(section,"module_aliases"):
            for (m,ma) in eval(config.get(section, "module_aliases")):
                self.results["module_aliases"][config_data["path"]+m] = config_data["alias"]+ma
                pass
            pass

        if config.has_option(section,"filters"):
            for f in eval(config.get(section, "filters")):
                if not self.read_config_filter( id, f, config_data ):
                    return False
                pass
            pass

        return True

    #f module_aliases
    def module_aliases( self ):
        return self.results["module_aliases"]

    #f filters
    def filters( self ):
        return self.results["filters"]

