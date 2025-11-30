<script>
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

var HELP =
    {
    Show:		"List of debug output flags",
    Seed:		"Random number seed to reset to for each test\n"
	+		"(0 means set seed to time and have total randomness)",
    Iterations:		"Number of times to perform test with same variables"
        +		" for averaging",
    Tests:		"Number of tests to run to spread out ranges of"
        +		" variables",
    Output_File:	"File to put resulting data in",
    Max_Dist:		"Maximum distance organisms can be apart and get"
        +		" infected",
    Infected:		"Number of infected organisms at start of test",
    BContagious:	"Number of turns since infection before organism is"
        +		" contagious",
    EContagious:	"Number of turns since infection before\n"
	+		"organism ceases to be contagious",
    BSymptomatic:	"Number of turns since infection before\n"
	+		"organism shows symptoms",
    ESymptomatic:	"Number of turns since infection before\n"
	+		"organism ceases to show symptoms",
    TDeath:		"Number of turns before sick organism dies"
        +		" (if it's going to)",
    RDeath:		"Ratio of sick organisms that will die\n"
	+		"(0.00=none,1.00=all)",
    TQuarantine:	"Time after infection organism will be quarantined",
    RQuarantine:	"Ratio of sick organisms that will be quarantined\n"
	+		"(0.00=none,1.00=all)",
    EResistant:		"Time after infection that organism will not get"
	+		" infected again",
    Population:		"Number of organisms to test",
    Volume:		"Volume of workspace (will be approximately a cube)",
    Area:		"Area of workspace (will be approximately a square)",
    X:			"Width of workspace",
    Y:			"Height of workspace",
    Z:			"Depth of workspace"
    };

var PLOTTING_STATES =
    {
    Healthy:		{ color:"green",	default_on:1	},
    Replicating:	{ color:"pink",		default_on:0	},
    Contagious:		{ color:"red",		default_on:1	},
    Symptomatic:	{ color:"orange",	default_on:0	},
    Resistant:		{ color:"blue",		default_on:0	},
    Quarantined:	{ color:"brown",	default_on:0	},
    Dead:		{ color:"black",	default_on:1	},
    R:			{ color:"purple",	default_on:0	}
    };

</script>
