#!/usr/bin/perl -w
#
#indx#	pandaplot.pl - Generate the grim plot of how a pandemic spreads
#@HDR@	$Id$
#@HDR@
#@HDR@	Copyright (c) 2024-2026 Christopher Caldwell (Christopher.M.Caldwell0@gmail.com)
#@HDR@
#@HDR@	Permission is hereby granted, free of charge, to any person
#@HDR@	obtaining a copy of this software and associated documentation
#@HDR@	files (the "Software"), to deal in the Software without
#@HDR@	restriction, including without limitation the rights to use,
#@HDR@	copy, modify, merge, publish, distribute, sublicense, and/or
#@HDR@	sell copies of the Software, and to permit persons to whom
#@HDR@	the Software is furnished to do so, subject to the following
#@HDR@	conditions:
#@HDR@	
#@HDR@	The above copyright notice and this permission notice shall be
#@HDR@	included in all copies or substantial portions of the Software.
#@HDR@	
#@HDR@	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
#@HDR@	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#@HDR@	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
#@HDR@	AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#@HDR@	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#@HDR@	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#@HDR@	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#@HDR@	OTHER DEALINGS IN THE SOFTWARE.
#
#hist#	2026-02-19 - Christopher.M.Caldwell0@gmail.com - Created
########################################################################
#doc#	pandaplot.pl - Generate the grim plot of how a pandemic spreads
########################################################################

use lib "/usr/local/lib/perl";

use cpi_file qw( fatal read_file cleanup tempfile);
use cpi_arguments qw( parse_arguments );
use cpi_vars;

use strict;

my @problems;
my @variables;
my @title;
my $plot_type;
my @testlist;
my %lc_arg;
my %axes_args;
my $max_cycles;
my $tests;
my %indmap;

my %ARGS;

#########################################################################
#	Print usage message and exit.					#
#########################################################################
sub usage()
    {
    print join("\n",@_,"",
        "Usage:  $cpi_vars::PROG -i=input.data -o=output.type { <variable> }"),"\n";
    exit(1);
    }

##########################################################################
##	Parse arguments							#
##########################################################################
#sub parse_arguments
#    {
#    while( defined( $_ = shift(@ARGV) ) )
#	{
#	if( /^-i(.*)$/ )
#	    {
#	    if( $ARGS{input_file} )
#		{ push( @problems, "-i specified multiple times" ); }
#	    else
#		{
#		$ARGS{input_file} = ( ( defined($1) && ($1 ne "")) ? $1 : shift(@ARGV) );
#		push(@problems,"$ARGS{input_file} not readable") if( ! -r $ARGS{input_file} );
#		}
#	    }
#	elsif( /^-o(.*)$/ )
#	    {
#	    if( $ARGS{output_file} )
#		{ push( @problems, "-o specified multiple times" ); }
#	    else
#		{
#		$ARGS{output_file} = ( ( defined($1) && ($1 ne "")) ? $1 : shift(@ARGV) );
#		#push(@problems,"$ARGS{output_file} not writeable") if( ! -w $ARGS{output_file} );
#		}
#	    }
#	elsif( /^-t(.*)$/ )
#	    {
#	    push( @testlist, ( defined($1) && ($1 ne "") ) ? $1 : shift(@ARGV) );
#	    }
#	elsif( /^(.*)=(.*)/ )
#	    {
#	    push( @variables, $1 );
#	    $lc_arg{$1} = "lc rgbcolor \"$2\"";
#	    }
#	else
#	    { push( @variables, $_ ); }
#	}
#
#    push( @problems, "No -i specified" ) if( ! defined($ARGS{input_file}) );
#    if( !defined($ARGS{output_file}) )
#        {
#	if( !defined($ARGS{input_file}) )
#	    { push( @problems, "No -o specified" ); }
#	elsif( $ARGS{input_file} =~ /(.*)\.([^\.\/]+)$/ )
#	    { $ARGS{output_file} = "$1.screen"; }
#	}
#    if( defined($ARGS{output_file}) && $ARGS{output_file} =~ /.*\.(.+)$/ )
#	{ $ARGS{plot_dest} = ( $1 eq "jpg" ? "jpeg" : $1 ); }
#    else
#	{ push( @problems, "$ARGS{output_file} has no extension"); }
#
#    &usage( @problems ) if( @problems );
#    }

