#!/usr/bin/perl

#a Copyright
#  
#  This file 'logger_load' copyright Gavin J Stark 2008
#  
#  This is free software; you can redistribute it and/or modify it however you wish,
#  with no obligations
#  
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.

#a API documentation
# A log file consists of event types (module, event name, event id, argument names)
#  and event occurrences (module, event_id, cycle, occurrence number, argument values)
#
# First call
#   log_file_init();
#
# Then read a log file with
#   log_file_read( file handle );
#
# After a read all events and occurrences are marked
#
# Various API calls will mark/unmark events depending on various arguments
#    This is done with a 'mark_operation', and two arguments - the current mark value, and a conditional (usually from a regexp result or similar)
#    mark_operation==0 => clear mark independent of conditional and current mark value
#    mark_operation==1 => set mark independent of conditional and current mark value
#    mark_operation==2 => Make mark = !conditional
#    mark_operation==3 => Make mark = conditional
#    mark_operation==4 => clear mark only if conditional is set
#    mark_operation==5 => clear mark only if conditional is clear (current mark AND condition)
#    mark_operation==6 => set mark only if conditional is clear
#    mark_operation==7 => set mark only if conditional is set (current mark OR condition)
#    Furthermore, mark_operation+8 will invert the result
#    Most important are 3, 5 and 7, plus it is useful to have a conditional of 0 and use 12 to invert the marks
#
# Further, some operations (log_file_write for example) will output depending on the mark value and a mark_type
#    mark_type==0 => events/occurrences will not match independent of mark
#    mark_type==1 => events/occurrences will all match independent of mark
#    mark_type==2 => events/occurrences with the mark clear
#    mark_type==3 => events/occurrences with the mark set
#    mark_type==4 => all occurrences independent of mark and events with the mark clear
#    mark_type==5 => all occurrences independent of mark and events with the mark set
#    mark_type==6 => all events independent of mark and occurrences with the mark clear
#    mark_type==7 => all events independent of mark and occurrences with the mark set
#
#
# The set of occurrences can be pruned by deleting unmarked events
#
# Finally the events can be written out in log file format or human readable format
#   log_file_write( file handle, mark_type, human_readable )
#       human_readable==1 for human readable output, 0 for log_file output (i.e. it can be handled again be log_fil_read)
#       Only events and occurrences that match the mark_type are written out; occurrences are only output if their events also match the mark_type
#
# The marking functions are:
#   log_file_module_mark_with_name_regexp( mark_operation, regexp )
#      Perform 'mark_operation' on all events with condition set if the module name of the event matches regexp
#      log_file_module_mark_with_name_regexp( 3, "^fred" ) - mark all events with a module name starting with fred
#
#   log_file_event_mark_with_name_regexp( mark_operation, regexp )
#      Perform 'mark_operation' on all events with condition set if the event name matches regexp
#      log_file_event_mark_with_name_regexp( 5, "request" ) - clear mark on all events with a module name not containing 'request'
#
#   log_file_event_mark_with_arg_name_regexp( mark_operation, regexp )
#      Perform 'mark_operation' on all events with condition set if ANY of the events arg names match regexp
#      log_file_event_mark_with_arg_name_regexp( 7, "index" ) - set mark on all events with an argument containing 'index'
#
#   log_file_occurrence_mark_with_arg_value_regexp( mark_operation, arg_name, regexp )
#      Perform 'mark_operation' on all occurrences with condition set if the event has an argname that matches AND if the value of that arg for the occurences matches regexp
#      log_file_occurrence_mark_with_arg_value_regexp( 3, "index", "^0\$" ) - mark all occurrences with an event argument 'index' of value '0'
#
#
# The pruning functions are currently
#   log_file_occurrence_delete_marked_events( mark_type )
#      Delete all occurrences whose events marks match the mark_type
#
#   log_file_occurrence_delete_marked( mark_type )
#      Delete all occurrences whose marks match the mark type AND whose events match the mark type
#
#
# Example
#    Read from stdin, select all events from modules with '.cpu' with an event type of 'pipeline' and occurrences of these with an instruction_address of 'cafc', then write that back out for humans to stdout
#        log_file_init();
#        log_file_read(STDIN);
#        log_file_module_mark_with_name_regexp( 3, ".cpu" );   # Events with modules whose name have '.cpu'
#        log_file_event_mark_name_regexp( 5, "^pipeline\$" );  #   and with event name of 'pipeline'
#        log_file_occurrence_mark_with_arg_value_regexp( 5, "instruction_address", "^cafc\$" ); # and all occurrences with an event arg 'instruction_address' with value 'cafc'
#        log_file_write(STDOUT,3,1); # All marked events/occurrences in human format
#

