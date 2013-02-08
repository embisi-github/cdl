#!/usr/bin/env python2.7

#a Imports
import Queue
import threading
import SocketServer
import socket
import time
import sys
import traceback
#import curses.ascii
#import curses.has_key
#import curses
#curses.setupterm()
import re
if not hasattr(socket, 'SHUT_RDWR'):
    socket.SHUT_RDWR = 2

#a Globals
__all__ = ["TelnetHandler", "TelnetCLIHandler"]

#c c_telnet_protocol_codes
class c_telnet_protocol_codes:
    IAC  = chr(255) # "Interpret As Command"
    DONT = chr(254)
    DO   = chr(253)
    WONT = chr(252)
    WILL = chr(251)
    theNULL = chr(0)

    SE  = chr(240)  # Subnegotiation End
    NOP = chr(241)  # No Operation
    DM  = chr(242)  # Data Mark
    BRK = chr(243)  # Break
    IP  = chr(244)  # Interrupt process
    AO  = chr(245)  # Abort output
    AYT = chr(246)  # Are You There
    EC  = chr(247)  # Erase Character
    EL  = chr(248)  # Erase Line
    GA  = chr(249)  # Go Ahead
    SB =  chr(250)  # Subnegotiation Begin

    BINARY = chr(0) # 8-bit data path
    ECHO = chr(1) # echo
    RCP = chr(2) # prepare to reconnect
    SGA = chr(3) # suppress go ahead
    NAMS = chr(4) # approximate message size
    STATUS = chr(5) # give status
    TM = chr(6) # timing mark
    RCTE = chr(7) # remote controlled transmission and echo
    NAOL = chr(8) # negotiate about output line width
    NAOP = chr(9) # negotiate about output page size
    NAOCRD = chr(10) # negotiate about CR disposition
    NAOHTS = chr(11) # negotiate about horizontal tabstops
    NAOHTD = chr(12) # negotiate about horizontal tab disposition
    NAOFFD = chr(13) # negotiate about formfeed disposition
    NAOVTS = chr(14) # negotiate about vertical tab stops
    NAOVTD = chr(15) # negotiate about vertical tab disposition
    NAOLFD = chr(16) # negotiate about output LF disposition
    XASCII = chr(17) # extended ascii character set
    LOGOUT = chr(18) # force logout
    BM = chr(19) # byte macro
    DET = chr(20) # data entry terminal
    SUPDUP = chr(21) # supdup protocol
    SUPDUPOUTPUT = chr(22) # supdup output
    SNDLOC = chr(23) # send location
    TTYPE = chr(24) # terminal type
    EOR = chr(25) # end or record
    TUID = chr(26) # TACACS user identification
    OUTMRK = chr(27) # output marking
    TTYLOC = chr(28) # terminal location number
    VT3270REGIME = chr(29) # 3270 regime
    X3PAD = chr(30) # X.3 PAD
    NAWS = chr(31) # window size
    TSPEED = chr(32) # terminal speed
    LFLOW = chr(33) # remote flow control
    LINEMODE = chr(34) # Linemode option
    XDISPLOC = chr(35) # X Display Location
    OLD_ENVIRON = chr(36) # Old - Environment variables
    AUTHENTICATION = chr(37) # Authenticate
    ENCRYPT = chr(38) # Encryption option
    NEW_ENVIRON = chr(39) # New - Environment variables
    NOOPT = chr(0)

    # Codes used in SB SE data stream for terminal type negotiation
    IS = chr(0)
    SEND = chr(1)

