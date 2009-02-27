#!/usr/bin/php -q
<?php

#a Documentation
# Xilinx bugs...
#   map must be run with -ignore_keep_hierarchy on 7.1 else it core dumps
#   xst adds 'keep_hierarchy' to _all_ submodules of a model when it synthesizes it if the xcf file has 'model $model keep_hierarchy=yes'
# File format
# Each line is of the format:
# c_model <dir> <src> [c_options]
# c_src <dir> <src> [c_options]
#   source is in <source root>/<dir>/<src>.cpp
#   if a c_model, then it is added to the list of models for initialization at library startup, else its just source code
# ef <dir> <model> - 
# clib <libray>
#   specifies a required C library for simulation
# cdl <dir> <model> <cdl_options>
#
# c_options are one or more of:
#  inc:<path> - add include path, relative to the source root
#  def:<thing> - use for -D<thing>
# cdl_options are one or more of:
#  model:<name> - change the output model name to be <name>, and its C code goes to <name>.cpp, object to <name>.o
#  rmn:<mapping> - add --remap-module-name <mapping> to the CDL run, so that the module name may be remapped
#  rit:<mapping> - add --remap-instance-type <mapping> to the CDL run, so that submodules may be remapped
#  inc:<path> - add include path, relative to the source root
#  dc:<constant setting> - define a constant override for the CDL run (adds --constant <constant setting> to the CDL command line)
#
# XILINX STUFF TBA
#
# Simulation makefile output
#
# The following variables are created:
# MODEL_LIBS - set of libraries required by the models, ready for 'link', i.e. -l<lib> *
# MODELS - list of models that are required for the simulation (from CDL, C and EF)
# C_MODEL_SRCS - list of the source files for the C models (from C only) that are required for the simulation - must be .cpp files
# C_MODEL_OBJS - list of the object files for the models for simulation (from CDL, C and EF) - in target_dir
# VERILOG_FILES - list of the verilog files created by the CDL models, in target_dir
#
# The following rules are created
#  For each C model, <obj> from <src>
#   c++ -o <obj> <src> <options>
#  For each CDL model, <obj> from <src>
#   cdl --model <model> --cpp <cobj> <options> <src>
#   c++ -o <obj> <cobj>
#  For each CDL model, <verilog> from <src>
#   cdl --model <model> --verilog <v> <options> <src>
#
# The simulation Makefile (simulation_build_make) builds a simulation engine with all the models in it
#  It uses ${MODELS} for initialization
#  It links all C_MODEL_OBJS with MODEL_LIBS and the simulation engine for the simulation executable
#  It links all C_MODEL_OBJS with MODEL_LIBS and the simulation engine for the python library

#a Copyright
#  
#  This file 'create_make' copyright Gavin J Stark 2006
#  
#  This is free software; you can redistribute it and/or modify it however you wish,
#  with no obligations
#  
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.

function usage()
{
    echo "Syntax: create_make -f <model_list> [-x <xilinx makefile>] [-m simulation/emulation makefile]\n";
    exit(4);
}

#a Main
$base_options = "";
$options = getopt("f:x:m:t:");
if (!isset($options["f"]))
{
    usage();
}

$model_list = $options["f"];

#a Read the descriptor list
#f read_objs
function read_objs( $file )
{
    $nobj=0;
    while (!feof($file))
    {
        $buffer = fgets($file);
        if ( (!preg_match("/^\s*#/", $buffer)) &&
             (!preg_match("/^\s*;/", $buffer)) )
        {
            #echo "\n$buffer";
            if ( preg_match("/^\s*(\S+)\s+(\S.*)/", $buffer, $matches ) )
            {
                $objs[$nobj]["type"] = $matches[1];
                $objs[$nobj]["args"] = preg_split("/\s+/",$matches[2]);
                #echo $matches[1].":".$objs[$nobj]["args"][0].":".$objs[$nobj]["args"][1].":::".$matches[2]."\n";
                $nobj++;
            }
        }
    }
    return $objs;
}

#f objs_of_type
function objs_of_type( $objs, $type_re )
{
    $nsel = 0;
    $selection = array();
    foreach ($objs as $obj)
    {
        if (preg_match($type_re,$obj["type"]))
        {
            $selection[$nsel] = $obj;
            $nsel++;
        }
    }
    return $selection;
}

