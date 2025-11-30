#! /usr/bin/perl -w
#@HDR@	$Id$
#@HDR@		Copyright 2024 by
#@HDR@		Christopher Caldwell/Brightsands
#@HDR@		P.O. Box 401, Bailey Island, ME 04003
#@HDR@		All Rights Reserved
#@HDR@
#@HDR@	This software comprises unpublished confidential information
#@HDR@	of Brightsands and may not be used, copied or made available
#@HDR@	to anyone, except in accordance with the license under which
#@HDR@	it is furnished.

use strict;
use lib "/usr/local/lib/perl";

use cpi_file qw( fatal read_file cleanup );
use cpi_cgi qw( CGIreceive CGIheader );
use cpi_translate qw( xprint );
use cpi_user qw( admin_page logout_select );
use cpi_mime qw( mime_string );
use cpi_setup qw( setup );
use cpi_english qw( plural );
use cpi_vars;

&setup(
    stderr		=> "pandemic",
    preset_language	=> "en"
    );

my $TMP		= "/tmp/$cpi_vars::PROG.$$";
my $BIN		= "$cpi_vars::BASEDIR/bin/$cpi_vars::PROG";
my $PANDAPLOT	= "$cpi_vars::BASEDIR/bin/pandaplot";
my $LIBDIR	= "$cpi_vars::BASEDIR/lib";
my $JS		= "$LIBDIR/$cpi_vars::PROG.js";
my $VARIABLESJS	= "$LIBDIR/variables.js";
my $DISEASESJS	= "$LIBDIR/diseases.js";

my @STATES	= ("Healthy","Replicating","Contagious","Symptomatic",
		    "Resistant","Quarantined","Dead","R");

$| = 1;

my @varlist;
my %var;

