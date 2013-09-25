#!/usr/bin/python2

#a Copyright
#  
#  This file 'test.py' copyright Gavin J Stark 2003, 2004
#  
#  This is free software; you can redistribute it and/or modify it however you wish,
#  with no obligations
#  
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.

#a Imports
from Tkinter import *
from tkFileDialog import *
from tkMessageBox import *
import re
from math import *
import py_engine
# We cannot do the following in Linux as Python has a broken C API system
#  Basically Guido appears to be scared of a global namespace for function names,
#  so each module in Linux is imported in a sandbox that exports no C to other modules
#  This seems to make the librarying pretty useless from C++, and we need to use
#  one single large library.
#  Bah.
#  So we probably need a script that ties all the modules together prior to loading in to Python
#  Of course we now need to init all of the modules too...
#import c_apb_master
#import c_apb_target
#import c_apb_target_sram

#a Functions
#f print_ui_list
def print_ui_list( ui, prefix, indent ):
    for item in ui:
        if (item[0] == "module"):
            sub_prefix = prefix
            if (prefix!=""):
                sub_prefix = prefix+"."
            print indent, "module", item[1], sub_prefix
            print_ui_list( item[2], sub_prefix+item[1], indent+"  " )
        elif (item[0] == "grid"):
            print indent, "grid", item[1], item[2], item[3], item[4], item[5], item[6]
            print_ui_list( item[7], prefix, indent + "  " )
        elif (item[0] == "value"):
            name = item[6]
            if (prefix!=""):
                name = prefix+"."+item[6]
            print "value", name, eng.find_state( name )
        else:
            print indent, item

#a c_ui_frame
class c_ui_frame(Frame):
    #f __init__
    def __init__( self, engine, ui, title, prefix, master=None ):
        #print_ui_list( ui, "", "" )
        Frame.__init__( self, master )
        self.engine = engine
        self.update_list = []
        self.title = title
        self.pack(expand=1,fill="both")
        #self.configure( bg="red" )
        #self.columnconfigure(0,weight=1)
        #self.rowconfigure(0,weight=1)
        self.create_ui( self, ui, prefix )
    #f add_prefix
    def add_prefix( self, prefix, item ):
        if (prefix!=""):
            return prefix+"."+item
        return item
    #f update
    def update( self ):
        for upd in self.update_list:
            var = upd[0]
            state = upd[1]
            if (state!=None):
                if (state[0]==2):
                    if (state[3]==1): # bits
                        var.set( self.engine.get_state_value_string( state[1], state[2] ) )
                    elif (state[3]==2): # memory
                        var.set( "memory " + self.engine.get_state_value_string( state[1], state[2] ) )
                        print self.engine. get_state_memory( state[1], state[2], 0, 8 )
                    elif (state[3]==3): # fsm
                        var.set( self.engine.get_state_value_string( state[1], state[2] ) )
                    else:
                        print "Unsupported type",state[3]
    #f create_ui
    def create_ui( self, frame, ui, prefix ):
        for item in ui:
            if (item[0] == "module"):
                self.create_ui( frame, item[2], self.add_prefix( prefix, item[1] ) )
            elif (item[0] == "grid"):
                sub_frame = Frame( master=frame )
                #sub_frame.configure(bg="blue")
                sub_frame.configure(borderwidth=item[7][0] )
                if (item[7][1]=="0"):
                    sub_frame.configure(relief="sunken")
                elif (item[7][1]=="1"):
                    sub_frame.configure(relief="raised")
                for i in range(0, len(item[5])):
                    if (item[5][i]=="1"):
                        sub_frame.columnconfigure(i,weight=1)
                for i in range(0, len(item[6])):
                    if (item[6][i]=="1"):
                        sub_frame.rowconfigure(i,weight=1)
                sub_frame.grid( column=item[1], row=item[2], columnspan=item[3], rowspan=item[4], sticky="nsew" )
                self.create_ui( sub_frame, item[8], prefix )
            elif (item[0] == "label"):
                label = Label( master=frame, text=item[5] )
                label.grid( column=item[1], row=item[2], columnspan=item[3], rowspan=item[4], sticky="nsew" )
            elif (item[0] == "title"):
                label = Label( master=frame, text=self.title )
                label.grid( column=item[1], row=item[2], columnspan=item[3], rowspan=item[4], sticky="nsew" )
            elif (item[0] == "value"):
                var = Variable()
                label = Label( master=frame, textvariable=var, width=item[5] )
                state = self.engine.find_state( self.add_prefix(prefix,item[6]) )
                if (state==None):
                    var.set( "<None>" )
                else:
                    stin = eng.get_state_info( state[1], int(state[2]) )
                    var.set( "" )
                    self.update_list.append( [ var, [state[0], state[1], state[2], stin[0], stin[4], stin[5]]] )
                label.grid( column=item[1], row=item[2], columnspan=item[3], rowspan=item[4], sticky="nsew" )
            elif (item[0] == "memory"):
                sub_frame = Frame( master=frame )
                sub_frame.configure(bg="yellow")
                sub_frame.configure(borderwidth=0)
                sub_frame.grid( column=item[1], row=item[2], columnspan=item[3], rowspan=item[4], sticky="nsew" )
                text = Text( master=sub_frame, width=item[5], height=item[6] )
                text.grid( column=0, row=0, sticky="nsew" )
                state = self.engine.find_state( self.add_prefix(prefix,item[6]) )
                if (state==None):
                    x = 0
                else:
                    # check type is memory
                    # else put text in that says so...
                    # if so add something to the update list
                    # put in some scrollbars...
                    self.update_list.append( [ var, [state[0], state[1], state[2], stin[0], stin[4], stin[5]]] )
                    print self.engine.get_state_memory( state[0], state[1], 0, 8 )
            else:
                print item
                x=item

