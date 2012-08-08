#!/usr/bin/env python
import re
import getopt
import sys

#f usage
def usage(rc):
    print >>sys.stderr, r"""
cdl_timing --cdlh <cdl timing file> --hdr <designers timing header file>
    Other options

The program allows correction of timings for a designer header file
"""
    exit(rc)

#b read in options
# args is the list of options that need an argument - the text here is the default values
args={}
arg_options = ["cdlh", "hdr", "module", "overwrite"]
arg_shoptions = {"c":"cdlh", "h":"hdr", "m":"module"}

getopt_options = []
for x in arg_options:
    if x not in args.keys():
        args[x] = None
    getopt_options.append( x+"=" )

getopt_shopts = ""
for x in arg_shoptions.keys():
    getopt_shopts = getopt_shopts + x + ":"

try:
    (optlist,file_list) = getopt.getopt(sys.argv[1:],getopt_shopts,getopt_options)
except:
    print >>sys.stderr, "Exception parsing options:", sys.exc_info()[1]
    usage(4)

for opt,value in optlist:
    if opt[1:] in arg_shoptions.keys():
        opt = "--"+arg_shoptions[opt[1:]]
    if (opt[0:2]=='--') and (opt[2:] in arg_options):
        args[opt[2:]] = value
    else:
        print >>sys.stderr, "Unknown option", opt, "with value", value
        usage(4)

if args["cdlh"] is None:
    usage(4)
if args["hdr"] is None:
    usage(4)


class c_file_data( object ):
    def __init__( self ):
        self.file_data = []
        self.modules = {}
    def add_text( self, text ):
        self.file_data.append(text)
    def read_in_module_declaration( self, f, l ):
        module_name = None
        r = re.search(r"extern\s+module\s+([^\( \t]*)",l)
        if r is not None:
            module_name = r.group(1)
        declaration_header = [l]
        declaration_timing = []
        for l in f:
            if l.find("{")>=0:
                declaration_timing.append(l)
                break
            declaration_header.append(l)
        for l in f:
            declaration_timing.append(l)
            if l.find("}")>=0:
                break
        if module_name is not None:
            if module_name not in self.modules.keys():
                self.modules[module_name] = ( len(self.file_data), declaration_header, declaration_timing )
        self.file_data.append( ("module", module_name) )
    def read_in_modules( self, f ):
        for l in f:
            if l.find("extern ")>=0:
                self.read_in_module_declaration(f, l)
            else:
                self.add_text( l )
    def has_module( self, module_name ):
        return module_name in self.modules.keys()
    def module_timing( self, module_name ):
        if module_name not in self.modules.keys():
            return None
        return self.modules[module_name][2]
    def replace_timing( self, module_name, timing ):
        if module_name not in self.modules.keys():
            return
        self.modules[module_name] = (self.modules[module_name][0], self.modules[module_name][1], timing )
    def __repr__( self ):
        r = ""
        for m in self.modules.keys():
            r = r + ("Module '%s'\n"%m)
            for l in self.modules[m][1]:
                r = r + ("...%s"%l)
            r = r + "\n"
            for l in self.modules[m][2]:
                r = r + ("...%s"%l)
        return r
    def write_module( self, f, module_name ):
        for l in self.modules[module_name][1]:
            f.write(l)
        for l in self.modules[module_name][2]:
            f.write(l)

    def write( self, f ):
        for l in self.file_data:
            if (type(l)==type((1,2))):
                self.write_module(f,l[1])
            else:
                f.write(l)

overwrite = False
if args["overwrite"] is not None:
    overwrite = (args["overwrite"].lower()=="yes")

print "Opening cdlh file '%s'"%(args["cdlh"])
cdlh_f = open(args["cdlh"])

print "Opening hdr file '%s'"%(args["hdr"])
hdr_f  = open(args["hdr"])
cdl_fd = c_file_data()
cdl_fd.read_in_modules( cdlh_f )

hdr_fd = c_file_data()
hdr_fd.read_in_modules( hdr_f )
cdlh_f.close()
hdr_f.close()


module_name = args["module"]
if module_name is not None:
    print "Looking for module '%s' in the two files"%(module_name)
    if cdl_fd.has_module(module_name) and hdr_fd.has_module(module_name):
        print "Found module in both files"
        hdr_fd.replace_timing( module_name, cdl_fd.module_timing( module_name ) )
        if overwrite:
            print "Overwriting header file %s as requested"%(args["hdr"])
            hdr_f = open(args["hdr"],"w")
            hdr_fd.write(hdr_f)
            hdr_f.close()
        else:
            hdr_fd.write(sys.stdout)
    else:
        print "Did not find in both files"
        print "CDLH includes:"
        for m in cdl_fd.modules.keys():
            print "    '%s'"%m
        print "HDR includes:"
        for m in hdr_fd.modules.keys():
            print "    '%s'"%m