#f parse_models - c_model, c_src, ef, cdl, lib
function parse_models( $objs )
{
    global $models, $libs;
    global $base_options;

    $models["c"] = array();
    $models["ef"] = array();
    $models["cdl"] = array();
    $libs["c"] = array();
    $models["hash"] = array();
    foreach (objs_of_type($objs,"/^c_(model|src)/") as $sel)
    {
        $obj["type"]  = $sel["type"];
        $obj["dir"]   = $sel["args"][0];
        $obj["model"] = $sel["args"][1];
        $args = "";
        if ($sel["args"])
        {
            $args =  array_slice($sel["args"],2);
        }

        $c_opts = "";
        foreach ($args as $option)
        {
            if (preg_match("/^inc:(\S*)/",$option, $matches))
            {
                $c_opts .=" -I\${SRC_ROOT}/".$matches[1];
            }
            if (preg_match("/^def:(\S*)/",$option, $matches))
            {
                $c_opts .=" -D".$matches[1];
            }
        }
        $obj["opts"] = $c_opts;
        $models["c"][] = $obj;
        $models["hash"][$obj["model"]] = "c";
    }

    foreach (objs_of_type($objs,"/^ef/") as $sel)
    {
        $obj["type"]  = $sel["type"];
        $obj["dir"]   = $sel["args"][0];
        $obj["model"] = $sel["args"][1];
        $obj["src"]   = $sel["args"][1].".ef";
        $obj["c"]     = $sel["args"][1].".c";
        $obj["obj"]   = $sel["args"][1].".o";
        $args = "";
        if ($sel["args"])
        {
            $args =  array_slice($sel["args"],2);
        }

        $ef_opts = $base_options;
        foreach ($args as $option)
        {
            if (preg_match("/^model:(\S+)/",$option, $matches))
            {
                $obj["model"] = $matches[1];
                $obj["c"]     = $matches[1].".cpp";
                $obj["obj"]   = $matches[1].".o";
            }
            if (preg_match("/^rmn:(\S+)/",$option, $matches))
            {
                $ef_opts .= "@@@"."--remap-module-name@@@".$matches[1];
            }
            if (preg_match("/^rit:(\S+)/",$option, $matches))
            {
                $ef_opts .= "@@@"."--remap-instance-type@@@".$matches[1];
            }
        }
        $obj["opts"] = $ef_opts;
        $models["ef"][] = $obj;
        $models["hash"][$obj["model"]] = "ef";
    }

    foreach (objs_of_type($objs,"/clib/") as $sel)
    {
        $obj["lib"]   = $sel["args"][0];
        $libs["c"][] = $obj;
    }

    $cdl_options = array();
    $cdl_options["coverage"]="off";
    $cdl_options["assertions"]="off";
    $cdl_options["statements"]="off";
    foreach (objs_of_type($objs,"/^cdl(|_options)/") as $sel)
    {
        if ($sel["type"]=="cdl_options")
        {
            foreach ($sel["args"] as $option)
            {
                if (preg_match("/^ass:(\S+)/",$option, $matches))
                {
                    $cdl_options["assertions"]=($matches[1]=="on")?"on":"off";
                }
                if (preg_match("/^cov:(\S+)/",$option, $matches))
                {
                    $cdl_options["coverage"]=($matches[1]=="on")?"on":"off";
                }
                if (preg_match("/^stmt:(\S+)/",$option, $matches))
                {
                    $cdl_options["statements"]=($matches[1]=="on")?"on":"off";
                }
            }
        }
        else
        {
            $obj["dir"]   = $sel["args"][0];
            $obj["model"] = $sel["args"][1];
            $obj["src"]   = $sel["args"][1].".cdl";
            $obj["c"]     = $sel["args"][1].".cpp";
            $obj["v"]     = $sel["args"][1].".v";
            $obj["obj"]   = $sel["args"][1].".o";
            $args = "";
            if ($sel["args"])
            {
                $args =  array_slice($sel["args"],2);
            }

            $base_options = "";
            if ($cdl_options["assertions"]=="on") { $base_options.="--include-assertions@@@"; }
            if ($cdl_options["coverage"]=="on")   { $base_options.="--include-coverage@@@"; }
            if ($cdl_options["statements"]=="on") { $base_options.="--include-stmt-coverage@@@"; }
            $cdl_opts = $base_options;
            foreach ($args as $option)
            {
                if (preg_match("/^model:(\S+)/",$option, $matches))
                {
                    $obj["model"] = $matches[1];
                    $obj["c"]     = $matches[1].".cpp";
                    $obj["obj"]   = $matches[1].".o";
                    $obj["v"]     = $matches[1].".v";
                }
                if (preg_match("/^rmn:(\S+)/",$option, $matches))
                {
                    $cdl_opts .= "@@@"."--remap-module-name@@@".$matches[1];
                }
                if (preg_match("/^rit:(\S+)/",$option, $matches))
                {
                    $cdl_opts .= "@@@"."--remap-instance-type@@@".$matches[1];
                }
                if (preg_match("/^inc:(\S+)/",$option, $matches))
                {
                    $cdl_opts .= "@@@"."--include-dir@@@\${SRC_ROOT}/".$matches[1];
                }
                if (preg_match("/^dc:(\S+)/",$option, $matches))
                {
                    $cdl_opts .= "@@@"."--constant@@@".$matches[1];
                }
            }
            $cdl_opts .= "@@@"."--coverage-desc-file@@@"."\${TARGET_DIR}/".$obj["model"].".map";
            $obj["opts"] = $cdl_opts;
            $models["cdl"][] = $obj;
            $models["hash"][$obj["model"]] = "cdl";
        }
    }
}

