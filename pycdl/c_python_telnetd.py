#!/usr/bin/env python2.7

#a Imports
import sys, copy
import code
import c_telnetd
import SocketServer

#a Classes
#c c_telnet_python_interactive_console
class c_telnet_python_interactive_console( code.InteractiveConsole ):
    def __init__( self, telnet_server, **kwargs ):
        self.telnet_server = telnet_server
        code.InteractiveConsole.__init__( self, **kwargs )
    def raw_input( self, prompt=None ):
        if prompt is not None:
            self.write(prompt)
            pass
        input_line = self.telnet_server.readline()
        return input_line
    def write( self, data ):
        self.telnet_server.write(data)
        pass

#c c_python_telnet_server_handler
class c_python_telnet_server_handler(c_telnetd.c_telnet_server):
    locals = {}
    #f handle
    def handle(self):
        std_files = (sys.stdin, sys.stdout, sys.stderr)
        locals = self.server.locals.copy()
        locals["console"] = self
        locals["server"] = self.server
        def console_exit(server=self.server):
            server.running = False
            exit()
        locals["console_exit"] = console_exit
        console = c_telnet_python_interactive_console( telnet_server=self, locals=locals )
        sys.stdout = console
        sys.stderr = console
        try:
            console.interact()
        except SystemExit:
            pass
        except:
            print >>std_files[2], traceback.format_exc()
            pass
        (sys.stdin, sys.stdout, sys.stderr) = std_files

#c c_python_telnet_server
class c_python_telnet_server( SocketServer.TCPServer ):
    allow_reuse_address = True
    def __init__( self, ip_address="127.0.0.1", port=8745, locals={} ):
        self.locals = copy.copy(locals)
        SocketServer.TCPServer.__init__( self, (ip_address, port), c_python_telnet_server_handler )
    def serve( self ):
        self.running = True
        while self.running:
            print "c_python_telnet_server loop start"
            self.handle_request()
        print "c_python_telnet_server complete"
        
#a Toplevel
if __name__ == '__main__':
    "Testing - Accept a single connection"
    dut = c_python_telnet_server( port=8023 )
    dut.serve()

