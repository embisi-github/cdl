import xml.dom.minidom
import sys

color_list = ["#000000",
              "#ff0000",
              "#00ff00",
              "#cccc00",
              "#0000ff",
              "#cc00cc",
              "#00cccc",
              "#aaaaaa",
              "#222222",
              "#770000",
              "#007700",
              "#555500",
              "#000077",
              "#550055",
              "#005555",
              "#333333",
]

doc = xml.dom.minidom.parse("a.xml")

def signal_is_clocked( signal ):
    x=signal.getAttribute("derived_combinatorially")
    if (x=="0"):
        return True
    if (x=="1"):
        return False
    is_clocked = False
    dependencies = signal.getElementsByTagName("dependencies")
    if dependencies:
        is_clocked=True
        for dependency in dependencies:
            if dependency.parentNode == signal:
                for d in dependency.childNodes:
                    if d.nodeType==doc.ELEMENT_NODE:
                        print >>sys.stderr, d, d.nodeName
                        if d.nodeName!="clock":
                            is_clocked = False
    return is_clocked

def output_node_id( signal, is_clocked=False ):
    is_clocked = signal_is_clocked( signal )
    for bits in module.getElementsByTagName("bits"):
        if bits.parentNode == signal:
            name = bits.getAttribute("name")

def output_node_dependents( signal ):
    is_clocked = signal_is_clocked( signal )
    for bits in signal.getElementsByTagName("bits"):
        if bits.parentNode == signal:
            break
    for dependents in signal.getElementsByTagName("dependents"):
        if dependents.parentNode == signal:
            break
    print bits.getAttribute("name"),"->",
    for instance in dependents.getElementsByTagName("instance"):
        print instance.getAttribute("name"),
    print

def get_signal_data( signal, chain_tag_name="dependents" ):
    is_clocked = signal_is_clocked( signal )
    name = None
    output_args = None
    dependents_list = []
    for instance in module.getElementsByTagName("instance"):
        if instance.parentNode == signal:
            output_args = int(instance.getAttribute("output_args_0"))
            for bits in instance.getElementsByTagName("bits"):
                if bits.parentNode == instance:
                    name = bits.getAttribute("name")
    for dependents in signal.getElementsByTagName(chain_tag_name):
        if dependents.parentNode == signal:
            for instance in dependents.getElementsByTagName("instance"):
                for bits in instance.getElementsByTagName("bits"):
                    if bits.parentNode == instance:
                        dependents_list.append(bits.getAttribute("name"))
    print >>sys.stderr, "get_signal_data",signal,name,is_clocked,output_args
    return ( name, signal.nodeName, is_clocked, dependents_list, output_args )

def output_graph( module ):
    print "digraph",module.getAttribute("name")
    print "{"
    instance_list = {}
    for signal_type in ["input", "net", "comb", "clocked"]:
        for signal in module.getElementsByTagName(signal_type):
            if signal.parentNode == module:
                (name, signal_type, is_clocked, dependents_list, output_args ) = get_signal_data( signal, chain_tag_name="dependencies" )
                if signal_type=="clocked":
                    is_clocked = True
                if is_clocked:
                    dependents_list=[]
                instance_list[ name ] = (signal_type, is_clocked, dependents_list, output_args )
    print >>sys.stderr, instance_list

    for (name,(signal_type,is_clocked,dependents_list,output_args)) in instance_list.iteritems():
        shape = ""
        color = ""
        if output_args!=None:
            if ((output_args>=0) and (output_args<len(color_list))):
                color='fillcolor="%s" fontcolor="white"'%(color_list[output_args])
        if is_clocked:
            shape = ',shape="box"'
        print name, """[ label="%s.%s" %s %s ]"""%(signal_type, name, shape, color)

    for (name,(signal_type,is_clocked,dependents_list,output_args)) in instance_list.iteritems():
        for d in dependents_list:
            print name, "->",d,";"

    print "}"

for module in doc.getElementsByTagName("module"):
    external = int(module.getAttribute("external"))
    if external:
        pass
        #print "Found external reference for module",module.getAttribute("name")
    else:
        #print "Found definition for module",module.getAttribute("name")
        output_graph( module )