#########################################################################
#	Run the software to get the error message which lists the	#
#	variables.							#
#########################################################################
sub get_variables
    {
    open( INF, "$BIN usage 2>&1 |" ) || &fatal("Cannot run $BIN:  $!");

    while( $_ = <INF> )
	{
	chomp( $_ );
	if (/\s*([0-9a-zA-Z_]+)=([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+([^\s+]+)/ )
	    {
	    next if( grep( $1 eq $_, "Volume", "X", "Y", "Z" ) );
	    push( @varlist, $1 );
	    $var{$1}{descriptor} = $2;
	    $var{$1}{default} = $3;
	    $var{$1}{minv} = $4;
	    $var{$1}{maxv} = $5;
	    }
	}
    close( INF );
    }

#########################################################################
#	Output a numeric field with accompanying slider.		#
#########################################################################
sub input_field
    {
    my( $v, $prefix, $stp ) = @_;
    return join("",
	"<input type=number",
	" name=", $prefix, ${v},
	" min=", $var{$v}{minv},
	" max=", $var{$v}{maxv},
	" step=", $stp,
	" value=", $var{$v}{default},
	" onChange='change(\"$v\",\"$prefix\",\"r_\",this.value);'",
	"><br>",
	"<input type=range",
	" name=r_", $prefix, ${v},
	" min=", $var{$v}{minv},
	" max=", $var{$v}{maxv},
	" step=", $stp,
	" value=", $var{$v}{default},
	" class=slider",
	" style='height: 2px;color:blue;'",
	" onChange='change(\"$v\",\"$prefix\",\"\",this.value);'",
	">" );
    }

#########################################################################
#	Used by the common administrative functions.			#
#########################################################################
sub footer
    {
    my( $mode ) = @_;

    $mode = "admin" if( !defined($mode) );

    my @toprint = (<<EOF );
<script>
function footerfunc( func0 )
    {
    with( window.document.footerform )
	{
	func.value = func0;
	submit();
	}
    }
</script>
<form name=footerform method=post>
<input type=hidden name=func>
<input type=hidden name=SID value="$cpi_vars::SID">
<input type=hidden name=USER value="$cpi_vars::USER">
<center class='no-print'><table $cpi_vars::TABLE_TAGS border=1>
<tr><th><table $cpi_vars::TABLE_TAGS><tr><th
EOF
    push( @toprint,
	"><input type=button help='index' onClick='show_help(\"index.html\");' value='XL(Help)'" );
    foreach my $button (
	( map {"${_}:XL(".&plural($_).")"} "Main:XL(Main)" ) )
        {
	my( $butdest, $buttext ) = split(/:/,$button);
	push( @toprint, "><input type=button onClick='footerfunc(\"Search,$butdest,\");'",
	    " help='search_$butdest'",
	    ( ($butdest eq $mode) ? " style='background-color:#50a0c0'" : "" ),
	    " value=\"$buttext\"\n" );
	}
    foreach my $button ( "admin:$cpi_vars::USER XL(account)" )
        {
	my( $butdest, $buttext ) = split(/:/,$button);
	$buttext = ucfirst( $buttext );
	push( @toprint, "><input type=button onClick='footerfunc(\"$butdest\");'",
	    " help='menu_$butdest'",
	    ( ($butdest eq $mode) ? " style='background-color:#50a0c0'" : "" ),
	    " value=\"$buttext\"\n" );
	}
    push( @toprint, ">",
	&logout_select(
	    ($mode eq "admin" ? "form" : "footerform"),
	    "footerfunc"
	    ),<<EOF);
	</th></tr>
	</table></th></tr></table></center></form></div>
EOF
    &xprint( @toprint );
    }

#########################################################################
#	***HACK***							#
#									#
#	Print out form to set variables for pandemic program.		#
#########################################################################
sub print_form
    {
    my @toprint =
	(
	"<!doctype html><html lang=en><head>",
	&read_file( $VARIABLESJS ),
	&read_file( $DISEASESJS ),
	&read_file( $JS ),
	<<EOF );
	</head><body onLoad='setup_javascript();'>
	$cpi_vars::HELP_IFRAME
	<form name=form target=_new method=post>
	<center><table cellpadding=0 cellspacing=0>
	    <tr>
	    <th>XL(Variable)</th>
	    <th>XL(Value)</th>
	    </tr>
EOF

    foreach my $v ( @varlist )
        {
	my $enabled = "none";
	my $disabled = "block";
	#my $enabled	= ( $v eq "EContagious" ? "qnone" : "none" );
	#my $disabled	= ( $v eq "EContagious" ? "none" : "qnone" );
	my $step	= ( ( $var{$v}{descriptor} =~ /float/ ) ? 0.02 : 1 );
	my $isrange	= ( ( $var{$v}{descriptor} =~ /\[/ ) ? 1 : 0 );

	push(@toprint,
	    "<tr><td width=100% colspan=2><hr width=100%></td></tr>\n")
	    if( $v eq "Max_Dist" || $v eq "Volume" );
	push(@toprint,"<tr><th align=left help='$v'>",
	    "<a href='javascript:show_input(\"$v\");'>${v}:</a></th>",
	    "<th width=80%>",
	        "<table width=100% id=enabled_$v style='display:$enabled'>",
		"<tr><td colspan=5 id=help_$v></td></tr>\n",
		"<tr><td align=right width=50%>", $var{$v}{minv}, "</td>" );
	if( $isrange )
	    {
	    push( @toprint, "<td>",
		&input_field( $v, "mn_", $step ),
		"</td><td>-</td><td>",
		&input_field( $v, "mx_", $step ),
		"</td>" );
	    }
	else
	    {
	    push( @toprint, "<td colspan=3>",
		&input_field( $v, "", $step ),
		"</td>" );
	    }
	push( @toprint,
	     "<td align=left width=50%>", $var{$v}{maxv}, "</td></tr></table>",
	    "<output name=range_$v id=disabled_$v style='display:$disabled'>",
	    ( (0 && $isrange)
	    ? $var{$v}{default}."-".$var{$v}{default}
	    : $var{$v}{default}
	    ), "</output>",
	    "</td></tr>\n" );
	}
    push( @toprint,
	"<tr><th colspan=2>",
	    "<span id='State_Buttons'></span><br><nobr>",
	    "<span id=Disease_Selector>XL(Disease Selector)</span>",
	    "<select help='output_format' name=output_format>",
	    "<option value=page>XL(Select output format)</option>",
	    "<option value=page>XL(Page)</option>",
	    "<option value=jpeg>XL(jpeg)</option>",
	    "<option value=pdf>XL(pdf)</option>",
	    "<option value=data>XL(Data)</option>",
	    "</select>",
	    "<input type=submit name=submit help='Start_simulation' value='XL(Start simulation)'>",
	    "</nobr></th></tr></table></center></form>" );
    &xprint( @toprint );
    &footer( "Main" );
    }

#########################################################################
#	Actually run a command and trap all of its output.		#
#########################################################################
sub run_cmd
    {
    my( $cmd ) = @_;
    print "$cmd<pre>";
    open( INF, "($cmd) 2>&1 |" ) || &fatal("Cannot run [$cmd]:  $!");
    while( $_ = <INF> )
        {
	s/&/&amp;/g;
	s/</&lt;/g;
	s/>/&gt;/g;
	print $_;
	}
    print "</pre>";
    close( INF );
    }

#########################################################################
#	Run the test.							#
#########################################################################
sub run_test
    {
    my @cmds;
    my @args = ( $BIN );

    my $data_file	= "$TMP.data";
    my $encoded_file	= "$TMP.encoded";
    my $log_file	= "$TMP.log";
    my $plot_file;

    foreach my $v ( @varlist )
        {
	if( $var{$v}{descriptor} !~ /\[/ )
	    { push( @args, $v . "=" . ($cpi_vars::FORM{$v}||0) ); }
	elsif( ($cpi_vars::FORM{"mn_$v"}||0) == ($cpi_vars::FORM{"mx_$v"}||0) )
	    { push( @args, $v . "=" . ($cpi_vars::FORM{"mx_$v"}||0) ); }
	else
	    {
	    push( @args,
	        $v . "="
		. ($cpi_vars::FORM{"mn_$v"}||0)
		. ","
		. ($cpi_vars::FORM{"mx_$v"}||0)
		);
	    }
	}

    push( @args, "output_file=$data_file" );

    my $generate_data_cmd = join(" ",@args);

    if( $cpi_vars::FORM{output_format} eq "page" )
       { $plot_file = "$TMP.jpg"; }
    elsif( $cpi_vars::FORM{output_format} ne "data" )
       { $plot_file = $TMP.".".$cpi_vars::FORM{output_format}; }

    push( @cmds, $generate_data_cmd );

    if( $plot_file )
	{
	my @pandaplot_args = ( "$PANDAPLOT -i $data_file -o $plot_file" );
	foreach my $hs ( @STATES )
	    {
	    my $col = $cpi_vars::FORM{"show_${hs}"};
	    push( @pandaplot_args, "$hs=$col" ) if( $col );
	    }

	push( @cmds, join(" ",@pandaplot_args) );
	}

    if( $cpi_vars::FORM{output_format} ne "page" )
	{
	system( "(" . join(";",@cmds) . ") > $log_file 2>&1" );
	print "Content-type:  ",
	    &mime_string( "x.$cpi_vars::FORM{output_format}" ),
	    "\n\n",
	    &read_file($data_file);
	}
    else
        {
	&CGIheader();

	push( @cmds, "base64 -w 0 < $plot_file > $encoded_file" );
	&run_cmd( join(";",@cmds) );

	if( ! -s $encoded_file )
	    { &fatal("pandemic failed."); }
	else
	    {
	    print "<center><img src='data:image/jpeg;base64,",
		&read_file( $encoded_file ),
		"'></center>\n";
	    }
	}
    }

#########################################################################
#	Main								#
#########################################################################

if( $ENV{SCRIPT_NAME} )
    {
    &CGIreceive();
    &get_variables();
    if( $cpi_vars::FORM{func} && $cpi_vars::FORM{func} eq "admin" )
	{ &admin_page(); }
    elsif( $cpi_vars::FORM{submit} )
	{
        &run_test();
	}
    else
        {
	&CGIheader();
	&print_form();
	}
    }
else
    {
    }
#&cleanup( 0 );