#f parse_xilinx_descs
function parse_xilinx_descs( $objs )
{
    global $xilinx;

    $xilinx["srcs"] = array();
    $xilinx["macros"] = array();
    $xilinx["rpms"] = array();
    $xilinx["rams"] = array();
    $xilinx["cores"] = array();
    $xilinx["subs"] = array();
    $xilinx["comps"] = array();
    $xilinx["toplevel"] = array();
    $xilinx["type"] = array();

    # A Xilinx source file is expected to define a model of the same name, and it should be a .v file in the 'src' directory of the emulation tree
    # Xilinx source files should only be referenced by Xilinx subcomponents and components
    # A Xilinx source file with the same name as another model takes preference over that other model in Xilinx components
    foreach (objs_of_type($objs,"/^xilinx_src/") as $sel)
    {
        $src = $sel["args"][0];
        $obj = array();
        $obj["model"] = $src;
        $xilinx["srcs"][$src] = $obj;
        $xilinx["type"][$src] = "src";
    }

    # A Xilinx macro is expected to define a model of the same name, and it should be a .v file in the 'macro' directory of the emulation tree
    # Xilinx macro should only be referenced by Xilinx subcomponents and components
    # A Xilinx macro with the same name as another model takes preference over that other model in Xilinx components
    foreach (objs_of_type($objs,"/^xilinx_macro/") as $sel)
    {
        $src = $sel["args"][0];
        $obj = array();
        $obj["model"] = $src;
        $xilinx["macros"][$src] = $obj;
        $xilinx["type"][$src] = "macro";
    }

    # A RAM is a Xilinx-specific item that is used in cores; it does not define a model
    foreach (objs_of_type($objs,"/^xilinx_ram/") as $sel)
    {
        $src = $sel["args"][0];
        $xilinx["rams"][] = $src;
        $xilinx["type"][$src] = "ram";
    }

    # A core has a '.xco' file in 'cores', and may depend on RAMs or perhaps other things
    # It defines a Xilinx-specific model.
    # If a core is specified with the same name as another model, the core takes preference over that other model in Xilinx components
    # A core will generate its own '.ngc' file
    # The core should also generate a behavioural verilog file
    foreach (objs_of_type($objs,"/^xilinx_core/") as $sel)
    {
        $src = $sel["args"][0];
        $obj = array();
        $obj["model"] = $src;
        $obj["src"] = $src;
        $obj["subs"] = array_slice($sel["args"],1);
        $xilinx["cores"][$src] = $obj;
        $xilinx["type"]["$src"] = "core";
    }
    # subcomponents are groups of source files, subcomponents, cores, components, or models
    # a subcomponent is expected to have a model or source file that defines it, that is also declared
    # subcomponents are basically handy groupings
    foreach (objs_of_type($objs,"/^xilinx_subcomponent/") as $sel)
    {
        $src = $sel["args"][0];
        $obj = array();
        $obj["model"] = $src;
        $obj["subs"] = array_slice($sel["args"],1);
        $obj["src_models"] = array(); # The source models that make up the component
        $obj["srcs"] = array(); # The sources (models/components/cores) (and their types) that make up the component
        $xilinx["subs"][$src] = $obj;
        $xilinx["type"][$src] = "sub";
    }
    # components are items that are built in the Xilinx world, and they consist of models, Xilinx source, and cores
    # They may utilize subcomponents as a convenience, but subcomponents are not separately built (just used to gather files together neatly)
    # A component will generate its own '.ngc' file
    foreach (objs_of_type($objs,"/^xilinx_component/") as $sel)
    {
        $src = $sel["args"][0];
        $obj = array();
        $obj["model"] = $src;
        $obj["subs"] = array_slice($sel["args"],1);
        $obj["src_models"] = array(); # The source models that make up the component
        $obj["srcs"] = array(); # The sources (models/components/cores) (and their types) that make up the component
        $xilinx["comps"][$src] = $obj;
        $xilinx["type"][$src] = "comp";
    }
    # A Xilinx RPM converts a component into a relationally-placed macro; it needs a floorplanner file as well as the component
    # It requires some offline work to create a .ngc file from the floorplanner file and the synthesized ngc file.
    foreach (objs_of_type($objs,"/^xilinx_rpm/") as $sel)
    {
        $src = $sel["args"][0];
        $obj = array();
        $obj["model"] = $src;
        $xilinx["rpms"][$src] = $obj;
        $xilinx["type"][$src] = "rpm";
    }
    # toplevels are built (if components...) with IOBs
    foreach (objs_of_type($objs,"/^xilinx_toplevel/") as $sel)
    {
        echo "Parsing toplevel '".$sel["args"][0]."'\n";
        $src = $sel["args"][0];
        $obj["model"] = $sel["args"][1];
        $obj["device"] = $sel["args"][2];
        $obj["constraints"] = $sel["args"][3];
        $xilinx["toplevels"][$src] = $obj;
    }
}

#f find_xilinx_source
# Find the source of a model
# Look first in $xilinx["type"][$model]
function find_xilinx_source( $model )
{
    global $models, $xilinx;
    $type = NULL;
    $xtype = NULL;
    if (isset($models["hash"][$model]))
    {
        $type = $models["hash"][$model];
    }
    if (isset($xilinx["type"][$model]))
    {
        $xtype = $xilinx["type"][$model];
    }
    if ($xtype)
    {
        return array("xmodel", $xtype );
    }
    if ($type)
    {
        return array("model", $type );
    }
    return NULL;
}

#a Build hierarchy
#f build_xilinx_hierarchy
# We here build the hierarchy for a Xilinx build
# In the Xilinx world we build components, which are made up of other components, cores, or source files
# The hierarchy is dug back as far as a component, no further.
# We use subcomponents to group source files or other components together; they are for convenience, and do not effect build hierarchy
# So for each component we need to identify all its constituents, and where they are subcomponents we must dig back deeper.
# The first thing to do, therefore, is hack down the subcomponents
function build_xilinx_hierarchy()
{
    global $models, $xilinx;

    #b Build subcomponents down to their base constituents
    $iter = 0;
    # We need to iterate whilst changes occur...
    $done = 0;
    while (!$done)
    {
        #b Stop us going round forever!
        $iter++;
        if ($iter>50)
        {
            echo "Too many iterations in subcomponent hierarchy examination - probably a loop in the Xilinx components; check '$last_model'\n";
            exit(4);
        }
        #b Run through all xilinx_subcomponents
        $done = 1;
        foreach ($xilinx["subs"] as $key=>$obj)
        {
            #b the subcomponent must be a xilinx source or model
            # we expect to find its source and type in the source list
            $src_models = $obj["src_models"];
            $srcs = $obj["srcs"];
            $model = $obj["model"];
            #echo "Model $model now has source ";print_r($src_models);echo"\n";
            #b Ensure the source for the subcomponent is in its own list
            if (!in_array($model, $src_models))
            {
                $type = find_xilinx_source( $model );
                if (!$type)
                {
                    echo "Subcomponent $model has no associated source file\n";
                    exit(4);
                }
                #echo "Adding self $model\n";
                $src_models[] = $model;
                $srcs[] = array( $type, $model );
                $done = 0;
                $last_model = $model;
            }
            #b Run through all of the subcomponent ($obj)'s subs 
            foreach ($obj["subs"] as $model)
            {
                #b Find the type of the subcomponent's sub ($model)
                $type = find_xilinx_source( $model );
                #echo "Sub $model type ";print_r($type);
                if (!$type)
                {
                    echo "Subcomponent constituent $model has no associated source file\n";
                    exit(4);
                }
                #b Add its source to the list for the subcomponent if not already present
                if (!array_search($model, $src_models))
                {
                    $src_models[] = $model;
                    $srcs[] = array( $type, $model );
                    $last_model = $model;
                    $done = 0;
                }
                #b If $model is a xilinx_subcomponent, then add all its source models to the set (if not already there)!
                if ( ($type[0]=="xmodel") &&
                     ($type[1]=="sub") )
                {
                    foreach ($xilinx["subs"][$model]["src_models"] as $src_model)
                    {
                        if (!in_array($src_model, $src_models))
                        {
                            $subtype = find_xilinx_source( $src_model );
                            $src_models[] = $src_model;
                            $srcs[] = array( $subtype, $src_model );
                            $last_model = $model;
                            $done = 0;
                        }
                    }
                }
            }
            #b Set the sources for the subcomponent $obj
            $xilinx["subs"][$key]["src_models"] = $src_models;
            $xilinx["subs"][$key]["srcs"] = $srcs;
            #print_r($src_models);
        }
    }

    #b Now build components down to their base constituents
    foreach ($xilinx["comps"] as $key=>$obj)
    {
        #b Clear arrys of sources and source models
        $src_models = array();
        $srcs = array();
        #b the component must be a xilinx source or model
        $model = $obj["model"];
        $type = find_xilinx_source( $model );
        if (!$type)
        {
            echo "Component $model has no associated source file\n";
            exit(4);
        }
        #b Okay, so we have a source model - it may be a xilinx_src, xilinx_macro, or CDL source
        $src_models[] = $model;
        $srcs[] = array( $type, $model );
        #b Now run through all the subcomponents
        foreach ($obj["subs"] as $model)
        {
            #b Find type of subcomponent of our component
            $type = find_xilinx_source( $model );
            if (!$type)
            {
                echo "Component constituent $model has no associated source file\n";
                exit(4);
            }
            #b Add it to the source models and sources, if it is not already there
            if (!array_search($model, $src_models))
            {
                $src_models[] = $model;
                $srcs[] = array( $type, $model );
            }
            #b If it is a xilinx_subcomponent, then push down in to that to add its sources also
            if ( ($type[0]=="xmodel") &&
                 ($type[1]=="sub") )
            {
                #b Add each source of our component's subs
                foreach ($xilinx["subs"][$model]["src_models"] as $submodel)
                {
                    if (!in_array($submodel, $src_models))
                    {
                        $subtype = find_xilinx_source( $submodel );
                        $src_models[] = $submodel;
                        $srcs[] = array( $subtype, $submodel );
                    }
                }
            }
            #b Done that subcomponent
        }
        #b Set the sources and source models for the component to be the lists generated
        $xilinx["comps"][$key]["src_models"] = $src_models;
        $xilinx["comps"][$key]["srcs"] = $srcs;
        #b Done that component
    }
}

