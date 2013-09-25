#!/usr/bin/env python
# find /opt/local/share/ -name '*media*' -print

#a Copyright
#  
#  This file 'sim_gui.py' copyright Gavin J Stark 2003, 2004, 2007
#  
#  This is free software; you can redistribute it and/or modify it however you wish,
#  with no obligations
#  
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.

#a Handle arguments
import sys, getopt
args={}
args["lib_dir"] = []
args["sim_files"] = []
args["hw_file"] = ""
args["glade_dirs"] = []
(optlist,file_list)=getopt.getopt(sys.argv[1:],'',[ 'lib=', 'sim_file=', 'hw_file=', 'glade=' ])
for opt,value in optlist:
    if (opt in ('--lib',)):
        args["lib_dir"].append(value)
    if (opt in ('--glade',)):
        args["glade_dirs"].append(value)
    if (opt in ('--sim_file',)):
        args["sim_files"].append(value)
    if (opt in ('--hw_file',)):
        args["hw_file"] = value
for file in file_list:
    args["sim_files"].append(file)

#a Imports
import sys, platform, os
arch = ""
if (platform.system()=='Darwin'):
    arch = "osx"
elif (platform.system()=='Linux'):
    arch = "linux"
for lib in args["lib_dir"]:
    sys.path.append(lib+"/"+arch)
    sys.path.append(lib)

import os.path
import gobject, gtk, gtk.glade

imports = []
for file in args["sim_files"]:
    imports.append((file,__import__(file)))

def run_fns(type, arg):
    for file,imp in imports:
        try:
            fn = eval("imp."+file+type)
            fn(arg)
        except:
            i =  sys.exc_info()
            traceback.print_exception(i[0],i[1],i[2])

import traceback
run_fns("__init",[])

import py_engine

#a Statics
(
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE
) = range(3)

(
  SHAPE_SQUARE,
  SHAPE_RECTANGLE,
  SHAPE_OVAL,
) = range(3)

ui_info = \
'''<ui>
  <menubar name='MenuBar'>
    <menu action='FileMenu'>
      <menuitem action='New'/>
      <menuitem action='Open'/>
      <menuitem action='Save'/>
      <menuitem action='SaveAs'/>
      <separator/>
      <menuitem action='Quit'/>
    </menu>
    <menu action='PreferencesMenu'>
      <menu action='ColorMenu'>
        <menuitem action='Red'/>
        <menuitem action='Green'/>
        <menuitem action='Blue'/>
      </menu>
      <menu action='ShapeMenu'>
        <menuitem action='Square'/>
        <menuitem action='Rectangle'/>
        <menuitem action='Oval'/>
      </menu>
      <menuitem action='Bold'/>
    </menu>
    <menu action='HelpMenu'>
      <menuitem action='About'/>
    </menu>
  </menubar>
  <toolbar  name='ToolBar'>
    <toolitem action='Open'/>
    <toolitem action='Quit'/>
    <separator/>
    <toolitem action='Logo'/>
  </toolbar>
</ui>'''

menu_bar_info = \
'''<ui>
  <menubar name='MenuBar'>
    <menu action='FileMenu'>
      <menuitem action='Open'/>
      <menuitem action='Reset'/>
      <menuitem action='Step'/>
      <menuitem action='Fwd'/>
      <separator/>
      <menuitem action='Quit'/>
    </menu>
    <menu action='HelpMenu'>
      <menuitem action='About'/>
    </menu>
  </menubar>
  <toolbar  name='ToolBar'>
    <toolitem action='Open'/>
    <toolitem action='Reset'/>
    <toolitem action='StepBackMany'/>
    <toolitem action='StepBack'/>
    <toolitem action='Step'/>
    <toolitem action='StepMany'/>
    <toolitem action='Fwd'/>
    <toolitem action='Ckpt'/>
    <toolitem action='Rcvr'/>
    <toolitem action='Print'/>
    <toolitem action='Quit'/>
  </toolbar>
</ui>'''


