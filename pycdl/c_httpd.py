#a Imports
import BaseHTTPServer
from urlparse import urlparse
from cgi      import parse_qs
import os.path

#a Classes
#c c_http_request_handler
class c_http_request_handler( BaseHTTPServer.BaseHTTPRequestHandler ):
    #f __init__
    def __init__( self, request, client_address, server ):
        BaseHTTPServer.BaseHTTPRequestHandler.__init__( self, request, client_address, server )
        self.server = server
    #f do_GET - invoked when an http 'get' request comes in, with self.path set to the path
    def do_GET( self ):
        if self.server.verbose_level("get",8):
            print "do_GET of path '%s'"%(self.path)
        string = ""
        if (self.path=="/"):
            self.path = "/index.html"
        url_dict = urlparse(self.path)
        query_dict = parse_qs(url_dict.query)
        if self.server.verbose_level("get",8):
            print "do_GET url_dict %s"%(str(url_dict))
            print "do_GET query_dict %s"%(str(query_dict))
        if url_dict.query=="":
            file = None
            url_path = url_dict.path
            if url_path[0]=="/": url_path=url_path[1:]
            for fb in self.server.file_path:
                filename = os.path.join( fb, url_path )
                if self.server.verbose_level("get",12):
                    print "attempting to open file '%s'"%(filename)
                    pass
                if os.path.exists( filename ):
                    try:
                        file = open( filename )
                        pass
                    except:
                        pass
                if file is not None:
                    break
                pass
            if file is None:
                self.send_response(404)
                self.end_headers()
                self.wfile.write("File '%s' not found on server path '%s'\n"%(url_dict.path, str(self.server.file_path)))
                return
            self.send_response(200)
            self.send_header('Content-type', 'text')
            self.end_headers()
            string = ""
            while (1):
                line = file.readline()
                if (not line):
                    break
                string = string + line
                pass
            self.wfile.write(string+"\n")
        else:
            (value_type, value) = self.server.client_callback( self.server, url_dict, query_dict )
            if self.server.verbose_level("response",4):
                print (value_type, value)
            if (value_type=="text"):
                self.send_response(200)
                self.send_header('Content-type', 'text')
                self.end_headers()
                if type(value)==type([]):
                    for v in value:
                        self.wfile.write(v+"\n")
                else:
                    self.wfile.write(value+"\n")
            elif (value_type=="xml"):
                self.send_response(200)
                self.send_header('Content-type', 'text/xml')
                self.end_headers()
                self.wfile.write(value.toxml()+"\n")
            else:
                self.send_response(404)
                self.end_headers()
                self.wfile.write(string+"\n")

#c c_http_server
class c_http_server(BaseHTTPServer.HTTPServer ):
    def verbose_level( self, area, level=0 ):
        if area not in self.verbose.keys(): return False
        return self.verbose[area]>level

    def __init__( self, server_address, client_callback, file_path, verbose={} ):
        """
        verbose is a dictionary of things to be verbose about, and how verbose to be
        """
        self.client_callback = client_callback
        self.file_path = file_path
        self.verbose = verbose
        print "Starting http server on port",server_address[1]
        BaseHTTPServer.HTTPServer.__init__( self, server_address, c_http_request_handler )

    def serve( self ):
        self.running = True
        print "c_httpd loop start"
        while self.running:
            self.handle_request()
        print "c_httpd complete"
