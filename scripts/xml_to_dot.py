import xml.dom.minidom
import sys

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
    dependents_list = []
    for bits in module.getElementsByTagName("bits"):
        if bits.parentNode == signal:
            name = bits.getAttribute("name")
    for dependents in signal.getElementsByTagName(chain_tag_name):
        if dependents.parentNode == signal:
            for instance in dependents.getElementsByTagName("instance"):
                dependents_list.append(instance.getAttribute("name"))
    return ( name, signal.nodeName, is_clocked, dependents_list )

def output_graph( module ):
    print "digraph",module.getAttribute("name")
    print "{"
    instance_list = {}
    for signal_type in ["input", "net", "comb"]:
        for signal in module.getElementsByTagName(signal_type):
            if signal.parentNode == module:
                (name, signal_type, is_clocked, dependents_list) = get_signal_data( signal, chain_tag_name="dependencies" )
                instance_list[ name ] = (signal_type, is_clocked, dependents_list )
    print >>sys.stderr, instance_list

    for (name,(signal_type,is_clocked,dependents_list)) in instance_list.iteritems():
        if is_clocked:
            print name, """[ label="%s.%s",shape="box" ]"""%(signal_type, name)
        else:
            print name, """[ label="%s.%s" ]"""%(signal_type, name)

    for (name,(signal_type,is_clocked,dependents_list)) in instance_list.iteritems():
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