#a register_stock_icons
# It's totally optional to do this, you could just manually insert icons
# and have them not be themeable, especially if you never expect people
# to theme your app.
def register_stock_icons():
    ''' This function registers our custom toolbar icons, so they
        can be themed.
    '''
    items = [('demo-gtk-logo', '_GTK!', 0, 0, '')]
    # Register our stock items
    gtk.stock_add(items)

    # Add our custom icon factory to the list of defaults
    factory = gtk.IconFactory()
    factory.add_default()

    import os
    img_dir = os.path.join(os.path.dirname(__file__), 'images')
    img_path = os.path.join(img_dir, 'gtk-logo-rgb.gif')
    try:
        pixbuf = gtk.gdk.pixbuf_new_from_file(img_path)

        # Register icon to accompany stock item

        # The gtk-logo-rgb icon has a white background, make it transparent
        # the call is wrapped to (gboolean, guchar, guchar, guchar)
        transparent = pixbuf.add_alpha(True, chr(255), chr(255),chr(255))
        icon_set = gtk.IconSet(transparent)
        factory.add('demo-gtk-logo', icon_set)

    except gobject.GError, error:
        print 'failed to load GTK logo for toolbar'

#a EngineState - complete state of the simulation engine for easy access, in tree and hashed format
# state entries are
#  (type, module, prefix, state_name within module, size0, size 1, offset in to module, full_path )
# e.g.
#  (
#
# A-->B-->C
#  |   +->D
#  +->E-->F
#  +->G
#
# state_tree entries are:
# ( <name part>, [ <list of state tree entries for children> ], <state entry for this> )
#
# has state_tree:
#
# [ ( 'A', [
#            ( 'B', [
#                    ( 'C', None, (state entry for 'A.B.C') ),
#                    ( 'D', None, (state entry for 'A.B.D') )
#                   ], None ),
#            ( 'E', [
#                   ( 'F', None, (state entry for 'A.E.F') )
#                   ], None ),
#            ( 'G', None, (state entry for 'A.G') )
#           ], None )
# ]
class EngineState:
    def __init__(self, engine):
        self.engine = engine
        insts = engine.list_instances()
        self.states = []
        for inst in insts:
            cont = 1
            j = 0
            # get_state_info returns type (int), module, prefix, state_name, size 0, size 1 - we add 
            while (cont):
                cont = 0
                stin = engine.get_state_info( inst[0], j )
                if (stin!=None):
                    self.states.append(stin+[j,stin[1]+"."+stin[3]])
                    cont = 1
                j = j+1
        self.states.sort()
        #b add_to_state_tree - nodes are ( id, [ nodes ], state )
        def add_to_state_tree( state_tree, id_split, state ):
            for branch in state_tree:
                if (branch[0]==id_split[0]):
                    if (len(id_split)<2):
                        branch = (branch[0], branch[1], state )
                    else:
                        branch = (branch[0], add_to_state_tree( branch[1], id_split[1:], state), branch[2])
                    return state_tree
            if (len(id_split)<2):
                state_tree.append( (id_split[0], None, state) )
            else:
                state_tree.append( (id_split[0], add_to_state_tree( [], id_split[1:], state ), None ) )
            return state_tree
        #b Create state tree
        self.state_tree = []
        for state in self.states: # state[7] is the full path of the state
            self.state_tree = add_to_state_tree( self.state_tree, state[7].split('.'), state )
        #b enumerate_state_tree
        def enumerate_state_tree( tree, hierarchy, entry, path ):
            if (tree==None):
                return
            if (len(tree)<=entry): # Should only come from 'next' path
                return
            if (hierarchy):
                here = hierarchy+"."+tree[entry][0]
            else:
                here = tree[entry][0]
            self.name_part[path] = tree[entry][0]
            self.path_of_hierarchy[ here ] = path
            #print path, here, tree[entry][2]
            children = tree[entry][1]
            if (children): # Have children? Enumerate them
                self.n_children_of_path[path] = len(children)
                enumerate_state_tree(children, here, 0, path+(0,))
            else:
                self.n_children_of_path[path] = 0
            if (tree[entry][2]): # Have state? Enumerate that
                state = tree[entry][2]
                self.state_of_path[ path ] = state
            # Enumerate next at this level
            return enumerate_state_tree( tree, hierarchy, entry+1, path[:-1]+(path[-1]+1,) )
        self.name_part = {}
        self.n_children_of_path = {}
        self.state_of_path = {}
        self.path_of_hierarchy = {}
        enumerate_state_tree( self.state_tree, None, 0, (0,) )
        self.text_from_state_fns = {}
        def hex( args ):
            if (args != None):
                if (args[0][0]=="bits"):
                    return args[0][1]
                print "hex wanted from ",args[0]
                return str(args[0])
            return "<None>"
        self.register_text_from_state_fn( None, "hex", hex )
    #f register_text_from_state_fn
    def register_text_from_state_fn( self, fn_list, text, fn ):
        if (not fn_list):
            fn_list = self.text_from_state_fns
        fn_list[text] = fn
    #b get_name_part - from a path (a,b,c,d,...,f) return the name part that it belongs to
    def get_name_part( self, path ):
        if (self.name_part.has_key(path)):
            return self.name_part[path]
        return None
    #b get_entry - from a path (a,b,c,d,...,f) return a node or None
    def get_entry( self, path ):
        if (self.state_of_path.has_key(path)):
            return self.state_of_path[path]
        return None
    #b get_path_of_hierarchy - from a hierarchical name, get the path
    def get_path_of_hierarchy( self, path ):
        if (self.path_of_hierarchy.has_key(path)):
            return self.path_of_hierarchy[path]
        return None
    #b get_n_children - return number of children of a path
    def get_n_children( self, path ):
        if (path==None):
            return len(self.state_tree)
        if (self.n_children_of_path.has_key(path)):
            return self.n_children_of_path[path]
        return 0
    #b within_tree - return 1 if within the tree (i.e. if the node has children or is a valid leaf)
    def within_tree( self, path ):
        if (self.n_children_of_path.has_key(path)):
            return 1
        return 0
    #b call_foreach - call a callback for each state item; callback is called with path and state
    def call_foreach( self, callback ):
        for path in self.state_of_path.keys():
            callback( path, self.state_of_path[path] )
    #b text_from_state_fn - text from a textual input (function) and a hierarchy location
    def text_from_state_fn( self, fn_list, hierarchy, fn ):
        if (not fn_list):
            fn_list = self.text_from_state_fns
        if (fn[-1]!=')'):
            return "Expected ) at end of fn"
        fn = fn[:-1]
        fn_name = fn.split('(')[0]
        if (not (fn_list.has_key(fn_name))):
            return "Unknown fn "+fn_name
        arg_list = ()
        for state_name in fn[len(fn_name)+1:].split(','):
            path = self.get_path_of_hierarchy( hierarchy+"."+state_name )
            value = ("text",state_name)
            if (path):
                state = self.get_entry( path )
                if (state):
                    if (state[0]==1): # state bits
                        value = ("bits",self.engine.get_state_value_string(state[1], state[6]))
                    elif (state[0]==2): # state memory
                        value = ("memory",state[1],state[6],state[4],state[5])
            arg_list = arg_list + (value,)
        return fn_list[fn_name](arg_list)
    #b text_from_state - text from a textual input (format string) and a hierarchy location
    def text_from_state( self, fn_list, hierarchy, text ):
        result = ""
        while (len(text)>0):
            if (text[0]=='$'):
                fn = ""
                text=text[1:]
                while ((len(text)>0) and (text[0]!='$')):
                    fn = fn + text[0]
                    text=text[1:]
                result = result + self.text_from_state_fn( fn_list, hierarchy, fn )
                if (len(text)>0):
                    text=text[1:]
            else:
                result = result + text[0]
                text = text[1:]
        return result

