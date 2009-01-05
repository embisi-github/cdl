/*a Copyright
  
  This file 'cyclicity.py' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
#!/usr/bin/python2

#a Imports
from Tkinter import *
from tkFileDialog import *
from tkMessageBox import *
import re
from math import *
import py_cyclicity

#a c_cyclicity_ui
class c_cyclicity_ui:
    #f __init__
    def __init__( self, root=None, cyclicity=None ):
        #b Grab the root Tk window if necessary
        if (root == None):
            root = Tk()
        self.root = root
        self.cyclicity = cyclicity
        
        #b Create menus
        self.main_menu = Menu( self.root )
        self.file_menu = Menu( self.main_menu )
        self.source_menu = Menu( self.main_menu )
        self.root.configure( menu=self.main_menu )
    
        self.main_menu.add_cascade( label="File", menu=self.file_menu )
        self.main_menu.add_cascade( label="Source", menu=self.source_menu )
    
        def local_file_open_source_file( self=self ):
            self.file_menu_open_source_file()
        def local_file_exit( self=self ):
            self.file_exit()
        self.file_menu.add_command( label="Open source file", command=local_file_open_source_file )
        self.file_menu.add_command( label="Quit", command=local_file_exit )

        #b Create source area
        def xv_cmd( a, b, self=self ):
            self.source_area_x_scroll.set( a, b )
        def yv_cmd( a, b, self=self ):
            self.source_area_y_scroll.set( a, b )
        self.source_area = Text( self.root, xscrollcommand=xv_cmd, yscrollcommand=yv_cmd, height=20 );
        def xsc_cmd( cmd, amt=None, units=None, self=self ):
            if (cmd == "moveto" ):
                self.source_area.xview_moveto( amt )
            elif (cmd == "scroll" ):
                self.source_area.xview_scroll( amt, units )
        def ysc_cmd( cmd, amt=None, units=None, self=self ):
            if (cmd == "moveto" ):
                self.source_area.yview_moveto( amt )
            elif (cmd == "scroll" ):
                self.source_area.yview_scroll( amt, units )
        self.source_area_x_scroll = Scrollbar( orient="horizontal", command=xsc_cmd )
        self.source_area_y_scroll = Scrollbar( orient="vertical", command=ysc_cmd )
        self.source_area.grid( row=0, column=0, sticky="nsew" );
        self.source_area_x_scroll.grid( row=1, column=0, sticky="ew" );
        self.source_area_y_scroll.grid( row=0, column=1, sticky="ns" );
        self.root.grid_columnconfigure( 0, weight=4 );
        self.root.grid_rowconfigure( 0, weight=4 );

        #b Create info area
        self.info_area = Text( self.root, height=5 );
        self.info_area.grid( row=2, column=0, columnspan=2, sticky="nsew" );
        self.root.grid_rowconfigure( 2, weight=1 );

    #f set_menus
    def set_menus( self ):
        self.source_menu.delete( 0, "end" )
        self.source_files_list = self.cyclicity.list_source_files()
        for i in range( 0, len(self.source_files_list) ):
            def local_select_source( number=i, self=self ):
                self.select_source( number )
            self.source_menu.add_command( label=self.source_files_list[i], command=local_select_source )
    #f open_source_file
    def open_source_file( self, filename=None ):
        if (filename):
            self.cyclicity.read_file( filename )
            self.set_menus()
    #f populate_info_window
    def populate_info_window( self, handle="" ):
        self.info_area.configure( state="normal" )
        self.info_area.delete( "0.0", "end" )
        self.info_handles = []
        i = 0
        while (handle):
            self.info_handles.append( handle )
            data = self.cyclicity.get_object_data( handle )
            self.info_area.insert( "0.0", data[0]+"\n", "line_"+str(i) )
            def bcmd( event, self=self, line=i ):
                self.highlight_handle_in_source( self.info_handles[line] )
            self.info_area.tag_bind( "line_"+str(i), "<Button-1>", bcmd )
            print handle, data
            # 2 is next link, 3 is parent
            handle = data[3]
            i = i+1
        self.info_area.configure( state="disabled" )
    #f highlight_handle_in_source
    def highlight_handle_in_source( self, handle ):
        data = self.cyclicity.get_object_data( handle )
        start = data[4]
        end = data[5]
        self.source_area.configure( state="normal" )
        self.source_area.tag_delete( "highlit" )
        self.source_area.tag_add( "highlit", "0.1 + "+str(start[1])+" chars", "0.1 + "+str(end[1])+" chars" )
        self.source_area.tag_configure( "highlit", background="red" )
        self.source_area.configure( state="disabled" )
    #f select_source
    def select_source( self, source_number=None ):
        if (source_number <> None):
            self.source_area.configure( state="normal" );
            self.source_area.delete( "0.0", "end" )
            length = self.cyclicity.get_file_length( source_number )
            self.source_area.insert( "0.0", self.cyclicity.read_file_data( source_number, 0, length) )
            self.source_area.configure( state="disabled" );
            def cmd( event=None, self=self, source_number=source_number ):
                line_n_char = self.source_area.search(".","@"+str(event.x)+","+str(event.y),regexp=1)
                line = 0
                char = 0
                decode = re.search( "(\S+)\.(\S+)", line_n_char )
                if (decode):
                    line = int(decode.group(1))-1
                    char = int(decode.group(2))-1
                file_pos = self.cyclicity.read_file_line_start_posn( source_number, line )+char
                handle = self.cyclicity.get_object_handle( source_number, file_pos )
                self.populate_info_window( handle )
                self.highlight_handle_in_source( handle )
            self.source_area.bind( sequence="<Button-1>", func=cmd );
    #f file_open_source
    def file_menu_open_source_file( self ):
        source_filename = askopenfilename(filetypes=[("source","*.cyc")]) 
        self.open_source_file( source_filename )
    #f file_exit
    def file_exit(self):
        self.root.destroy()
    
#main_menu.add_command( label="Cyclicity", command=menu_click )

#a Main
cyc = py_cyclicity.new()
cycui = c_cyclicity_ui( root=None, cyclicity=cyc )
cycui.open_source_file( filename="test/test.cyc" )
cycui.select_source( 0 )
#handle = cyc.get_toplevel_handle()
def build_list( handle ):
    items = []
    while (handle <> ""):
        data = cyc.get_data( handle )
        links = cyc.get_links( handle )
        subitems = []
        for link in links:
            if (link[3]==0):
                subitems.append([1,build_list(link[1])])
            else:
                subitems.append([0,link[1]])
        items.append([handle, subitems])
        handle = data[2] 
    return items

#print build_list(handle)

cycui.root.mainloop()


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

