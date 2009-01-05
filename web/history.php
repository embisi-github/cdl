<?php

include "web_globals.php";
site_set_location( "top.history" );
$page_title = "Cyclicity CDL History";
include "web_header.php";

page_header( "History of CDL" );

page_sp();
?>
<p>

Cyclicity CDL was created by Gavin Stark. Its purpose is to make
hardware design more productive, and it comes from a history of
software development for ASIC design. In previous projects C models
have been created by hand, or perhaps automatically from Verilog
(e.g. the RACE simulator acquired by Cadence, a long time ago
now). The first course duplicates design work; the latter requires
some tool to take a non-cycle based language and convert its output to
something cycle based. Neither of these is a good path.

<p>

Some vendors have taken the latter path, though, since Cadence dropped
RACE. TenisonEDA, for example, now touts VTOC to convert verilog to
SystemC, C or C++. This is mainly viewed as a path to create C models from golden verilog, though.

<p>

SystemC is then an attempt to create hardware design at a higher level
with C models; the major issue with SystemC, though, is the verbosity
required to design anything within SystemC, as the C language itself
(which SystemC comes under) is not an HDL. Hardware designs are
difficult enough to understand on their own, without many layers of
extra rubrik obfuscating the design.

<p>

SystemVerilog is a good step forward from the Verilog world, and
Accellera should be praised for having some insight
here. SystemVerilog is a practical EDA solution to the problem, if it
is incorporated with a good compiler. However, it suffers from the
same problem that Verilog does, in that it is inherently a behavourial
test bench language and hardware design language, which means bad
design practice easily inflicts pain on hardware designs (more bugs
etc) and on compiler writers (far too many constructs are possible,
which probably all need to be handled to cater to the whims of
designers).

<p>

Finally, no good solutions in the compiler realm are available in the
public domain. What about low cost hardware design? How do we more
readily push the bounds of hardware design practice? Waiting for EDA
tool companies is a painful occupation. SystemVerilog is catching up
to software design practices of 20 years ago.

<p>

So, with time available, and a desire for a good C-model and
synthesizable verilog EDA tool, where to go? That was the birth of
Cyclicity cycle design language. Take a wealth of programming
experience (Verilog, VHDL, functional languages, C, C++, Java,
assemblers etc), a wealth of design experience (silicon, board level,
FPGA, device driver, OS, tools, application), and focus on a language
for ASIC design. Where readability of code is a primary goal; if code
is transparent, then bugs are fewer initially. Where good design
practice is built in; no unexpected transparent latches, no unreset
flops, etc. Where documentation is visible, and can be easily
extracted to form supporting release documents. Where hardware design,
which is difficult at the best of times, is the focus of the tool.

<p>

The obvious starting places are Verilog or VHDL and C. VHDL, with its
cumbersome type mechanisms and libraries to do anything, and its
ADA-like heritage, is a less good starting point, particularly as
almost every low-level designer (hardware or software) needs
profficiency in C. Verilog suffers also from a hardware design
heritage; it is so focussed on producing the whole environment, that
it is easy to miss the wood for the trees in such designs. C suffers
also, with no focus on hardware design at all. But a good hybrid of
styles, and even syntax, for Verilog and C is workable. This, then, is
the basis of Cyclicity CDL.

<?php
page_ep();

page_header( "About the originator" );

page_sp();
?>

Gavin Stark is a Briton transplanted to California's Bay Area, his life uprooted by the demands of the silicon industry.

<p>

He was awarded his PhD by Cambridge University, England, whilst
working for his first startup called (at the time) ATML, a company
which has since become Virata, Globespan/Virata, and now something to
probably become Conexant.

<p>

Designing ATML's first system-on-a-chip (i.e. integrated CPU with
networking functions) took him to work with Cirrus Logic in Fremont,
California, where he eventually took up employment, working as the
architect for Cirrus Logic's communications silicon division.  This
division spun off to become Basis Communications, a privately held
communication silicon vendor, of which Gavin was Chief Technical
Officer. Here Gavin designed most of the silicon product for Basis,
utilizing hand-coded cycle-based C models to do the design, coupled to
simple models of an ARM CPU; Gavin also then wrote verilog models
using the C models, running both in parallel in simulation to ensure
identical functionality.

<p>

Upon completion of the design of its first silicon (where the design
worked first time), Basis was acquired by Intel Corporation, where it
was taken under the wing of the Network Processor Group. Here, Gavin
held a post as an architect for the lower end of the network processor
line, being chiefly responsible for the architecture of the IXP425
amongst other devices. Gavin left Intel in March 2003.

<?php
page_ep();

include "web_footer.php"; ?>