#a c_engine_control
class c_engine_control:
    #f __init__
    def __init__( self, engine, callbacks=[], root=None ):
        if (root == None):
            root = Tk()
        self.root = root
        self.engine = engine
        self.callbacks = callbacks
        def action_reset( self=self ):
            self.action( "reset", 0 )
        def action_step_1( self=self ):
            self.action( "step", 1 )
        def action_step_100( self=self ):
            self.action( "step", 100 )
        def action_step_10000( self=self ):
            self.action( "step", 10000 )
        def action_debug_off( self=self ):
            self.action( "debug", -1 )
        def action_debug_max( self=self ):
            self.action( "debug", 0 )
        def action_debug_lite( self=self ):
            self.action( "debug", 3 )
        def action_quit( self=self ):
            self.action( "quit", 0 )
        cmd_frame = Frame( master=self.root )
        cmd_frame.pack()
        Button( master=cmd_frame, text="Reset", command=action_reset ).grid( column=0, row=0 )
        Button( master=cmd_frame, text="Step 1", command=action_step_1 ).grid( column=1, row=0 )
        Button( master=cmd_frame, text="Step 100", command=action_step_100 ).grid( column=2, row=0 )
        Button( master=cmd_frame, text="Step 10000", command=action_step_10000 ).grid( column=3, row=0 )
        Button( master=cmd_frame, text="Debug off", command=action_debug_off ).grid( column=4, row=0 )
        Button( master=cmd_frame, text="Debug lite", command=action_debug_lite ).grid( column=5, row=0 )
        Button( master=cmd_frame, text="Debug max", command=action_debug_max ).grid( column=6, row=0 )
        Button( master=cmd_frame, text="Quit", command=action_quit ).grid( column=7, row=0 )
    #f action
    def action( self, action, arg ):
        if (action == "reset"):
            self.engine.reset()
            self.update()
        elif (action == "step" ):
            self.engine.step( arg )
            check_errors(self.engine)
            self.update()
        elif (action == "debug" ):
            if (arg>=0):
                py_engine.debug(arg)
            else:
                py_engine.debug()
        elif (action == "quit" ):
            self.root.destroy()
    #f update
    def update( self ):
        if (self.callbacks[0]!=None):
            self.callbacks[0]()

#a Main
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
                print stin, "::", value
                cont=1
                j=j+1

#f Main code
#py_engine.debug( 3 )
eng = py_engine.py_engine()
eng.read_hw_file( "my.hw" )
if (check_errors(eng)):
    exit

cui_list = []
def update_all():
    for cui in cui_list:
        cui.update()

gui = c_engine_control( engine=eng, callbacks=[update_all] )

insts = eng.list_instances() # list of instance name/type pairs
ui_descs = {}
for inst in insts:
    if ( inst[1][0:10]=="__internal" ):
        x = 0
    elif (ui_descs.has_key( inst[1] )):
        x = 0
    else:
        ui = eng.read_gui_file( inst[1]+".ui" )
        if (ui != None):
            ui_descs[ inst[1] ] = ui

for inst in insts:
    if (ui_descs.has_key( inst[1] )):
        cui_list.append(c_ui_frame( eng, ui_descs[inst[1]], inst[0], inst[0] ))

#print_ui_list( ui, "", "" )
if (check_errors(eng)):
    exit

eng.reset()
gui.update()
gui.root.mainloop()

#if (check_errors(eng)==0):
#    exit

