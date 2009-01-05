<?php

$toplevel = "../../";
include "${toplevel}/web_globals.php";
site_set_location( "doc.support_libraries" );

$page_title = "CDL Support Libraries Documentation";
include "${toplevel}/web_header.php";


page_header( "CDL Support Libraries" );

page_sp();
?>

<p>

The CDL support libraries include many modules including:

<dl>

<dt>c_sl_error

<dd>TBA

<dt>sl_cons

<dd>TBA

<dt>sl_data_stream

<dd>TBA

<dt>sl_debug

<dd>TBA

<dt>sl_exec_file

<dd>

Okay, the exec files are crude and horrible. But they work. Okay, got that off my chest.

<p>

    Exec files are very simple extensible multithreaded control files, basically. They are exceedling primitive in their structure (one command per line, weird or no syntax, etc). But they support multithreading where any thread may be waiting for any particular external event, for example; they are also very lightweight, which is required for the simulation engine. Exec files are used for regression batch files and for primitive test harnesses in simulation. Use for anything else is not, er, recommended.

<dt>sl_fifo

<dd>TBA

<dt>sl_general

<dd>

This is the basic top level of the support library, and is included most of the time. It defines useful items such as t_sl_uint64, the unsigned 64-bit integer. It also has many simple functions that just make life easier.

<dt><a href="sl_hier_mem.php">sl_hier_mem</a>

<dd>

    This module implements hierarchical memories; these are sparse memories that utilize multiple layers of page tables to provide a wide address space where only a small portion of the address space is used. The memory may be read or written, and an iterator function is supplied to ease determination as to what addresses are actually in use (for checkpoint/restore reasons, for example).

<dt>sl_indented_file

<dd>TBA

<dt><a href="sl_mif.php">sl_mif</a>

<dd>

MIF files are memory interface format files. This module is designed to support simple hex, decimal or binary loading of memories for the simulation engine, in a standard format.

<dt>sl_option

<dd>TBA

<dt>sl_random

<dd>TBA

<dt>sl_timer.h

<dd>This is purely a header file; it defines a number of macros and various types for profiling. Particularly on Intel x86 systems (and probably AMD systems), it is very useful for code profiling as it uses the internal CPU tick counter for its 'clock'. This is a very precise and low overhead timing mechanism; it suffers, though, if your process can be switched out, so ensure that you do not try to use the timing macros across potential scheduling events (or where your code may get timesliced by the OS); or if you do, manage the effects.

<dt>sl_token

<dd>TBA

</dl>


<?php
page_ep();

include "${toplevel}web_footer.php"; ?>