#a Model makefile
#f output_model_makefile
function output_model_makefile( $file )
{
    global $models, $libs;

    fwrite( $file, "MODEL_LIBS := \\\n" );
    foreach ($libs["c"] as $i)
    {
        fwrite( $file, "    -l".$i["lib"]." \\\n");
    }
    fwrite( $file, " \n" );

    fwrite( $file, "MODELS := \\\n" );
    foreach ($models["c"] as $i)
    {
        if ($i["type"] == "c_model")
        {
            fwrite( $file, "    ".$i["model"]." \\\n");
        }
    }
    foreach ($models["ef"] as $i)
    {
        fwrite( $file, "    ".$i["model"]." \\\n");
    }
    foreach ($models["cdl"] as $i)
    {
        fwrite( $file, "    ".$i["model"]." \\\n");
    }
    fwrite( $file, " \n" );

    fwrite( $file, "C_MODEL_SRCS := \\\n" );
    foreach ($models["c"] as $i)
    {
        fwrite( $file, "    \${SRC_ROOT}/".$i["dir"]."/".$i["model"].".cpp \\\n");
    }
    fwrite( $file, " \n" );

    fwrite( $file, "C_MODEL_OBJS := \\\n" );
    foreach ($models["c"] as $i)
    {
        fwrite( $file, "    \${TARGET_DIR}/".$i["model"].".o \\\n");
    }
    foreach ($models["ef"] as $i)
    {
        fwrite( $file, "    \${TARGET_DIR}/".$i["obj"]." \\\n");
    }
    foreach ($models["cdl"] as $i)
    {
        fwrite( $file, "    \${TARGET_DIR}/".$i["obj"]." \\\n");
    }
    fwrite( $file, " \n" );

    fwrite( $file, "VERILOG_FILES := \\\n" );
    foreach ($models["cdl"] as $i)
    {
        fwrite( $file, "    \${TARGET_DIR}/".$i["v"]." \\\n");
    }
    fwrite( $file, " \n" );

    foreach ($models["c"] as $i)
    {
        $c_file = $i["dir"]."/".$i["model"].".cpp";
        $object_file = $i["model"].".o";
        $options = preg_replace( "/@@@/", " ", $i["opts"] );
        fwrite( $file, "\${TARGET_DIR}/$object_file : \${SRC_ROOT}/$c_file\n" );
        fwrite( $file, "\t@echo \"CC \$< -o -\$@\"\n" );
        fwrite( $file, "\t\$(Q)\$(CXX) \$(CXXFLAGS) -c -o \${TARGET_DIR}/$object_file \${SRC_ROOT}/$c_file $options\n\n" );
    }
    
    foreach ($models["ef"] as $i)
    {
        $directory = $i["dir"];
        $model = $i["model"];
        $source_file = $i["src"];
        $c_file = $i["c"];
        $object_file = $i["obj"];
        $options = preg_replace( "/@@@/", " ", $i["opts"] );
        fwrite( $file, "\${TARGET_DIR}/$c_file : \${SRC_ROOT}/$directory/$source_file \$(CYCLICITY_BIN_DIR)/ef\n" );
        fwrite( $file, "\t@echo \"EF \$< -cpp -\$@\"\n" );
        fwrite( $file, "\t\$(Q)\$(CYCLICITY_BIN_DIR)/ef --model $model --cpp \${TARGET_DIR}/$c_file $options \${SRC_ROOT}/$directory/$source_file\n\n" );
        fwrite( $file, "\${TARGET_DIR}/$object_file : \${TARGET_DIR}/$c_file\n" );
        fwrite( $file, "\t@echo \"CC \$< -o -\$@\"\n" );
        fwrite( $file, "\t\$(Q)\$(CXX) \$(CXXFLAGS) -c -o \${TARGET_DIR}/$object_file \${TARGET_DIR}/$c_file\n\n" );
    }

    foreach ($models["cdl"] as $i)
    {
        $directory = $i["dir"];
        $model = $i["model"];
        $source_file = $i["src"];
        $c_file = $i["c"];
        $object_file = $i["obj"];
        $v_file = $i["v"];
        $options = preg_replace( "/@@@/", " ", $i["opts"] );
        fwrite($file, "\${TARGET_DIR}/$c_file : \${SRC_ROOT}/$directory/$source_file \${CREATE_MAKE} \$(CYCLICITY_BIN_DIR)/cdl\n" );
        fwrite($file, "\t@echo \"CDL \$< -cpp -\$@\"\n" );
        fwrite($file, "\t\$(Q)\$(CYCLICITY_BIN_DIR)/cdl --model $model --cpp \${TARGET_DIR}/$c_file $options \${SRC_ROOT}/$directory/$source_file\n\n" );
        fwrite($file, "\${TARGET_DIR}/$object_file : \${TARGET_DIR}/$c_file\n" );
        fwrite($file, "\t@echo \"CC \$< -o -\$@\"\n" );
        fwrite($file, "\t\$(Q)\$(CXX) \$(CXXFLAGS) -c -o \${TARGET_DIR}/$object_file \${TARGET_DIR}/$c_file\n\n" );
        fwrite($file, "\${TARGET_DIR}/$v_file : \${SRC_ROOT}/$directory/$source_file \${CREATE_MAKE} \$(CYCLICITY_BIN_DIR)/cdl\n" );
        fwrite( $file, "\t@echo \"CDL \$< -v -\$@\"\n" );
        fwrite($file, "\t\$(Q)\$(CYCLICITY_BIN_DIR)/cdl \$(CDL_TO_VERILOG_OPTIONS) --model $model --verilog \${TARGET_DIR}/$v_file $options \${SRC_ROOT}/$directory/$source_file\n\n" );
    }
}

