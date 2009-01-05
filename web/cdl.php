<?php

include "web_globals.php";
site_set_location( "top.cdl" );

$page_title = "Cyclicity CDL";
include "web_header.php";

page_header( "Cyclicity CDL (Cycle Design Language)" );

page_sp();
?>

The Cyclicity cycle design language (CDL) is a hardware design language focussed on a high productivity design flow for ASIC-like designs.

<p>

CDL has a very C-like syntax. It supports types such as structures and
enumerations, as well as hardware-specific types such as FSMs. It
supports cycle-based design, with combinatorial modules; it will never
generate a transparent latch. It provides for a flow-through design
style, with no combinatorial module having feedback signals through it
creating potential false paths that make synthesis much harder. It
centralizes the design so that it may be run as a C model, or to
create verilog RTL for FPGA synthesis or silicon synthesis with timing
specified in the design, so synthesis scripts may be automatically
generated. It incorporates features such as assertions (both immediate
and sequential) and code coverage. It supports GUI front-ends for code
navigation. It can generate VCD files from its C models for waveform
viewing, or a simple GUI frontend may be used to step through code in
a C model simulation, permitting all signal and state values to be
seen simply. Basically it is a fully-fledged EDA tool.

<?php
page_ep();

page_header( "Availability" );

page_sp();
?>

Cyclicity CDL is many things. First, it is a language
description. Second, there is a CDL frontend to a tool that provides
for creation of C models and Verilog RTL, so that designs written in
CDL may be converted to C models or synthesizable Verilog RTL for
silicon design or FPGA emulation. Third, there is a cycle based C
model simulation system which takes C models from the CDL frontend, or
written by hand, and runs those at speeds considerably greater than
compiled Verilog or VHDL. Fourth, there are viewers for CDL source code for
browsing, and GUI tools for running cycle simulations and seeing the
results visually.

<ul>

<li>
The language description is basically the BNF and functionality overview of the language; it is a paper/electronic specification, in the public domain.
</li>

<li>

The CDL frontend and backends are tools that are released under the
GNU Public License. They may be downloaded from this site, and
modified freely. This is encouraged. At present some of the code is
well documented, quite a lot poorly documented. This will improve over
time. Being released under the GPL means that any modifications that
are done by anybody should be released back to the software community,
particularly back to this site, so the community may share with the
developments. Any tools that are developed that link with any code in
the CDL frontend and backend (excepting only its support library; see
below) must also be released to the community: this is the aim of the
GPL, and the aim of the CDL frontend and backend tools.
</li>

<li>
The CDL cycle simulation engine is released under the Lesser GNU Public
License (LGPL). This is less burdening for users of the simulation
engine, as it only requires modifications to the engine itself to be
released back to this site and the community at large. This means that
any designs written in CDL, which are then compiled using the CDL
frontend and backend to create models that run within the engine, DO
NOT need to be released to the community. The simulation engine may be
dynamically linked with any code you like, and the LGPL license for
the simulation engine means that code need not be released in any
form. Statically linking is another issue; look more closely at the
LGPL if you which to statically link with the CDL cycle simulation
engine library, and how to release the resulting object files.
</li>

<li>
The support library is released into the public domain. This is a set of
functions that provide some simple and some complex services to more
than one entity in the CDL suite.
</li>

<li>
Other tools are in general released under the GPL.
</li>

</ul>

<?php
page_ep();

include "web_footer.php"; ?>