#a EngineStateTreeModel
class EngineStateTreeModel(gtk.GenericTreeModel):
    #b __init__
    def __init__(self, engine_state):
        self.engine_state = engine_state
        self.clk = 0
        gtk.GenericTreeModel.__init__(self)
    #b on_get_flags - not a clue
    def on_get_flags(self):
        return 0
    #b on_get_n_columns - get number of columns for the tree view
    def on_get_n_columns(self):
        return 2
    #b on_get_column_type - return type of a column from an index 0..n_columns-1
    def on_get_column_type(self, index):
        return gobject.TYPE_STRING
    #b on_get_path - return the path from one of our iter, which is a path
    def on_get_path(self, iter):
        return iter
    #b on_get_iter - return an external iter from a path - the path, if it exists
    def on_get_iter(self, path):
        if (self.engine_state.within_tree( path )):
            return path
        return None
    #b on_get_value - return the value of an iter -
    def on_get_value(self, iter, column):
        if (column==0):
            return self.engine_state.get_name_part( iter )
        state = self.engine_state.get_entry( iter )
        if (state):
            if (state[0]==1):
                #return str(iter)+":"+str(state)+":"+self.engine_state.engine.get_state_value_string(state[1], state[6])
                return self.engine_state.engine.get_state_value_string(state[1], state[6])
            return "How to do state"+str(state[0])+"? ("+str(state[3])+")"
        return ""
    #b on_iter_next - get next iter from current iter at same level - we do this from the path
    def on_iter_next(self, iter):
        if (iter==None):
            return None
        return self.on_get_iter( iter[:-1] +(iter[-1]+1,) )
    #b on_iter_children - return first child of an external iter
    def on_iter_children(self, iter):
        if (iter==None):
            iter = (0,)
        else:
            iter = iter + (0,)
        return self.on_get_iter( iter )
    #b on_iter_has_child - return 1 if there is a child of the iter - try it!
    def on_iter_has_child(self, iter):
        if (self.on_iter_n_children(iter)>0):
            return 1
        return 0
    #b on_iter_n_children - return number of children of the external iter, i.e. length of subiters if not None
    def on_iter_n_children(self, iter):
        return self.engine_state.get_n_children( iter )
    #b on_iter_nth_child - return nth child of an external iter - i.e entry of path,n
    def on_iter_nth_child(self, iter, n):
        if (iter==None):
            iter = (n,)
        else:
            iter = iter + (n,)
        return self.on_get_iter( iter )
    #b on_iter_parent - return parent of an external iter, i.e. entry of path with last step dropped
    def on_iter_parent(self, iter):
        if (iter==None):
            return None
        return self.on_get_iter( iter[:-1] )
    #b all_state_changed
    def all_state_changed( self ):
        def state_changed( path, state ):
            self.row_changed(path,self.get_iter(path))
        self.engine_state.call_foreach( state_changed )
        self.clk = self.clk+1