#a Xilinx makefile
#f output_xilinx_makefile
function output_xilinx_makefile( $file, $toplevel )
{
    global $models, $xilinx;

    $device         = $xilinx["toplevels"][$toplevel]["device"];
    $toplevel_model = $xilinx["toplevels"][$toplevel]["model"];
    $constraints = "\${CONSTRAINTS}/".$xilinx["toplevels"][$toplevel]["constraints"];

    #b Header
    fwrite( $file, "export XIL_PLACE_ALLOW_LOCAL_BUFG_ROUTING := 1\n");
    fwrite( $file, "REPOSITORY = ../../repository/emulation/xilinx\n");
    fwrite( $file, "XMACRO = \${REPOSITORY}/macros\n");
    fwrite( $file, "XSRC = \${REPOSITORY}/src\n");
    fwrite( $file, "XRPM = \${REPOSITORY}/rpm\n");
    fwrite( $file, "RAMS = \${REPOSITORY}/rams\n");
    fwrite( $file, "CORES = \${REPOSITORY}/cores\n");
    fwrite( $file, "CONSTRAINTS = \${REPOSITORY}/constraints\n");
    fwrite( $file, "SRC = ../../build/emulation/linux\n");
    fwrite( $file, "COREGEN_OBJ = obj/coregen\n");
    fwrite( $file, "RPM_OBJ = obj/rpm\n");
    fwrite( $file, "XST_OBJ = obj/xst\n");
    fwrite( $file, "XST_TMP = tmp/xst\n");
    fwrite( $file, "XST_DONE = obj/xst/.built\n");
    fwrite( $file, "NGD_OBJ = obj/ngd\n");
    fwrite( $file, "NGD_DONE = obj/ngd/.built\n");
    fwrite( $file, "MAP_OBJ = obj/map\n");
    fwrite( $file, "MAP_DONE = obj/map/.built\n");
    fwrite( $file, "PAR_OBJ = obj/par\n");
    fwrite( $file, "PAR_DONE = obj/par/.built\n");
    fwrite( $file, "BIT_OBJ = obj/bit\n");
    fwrite( $file, "BIT_DONE = obj/bit/.built\n");
    fwrite( $file, "LOG = log\n");
    fwrite( $file, "XST_OPTIONS = -p $device\n");
    fwrite( $file, "XST_OPTIONS += -opt_mode Speed\n");
    fwrite( $file, "XST_OPTIONS += -opt_level 1\n");
    fwrite( $file, "XST_OPTIONS += -glob_opt AllClockNets\n");
    fwrite( $file, "XST_OPTIONS += -rtlview No\n");
    fwrite( $file, "XST_OPTIONS += -read_cores YES\n");
    fwrite( $file, "XST_OPTIONS += -write_timing_constraints NO\n");
    fwrite( $file, "XST_OPTIONS += -cross_clock_analysis NO\n");
    fwrite( $file, "XST_OPTIONS += -hierarchy_separator /\n");
    fwrite( $file, "XST_OPTIONS += -bus_delimiter <>\n");
    fwrite( $file, "XST_OPTIONS += -case maintain\n");
    fwrite( $file, "XST_OPTIONS += -slice_utilization_ratio 100\n");
    fwrite( $file, "XST_OPTIONS += -verilog2001 YES\n");
    fwrite( $file, "XST_OPTIONS += -fsm_extract YES\n");
    fwrite( $file, "XST_OPTIONS += -fsm_encoding Auto\n");
    fwrite( $file, "XST_OPTIONS += -safe_implementation No\n");
    fwrite( $file, "XST_OPTIONS += -fsm_style lut\n");
    fwrite( $file, "XST_OPTIONS += -ram_extract Yes\n");
    fwrite( $file, "XST_OPTIONS += -ram_style Auto\n");
    fwrite( $file, "XST_OPTIONS += -rom_extract Yes\n");
    fwrite( $file, "XST_OPTIONS += -rom_style Auto\n");
    fwrite( $file, "XST_OPTIONS += -mux_extract YES\n");
    fwrite( $file, "XST_OPTIONS += -mux_style Auto\n");
    fwrite( $file, "XST_OPTIONS += -decoder_extract YES\n");
    fwrite( $file, "XST_OPTIONS += -priority_extract YES\n");
    fwrite( $file, "XST_OPTIONS += -shreg_extract YES\n");
    fwrite( $file, "XST_OPTIONS += -shift_extract YES\n");
    fwrite( $file, "XST_OPTIONS += -xor_collapse YES\n");
    fwrite( $file, "XST_OPTIONS += -resource_sharing YES\n");
    #Not for spartan3 fwrite( $file, "XST_OPTIONS += -use_dsp48 auto\n");
    fwrite( $file, "XST_OPTIONS += -max_fanout 500\n");
    fwrite( $file, "XST_OPTIONS += -bufg 32\n");
    #Not for spartan3 fwrite( $file, "XST_OPTIONS += -bufr Default\n");
    fwrite( $file, "XST_OPTIONS += -register_duplication YES\n");
    fwrite( $file, "XST_OPTIONS += -equivalent_register_removal YES\n");
    fwrite( $file, "XST_OPTIONS += -register_balancing No\n");
    fwrite( $file, "XST_OPTIONS += -slice_packing YES\n");
    fwrite( $file, "XST_OPTIONS += -optimize_primitives NO\n");
    fwrite( $file, "XST_OPTIONS += -use_clock_enable Auto\n");
    fwrite( $file, "XST_OPTIONS += -use_sync_set Auto\n");
    fwrite( $file, "XST_OPTIONS += -use_sync_reset Auto\n");
    fwrite( $file, "XST_OPTIONS += -enable_auto_floorplanning No\n");
    fwrite( $file, "XST_OPTIONS += -iob auto\n");
    fwrite( $file, "XST_OPTIONS += -slice_utilization_ratio_maxmargin 5\n");
    fwrite( $file, "\n");
    fwrite( $file, "XST_RPM_OPTIONS = \${XST_OPTIONS} -iobuf no -ofmt ngc\n");
    fwrite( $file, "XST_COMP_OPTIONS = \${XST_OPTIONS} -iobuf no -ofmt ngc\n");
    fwrite( $file, "XST_TOP_OPTIONS = \${XST_OPTIONS} -keep_hierarchy yes -iobuf yes -ofmt ngc\n");
    fwrite( $file, "\n");

    #b Core generation
    foreach ($xilinx["cores"] as $i)
    {
        $model = $i["model"];
        fwrite( $file, "\${COREGEN_OBJ}/$model.edn: \${CORES}/".$i["src"].".xco");
        if (isset($i["subs"][0]))
        {
            fwrite ($file, " \${RAMS}/".$i["subs"][0] );
        }
        fwrite ($file, "\n");

        fwrite( $file, "\t@mkdir -p \${COREGEN_OBJ}\n" );
        fwrite( $file, "\trm -f \${COREGEN_OBJ}/$model*\n" );
        fwrite( $file, "\tcoregen -b \${CORES}/".$i["src"].".xco\n\n");
        fwrite( $file, "\${XST_OBJ}/$model.ngo: \${COREGEN_OBJ}/$model.edn\n");
        fwrite( $file, "\t@mkdir -p \${XST_OBJ}\n");
        fwrite( $file, "\tedif2ngd \${COREGEN_OBJ}/$model.edn \${XST_OBJ}/$model.ngo\n\n");
    }
    fwrite( $file, "coregen: \\\n" );
    foreach ($xilinx["cores"] as $i)
    {
        $model = $i["model"];
        fwrite( $file, "\t\${XST_OBJ}/$model.ngo\\\n");
    }
    fwrite( $file, "\n\n" );

    #b RPM generation
    foreach ($xilinx["rpms"] as $model=>$obj)
    {
        #b $obj["model"] is the source ngc and fnf file, and there should be a macro .v file too
        if (!isset($xilinx["macros"][$obj["model"]]))
        {
            echo "For a Xilinx RPM there should be an associated macro verilog file\n";
            exit(4);
        }
        #b Dependencies - basically the macro and the fnf
        fwrite( $file, "\${RPM_OBJ}/$model.ngc: \\\n" );
        fwrite( $file, "\t\${XMACRO}/".$obj["model"].".v \\\n");
        fwrite( $file, "\t\${XST_OBJ}/".$obj["model"].".rpm.ngc \\\n");
        fwrite( $file, "\t\${XRPM}/$model.ucf \n");
        fwrite( $file, "\t@mkdir -p \${RPM_OBJ}\n");
        fwrite( $file, "\tngcbuild -p xc3s4000 -uc \${XRPM}/$model.ucf \${XST_OBJ}/$model.rpm.ngc \${RPM_OBJ}/$model.ngc\n" );
        fwrite( $file, "\n" );
        #b Now we also need an ngdbuild if the RPM is to be able to be built...
        fwrite( $file, "\${NGD_OBJ}/$model.rpm.ngd: \${XST_OBJ}/$model.rpm.ngc\n");
        fwrite( $file, "\t@mkdir -p \${NGD_OBJ}\n");
        fwrite( $file, "\t@echo \"\"\n");
        fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
        fwrite( $file, "\t@echo \"                               NGD Build for RPM $model                     \"\n");
        fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
        fwrite( $file, "\tngdbuild -aul -insert_keep_hierarchy -dd ./tmp -nt timestamp \${XST_OBJ}/$model.rpm.ngc \${NGD_OBJ}/$model.rpm.ngd\n");
        fwrite( $file, "$model.rpm: \${NGD_OBJ/$model.ngd}\n\n\n");
    }
    
    #b NGC generation (synthesis of components)
    foreach ($xilinx["comps"] as $model=>$obj)
    {
        #b Find all the verilog sources and hierarchical models that make the component, and output these as dependencies
        $v_files = array();
        $hier_models = array();
        $target_ngc = "\${XST_OBJ}/$model";
        if (isset($xilinx["rpms"][$model]))
        {
            $target_ngc = "\${XST_OBJ}/$model.rpm";
        }
        fwrite( $file, "$target_ngc.ngc: \\\n" );
        foreach ($obj["srcs"] as $src)
        {
            #b First, if it is a core, then dependency is the .ngo, and there is a verilog source from the coregen
            $handled = 0;
            if ( ($src[0][0]=="xmodel") && ($src[0][1]=="core") )
            {
                $v = "\${COREGEN_OBJ}/".$src[1].".v";
                $handled = 1;
                fwrite( $file, "\t\${XST_OBJ}/".$src[1].".ngo \\\n");
                $hier_models[] = $src[1];
                $v_files[] = $v;
            }
            #b Else if it is a component (hierarchical build :-), then the dependency is xst's .ngc, no source required
            if ( ($model!=$src[1]) && (($src[0][0]=="xmodel") && ($src[0][1]=="comp") ))
            {
                $handled = 1;
                fwrite( $file, "\t\${XST_OBJ}/".$src[1].".ngc \\\n");
                $hier_models[] = $src[1];
            }
            #b Else if it is a xilinx_src, then the dependency is the verilog source, and that is needed in the .prj
            if (!$handled && isset($xilinx["srcs"][$src[1]]))
            {
                $v = "\${XSRC}/".$src[1].".v";
                fwrite( $file, "\t$v \\\n");
                $v_files[] = $v;
                $handled = 1;
            }
            #b Else if it is a xilinx_rpm, then the dependency is the RPM ngc, and a black box wrapper is needed in the .prj
            if (!$handled && ($model!=$src[1]) && isset($xilinx["rpms"][$src[1]]))
            {
                $v = "\${XRPM}/".$src[1].".v";
                fwrite( $file, "\t\${RPM_OBJ}/".$src[1].".ngc \\\n");
                $v_files[] = $v;
                $handled = 1;
            }
            #b Else if it is a xilinx_macro, then the dependency is the macro .v, and that is needed in the .prj
            if (!$handled && isset($xilinx["macros"][$src[1]]))
            {
                $v = "\${XMACRO}/".$src[1].".v";
                fwrite( $file, "\t$v \\\n");
                $v_files[] = $v;
                $handled = 1;
            }
            #b Else we had better have some CDL source... the dependency is the emulation source .v, and that is needed in the .prj
            if (!$handled && isset($models["hash"][$src[1]]))
            {
                $v = "\${SRC}/".$src[1].".v";
                fwrite( $file, "\t$v \\\n");
                $v_files[] = $v;
                $handled = 1;
            }
            if (!$handled)
            {
                echo "Could not handle source file $src[1]\n";
            }
        }
        fwrite( $file, "\n" );
        #b Routine housekeping for directories
        fwrite( $file, "\t@mkdir -p \${LOG}\n" );
        fwrite( $file, "\t@mkdir -p \${XST_OBJ}\n" );
        fwrite( $file, "\t@mkdir -p \${XST_TMP}\n" );
        fwrite( $file, "\t@rm -f $target_ngc.ngc\n" );
        fwrite( $file, "\t@rm -f \${XST_DONE}\n" );
        #b Set filenames
        $xst_file = "\${XST_TMP}/$model.xst";
        $prj_file = "\${XST_TMP}/$model.prj";
        $xcf_file = "\${XST_TMP}/$model.xcf";
        $lso_file = "\${XST_TMP}/$model.lso";
        #b Create LSO file - library search order; this is probably unnecessary, but it works
        fwrite( $file, "\t@echo \"work\" > $lso_file\n");
        #b Create xcf file - basically we would want to handle hierarchical builds here, but we cannot as the Xilinx tools don't work
        fwrite( $file, "\t@echo \"\" > $xcf_file\n");
        foreach ($xilinx["comps"] as $submodel=>$dummy)
        {
            #if ($submodel!=$model) # Don't include itself, that messes up xst
            #{
            #    fwrite( $file, "\t@echo \"MODEL \\\"$submodel\\\" incremental_synthesis=yes;\" >> $xcf_file\n");
            #}
        }
        #b Create prj file - all the submodules that are needed as verilog
        fwrite( $file, "\t@echo \"\" > $prj_file\n");
        foreach ($v_files as $v)
        {
            fwrite( $file, "\t@echo \"verilog work $v\" >> $prj_file\n");
        }
        #b Create xst file - the build options required for XST to run
        fwrite( $file, "\t@echo \"set -tmpdir ./tmp\" > $xst_file\n");
        fwrite( $file, "\t@echo \"set -xsthdpdir ./xst\" >> $xst_file\n");
        fwrite( $file, "\t@echo \"run\" >> $xst_file\n");
        #if (isset($xilinx["toplevels"][$model]))
        if ($model==$toplevel_model)
        {
            fwrite( $file, "\t@echo \"\${XST_TOP_OPTIONS} -lso $lso_file -uc $xcf_file -ifmt mixed -ifn $prj_file -ofn $target_ngc.ngc -top $model\" >> $xst_file\n" );
        }
        else if (isset($xilinx["rpms"][$model]))
        {
            fwrite( $file, "\t@echo \"\${XST_RPM_OPTIONS} -lso $lso_file -uc $xcf_file -ifmt mixed -ifn $prj_file -ofn $target_ngc.ngc -top $model\" >> $xst_file\n" );
        }
        else
        {
            fwrite( $file, "\t@echo \"\${XST_COMP_OPTIONS} -lso $lso_file -uc $xcf_file -ifmt mixed -ifn $prj_file -ofn $target_ngc.ngc -top $model\" >> $xst_file\n" );
        }
        #b Add the xst line to the makefile with a message
        fwrite( $file, "\t@echo \"\"\n");
        fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
        fwrite( $file, "\t@echo \"                               Synthesizing $model                          \"\n");
        fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
        fwrite( $file, "\txst -ifn $xst_file -ofn \${LOG}/$model.syr" );
        fwrite( $file, "\n\n" );
        #b Done that xst
    }

    #b Mark NGC as done when the toplevel NGC file is ready
    fwrite( $file, "\${XST_DONE}: \${XST_OBJ}/$toplevel_model.ngc\n" );
    fwrite( $file, "\ttouch \${XST_DONE}\n" );
    fwrite( $file, "synth: \${XST_DONE}\n\n" );

    #b NGDbuild
    fwrite( $file, "\${NGD_OBJ}/$toplevel_model.ngd: \${XST_OBJ}/$toplevel_model.ngc\\\n");
    $obj = $xilinx["comps"][$toplevel_model];
    $ngos = array();
    foreach ($obj["srcs"] as $src)
    {
        #b First, if it is a core, then dependency is the .ngo, and there is a verilog source from the coregen
        if ( ($src[0][0]=="xmodel") && ($src[0][1]=="core") )
        {
            fwrite( $file, "\t\${XST_OBJ}/".$src[1].".ngo \\\n");
        }
        #b Else if it is a xilinx_rpm, then the dependency is the RPM ngc, and a black box wrapper is needed in the .prj
        if (isset($xilinx["rpms"][$src[1]]))
        {
            fwrite( $file, "\t\${RPM_OBJ}/".$src[1].".ngc \\\n");
        }
    }
    fwrite( $file, "\t\${XST_OBJ}/.built $constraints\n" );
    #fwrite( $file, "\${NGD_OBJ}/$toplevel_model.ngd: \${XST_OBJ}/boot_rom.ngo \${XST_OBJ}/.built $constraints \${XST_OBJ}/$toplevel_model.ngc\n");
    fwrite( $file, "\t@mkdir -p \${NGD_OBJ}\n");
    fwrite( $file, "\t@rm -f \${NGD_DONE}\n");
    fwrite( $file, "\t@echo \"\"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\t@echo \"                               NGD Build                                    \"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\tngdbuild -aul -insert_keep_hierarchy -dd ./tmp -nt timestamp -sd \${RPM_OBJ} -uc $constraints -p $device \${XST_OBJ}/$toplevel_model.ngc \${NGD_OBJ}/$toplevel_model.ngd\n");
    fwrite( $file, "\${NGD_DONE}: \${NGD_OBJ}/$toplevel_model.ngd\n" );
    fwrite( $file, "\ttouch \${NGD_DONE}\n" );
    fwrite( $file, "ngd: \${NGD_DONE}\n\n\n");

    #b Map
    fwrite( $file, "\${MAP_OBJ}/$toplevel_model.ncd: \${NGD_DONE} \${NGD_OBJ}/$toplevel_model.ngd\n");
    fwrite( $file, "\t@mkdir -p \${MAP_OBJ}\n");
    fwrite( $file, "\t@rm -f \${MAP_DONE}\n");
    fwrite( $file, "\t@echo \"\"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\t@echo \"                               Mapping                                      \"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\tmap -ignore_keep_hierarchy -p $device -cm area -detail -pr b -k 4 -c 100 -o \${MAP_OBJ}/$toplevel_model.ncd \${NGD_OBJ}/$toplevel_model.ngd \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\${MAP_DONE}: \${MAP_OBJ}/$toplevel_model.ncd\n" );
    fwrite( $file, "\ttouch \${MAP_DONE}\n");
    fwrite( $file, "\${MAP_OBJ}/$toplevel_model.pcf: \${MAP_DONE}\n");
    fwrite( $file, "map: \${MAP_DONE}\n\n\n");

    #b PAR
    fwrite( $file, "\${PAR_OBJ}/$toplevel_model.par:\${MAP_DONE} \${MAP_OBJ}/$toplevel_model.ncd \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\t@mkdir -p \${PAR_OBJ}\n");
    fwrite( $file, "\t@rm -f \${PAR_DONE}\n");
    fwrite( $file, "\t@echo \"\"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\t@echo \"                               Place and route                              \"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\tpar -w -ol med -t 1 \${MAP_OBJ}/$toplevel_model.ncd \${PAR_OBJ}/$toplevel_model.ncd \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\${PAR_DONE}: \${PAR_OBJ}/$toplevel_model.par\n");
    fwrite( $file, "\ttouch \${PAR_DONE}\n");
    fwrite( $file, "par: \${PAR_DONE}\n\n\n");

    #b Trace (timing analysis)
    fwrite( $file, "obj/trace/.built: \${PAR_DONE} \${PAR_OBJ}/$toplevel_model.ncd \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\t@mkdir -p obj/trace\n");
    #fwrite( $file, "\t@rm -f obj/trace/.built\n");
    fwrite( $file, "\t@echo \"\"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\t@echo \"                               Timing analysis                              \"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\ttrce -e 3 -l 3 -s 12 -xml $toplevel_model \${PAR_OBJ}/$toplevel_model.ncd -o obj/trace/$toplevel_model.twr \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\ttouch obj/trace/.built\n");
    fwrite( $file, "\n");
    fwrite( $file, "trace: obj/trace/.built\n\n\n");

    #b Bit file generation
    fwrite( $file, "\${BIT_DONE}: \${PAR_DONE} \${PAR_OBJ}/$toplevel_model.ncd \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\t@mkdir -p \${BIT_OBJ}\n");
    #fwrite( $file, "\t@rm -f \${BIT_DONE}\n");
    fwrite( $file, "\t@echo \"\"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\t@echo \"                               Bitfile generation                           \"\n");
    fwrite( $file, "\t@echo \"----------------------------------------------------------------------------\"\n");
    fwrite( $file, "\tbitgen -f build/$toplevel_model.ut \${PAR_OBJ}/$toplevel_model.ncd \${BIT_OBJ}/$toplevel_model.bit \${MAP_OBJ}/$toplevel_model.pcf\n");
    fwrite( $file, "\ttouch \${BIT_DONE}\n");
    fwrite( $file, "bit: \${BIT_DONE}\n\n\n");
}

#a Main
$desc = fopen( $model_list, 'r' );
if (!$desc)
{
    echo "Could not open file $model_list\n";
    exit();
}
$objs = read_objs( $desc );
fclose( $desc );

parse_models( $objs );
parse_xilinx_descs( $objs );

build_xilinx_hierarchy();

if (isset($options["m"]))
{
    $file = fopen( $options["m"], 'w' );
    output_model_makefile( $file );
    fclose($file);
}

if (isset($options["x"]))
{
    if (!isset($options["t"]) && !!isset($options["r"]))
    {
        echo "For a Xilinx build you must specify a xilinx_toplevel or xilinx_rpm to build\n";
        exit(4);
    }
    $toplevel = $options["t"];
    if (!$xilinx["toplevels"][$toplevel])
    {
        echo "xilinx_toplevel '$toplevel' not found in model list when building Xilinx makefile\n";
        exit(4);
    }

    $file = fopen( $options["x"], 'w' );
    output_xilinx_makefile( $file, $toplevel );
    fclose($file);
}

#a Editor preferences and notes
# Local Variables: ***
# mode: perl ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***

