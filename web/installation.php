<?php

include "web_globals.php";
site_set_location( "top.installation" );
$page_title = "Cyclicity CDL Installation";
include "web_header.php";

page_header( "Current Status" );

page_sp();
?>
Currently Cyclicity CDL is released in source code only; this was originally a choice, and now is the
default mechanism for SourceForge.net, so there we go.

<p>

The releases do include build scripts. But it does not include scripts for installing on a Linux or Unix system under /usr/bin or similar locations.

<p>

CDL is has been tested under Linux (specifically RedHat 8, although
the build scripts are quite agnostic) and under Cygwin. It is used regularly on both Windows and Linux installations.

<p>

The source code is C, C++, Makefiles, shell scripts, and PERL scripts, with the GUI (when it is rebuilt) running
with Python and Tk.

<p>

Editing of the source code is down with Emacs, and outlining is used to improve code readability and accessibility.
The coding style is no tabs, 4 spaces per indent; most files include these options for emacs as local editor variables.

<?php
page_ep();

page_header( "Downloading" );

page_sp();
?>

The best approach is to download the latest file release. This is accessible from
<a href="http://sourceforge.net/project/showfiles.php?group_id=108143">the SourceForge file list</a> for the project.Put the tarball anywhere you like.

<p>

Untar the tarball; the tarball should contain a single directory. Do this in a working directory. (Note: if you cannot untar a tarball, then CDL is not ready for you)

<?php
page_ep();

page_header( "Building" );

page_sp();
?>

Go to the directory cyclicity/bin

<p>

Under cygwin set an environment variable 'arch' to be 'cygwin'; under Linux, nothing is required (but setting it to 'linux' will not hurt)

<p>

Type 'make'

<p>

This should build all the libraries and binaries

<?php
page_ep();

page_header( "Testing" );

page_sp();
?>

To test it a little, run the CDL regression:

<p>

Enter cyclicity/cdl_frontend/test, and type 'make'.
This will build some models from CDL files into C++, and then compile those C++ files for simulation.

<p>

Ensure (i.e. add) the simulation engine is on your dynamic library path. The easiest method for those running csh or tcsh for the shell
is to go to the scripts directory and run 'setup'; this adds the correct path to LD_LIBRARY_PATH.

<p>

Now type './regress'.
This runs a batch simulation of some of the regression tests for the Cyclicity CDL tool, and it
produces a summary of the simulations at the end. All should pass.

<p>

To be honest, if you get as far as running 'regress', the tests are extremely likely to pass, as the
major hurdles will already have been cleared. The main failure reason would be a bad LD_LIBRARY_PATH.

<?php
page_ep();

page_header( "Writing and building CDL models" );

page_sp();
?>

The release builds binaries in cyclicity/bin, and libraries in cyclicity/lib,
either under 'linux' or under 'cygwin', depending on your machine choice.

<p>

The first important binary is 'cdl'; this is the CDL compiler, which can produce C++ models,
verilog models, or both (or neither). Althoug verilog output does work, at this point (for playing
with CDL) the C++ models should be the focus.

<p>

Look at some of the sample CDL files, under cycliclity/cdl_frontend/test/tests, preferably in the
<i>simple</i> or <i>vector</i> subdirectories.

<p>

Take one of these files, and modify it, play with it, whatever; then you can run 'cdl' on it,
and that will parse your file. To create a C model you need to tell CDL to do so:

<p>
cdl --model new_model --cpp new_model.cpp &lt;file.cdl&gt;

<p>

This C++ model can be build with gcc; however, it needs to include files from cyclicity/simulation_engine
and from cyclicity/support_libraries, so include those in the build.

<p>

Finally (for now), the model has to be linked with the simulation engine code, for simulations to be run,
together with a structure that tells the engine which hardware modules to register so that you can use them
in a simulation. The best approach to achieving this (for now) is to look more closely at the tests that
the cdl_frontend includes. There is a very good basic make file in the cdl_frontend/test/build directory; basically this
utilizes a common make file template in the scripts directory, and it allows a simple file of models (the 'model_list' file)
to describe what modules need to be built from what kind of sources, and it will build a batch mode simulation
which dynamically links with the simulation engine library.
<?php
page_ep();

include "web_footer.php"; ?>