#a StateWidget
class StateWidget(gtk.Frame):
    #b __init__
    def __init__(self, engine_state, parent=None):
        #b Set variables
        self.engine_state = engine_state

        #b Standard configuration
        gtk.Frame.__init__(self)

        #b Set up the text tree
        self.model = EngineStateTreeModel(self.engine_state)
        tree_view = gtk.TreeView(self.model)
        try:
            tree_view.set_enable_tree_lines(True)
        except:
            tree_view = tree_view
        cell = gtk.CellRendererText()
        # the text in the columns comes from columns 0 and 1
        column = gtk.TreeViewColumn("state", cell, text=0)
        tree_view.append_column(column)
        column = gtk.TreeViewColumn("content", cell, text=1)
        tree_view.append_column(column)

        #scrolled_window = gtk.ScrolledWindow()
        #scrolled_window.add_with_viewport(tree_view)
        #self.add(scrolled_window)
        self.add(tree_view)
        self.show_all()
    #b all_state_changed
    def all_state_changed( self ):
        self.model.all_state_changed()

#a EngineDisplayWidgets
# This class builds a visual representation of the contents of a module (or submodule) within a simulation
# It should be initialized with a file or buffer of glade XML, and the toplevel of the hierarchy that it represents
# The glade XML file should contain a set of windows, each of which is a different representation of the module
# The class instance can then be interrogated to get the number of representations, their types (from the window title), and they can be instanced and destroyed
# No interaction is currently possible with the module through these windows (e.g. entry boxes do not effect the state of the simulation engine)
class EngineDisplayWidgets:
    #b __init__
    def __init__(self, engine_state, hierarchy, file=None, buffer=None ):
        #b Copy variables to class
        self.hierarchy = hierarchy
        self.engine_state = engine_state
        #b Read in glade file
        self.glade = None
        if (file):
            self.glade = gtk.glade.XML( file )
        #b Determine which widgets need our help being updated
        self.toplevels = []
        self.fields_needing_updates = []
        def parse_user_value_text( text ):
            user_text = text
            value_text = None
            if (len(text)>0):
                if (text[0]=='%'):
                    end = text.find('%',1)
                    if (end>1):
                        value_text = text[1:end-1]
                        user_text = text[end+1:]
                    else:
                        value_text = text[1:]
                        user_text = value_text
            return (user_text, value_text)
        for widget in self.glade.get_widget_prefix(""):
            #b Check widget type
            class_path = widget.class_path().split('.')
            #b Window
            if (class_path[-1]=="GtkWindow"):
                self.toplevels.append((widget.path(),widget,hierarchy+"."+widget.path()))
            #b Label
            elif (class_path[-1]=="GtkLabel"):
                text = widget.get_text()
                (user_text,value_text) = parse_user_value_text(text)
                if (value_text):
                    self.fields_needing_updates.append( ("label", widget, value_text) )
                    widget.set_text( user_text )
            #b Combox box - use for enum
            elif (class_path[-1]=="GtkComboBox"):
                try:
                    text = widget.get_title()
                except:
                    text = "<Old version of gtk>"
                (user_text,value_text) = parse_user_value_text(text)
                if (value_text):
                    self.fields_needing_updates.append( ("index", widget, value_text) )
                    widget.set_title( user_text )
            #b Toggle button - use for on/off signals
            elif (class_path[-1]=="GtkToggleButton"):
                text = widget.get_label()
                (user_text,value_text) = parse_user_value_text(text)
                if (value_text):
                    self.fields_needing_updates.append( ("toggle", widget, value_text) )
                    widget.set_label( user_text )
            #b Text view - use for memories
            elif (class_path[-1]=="GtkTextView"):
                #b Get text and determine if it is special
                text_buffer = widget.get_buffer()
                (start,end) = text_buffer.get_bounds()
                text = text_buffer.get_text(start,end,True)
                text_buffer.apply_tag(text_buffer.create_tag("tagname", family="Monospace"),start,end)
                (user_text,value_text) = parse_user_value_text(text)
                #b If special, do a lot of stuff for a memory
                if (value_text):
                    self.memory = None
                    def check_memory( args ):
                        if (args[0][0]!="memory"):
                            return "Not a memory"
                        # args should be <memory>, <number per line>, <number_of_lines>, <options>
                        # memory should be "memory", <module>, <state number>, <width>, <n items>
                        memory_state = args[0]
                        (module, state_number, width, size) = memory_state[1:5]
                        width = int(width)
                        size = int(size)
                        per_line = int(args[1][1])
                        number_of_lines = int(args[2][1])
                        options = int(args[3][1])
                        self.memory = (module, state_number, width, size, per_line, number_of_lines, options )
                        return "Memory ok"
                    fn_list = {}
                    self.engine_state.register_text_from_state_fn( fn_list, "memory", check_memory )
                    text = self.engine_state.text_from_state( fn_list, self.hierarchy, value_text )
                    if (self.memory):
                        #b Find scrollbar
                        scrollbar = self.glade.get_widget(widget.name+"_scroll")
                        if (scrollbar):
                            (module, state_number, width, size, per_line, number_of_lines, options ) = self.memory
                            adj = scrollbar.get_adjustment()
                            self.memory = self.memory + (scrollbar,adj)
                            #associate widget somehow - size never changes if its a memory though
                            adj.set_all( value=0, lower=0, upper=(size+per_line-1)/per_line, step_increment=1, page_increment=number_of_lines, page_size=number_of_lines)
                            adj.connect( "value-changed", lambda adj, field: self.update_memory(field), ("textview_memory", widget,self.memory) )
                        #b Add to list of fields
                        self.fields_needing_updates.append( ("textview_memory", widget, self.memory ) )
        #b Done
        #print self.toplevels
        #print self.fields_needing_updates
    #b update_memory
    def update_memory( self, field ):
        (module, state_number, width, size, per_line, number_of_lines, options, scrollbar, adj ) = field[2]
        address = 0
        if (adj):
            address = int(adj.get_value())*per_line
        text = ""
        offset = 0
        while ((address+per_line<=size) and (offset<number_of_lines)):
            line = ""
            line += ("%04x" % address) + ":"
            data = self.engine_state.engine.get_state_memory( module, state_number, address, per_line )
            for x in data:
                line += " "+x
            address = address + per_line
            offset = offset + 1
            text = text + line + "\n"
        text_buffer = field[1].get_buffer()
        text_buffer.set_text(text[:-1])
        (start,end) = text_buffer.get_bounds()
        text_buffer.apply_tag_by_name("tagname",start,end)
    #b update_fields
    def update_fields( self ):
        for field in self.fields_needing_updates:
            if (field[0]=="label"):
                text = self.engine_state.text_from_state( None, self.hierarchy, field[2] )
                field[1].set_text(text)
            elif (field[0]=="toggle"):
                text = self.engine_state.text_from_state( None, self.hierarchy, field[2] )
                field[1].set_active(text!="0")
            elif (field[0]=="textview_memory"):
                self.update_memory(field)
            elif (field[0]=="index"):
                text = self.engine_state.text_from_state( None, self.hierarchy, field[2] )
                index = -1
                try:
                    index = int(text,16)
                except:
                    index=-1
                field[1].set_active( index )