#a Classes
#c c_telnet_input_stream
class c_telnet_input_stream( object ):
    #f __init__
    def __init__( self, socket, input_data_callback ):
        self.sock = socket
        self.input_data_callback = input_data_callback
        self.rawq = ""
        self.iacseq = None
        self.sbdataq = None
        self.thread = None
    #f spawn
    def spawn( self ):
        self.thread = threading.Thread( target=self.main_thread )
        self.thread.setDaemon( True ) # Why?
        self.thread.start()

    #f fetch_more_data
    def fetch_more_data(self, block=True):
        """Fetch more data from the socket"""
        if block == False:
            if select.select([self.sock.fileno()], [], [], 0) == ([], [], []):
                return ''
        ret = self.sock.recv(1024)
        if len(ret)==0:
            raise EOFError
        return ret

    #f getc
    def getc(self, block=True):
        """Get one character from the raw queue. Optionally blocking.
        Raise EOFError on end of stream.
        """
        if len(self.rawq)==0:
            self.rawq = self.fetch_more_data( block )
        if len(self.rawq)==0:
            return ''
        ret = self.rawq[0]
        self.rawq = self.rawq[1:]
        return ret

    #f ungetc
    def ungetc(self, char):
        """Put characters back onto the head of the rawq"""
        self.rawq = char + self.rawq

    #f store
    def store(self, char):
        """Store characters"""
        if self.sbdataq is not None:
            self.sbdataq = self.sbdataq + char
        else:
            self.input_data_callback( text=char )

    #f main_thread
    def main_thread(self):
        """
        Set self.eof when connection is closed.  Don't block unless in
        the midst of an IAC sequence.
        """
        # Possibly we should wait for a 'go' signal from the parent here
        try:
            while True:
                c = self.getc( block=True )
                if self.iacseq is None:
                    if c == c_telnet_protocol_codes.IAC:
                        self.iacseq = ""+c
                        continue
                    elif c == chr(13) and (self.sbdataq is None):
                        c2 = self.getc(block=False)
                        if c2 == c_telnet_protocol_codes.theNULL or c2 == '':
                            c = chr(10)
                        elif c2 == chr(10):
                            c = c2
                        else:
                            self.ungetc(c2)
                            c = chr(10)
                    self.store(c)
                elif len(self.iacseq) == 1:
                    'IAC: IAC CMD [OPTION only for WILL/WONT/DO/DONT]'
                    if c in (c_telnet_protocol_codes.DO, c_telnet_protocol_codes.DONT, c_telnet_protocol_codes.WILL, c_telnet_protocol_codes.WONT):
                        self.iacseq += c
                        continue
                    self.iacseq = None
                    if c == c_telnet_protocol_codes.IAC:
                        self.store(c)
                    else:
                        if c == c_telnet_protocol_codes.SB: # SB ... SE start.
                            self.sbdataq = ''
                        elif c == c_telnet_protocol_codes.SE: # SB ... SE end.
                            self.input_data_callback( sb_seq=self.sbdataq )
                            self.sbdataq = None
                elif len(self.iacseq) == 2:
                    self.input_data_callback( iac_seq=(self.iacseq[1], c) )
                    self.iacseq = None
        except EOFError:
            pass

