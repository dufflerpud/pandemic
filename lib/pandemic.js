<style type="text/css">
//@HDR@	$Id$
//@HDR@		Copyright 2024 by
//@HDR@		Christopher Caldwell/Brightsands
//@HDR@		P.O. Box 401, Bailey Island, ME 04003
//@HDR@		All Rights Reserved
//@HDR@
//@HDR@	This software comprises unpublished confidential information
//@HDR@	of Brightsands and may not be used, copied or made available
//@HDR@	to anyone, except in accordance with the license under which
//@HDR@	it is furnished.

.slider {
    height:		4px;
    border-radius:	2px;
    background-color:	#f00;
    color:		#0f0;
    border-color:	blue;
}

</style>

<script>
var disease_ptr;
var state_ptr;

var PREFIXES = ["","r_","mn_","r_mn_","mx_","r_mx_"];
//////////////////////////////////////////////////////////////////////////
//	Called when user selects a disease to change settings.		//
//	DISEASES defined in config.js.					//
//////////////////////////////////////////////////////////////////////////
function set_disease( disease )
    {
    for ( const feature in DISEASES[disease] )
        {
	var val = DISEASES[disease][feature];
	for ( const prefix of PREFIXES )
	    {
	    if( window.document.form[prefix+feature] )
		{ window.document.form[prefix+feature].value=val; }
	    }
	window.document.form["range_"+feature].value = val;
	}
    redraw_disease_selector();
    }

//////////////////////////////////////////////////////////////////////////
//	Called when user clicks on a variable.				//
//	var help defined in config.js					//
//////////////////////////////////////////////////////////////////////////
function old_show_help( v )
    {
    alert( HELP[v] );
    }

//////////////////////////////////////////////////////////////////////////
//	Called when a variable is "focused on" to bring up widgest.	//
//////////////////////////////////////////////////////////////////////////
function show_input( v0 )
    {
    var p = document.getElementById("help_"+v0);
    if( ! p ) alert("Cannot find help_"+v0);
    p.innerHTML = HELP[v0];
    for (const v1 in HELP )
	{
	var ep = document.getElementById("enabled_"+v1);
	if( ep )
	    {
	    var dp = document.getElementById("disabled_"+v1);
	    if( v1 == v0 )
		{ 
		ep.style.display = "block";
		dp.style.display = "none";
		}
	    else
		{ 
		ep.style.display = "none";
		dp.style.display = "block";
		}
	    }
	}
    }

//////////////////////////////////////////////////////////////////////////
//	Called when user changes a variable (either in number or range)	//
//////////////////////////////////////////////////////////////////////////
function change( vname, prefix, ityp, newval )
    {
    newval -= 0;
    // alert("change("+vname+","+prefix+","+ityp+","+newval+")");
    with ( window.document )
        {
	form[ityp+prefix+vname].value = newval;
	if( prefix == "mn_" && (form["mx_"+vname].value-0) < newval )
	    { form["mx_"+vname].value = form["r_mx_"+vname].value = newval; }
	else if( prefix == "mx_" && (form["mn_"+vname].value-0) > newval )
	    { form["mn_"+vname].value = form["r_mn_"+vname].value = newval; }
	
	if( prefix == "" )
	    { form["range_"+vname].value = newval; }
	else if( form["mn_"+vname].value == form["mx_"+vname].value )
	    form["range_"+vname].value = newval;
	else
	    {
	    form["range_"+vname].value =
		form["mn_"+vname].value + "-" + form["mx_"+vname].value;
	    }

	if( 0 && (vname == "Volume" || vname == "Area") )
	    {
	    var newval;
	    if( vname == "Volume" )
	        {
		newval = Math.ceil( Math.cbrt(newval) );
		form.mn_Area.value
		    = form.mx_Area.value
		    = form.r_mn_Area.value
		    = form.r_mx_Area.value
		    = 1;
		}
	    else
	        {
		newval = Math.ceil( Math.sqrt(newval) );
		form.mn_Volume.value
		    = form.r_mn_Volume.value
		    = form.mx_Volume.value
		    = form.r_mx_Volume.value
		    = 1;
		}
	    for ( const ind of [ "X", "Y", "Z" ] )
	        {
		if( vname == "Area" && ind == "Z" ) { newval = 1; }
		form[prefix+ind].value = newval;
		form["r_"+prefix+ind].value = newval;
		if( prefix == "mn_" && form["mx_"+ind].value < newval )
		    { form["mx_"+ind].value=form["r_mx_"+ind].value=newval; }
		else if( prefix == "mx_" && form["mn_"+ind].value > newval )
		    { form["mn_"+ind].value=form["r_mn_"+ind].value=newval; }
		}
	    }
	}
    redraw_disease_selector();
    }

//////////////////////////////////////////////////////////////////////////
//	Redraw disease <selector> noting disease currently selected	//
//////////////////////////////////////////////////////////////////////////
function redraw_disease_selector()
    {
    var selflags = { none: " selected" };
    with ( window.document )
	{
	for (const disease in DISEASES )
	    {
	    var couldbe = 1;

	    for ( const feature in DISEASES[disease] )
		{
		var val = DISEASES[disease][feature];
		for ( const prefix of ["","mn_","mx_"])
		    {
		    if( form[prefix+feature]
			&& form[prefix+feature].value != val )
			{ couldbe = 0; }
		    }
		}

	    if( couldbe )
	        { selflags[disease] = " selected"; selflags.none = ""; }
	    else
	        { selflags[disease] = ""; }
	    }
	}

    var w = "<select help='Select_disease' onChange='set_disease(this.value);'>"
	+ "<option value='Select disease'" + selflags.none + ">"
	+ "XL(Select disease)</option>";
    for ( const disease in DISEASES )
        {
	w += "<option value='" + disease + "'" + selflags[disease] + ">";
	w += disease + "</option>";
	}
    w += "</select>";
    disease_ptr.innerHTML = w;
    }

//////////////////////////////////////////////////////////////////////////
//	Draw state buttons						//
//////////////////////////////////////////////////////////////////////////
function redraw_state_buttons()
    {
    var w = "";
    for ( const hs in PLOTTING_STATES )
        {
	var c = PLOTTING_STATES[hs].color;
	w += "<input type=checkbox name=show_"+hs+" value="+c+""
	    + ( PLOTTING_STATES[hs].default_on ? " checked" : "" )
	    + "><font color="+c+">"+hs+"</font>";
	}
    state_ptr.innerHTML = w;
    }

//////////////////////////////////////////////////////////////////////////
//	Invoked when rest of page is drawn.				//
//////////////////////////////////////////////////////////////////////////
function setup_javascript()
    {
    disease_ptr = document.getElementById("Disease_Selector");
    state_ptr = document.getElementById("State_Buttons");
    redraw_disease_selector();
    redraw_state_buttons();
    }

</script>
