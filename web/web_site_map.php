<?php

site_map( "top", "index.php", "Home" );
site_push( "" );
site_map( "cdl", "cdl.php", "CDL" );
site_map( "history", "history.php", "History" );
site_map( "installation", "installation.php", "Installation" );
site_pop();
site_map( "doc", "documentation/index.php", "Documentation" );
site_push( "documentation/" );
site_map( "language_specification", "cdl/language_specification.php", "CDL specification" );
site_push( "cdl/" );
site_map( "lexical", "language_lexical.php", "Lexical analysis" );
site_map( "grammar", "language_grammar.php", "Grammar" );
site_map( "details", "language_details.php", "Details" );
site_map( "discussion", "language_discussion.php", "Discussion" );
site_map( "examples", "language_examples.php", "Examples" );
site_pop();
site_map( "simulation_engine", "simulation_engine/index.php", "Simulation engine" );
site_push( "simulation_engine/" );
site_map( "waveform", "waveform.php", "Waveform" );
site_map( "coverage", "coverage.php", "Coverage" );
site_map( "engine", "engine.php", "Engine" );
site_map( "instantiation", "instantiation.php", "Instantiation" );
site_map( "registration", "registration.php", "Registration" );
site_map( "simulation", "simulation.php", "Simulation" );
site_map( "internal_modules", "internal_modules/index.php", "Internal modules" );
site_push( "internal_modules" );
site_map( "se_test_harness", "se_test_harness.php", "Test harness" );
site_pop();
site_pop();
site_map( "support_libraries", "support_libraries/index.php", "Support libraries" );
site_push( "support_libraries/" );
site_map( "sl_hier_mem", "sl_hier_mem.php", "Hierarchical memories" );
site_pop();
site_pop();
site_done();

?>