#c c_telnet_server
class c_telnet_server(SocketServer.BaseRequestHandler):
    """
    A simple telnet server

    This can be extended with, for example, a better 'readline'
    """

    #b Variables
    # Options supported - tuple is (server support, client should perform)
    supported_options = { c_telnet_protocol_codes.ECHO        : (True,  False),
                          c_telnet_protocol_codes.SGA         : (True,  True),
                          c_telnet_protocol_codes.NEW_ENVIRON : (False, True),
                          c_telnet_protocol_codes.NAWS        : (None,  False),
                          c_telnet_protocol_codes.TTYPE       : (None,  True),
                          c_telnet_protocol_codes.LINEMODE    : (None,  False),
                          }

    #self.CODES['DEOL']     = curses.tigetstr('el')
    #self.CODES['DEL']      = 
    #self.CODES['INS']      = curses.tigetstr('ich1')
    #self.CODES['CSRLEFT']  = curses.tigetstr('cub1')
    #self.CODES['CSRRIGHT'] = curses.tigetstr('cuf1')

    #f __init__
    def __init__(self, request, client_address, server):
        self.DOECHO = True

        self.sock = None    # TCP socket
        self.cookedq = []    # This is the cooked input stream (list of charcodes)
        self.sbdataq = ''    # Sub-Neg string
        self.eof = 0        # Has EOF been reached?
        self.OQUEUELOCK = threading.Lock()
        self.input_queue = Queue.Queue()
        self.input_text_buffer = ""

        SocketServer.BaseRequestHandler.__init__(self, request, client_address, server)

    #f input_data_callback
    def input_data_callback( self, text=None, eof=None, sb_seq=None, iac_seq=None ):
        self.input_queue.put( (text, eof, sb_seq, iac_seq) )
        return

    #f getc
    def getc(self, block=True):
        """
        Process the front of the input queue, and if a list of characters, return one of those
        """
        while len(self.input_text_buffer)==0:
            try:
                input_item = self.input_queue.get( block=block, timeout=None )
                pass
            except:
                return ''
            (text, eof, sb_seq, iac_seq) = input_item
            if text is not None:
                self.input_text_buffer = text
            if eof: raise EOFError
            if sb_seq: pass # Used to interpret TTYPE IS
            if iac_seq:
                self.handle_iac_seq( iac_seq )
                pass
            pass
        c = self.input_text_buffer[0]
        self.input_text_buffer = self.input_text_buffer[1:]
        return c

    #f send_text
    def send_text(self, text):
        """Put data directly into the output queue (bypass output cooker)"""
        self.OQUEUELOCK.acquire()
        self.sock.sendall(text)
        self.OQUEUELOCK.release()

    #f write_untranslated
    def write_untranslated( self, text_list ):
        r = ""
        for t in text_list:
            r = r + t
            pass
        self.send_text( r )

    #f write
    def write(self, text):
        """Send a packet to the socket. This function cooks output."""
        text = text.replace(c_telnet_protocol_codes.IAC, c_telnet_protocol_codes.IAC+c_telnet_protocol_codes.IAC)
        text = text.replace(chr(10), chr(13)+chr(10))
        self.send_text(text)

    #f send_server_command
    server_options_sent = {}
    def send_server_command(self, opt, will=True):
        """
        Send a telnet will/wont command
        """
        if not self.server_options_sent.has_key(opt):
            self.server_options_sent[opt] = ''
            pass
        if (will and (self.server_options_sent[opt] != True)):
            self.server_options_sent[opt] = will
            self.write_untranslated( [c_telnet_protocol_codes.IAC, c_telnet_protocol_codes.WILL, opt ])
            pass
        elif (not will and (self.server_options_sent[opt] != False)):
            self.server_options_sent[opt] = will
            self.write_untranslated( [c_telnet_protocol_codes.IAC, c_telnet_protocol_codes.WONT, opt ])
            pass
        return

    #f send_client_command
    client_options_sent = {}
    def send_client_command(self, opt, do=True):
        """
        Send a telnet do/dont command
        """
        if not self.client_options_sent.has_key(opt):
            self.client_options_sent[opt] = ''
            pass
        if (do and (self.client_options_sent[opt] != True)):
            self.client_options_sent[opt] = do
            self.write_untranslated( [c_telnet_protocol_codes.IAC, c_telnet_protocol_codes.DO, opt ])
            pass
        elif (not do and (self.client_options_sent[opt] != False)):
            self.client_options_sent[opt] = do
            self.write_untranslated( [c_telnet_protocol_codes.IAC, c_telnet_protocol_codes.DONT, opt ])
            pass
        return

    #f send_command
    def send_command(self, cmd, opt=None):
        """
        Send a telnet command (IAC)
        """
        if cmd in [c_telnet_protocol_codes.DO]:
            self.send_client_command( opt=opt, do=True )
        elif cmd in [c_telnet_protocol_codes.DONT]:
            self.send_client_command( opt=opt, do=False )
        elif cmd in [c_telnet_protocol_codes.WILL]:
            self.send_server_command( opt=opt, willo=True )
        elif cmd in [c_telnet_protocol_codes.WONT]:
            self.send_server_command( opt=opt, will=False )
        else:
            self.write_untranslated( [c_telnet_protocol_codes.IAC, cmd] )

    #f send_options
    def send_options( self ):
        for (k, (srv, cln)) in self.supported_options.iteritems():
            if srv is not None:
                self.send_server_command( opt=k, will=srv )
                pass
            if cln is not None:
                self.send_client_command( opt=k, do=srv )
                pass
            pass
        pass

    #f handle_iac_seq
    def handle_iac_seq( self, iac_seq ):
        (cmd, opt) = iac_seq
        if cmd in [c_telnet_protocol_codes.WILL,c_telnet_protocol_codes.WONT]:
            if self.client_options_sent.has_key(opt):
                self.send_client_command( opt, self.client_options_sent[opt] )
            else:
                self.send_client_command( opt, do=False )
        elif cmd in [c_telnet_protocol_codes.DO,c_telnet_protocol_codes.DONT]:
            if self.server_options_sent.has_key(opt):
                self.send_server_command( opt, will=self.server_options_sent[opt] )
            else:
                self.send_client_command( opt, will=False )
            if opt == c_telnet_protocol_codes.ECHO:
                self.DOECHO = (cmd == c_telnet_protocol_codes.DO)
    #f setup
    def setup(self):
        """ Invoked when a client connects
        We grab the socket for the future, and send back our capabilties
        Then we spawn a new thread to take input data from the socket and convert it into
        This thread indicates completion with a message on the input queue
        Then we can return - the 'handle' routine will be called by the superclass to continue processing
        """
        self.sock = self.request._sock

        self.send_options()

        self.telnet_input_stream = c_telnet_input_stream( socket=self.sock, input_data_callback=self.input_data_callback )
        self.telnet_input_stream.spawn()

    #f finish
    def finish(self):
        "End this session"
        self.sock.shutdown(socket.SHUT_RDWR)

    #f readline
    def readline(self, echo=None):
        """
        Simple readline method
        This method interprets the characters it gets from the stream using getc
        getc() is expected to cope with the telnet control codes
        """
        #f _readline_echo
        def _readline_echo(char, echo=echo, self=self):
            if ((echo is None) and self.DOECHO) or echo:
                self.write(char)

        line = []
        while True:
            c = self.getc(block=True)
            if c == c_telnet_protocol_codes.theNULL:
                continue
            elif c == chr(4):
                return "console_exit();"
            elif c == chr(10):
                _readline_echo(c)
                return ''.join(line)
            elif c == chr(127) or c == chr(8):
                if len(line)>0:
                    #_readline_echo( curses.tigetstr('cub1') + curses.tigetstr('dch1') )
                    _readline_echo( chr(8) + chr(27) + chr(91) + chr(80) )
                    line = line[:-1]
                else:
                    _readline_echo(chr(7))
                continue
            elif ord(c) < 32:
                c = None
            else:
                _readline_echo(c)
            if c is not None:
                line.append(c)
                pass

    #f handleException
    def handleException(self, exc_type, exc_param, exc_tb):
        "Exception handler (False to abort)"
        self.write(traceback.format_exception_only(exc_type, exc_param)[-1]+chr(10))
        return True

    #f handle
    def handle(self):
        "The actual service to which the user has connected."
        while True:
            if self.DOECHO:
                self.write( "c_telnet_server > ")
            cmdlist = [item.strip() for item in self.readline().split()]
            print cmdlist

#a Toplevel
import code
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

class c_python_telnet_server(c_telnet_server):
    #f handle
    def handle(self):
        std_files = (sys.stdin, sys.stdout, sys.stderr)
        console = c_telnet_python_interactive_console( telnet_server=self, locals={"telnet":self} )
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
        
if __name__ == '__main__':
    "Testing - Accept a single connection"
    class TNS(SocketServer.TCPServer):
        allow_reuse_address = True

    #tns = SocketServer.TCPServer(("127.0.0.1", 8023), c_python_telnet_server)
    tns = TNS(("127.0.0.1", 8023), c_python_telnet_server)
    while 1:
        print "Tns server loop start"
        tns.handle_request()
    print "Tns server complete"