#a Log file I/O functions
#f log_read_string
sub log_read_string {
    my($line) = @_;
    $not_quote = "[^\"]";
    if ($line=~/^"($not_quote*)"(.*)/) {
        $string = $1;
        $line = $2;
    } elsif ($line =~ /^([^,]*)(.*)/) {
        $string = $1;
        $line = $2;
    } else {
        $string = $line;
        $line = "";
    }
    if ($line=~/^,(.*)/) { $line=$1;}
    return ($string, $line);
}


#f log_file_init
sub log_file_init {
    %log_module_events=();
    @log_module_occurrences=();
}

#f log_file_read
sub log_file_read {
    my($infile)=@_;
    my($line);
    while(<$infile>) {
        chomp;
        $line = $_;
        if ($line =~ /^#(.*)/) {
            my(%event_desc) = [];
            my($module, $event_id);
            ($module,$line) = log_read_string($1);
            ($event_id,$line) = log_read_string($line);
            ($event_desc{"name"},$line) = log_read_string($line);
            ($event_desc{"num_args"},$line) = log_read_string($line);
            $event_desc{'marked'}=1;
            $event_desc{'temp'}=0;
            #print "$module event $event_id is '$event_desc{'name'}' with $event_desc{'num_args'} arguments\n";
            my(@event_arg_names) = [];
            for ($i=0; $i<$event_desc{"num_args"}; $i++) {
                ($event_arg_names[$#event_arg_names++],$line) = log_read_string($line);
                #print "  arg $i is $event_arg_names[$#event_arg_names]\n";
            }
            $#event_arg_names--;
            $event_desc{'arg_names'} = \@event_arg_names;
            $log_module_events{$module} = [] unless exists $log_module_events{$module};
            $log_module_events{$module}->[$event_id] = \%event_desc;
        } else {
            my(%event_occurrence) = [];
            my(@event_args) = [];
            ($event_occurrence{'cycle'},$line) = log_read_string($line);
            ($event_occurrence{'occurrence'},$line) = log_read_string($line);
            ($event_occurrence{'module'},$line) = log_read_string($line);
            ($event_occurrence{'event_id'},$line) = log_read_string($line);
            ($event_occurrence{'num_args'},$line) = log_read_string($line);
            $event_occurrence{'marked'} = 1;
            my($module) = $event_occurrence{'module'};
            my($event_id) = $event_occurrence{'event_id'};
            my($num_args) = $event_occurrence{'num_args'};
            for ($i=0; $i<$num_args; $i++) {
                ($event_args[$#event_args++], $line) = log_read_string($line);
            }
            $#event_args--;
            $event_occurrence{'event_args'} = \@event_args;
            push @log_module_occurrences, \%event_occurrence;
        }
    }
}

#f log_file_write
sub log_file_write {
    my($outfile,$mark_type,$human_readable)=@_;
    my($module, $event_id, $event, $event_arg);
    foreach $module (sort keys %log_module_events) {
        for ($event_id=0; $event_id<=$#{$log_module_events{$module}};$event_id++) {
            $event = $log_module_events{$module}->[$event_id];
            if ($event) {
                if (log_file_mark_type_matches($event->{'marked'},$mark_type,0)) {  # If modules event type matches mark
                    if (!$human_readable) {
                        print $outfile "#$module,$event_id,\"$event->{'name'}\",$event->{'num_args'}";
                        foreach $event_arg (@{$event->{'arg_names'}}) {
                            print $outfile ",\"$event_arg\"";
                        }
                        print $outfile "\n";
                    }
                }
            }
        }
    }

    foreach $event_occurrence (@log_module_occurrences) { # For all event occurrences
        if (log_file_mark_type_matches($event_occurrence->{'marked'},$mark_type,1)) { # that match the mark type
            my($module_event_id) = $event_occurrence->{'event_id'};
            my($module) = $event_occurrence->{'module'};
            if ($module_event_id>=0) {
                if (log_file_mark_type_matches($log_module_events{$module}->[$module_event_id]->{'marked'},$mark_type,0)) {  # If modules event type matches mark
                    if ($human_readable) {
                        print $outfile "$event_occurrence->{'cycle'} $module event '$log_module_events{$module}->[$module_event_id]->{'name'}' occurrence $event_occurrence->{'occurrence'}\n";
                        for ($i=0; $i<$log_module_events{$module}->[$module_event_id]->{'num_args'}; $i++) {
                            printf $outfile "    %16s $log_module_events{$module}->[$module_event_id]->{'arg_names'}->[$i]\n", $event_occurrence->{'event_args'}->[$i];
                        }
                    } else {
                        print $outfile "$event_occurrence->{'cycle'},$event_occurrence->{'occurrence'},$event_occurrence->{'module'},$event_occurrence->{'event_id'},$event_occurrence->{'num_args'}";
                        foreach $event_arg (@{$event_occurrence->{'event_args'}}) {
                            print $outfile ",$event_arg";
                        }
                        print $outfile "\n";
                    }
                }
            }
        }
    }
}

#f log_file_display
sub log_file_display {
    my($outfile)=@_;
    foreach $module (sort keys %log_module_events) {
        print $outfile "Module $module $#{$log_module_events{$module}}\n";
        for ($i=0; $i<=$#{$log_module_events{$module}};$i++) {
            $event_id = $log_module_events{$module}->[$i];
            if ($event_id) {
                print "  $i ${$event_id}{'name'} ${$event_id}{'num_args'}\n";
                foreach $event_arg (@{${$event_id}{'arg_names'}}) {
                    print $outfile "    $event_arg\n";
                }
            }
        }
    }
}

#a Log file marking functions
#f log_file_mark_type_matches
sub log_file_mark_type_matches {
    my ($mark, $mark_type, $is_occurrence) = @_;
    my ($result);
    $result = 0                                 if ($mark_type==0); # No events or occurrences
    $result = 1                                 if ($mark_type==1); # All events or occurences
    $result = ($mark==0)                        if ($mark_type==2); # Event or occurence with mark clear
    $result = ($mark==1)                        if ($mark_type==3); # Event or occurence with mark set
    $result = (($mark==0) || ($is_occurrence))  if ($mark_type==4); # Event with mark clear or occurrence
    $result = (($mark==1) || ($is_occurrence))  if ($mark_type==5); # Event with mark set or occurrence
    $result = (($mark==0) || (!$is_occurrence)) if ($mark_type==6); # Occurrence with mark clear or event
    $result = (($mark==1) || (!$is_occurrence)) if ($mark_type==7); # Occurence with mark set or event
    return $result;
}

#f log_file_mark_op
sub log_file_mark_op {
    my ($mark_op, $condition, $current) = @_;
    my ($result);

    $result=0                       if (($mark_op&7)==0); # 0 => clear
    $result=1                       if (($mark_op&7)==1); # 1 => set
    $result=!$condition             if (($mark_op&7)==2); # 2 => !$cond
    $result=$condition              if (($mark_op&7)==3); # 3 => $cond
    $result=$current & !$condition  if (($mark_op&7)==4); # 4 => clear if $cond
    $result=$current & $condition   if (($mark_op&7)==5); # 5 => clear unless $cond
    $result=$current | !$condition  if (($mark_op&7)==6); # 6 => set unless $cond
    $result=$current | $condition   if (($mark_op&7)==7); # 7 => set if $cond
    if ($mark_op&8) { $result = !$result; } # Invert if $mark_op<8
    return $result;
}

#f log_file_mark_event
sub log_file_mark_event {
    my ($condition, $mark_op, $event) = @_;
    $event->{'marked'} = log_file_mark_op($mark_op, $condition, $event->{'marked'});
}

#f log_file_mark_occurrence
sub log_file_mark_occurence {
    my ($condition, $mark_op, $event) = @_;
    $event->{'marked'} = log_file_mark_op($mark_op, $condition, $event->{'marked'});
}

#f log_file_module_mark_with_name_regexp
sub log_file_module_mark_with_name_regexp {
    my($mark_op, $module_regexp) = @_;
    my($module, $event_id, $event, $event_arg);
    foreach $module (sort keys %log_module_events) {
        $mark_op_cond = ($module =~ /$module_regexp/);
#        print "Mark module $module $mark_op_cond $module_regexp\n";
        for ($event_id=0; $event_id<=$#{$log_module_events{$module}};$event_id++) {
            $event = $log_module_events{$module}->[$event_id];
            if ($event) {
                log_file_mark_event( $mark_op_cond, $mark_op, $event );
            }
        }
    }
}

#f log_file_event_mark_with_name_regexp
sub log_file_event_mark_with_name_regexp {
    my($mark_op, $module_regexp) = @_;
    my($module, $event_id, $event, $event_arg);
    foreach $module (sort keys %log_module_events) {
        for ($event_id=0; $event_id<=$#{$log_module_events{$module}};$event_id++) {
            $event = $log_module_events{$module}->[$event_id];
            if ($event) {
                $mark_op_cond = ($event->{'name'} =~ /$module_regexp/);
#               print "Mark module/event $module $event->{'name'} $mark_op_cond $module_regexp\n";
                log_file_mark_event( $mark_op_cond, $mark_op, $event );
            }
        }
    }
}

#f log_file_event_mark_with_arg_name_regexp
sub log_file_event_mark_with_arg_name_regexp {
    my($mark_op, $module_regexp) = @_;
    my($module, $event_id, $event, $event_arg);
    foreach $module (sort keys %log_module_events) {
        for ($event_id=0; $event_id<=$#{$log_module_events{$module}};$event_id++) {
            $event = $log_module_events{$module}->[$event_id];
            if ($event) {
                $mark_op_cond = 0;
                foreach $event_arg (@{$event->{'arg_names'}}) {
                    $mark_op_cond = $mark_op_cond | ($event_arg =~ /$module_regexp/);
                }
                log_file_mark_event( $mark_op_cond, $mark_op, $event );
            }
        }
    }
}

#f log_file_occurrence_delete_marked_events
sub log_file_occurrence_delete_marked_events {
    my($mark_type) = @_;
    my($module, $event_id, $event, $event_arg);
    foreach $event_occurrence (@log_module_occurrences) { # For all event occurrences
        my($module_event_id) = $event_occurrence->{'event_id'};
        my($module) = $event_occurrence->{'module'};
        if ($module_event_id>=0) {
            if (log_file_mark_type_matches($log_module_events{$module}->[$module_event_id]->{'marked'},$mark_type,0)) {  # If modules event type matches mark, remove the event
                #print "Removing $module $module_event_id $event_occurrence->{'cycle'}\n";
                $event_occurrence->{'event_id'}=-1;
            }
        }
    }
}

#f log_file_occurrence_mark_with_arg_value_regexp
sub log_file_occurrence_mark_with_arg_value_regexp {
    my($mark_op, $arg_name, $arg_regexp) = @_;
    my($module, $event_id, $event, $event_arg, $i, $arg_to_compare);
    foreach $module (sort keys %log_module_events) { # For all modules
        for ($event_id=0; $event_id<=$#{$log_module_events{$module}};$event_id++) { # For all events within the module
            $event = $log_module_events{$module}->[$event_id]; # Get the event_desc
            if ($event) {
                $event->{'temp'} = -1;
                for ($i=0; $i<=$#{$event->{'arg_names'}}; $i++) { # Run through the argument names to check for a match - we need the number
                    $event_arg = $event->{'arg_names'}[$i];
                    if ($event_arg eq $arg_name) {
                        $event->{'temp'}=$i;
                    }
                }
                #print "Checked $module $event_id $event, found $event->{'temp'}\n";
            }
        }
    }

    foreach $event_occurrence (@log_module_occurrences) { # For all event occurrences
        my($module_event_id) = $event_occurrence->{'event_id'};
        my($module) = $event_occurrence->{'module'};
        if ($module_event_id>=0) {
            my($event) = $log_module_events{$module}->[$module_event_id];
            $arg_to_compare = $event->{'temp'}; # Which argument number we need to look at in this occurrence
            $mark_op_cond = 0;
            if ($arg_to_compare>=0) {
                $mark_op_cond = ($event_occurrence->{'event_args'}->[$arg_to_compare] =~ /$arg_regexp/)?1:0;
                #print "Comparing $module arg $arg_to_compare value $event_occurrence->{'event_args'}->[$arg_to_compare] regexp $arg_regexp value $mark_op_cond\n";
            }
            log_file_mark_occurence( $mark_op_cond, $mark_op, $event_occurrence );
        }
    }
}

#f log_file_occurrence_delete_marked
sub log_file_occurrence_delete_marked {
    my($mark_type)=@_;
    my($module, $event_id, $event, $event_arg);
    foreach $event_occurrence (@log_module_occurrences) { # For all event occurrences
        if (log_file_mark_type_matches($event_occurrence->{'marked'},$mark_type,1)) {
            my($module_event_id) = $event_occurrence->{'event_id'};
            my($module) = $event_occurrence->{'module'};
            if ($module_event_id>=0) {
                if (log_file_mark_type_matches($log_module_events{$module}->[$module_event_id]->{'marked'},$mark_type,0)) {
                    $event_occurrence->{'event_id'}=-1;
                }
            }
        }
    }
}

#a Debug main and return true
sub log_file___debug_main {
    log_file_init;
    $file = shift @ARGV;
    open( INFILE, $file );
    log_file_read(INFILE);
#log_file_module_mark_with_name_regexp(3,"uc_array"); # Mark all events with a module name including 'uc_array', and unmark the rest
#log_file_occurrence_delete_marked_events(0); # Delete any occurrences with events that are not marked
#log_file_module_mark_with_name_regexp(3,"");
#log_file_occurrence_mark_with_arg_value_regexp(3,"quc_index","0"); # Mark all occurrences with an event with an argument 'quc_index' matching regex '0'
    log_file_occurrence_mark_with_arg_value_regexp(3,"data_3_data","11"); # Mark all occurrences with an event with an argument 'quc_index' matching regex '0'
    log_file_write(STDOUT,1,1); # Write out all occurrences with events that are marked (should be all of them) and that themselves are marked
}
return 1;

#a Editor preferences and notes
# Local Variables: ***
# mode: perl ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***