#a MainWindow
class MainWindow(gtk.Window):
    #b __init__
    def __init__(self, engine, parent=None):
        #b Set
        self.engine = engine
        self.engine_state = EngineState(engine=engine)
        self.engine_display_widgets_list = []
        for inst in engine.list_instances():
            for glade_dir in args["glade_dirs"]:
                glade_filename  = glade_dir+"/"+inst[1]+".glade"
                if (os.path.isfile(glade_filename)):
                    self.engine_display_widgets_list.append(EngineDisplayWidgets( self.engine_state, inst[0], glade_filename ))
        
        #b Get ourselves going with a window
        register_stock_icons()
        gtk.Window.__init__(self)
        try:
            self.set_screen(parent.get_screen())
        except AttributeError:
            self.connect('destroy', lambda *w: gtk.main_quit())

        self.set_title(self.__class__.__name__)
        self.set_default_size(200, 200)

        #b Create basic actions and shortcuts
        merge = gtk.UIManager()
        self.set_data("ui-manager", merge)
        merge.insert_action_group(self.__create_action_group(), 0)
        self.add_accel_group(merge.get_accel_group())

        #b Build menus and add to menu bar
        try:
            mergeid = merge.add_ui_from_string(menu_bar_info)
        except gobject.GError, msg:
            print "building menus failed: %s" % msg
        bar = merge.get_widget("/MenuBar")
        bar.show()

        #b Create main window contents
        # This is a table with the top bar containing the menu, the next bar containing tooltips, then the notebook for contents, then the status bar
        table = gtk.Table(1, 4, False)
        self.add(table)

        table.attach(bar,   0,1, 0,1,  gtk.EXPAND|gtk.FILL,0,  0,0 ) # fill/expand x, not y

        bar = merge.get_widget("/ToolBar")
        bar.set_tooltips(True)
        bar.show()
        table.attach(bar,   0,1, 1,2,  gtk.EXPAND|gtk.FILL,0,  0,0 ) # fill/expand x, not y

        self.scrwind = gtk.ScrolledWindow()
        table.attach( self.scrwind, 0, 1, 2,3,  gtk.EXPAND|gtk.FILL,gtk.EXPAND|gtk.FILL,  0,0 ) # fill/expand x and y
        self.notebook = gtk.Notebook()
        self.scrwind.add_with_viewport(self.notebook)

        self.state_tree = StateWidget(engine_state=self.engine_state)
        self.notebook.append_page(self.state_tree,gtk.Label("All state tree"))
        for edw in self.engine_display_widgets_list:
            module_notebook = gtk.Notebook()
            for toplevel in edw.toplevels:
                frame = toplevel[1].get_child()
                frame.reparent(module_notebook)
                module_notebook.set_tab_label_text(frame,str(toplevel[0]))
            self.notebook.append_page(module_notebook,gtk.Label(edw.hierarchy))

        # Create statusbar
        self.statusbar = gtk.Statusbar()
        table.attach(self.statusbar,   0,1, 3,4,  gtk.EXPAND|gtk.FILL,0,  0,0 ) # fill/expand x and y

        #b Show it all
        self.show_all()

    def __create_action_group(self):
        # GtkActionEntry
        entries = (
          ( "FileMenu", None, "_File" ),               # name, stock id, label
          ( "Open", gtk.STOCK_OPEN,                    # name, stock id
            "_Open","<control>O",                      # label, accelerator
            "Open a file",                             # tooltip
            self.activate_action ),
          ( "Reset", gtk.STOCK_SAVE,                    # name, stock id
            "_Reset","<control>R",                      # label, accelerator
            "Reset simulation state",                         # tooltip
            self.activate_reset ),
          ( "StepBackMany", 'gtk-media-rewind',                    # name, stock id
            "_Back","<control>B",                      # label, accelerator
            "Step back simulation 1",                         # tooltip
            self.activate_back ),
          ( "StepBack", 'gtk-media-previous',                    # name, stock id
            "_Back","<control>B",                      # label, accelerator
            "Step back simulation 1",                         # tooltip
            self.activate_back ),
          ( "Step", 'gtk-media-next',                    # name, stock id
            "_Step","<control>S",                      # label, accelerator
            "Step simulation",                         # tooltip
            self.activate_step ),
          ( "StepMany", 'gtk-media-play',                    # name, stock id
            "_Step","<control>S",                      # label, accelerator
            "Step simulation 10",                         # tooltip
            self.activate_step_10 ),
          ( "Fwd", 'gtk-media-forward',                    # name, stock id
            "_Fwd","<control>F",                      # label, accelerator
            "Step simulation 1000",                         # tooltip
            self.activate_step_1000 ),
          ( "Ckpt", 'gtk-media-record',                    # name, stock id
            "_Ckpt","<control>C",                      # label, accelerator
            "Checkpoint",                         # tooltip
            self.activate_ckpt ),
          ( "Rcvr", 'gtk-media-forward',                    # name, stock id
            "_Recover","<control>R",                      # label, accelerator
            "Recover",                         # tooltip
            self.activate_recover ),
          ( "Print", gtk.STOCK_SAVE,                    # name, stock id
            "_Print","<control>P",                      # label, accelerator
            "Print simulation state",                         # tooltip
            self.activate_print ),
          ( "Quit", gtk.STOCK_QUIT,                    # name, stock id
            "_Quit", "<control>Q",                     # label, accelerator
            "Quit",                                    # tooltip
            gtk.main_quit ),
          ( "About", None,                             # name, stock id
            "_About", "<control>A",                    # label, accelerator
            "About",                                   # tooltip
            self.activate_about ),
        );

        # Create the menubar and toolbar
        action_group = gtk.ActionGroup("MainWindowActions")
        action_group.add_actions(entries)

        return action_group

    def activate_about(self, action):
        dialog = gtk.AboutDialog()
        dialog.set_name("Simulation engine")
        dialog.set_copyright("No copyright")
        dialog.set_website("http://www.cyclicity-cdl.sourceforge.net/")
        ## Close dialog on user response
        dialog.connect ("response", lambda d, r: d.destroy())
        dialog.show()

    def activate_action(self, action):
        dialog = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_INFO, gtk.BUTTONS_CLOSE, 'You activated action: "%s" of type "%s"' % (action.get_name(), type(action)))
        # Close dialog on user response
        dialog.connect ("response", lambda d, r: d.destroy())
        dialog.show()

    def activate_reset(self, action):
        self.engine.reset()
        self.update()

    def activate_back(self, action):
        cycle = self.engine.cycle()
        if (cycle>0):
            self.engine.reset()
            self.engine.step(cycle-1)
        self.update()

    def activate_step(self, action):
        self.engine.step(1)
        self.update()

    def activate_step_10(self, action):
        self.engine.step(10)
        self.update()

    def activate_step_1000(self, action):
        self.engine.step(1000)
        self.update()

    def activate_ckpt(self, action):
        self.engine.checkpoint_add("Ckpt")
        self.update()

    def activate_recover(self, action):
        self.engine.checkpoint_restore("Ckpt")
        self.update()

    def update( self ):
        self.state_tree.all_state_changed()
        for edw in self.engine_display_widgets_list:
            edw.update_fields()
        self.update_statusbar()

    def activate_print(self, action):
        print_state(self.engine)

    def update_statusbar(self):
        # clear any previous message, underflow is allowed
        self.statusbar.pop(0)
        self.statusbar.push(0, "Cycle %d" % self.engine.cycle() )

#a Simulation engine support functions
#f check_errors
def check_errors( engine ):
    if (eng.get_error_level()==0):
        return 0
    i = 0
    while (1):
        err = eng.get_error(i)
        if (err == None):
            eng.reset_errors()
            return 1
        print err
        i = i+1

#f print_state
def print_state( eng ):
    insts = eng.list_instances()
    for inst in insts:
        j=0
        cont = 1
        while (cont):
            cont = 0
            stin = eng.get_state_info( inst[0], j )
            if (stin!=None):
                value = eng.get_state_value_string( inst[0], j )
                cont=1
                j=j+1

#a Main
#f toplevel code
eng = py_engine.new()

run_fns("__inst_hw", eng )

if (check_errors(eng)):
    exit

eng.reset()
eng.checkpoint_init()

toplevel = MainWindow(eng)

run_fns("__main",toplevel)
toplevel.update()
gtk.main()