#########################################################################
#	Knows how to interface with gnuplot.				#
#########################################################################
sub gnuplot_interface
    {
    my $MAX_LINES_ON_GRAPH = 8;
    my $Y2LABEL;
    my $GNUPLOT_INTERPRETER="/bin/gnuplot";

    my $script_file = &tempfile(".script");

    # Decide what CAN we do based on arguments

    if( $tests <= 1
     || ($tests*scalar(@variables) < $MAX_LINES_ON_GRAPH)
     || grep( $_ eq "R", @variables ) )
	{ $plot_type = "line_graph"; }
    else
	{ $plot_type = "3d"; }

    $_ = 0;
    foreach my $v ( @variables )
	{
	if( scalar(@variables) <= 1 )
	    { $lc_arg{$v} ||= ""; }
	else
	    { $lc_arg{$v} ||= "lc ".$_++; }
	if( $v ne "R" )
	    { $axes_args{$v} = "axes x1y1"; }
	else
	    {
	    $axes_args{$v} = "axes x1y2";
	    $Y2LABEL = "R"
	    }
	}

    my $constraints =
        ( @testlist
	? join( "||", ( map {"(\$2==$_)"} @testlist ) )
	: "1" );

    open( OUT, "> $script_file" ) || die("Cannot write $script_file:  $!");
    print OUT "#!$GNUPLOT_INTERPRETER\n";

    print OUT "set terminal $ARGS{plot_dest}\nset output \"$ARGS{output_file}\"\n"
	if( $ARGS{plot_dest} ne "screen" );

    if( $plot_type eq "3d" )
	{
	print OUT
	    "set xlabel \"Time\"\n",
	    "set ylabel \"Test\"\n",
	    "set zlabel \"Organisms\" rotate by 90\n",
	    #"set pm3d\n",
	    "set dgrid3d $tests,$max_cycles\n",
	    "set hidden3d\n",
	    "splot ";
	print OUT
	    join(",",map { "\"$ARGS{input_file}\" using 1:2:($constraints?\$$indmap{$_}:1/0) title \"$_\" with lines $lc_arg{$_}" } @variables);
	}
    else
	{
	print OUT "set xlabel \"Time\"\nset ylabel \"Organisms\"\n";
	print OUT "set y2label \"$Y2LABEL\"\n" if( $Y2LABEL );
	print OUT "plot ";
	if( $tests <= 1 )
	    {
	    print OUT join(",",map { "\"$ARGS{input_file}\" using 1:($constraints?\$$indmap{$_}:1/0) title \"$_\" $axes_args{$_} with lines $lc_arg{$_}" } @variables);
	    }
	else
	    {
	    my $ltind = 1;
	    my $lwind = 1;
	    my $sep = "";
	    $_ = 1;
	    foreach my $t ( @testlist )
		{
		my $pref = "T${t} ";
		my $lcarg = ( scalar(@variables)<=2 ? "lc ".$_++ : "" );
		print OUT $sep, join(",",map { "\"$ARGS{input_file}\" using 1:(\$2==$t?\$$indmap{$_}:1/0) title \"$pref$_\" $axes_args{$_} with lines $lcarg $lc_arg{$_} lw $lwind lt $ltind" } @variables);
		$sep = ", ";
		if( scalar(@variables) >= 2 )
		    {
		    $ltind += 2;
		    $lwind += 2;
		    }
		}
	    }
	}

    print OUT "\n";
    print OUT "pause -1 \"Hit CR when done:  \"\n" if( $ARGS{plot_dest} eq "screen" );
    close( OUT );

    system("chmod 755 $script_file; LANG=en_US.UTF-8 $script_file");
    }

#########################################################################
#	Main								#
#########################################################################

# Parse arguments.
%ARGS = parse_arguments({
    non_switches=>\@variables,
    switches=>
	{
	input_file	=>"",
	output_file	=>"",
	tests		=>""
	} } );

foreach my $v ( @variables )
    {
    if( $v =~ /(.*)=(.*)/ )
	{
        $lc_arg{$1} = "lc rgbcolor \"$2\"";
	$v = $1;
	}
    }

@testlist = split(/,/,$ARGS{tests});

push( @problems, "You must specify an input file with -input_file." )
    if( ! $ARGS{input_file} );
if( !defined($ARGS{output_file}) )
    {
    if( !defined($ARGS{input_file}) )
	{ push( @problems, "No -o specified" ); }
    elsif( $ARGS{input_file} =~ /(.*)\.([^\.\/]+)$/ )
	{ $ARGS{output_file} = "$1.screen"; }
    }
if( defined($ARGS{output_file}) && $ARGS{output_file} =~ /.*\.(.+)$/ )
    { $ARGS{plot_dest} = ( $1 eq "jpg" ? "jpeg" : $1 ); }
else
    { push( @problems, "$ARGS{output_file} has no extension"); }

&usage( @problems ) if( @problems );

my @titles;

foreach $_ ( split(/\n/,&read_file($ARGS{input_file})) )
    {
    chomp( $_ );
    if( /^#cycles=(\d+)#tests=(\d+)#(.*)/ )
        {
	($max_cycles,$tests,@titles) = ( $1, $2, split(/#/,$3) );
	$max_cycles += 0;
	last;
	}
    }

if( scalar(@testlist) )
    { $tests = scalar(@testlist); }
else
    { @testlist = ( 0 .. $tests-1 ); }

my $i = 3;
grep ( $indmap{$_}=$i++, @titles );

if( ! @variables )
    { @variables = @titles; }
else
    {
    my @bad = grep( !defined( $indmap{$_} ), @variables );
    push( @problems,
	"The following do not appear to be variables:  "
	    . join(",",@bad) )
	if( @bad );
    }

&usage( @problems ) if( @problems );

&gnuplot_interface();

&cleanup(0);
