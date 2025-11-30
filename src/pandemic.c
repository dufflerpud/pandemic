/*
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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "gen.h"
#include "text.h"

const char *progname = NULL;
const char *errlist[100];
const char **errs = errlist;

typedef const char *string;

#define NCOORDS			3
typedef struct position_struct
    {
    int				p[NCOORDS];
    } position;

#define HEALTH_STATES(V)	\
	V(hs_,Healthy)		\
	V(hs_,Replicating)	\
	V(hs_,Contagious)	\
	V(hs_,Symptomatic)	\
	V(hs_,Resistant)	\
	V(hs_,Quarantined)	\
	V(hs_,Dead)		\
	V(hs_,TOTAL)
string_enum_list( health_state_strings, health_state_enums, HEALTH_STATES );

#define FOREACH_HEALTH_STATE(v) for( health_state_enums v=0; v<hs_TOTAL; v++ )

struct state_count_struct
	{
	int			hs_count[hs_TOTAL];
	double			hs_R;
	};
typedef struct state_count_struct state_counts;

typedef state_counts *states_count_ptr;

#define VERBOSITIES(V)		\
	V(v_,Cycles)		\
	V(v_,World)		\
	V(v_,Events)		\
	V(v_,Movement)		\
	V(v_,Progress)		\
	V(v_,Rcalculations)	\
	V(v_,All)

int show_flags = 0;
#define SHOW(v)	(show_flags & (1<<(v)))
string_enum_list( show_strings, show_enums, VERBOSITIES );

typedef struct organism *organism_ptr;
struct organism
    {
    int			o_id;
    int			o_since_infection;	/* -2=dead -1=unInfected */
    int			o_has_infected;		/* Number organisms this I've infected */
    int			o_quarantined;		/* 0=no, 1=yes */
    organism_ptr	o_prev, o_next;
    position		o_pos;
    };

#define PARAMS(V)					\
	V(p_,Show,string,0,0,0,0)			\
	V(p_,Seed,int,0,0,100000,0)			\
	V(p_,Iterations,int,0,1,10000,1)		\
	V(p_,Tests,int,0,1,100000,1)			\
	V(p_,Output_File,string,0,0,0,0)		\
	V(p_,Max_Dist,double,1,0,10000.0,3.0)		\
	V(p_,Infected,int,1,0,1000,1)			\
	V(p_,BContagious,int,1,0,1000,2)		\
	V(p_,EContagious,int,1,1,1000,5)		\
	V(p_,BSymptomatic,int,1,0,1000,4)		\
	V(p_,ESymptomatic,int,1,1,1000,5)		\
	V(p_,TDeath,int,1,1,1000,10)			\
	V(p_,RDeath,double,1,0.0,1.0,0.5)		\
	V(p_,TQuarantine,int,1,1,1000,3)		\
	V(p_,RQuarantine,double,1,0.0,1.0,0.8)		\
	V(p_,EResistant,int,1,0,1000,500)		\
	V(p_,Population,int,1,1,100000,1250)		\
	V(p_,Volume,int,1,1,1000000,1)			\
	V(p_,Area,int,1,1,1000000,2500)			\
	V(p_,X,int,1,0,1000,1)				\
	V(p_,Y,int,1,0,1000,1)				\
	V(p_,Z,int,1,0,1000,1)				\
	V(p_,end,int,0,0,0,0)

#define PARAM_STRING(PREF,VAL,TYP,VRY,MINV,MAXV,DFLT)	#VAL,
#define PARAM_ENUM(PREF,VAL,TYP,VRY,MINV,MAXV,DFLT)	PREF##VAL,
#define PARAMV_STRUCT(PREF,VAR,TYP,VRY,MINV,MAXV,DFLT)	TYP VAR;
const char *param_strings[] = { PARAMS( PARAM_STRING ) NULL };
typedef enum { PARAMS( PARAM_ENUM ) } param_enums;

struct int_struct	{ int *ptr, vminv, vmaxv, vdiff, maxv, minv, dflt; };
struct double_struct	{ double *ptr, vminv, vmaxv, vdiff, maxv, minv, dflt; };
struct string_struct	{ char **ptr, *vminv, *vmaxv, *maxv, *minv, *dflt; };
struct params_struct
	{
	enum { t_int, t_double, t_string }	ps_type;
	int					ps_varies;
	union
	    {
	    struct int_struct			ps_int;
	    struct double_struct		ps_double;
	    struct string_struct		ps_string;
	    };
	} *params;

#define PARAMS_SETUP(PREF,VAR,TYP,VRY,MINV,MAXV,DFLT)		\
    params[PREF##VAR].ps_type=t_##TYP;				\
    params[PREF##VAR].ps_varies=VRY;				\
    params[PREF##VAR].ps_##TYP.minv = MINV;			\
    params[PREF##VAR].ps_##TYP.maxv = MAXV;			\
    params[PREF##VAR].ps_##TYP.ptr = (void*)&paramv.VAR;	\
    params[PREF##VAR].ps_##TYP.dflt =				\
    params[PREF##VAR].ps_##TYP.vminv =				\
    params[PREF##VAR].ps_##TYP.vmaxv =				\
    *(params[PREF##VAR].ps_##TYP.ptr) = DFLT;

struct paramv_struct
    {
    PARAMS( PARAMV_STRUCT )
    struct position_struct		size;
    } paramv;

#define FOREACH_PARAMETER(v) for( param_enums v=0; v<p_end; v++ )

#ifdef WORLD
organism_ptr world = NULL;
position world_multipliers = { 1, 1, 1 };
#endif

organism_ptr organisms = NULL;
organism_ptr first_org = NULL;
organism_ptr last_org = NULL;
int cycle;
int max_cycle = 0;

double calculated_R;
int total_infected;
int total_recovered;

FILE *temp_file = NULL;
FILE *data_file = NULL;
int baselen;
const char *temp_name = NULL;
const char *data_name = NULL;
const char *plot_name = NULL;

#define DEAD(o)		(  o->o_since_infection==-2 )
#define HEALTHY(o)	(  o->o_since_infection==-1 )
#define CONTAGIOUS(o)	(  o->o_since_infection>=paramv.BContagious \
			&& o->o_since_infection<=paramv.EContagious )
#define SYMPTOMATIC(o)	(  o->o_since_infection>=paramv.BSymptomatic \
			&& o->o_since_infection<=paramv.ESymptomatic )
#define RESISTANT(o)	(  o->o_since_infection>=0 \
			&& o->o_since_infection<=paramv.EResistant )
#define REPLICATING(o)	(  o->o_since_infection>=0 \
			&& o->o_since_infection<=paramv.BContagious \
			&& o->o_since_infection<=paramv.BSymptomatic )
#define QUARANTINED(o)	(  o->o_quarantined )

#define CYCLE_HEADER	"#cycles="

/************************************************************************/
/*	Add an error message to a list of error messages.		*/
/************************************************************************/
void eprintf( const char *fmt, ... )
    {
    char ebuf[100];
    va_list ap;
    va_start( ap, fmt );
    (void)vsprintf( ebuf, fmt, ap );
    va_end( ap );
    *errs++ = STRDUP(ebuf);
    }

/************************************************************************/
/*	Return string pointing to text showing health of organism.	*/
/************************************************************************/
const char *text_for( organism_ptr o )
    {
    return
	( DEAD(o) ? "=" :
	HEALTHY(o) ? "^" :
	REPLICATING(o) ? "R" :
	QUARANTINED(o) ? "Q" :
	CONTAGIOUS(o) ? (SYMPTOMATIC(o) ? "C" : "c") :
	SYMPTOMATIC(o) ? "s" :
	RESISTANT(o) ? "r" :
	":" );
    }

/************************************************************************/
/*	Return a random number between 0 and max			*/
/************************************************************************/
int rand_int( int max )		{ return ( max ? rand() % max : 0 ); }
int rand_double( double max )	{ return ( max ? max * drand48() : 0.0 ); }

/************************************************************************/
/*	Return a number in a base any base from 2 to 36.		*/
/************************************************************************/
const char *radix_string( int n, int radix )
    {
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    char stack[32];
    int i = 0;
    char *ret = cheap_buf();
    char *cp = ret;
    do  { stack[i++] = digits[n % radix]; } while( n/=radix );
    while( i-- > 0 ) *cp++ = stack[i];
    *cp++ = 0;
    return ret;
    }

/************************************************************************/
/*	Return a string representing a position.			*/
/************************************************************************/
const char *id_string( organism_ptr o )
    {
    return bprintf("[%s]",radix_string(o->o_id,36));
    }

/************************************************************************/
/*	Return a string representing a position.			*/
/************************************************************************/
char *position_string( position p )
    {
    char *position0 = cheap_buf();
    char *position1 = position0;

    *position1++ = '{';
    const char *sep = "";
    for( int i=0; i<NCOORDS && paramv.size.p[i]>1; i++ )
	{
	sprintf( position1, "%s%d", sep, p.p[i] );
	position1 += strlen(position1);
	sep = ",";
	}
    *position1++ = '}';
    *position1++ =0;
    return position0;
    }

/************************************************************************/
/*	Create a random position within dimension size			*/
/************************************************************************/
position random_position()
    {
    position result;
    for( int i=0; i<NCOORDS; i++ )
        result.p[i] = rand_int( paramv.size.p[i] );
    return result;
    }

/************************************************************************/
/*	Return Volume of current space.					*/
/************************************************************************/
int calculate_Volume()
    {
    int ret = 1;
    for( int d=0; d<NCOORDS && paramv.size.p[d]>1; d++ )
        ret *= paramv.size.p[d];
    return ret;
    }

/************************************************************************/
/*	Return true if these positions are the same.			*/
/************************************************************************/
int same_pos( position p0, position p1 )
    {
    for( int i=0; i<NCOORDS; i++ )
        if( p0.p[i] != p1.p[i] )
	     return 0;
    return 1;
    }

#ifdef WORLD_BOARD
/************************************************************************/
/*	Return pointer into world array from position.			*/
/************************************************************************/
int world_pointer( position pos )
    {
    int ind = 0;
    for( int i=0; i<NCOORDS; i++ )
        ind += pos.p[i]*world_multipliers[i];
    return &world[ind];
    }
#endif

/************************************************************************/
/*	Put organism in specified location by updating organism record	*/
/*	and placing it in world array.					*/
/************************************************************************/
void place_organism( organism_ptr o, position newpos )
    {
    if( o ) o->o_pos = newpos;
#ifdef WORLD_BOARD
    *(world_pointer(newpos)) = o;
#endif
    }

/************************************************************************/
/*	Return pointer to organism at specified position.		*/
/************************************************************************/
organism_ptr organism_at( position chkpos )
    {
#ifdef WORLD_BOARD
    return *(world_pointer(chkpos));
#else
    for( organism_ptr o=first_org; o; o=o->o_next )
	if( same_pos( o->o_pos, chkpos ) )
	    return o;
    
    return NULL;
#endif
    }

/************************************************************************/
/*	Return distance between two positions.				*/
/************************************************************************/
double distance( position p1, position p2 )
    {
    double accum = 0;
    for( int i=0; i<NCOORDS; i++ )
        {
	int d = p1.p[i] - p2.p[i];
	accum += (d*d);
	}
    return sqrt( accum );
    }

/************************************************************************/
/*	Return distance between two organisms				*/
/************************************************************************/
double organism_distance( organism_ptr o1, organism_ptr o2 )
    {
    double d = distance( o1->o_pos, o2->o_pos );
#ifdef BABBLE
    printf("Distance between %s %s and %s %s is %f.\n",
        id_string(o1), position_string(o1->o_pos),
        id_string(o2), position_string(o2->o_pos),
	d );
#endif
    return d;
    }

/************************************************************************/
/*	Create world according to variables specified on command line.	*/
/************************************************************************/
void experiment_setup()
    {
    if( organisms ) FREE( organisms );
    organisms =
	(organism_ptr)CALLOC( paramv.Population, sizeof(struct organism) );
    organism_ptr o;

#ifdef WORLD_BOARD
    if( world ) FREE( world );
    world = (organism_ptr)CALLOC( world_size, sizeof(organism_ptr) );
#endif

    if( paramv.Seed ) srand( paramv.Seed );

    for( int i=0; i<paramv.Population; i++ )   
        {
	o = &organisms[i];

	position try_pos;
	do {try_pos=random_position();}		/* Do this before schedule */
	    while( organism_at( try_pos ) );	/* list updated. */
	place_organism( o, try_pos );

	if( i == 0 )
	    first_org = o;
	else
	    {
	    o->o_prev = o - 1;
	    o->o_prev->o_next = o;
	    }
	last_org = o;

	o->o_id = i;
	o->o_since_infection = -1;
	}

    calculated_R = 0.0;

    for( int i=paramv.Infected; i-->0; )
        {
	do {o=&organisms[rand_int(paramv.Population)];}
	    while(o->o_since_infection>=0);
	o->o_since_infection = 0;
	}

    cycle = 0;
    }

/************************************************************************/
/*	Organism has just stopped being contagious (died, recovered).	*/
/************************************************************************/
void process_completion( organism_ptr cur_org )
    {
    total_infected += cur_org->o_has_infected;
    total_recovered++;
    if( SHOW(v_Rcalculations) )
	printf("%s completed, ti=%d tr=%d\n",
	    id_string(cur_org),total_infected,total_recovered);
    cur_org->o_has_infected = -1;
    }

/************************************************************************/
/*	Remove an organism from the schedule.				*/
/************************************************************************/
void kill_organism( organism_ptr o )
    {
    place_organism( NULL, o->o_pos );
    if( o->o_prev )
        o->o_prev->o_next = o->o_next;
    else
        first_org = o->o_next;
    if( o->o_next )
        o->o_next->o_prev = o->o_prev;
    else
        last_org = o->o_prev;
    if( SHOW(v_Events) ) printf("%s has died.\n",id_string(o));
    if( o->o_has_infected >= 0 ) process_completion( o );
    o->o_since_infection = -2;
    }

/************************************************************************/
/*	Determine if an organism is going to get Infected.		*/
/************************************************************************/
int check_Infected( organism_ptr cur_org )
    {
    for( organism_ptr o=first_org; o; o=o->o_next )
	{
	if( CONTAGIOUS(o) && ! QUARANTINED(o) )
	    {
	    /*	p=0 at maximum distance */
	    /*	p=1 on top of it */
	    double dist = organism_distance(cur_org,o);
	    double p = (paramv.Max_Dist - dist) / paramv.Max_Dist;
	    if( p > 1 ) p = 1.0; else if( p < 0 ) p = 0.0;
	    if( drand48() < p )
		{
		cur_org->o_since_infection = 0;
		cur_org->o_has_infected = 0;
		o->o_has_infected++;			/* For calculating R */
		if( SHOW(v_Events) )
		    printf( "%s is Infected by %s (dist=%f).\n",
		    id_string(cur_org), id_string(o), dist );
		return 1;
		}
	    }
	}
    return 0;
    }

/************************************************************************/
/*	Move an organism randomly (if its easy)				*/
/************************************************************************/
int live_life( organism_ptr cur_org )
    {
    for( int counter=4; counter>0; counter-- )
        {
	position try_coord = cur_org->o_pos;
	for( int i=0; i<NCOORDS; i++ )
	    if( paramv.size.p[i] > 1 )
	        {
		try_coord.p[i] += (rand_int(3) - 1);
		if( try_coord.p[i] < 0 )
		    try_coord.p[i] = 0;
		else if( try_coord.p[i] >= paramv.size.p[i] )
		    try_coord.p[i] = paramv.size.p[i] - 1;
		}
	if( ! organism_at( try_coord ) )
	    {
	    if( SHOW(v_Movement) )
		printf("Moving %s from %s to %s\n",
		    id_string(cur_org),
		    position_string(cur_org->o_pos),
		    position_string(try_coord) );
	    place_organism( NULL, cur_org->o_pos );	/* Remove from old */
	    place_organism( cur_org, try_coord );	/* Place in new */
	    return 1;
	    }
	}
    return 0;
    }

/************************************************************************/
/*	Move an organism and update its health.  Return its state.	*/
/************************************************************************/
void process_organism( organism_ptr cur_org )
    {
    if( SHOW(v_Progress) )
	printf("%d Begin processing %s (%s):  %d.\n", __LINE__,
	    id_string(cur_org),text_for(cur_org),cur_org->o_since_infection);
    
    if( QUARANTINED(cur_org) )
        {
	if( cur_org->o_since_infection > paramv.EContagious )
	    cur_org->o_quarantined = 0;
	}
    else
	{
	if( ! SYMPTOMATIC(cur_org) )	(void)live_life( cur_org );
	if( ! RESISTANT(cur_org) )	(void)check_Infected( cur_org );
	}

    if( cur_org->o_since_infection >= 0 )
        {
	cur_org->o_since_infection++;
	if( cur_org->o_since_infection == paramv.TDeath
	    && drand48() < paramv.RDeath )
	    kill_organism( cur_org );
	else
	    {
	    if( cur_org->o_since_infection > paramv.EContagious 
	        && cur_org->o_has_infected >= 0 )
	        process_completion( cur_org );
	    if( cur_org->o_since_infection == paramv.TQuarantine
		&& drand48() < paramv.RQuarantine )
		cur_org->o_quarantined = 1;
	    }
	}

    if( SHOW(v_Progress) )
        printf("%d End processing %s (%s):  %d.\n", __LINE__,
	    id_string(cur_org),text_for(cur_org),cur_org->o_since_infection);
    }

/************************************************************************/
/*	Count things we care about.  Return 1 if things could change.	*/
/************************************************************************/
int count( int test, int iteration )
    {
    state_counts in_state;

    memset( &in_state, 0, sizeof(in_state) );

    for( int i=paramv.Population; i-->0; )
        {
	organism_ptr o = &organisms[i];
	if( HEALTHY(o) )
	    in_state.hs_count[hs_Healthy]++;
	else if( DEAD(o) )
	    in_state.hs_count[hs_Dead]++;
	else
	    {
	    if( REPLICATING(o) )	in_state.hs_count[hs_Replicating]++;
	    if( CONTAGIOUS(o) )		in_state.hs_count[hs_Contagious]++;
	    if( SYMPTOMATIC(o) )	in_state.hs_count[hs_Symptomatic]++;
	    if( QUARANTINED(o) )	in_state.hs_count[hs_Quarantined]++;
	    if( RESISTANT(o) )		in_state.hs_count[hs_Resistant]++;
	    }
	}

    int in_flux =
	( ( in_state.hs_count[hs_Contagious]	+
	    in_state.hs_count[hs_Symptomatic]	+
	    in_state.hs_count[hs_Replicating]	) > 0 );

    if( total_recovered )
	calculated_R = (double)total_infected / total_recovered;
    in_state.hs_R = calculated_R;

    if( temp_file ) fwrite( &in_state, sizeof(in_state), 1, temp_file );

#ifdef debug
    printf("CMC t=%d i=%d",test,iteration);
    FOREACH_HEALTH_STATE(hs)
        {
	printf(" %s=%d",health_state_strings[hs],in_state.hs_count[hs]);
	}
    printf(" R=%f\n",in_state.hs_R);
#endif

    if( SHOW(v_Cycles) || !in_flux )
	{
	if( paramv.Tests > 1 )
	    printf("Test=%d ",test);
	if( paramv.Iterations > 1 )
	    printf("Iteration=%d ",iteration);
	printf("Cycle=%d",cycle);
	FOREACH_HEALTH_STATE(hs)
	    {
	    if( in_state.hs_count[hs] )
		printf( " %s=%d", health_state_strings[hs], in_state.hs_count[hs] );
	    }
	printf(" R=%f\n", in_state.hs_R );
	}
	
    return in_flux;
    }

/************************************************************************/
/*	Loop through all scheduled organisms keeping track of states.	*/
/************************************************************************/
int do_one_cycle( int test, int iteration )
    {
    total_infected = 0;
    total_recovered = 0;

    for( organism_ptr cur_org=first_org; cur_org; cur_org=cur_org->o_next )
	process_organism( cur_org );

    int in_flux = count( test, iteration );
    if( ++cycle > max_cycle ) max_cycle = cycle;
    return in_flux;
    }

/************************************************************************/
/*	Print state of every organism in the schedule.			*/
/************************************************************************/
void print_world( const char *fmt, ... )
    {
    if( fmt )
	{
	va_list ap;
	va_start( ap, fmt );
	(void)vprintf( fmt, ap );
	printf( ":\n" );
	va_end( ap );
	}
    position cp;
#ifndef notdef
    printf("x=%d y=%d z=%d 0=%d 1=%d 2=%d.\n",
        paramv.X,paramv.Y,paramv.Z,
	paramv.size.p[0], paramv.size.p[1], paramv.size.p[2]);
#endif
    for( cp.p[2]=0; cp.p[2]<paramv.size.p[2]; cp.p[2]++ )
	{
	printf("/");
	for(int i=0;i<paramv.size.p[0];i++)printf("----");
	printf("--\\\n");
	for( cp.p[1]=0; cp.p[1]<paramv.size.p[1]; cp.p[1]++ )
	    {
	    printf("|");
	    for( cp.p[0]=0; cp.p[0]<paramv.size.p[0]; cp.p[0]++ )
	        {
		organism_ptr o = organism_at( cp );
		if( ! o )
		    printf("    ");
		else
		    printf(" %2d%1.1s", o->o_since_infection, text_for(o) );
		}
	    printf("  |\n");
	    }
	printf("\\");
	for(int i=0;i<paramv.size.p[0];i++)printf("----");
	printf("--/\n\n");
	}
    fflush(stdout);
    }

/************************************************************************/
/*	Print error message and usage message.				*/
/************************************************************************/
void dump_params( FILE *out, const char *fmt, ... )
    {
    if( fmt )
	{
	va_list ap;
	va_start( ap, fmt );
	(void)vfprintf( out, fmt, ap );
	fprintf( out, ":\n" );
	va_end( ap );
	}
    fprintf(out,"%20s=%-18s%10s%10s%10s%10s\n",
	"<Variable>",
	"<Form>",
	"<Default>",
	"<Minimum>",
	"<Maximum>",
	( fmt ? "<Value>" : "") );
    FOREACH_PARAMETER(p)
	{
	fprintf(out,"%20s=",param_strings[p]);
	switch( params[p].ps_type )
	    {
	    case t_int:
		{
		struct int_struct *sp = &params[p].ps_int;
		fprintf(out,
		    "%-18s%10d%10d%10d%10s",
		    ( fmt
		    ? ( params[p].ps_varies
			? bprintf("%d,%d",sp->vminv,sp->vmaxv)
			: bprintf("%d",sp->vminv)
		      )
		    : ( params[p].ps_varies ? "<int>[<int>]" : "<int>" ) ),
		    sp->dflt, sp->minv, sp->maxv,
		    ( fmt ? bprintf("%d",*(sp->ptr)) : "") );
		}
		break;
	    case t_double:
		{
		struct double_struct *sp = &params[p].ps_double;
		fprintf(out,
		    "%-18s%10.2f%10.2f%10.2f%10s",
		    ( fmt
		    ? ( params[p].ps_varies
			? bprintf("%.2f,%.2f",sp->vminv,sp->vmaxv)
			: bprintf("%.2f",sp->vminv)
		      )
		    : ( params[p].ps_varies ? "<float>[<float>]" : "<float>" ) ),
		    sp->dflt, sp->minv, sp->maxv,
		    ( fmt ? bprintf("%.2f",*(sp->ptr)) : "") );
		}
		break;
	    case t_string:
		{
		struct string_struct *sp = &params[p].ps_string;
		fprintf(out,
		    "%-18s%10s%30s", "<string>", sp->dflt,
		    ( fmt ? *(sp->ptr) : "" ) );
		}
		break;
	    }
	fprintf(out,"\n");
	}
    }

/************************************************************************/
/*	Set x, y, and z arguments dependent on Volume and Area.		*/
/************************************************************************/
void calculate_dimensions()
    {
    if( paramv.Volume > 1 )
	paramv.X=paramv.Y=paramv.Z=cbrt(paramv.Volume);
    else if( paramv.Area > 1 )
	paramv.X=paramv.Y=sqrt(paramv.Area); paramv.Z=1;
    paramv.size.p[0] = paramv.X;
    paramv.size.p[1] = paramv.Y;
    paramv.size.p[2] = paramv.Z;
    }

/************************************************************************/
/*	Parse arguments.						*/
/************************************************************************/
void parse_arguments( int argc, const char *argv[] )
    {
    for( const char **ap=argv+1; *ap; ap++ )
        {
	char *lbuf = cheap_buf();
	const char *arg = *ap;
	int i;
	for( i=0; arg[i]; i++ )
	    if( arg[i] != '=' )
	        lbuf[i] = arg[i];
	    else
	        break;
	if( arg[i] != '=' )
	    eprintf("%s not in correct format.",arg);
	else
	    {
	    lbuf[i++] = 0;
	    int abv = abbrev( lbuf, param_strings );
	    switch( abv )
		{
		case ABRV_NONE_CLOSE:
		    eprintf("%s is not a recognized parameter.",lbuf);
		    break;
		case ABRV_AMBIGUOUS:
		    eprintf("%s is not an ambiguous parameter.",lbuf);
		    break;
		default:
		    {
		    param_enums p = (param_enums)abv;
		    if( params[p].ps_type == t_string )
			{
			/* params[p].ps_string.ptr = STRDUP(arg+i); */
			*(params[p].ps_string.ptr) = STRDUP(arg+i);
			/* paramv.Output_File = STRDUP(arg+i); */
			}
		    else
			{
			const char *maxv_str = lbuf;
			for( int j=0; (lbuf[j++]=arg[i]); i++ )
			    {
			    if( arg[i] == ',' )
				{
				lbuf[j-1] = 0;
				maxv_str = lbuf + j;
				}
			    }
			switch( params[p].ps_type )
			    {
			    case t_int:	
				{
				struct int_struct *sp = &params[p].ps_int;
				if( (sp->vminv=atoi(lbuf)) < sp->minv )
				    eprintf("%s less than minimum %s %d.\n",
					lbuf,param_strings[p],sp->minv);
				if( (sp->vmaxv=atoi(maxv_str)) > sp->maxv )
				    eprintf("%s more than maximum %s %d.\n",
					maxv_str,param_strings[p],sp->maxv);
				if( (sp->vdiff = sp->vmaxv - sp->vminv) < 0 )
				    eprintf("%s,%s probably swapped with %s.\n",
					lbuf,maxv_str,param_strings[p]);
				*(sp->ptr) = sp->vminv;
				}
				break;
			    case t_double:
				{
				struct double_struct *sp = &params[p].ps_double;
				if( (sp->vminv=atof(lbuf)) < sp->minv )
				    eprintf("%s less than minimum %f.\n",
					lbuf,sp->minv);
				if( (sp->vmaxv=atof(maxv_str)) > sp->maxv )
				    eprintf("%s more than maximum %f.\n",
					maxv_str,sp->maxv);
				if( (sp->vdiff = sp->vmaxv - sp->vminv) < 0 )
				    eprintf("%s,%s probably swapped.\n",
					lbuf,maxv_str);
				*(sp->ptr) = sp->vminv;
				}
				break;
			    case t_string:
				/* This will not happen */
				break;
			    }
			}
		    }
		}
	    }
	}

    calculate_dimensions();

    if( paramv.Tests <= 0 ) paramv.Tests = 1;

    /* Verify everything we need set has been set. */
    if( paramv.Population > calculate_Volume() / 2 )
        eprintf("Population too dense for Area specified.");
    if(paramv.Population<0)	eprintf("Must specify Population=");
    if(paramv.Max_Dist<0)	eprintf("Must specify Max_Dist=");
    if(paramv.Infected<=0)	eprintf("Must specify Infected=");
    if(paramv.EResistant<0)	eprintf("Must specify EResistant=");

    if( paramv.ESymptomatic < paramv.BSymptomatic )
        eprintf("ESymptomatic must be after BSymptomatic.");
    if( paramv.EContagious < paramv.BContagious )
        eprintf("EContagious must be after BContagious.");
    if( paramv.TDeath < paramv.BSymptomatic )
        eprintf("TDeath must be after BSymptomatic.");
    if(paramv.TDeath<0)		eprintf("Must specify TDeath=");
    if(paramv.EContagious<0)	eprintf("Must specify EContagious=");
    if(paramv.RDeath<0)		eprintf("Must specify RDeath=");

    if( paramv.Show )
        {
	char *cp0 = STRDUP( paramv.Show );
	char *cp1 = cp0;
	while( (cp1 = strtok(cp1,",")) )
	    {
	    int abv = abbrev(cp1,show_strings);
	    if( abv == ABRV_NONE_CLOSE )
	        eprintf("No show flag called [%s]",cp1);
	    else if( abv == ABRV_AMBIGUOUS )
	        eprintf("[%s] is an abiguous show flag",cp1);
	    else
		show_flags |= ( abv==v_All ? -1 : (1<<abv) );
	    cp1 = NULL;
	    }
	FREE( cp0 );
	}
    }

/************************************************************************/
/*	Set parameters according to test number				*/
/************************************************************************/
void set_test_parameters( int test )
    {
    int div_by = paramv.Tests - 1;

    FOREACH_PARAMETER(p)
	{
	switch( params[p].ps_type )
	    {
	    case t_string:	break;
	    case t_int:
		{
		struct int_struct *sp = &params[p].ps_int;
		*(sp->ptr) =
		    sp->vminv +
		    ( test == 0
		    ? 0
		    : test*sp->vdiff/div_by
		    );
		}	
		break;
	    case t_double:
		{
		struct double_struct *sp = &params[p].ps_double;
		*(sp->ptr) =
		    sp->vminv +
		    ( test == 0
		    ? 0
		    : test*sp->vdiff/div_by
		    );
		}	
		break;
	    }
	}
    calculate_dimensions();

#ifdef WORLD_BOARD
    int world_size = 1;
    for( int i=0; i<NCOORDS; i++ )
	{
	world_multipliers.p[i] = world_size;
        world_size *= paramv.size.p[i];
	}
#endif
    }

/************************************************************************/
/*	Open data files to track iteration results and run tests.	*/
/************************************************************************/
void run_all_tests()
    {
    struct iteration_struct
        {
	int			is_number_cycles;
	states_count_ptr	is_states;
	} *iter;

    dump_params( stdout, "Parameters" );

    iter = (struct iteration_struct *)
        malloc( sizeof(struct iteration_struct)*paramv.Iterations );
    for( int test=0; test < paramv.Tests; test++ )
	{
	set_test_parameters( test );

	if( temp_name && (temp_file=fopen(temp_name,"wb")) == NULL )
	    fatal(F,"Cannot write %s:  %m",temp_name);

	size_t total_mem = 0;
	for( int iteration=0; iteration < paramv.Iterations; iteration++ )
	    {
	    experiment_setup();

	    if( SHOW(v_World) ) print_world("Pre experiment loop");
	    if( SHOW(v_Progress) )
		dump_params(stdout,"Parameters at %d, test=%d iteration=%d",
		    __LINE__,test,iteration);
	    while( do_one_cycle( test, iteration ) )
		if( SHOW(v_Progress) )
		    print_world("Experiment loop");
	    iter[iteration].is_number_cycles = cycle;
	    total_mem += sizeof(state_counts)*cycle;
	    }

	if( temp_file )
	    {
	    fclose( temp_file );
	    temp_file = NULL;

	    states_count_ptr scp = (states_count_ptr)malloc( total_mem );

	    int fd;
	    if( (fd = open(temp_name,0)) < 0 )
		fatal(F,"Cannot open %s:  %m",temp_name);
	    if( read(fd,scp,total_mem) != total_mem )
		fatal(F,"Cannot read %s:  %m",temp_name);
	    close( fd );

	    states_count_ptr tscp = scp;
	    for( int iteration=0; iteration<paramv.Iterations; iteration++ )
		{
		iter[iteration].is_states = tscp;
		tscp += iter[iteration].is_number_cycles;
		}

	    for( cycle=0; cycle<max_cycle; cycle++ )
		{
		if( data_file )
		    fprintf(data_file,"%d %d",cycle,test);
		else
		    {
		    if( paramv.Tests > 1 ) printf("Test=%d ",test);
		    printf("Cycle=%d",cycle);
		    }
		FOREACH_HEALTH_STATE(hs)
		    {
		    int sum = 0;
		    for( int iteration=0; iteration<paramv.Iterations; iteration++ )
			{
			int c = iter[iteration].is_number_cycles - 1;
			if( cycle < c ) c = cycle;
			int n = iter[iteration].is_states[c].hs_count[hs];
			sum += n;
			}
		    double avg = (double)sum/paramv.Iterations;
		    if( data_file )
			fprintf(data_file," %f",avg);
		    else if( avg > 0.0 )
		        printf(" %s=%.2f",health_state_strings[hs],avg);
		    }
		double rsum = 0.0;
		for( int iteration=0; iteration<paramv.Iterations; iteration++ )
		    {
		    int c = iter[iteration].is_number_cycles - 1;
		    if( cycle < c ) c = cycle;
		    rsum += iter[iteration].is_states[c].hs_R;
		    }
		rsum /= paramv.Iterations;
		if( data_file )
		    fprintf( data_file, " %f", rsum );
		else if( rsum > 0.0 )
		    printf(" R=%.2f", rsum );
		fprintf( ( data_file ? data_file : stdout ),"\n");
		}
	    free( scp );
	    }
	}
    free( iter );
    }

/************************************************************************/
/*	Setup output files.						*/
/************************************************************************/
void output_setup()
    {
    baselen = strlen( paramv.Output_File );
    while( baselen>0 && paramv.Output_File[baselen-1] != '.' )
	{ baselen--; }
    if( baselen <= 0 )
	fatal(F,"Output file %s does not have an extension.",
	    paramv.Output_File);
    const char *ext = paramv.Output_File + baselen;

    temp_name   = mprintf("%.*s%s",baselen,paramv.Output_File,"temp"  );
    data_name   = mprintf("%.*s%s",baselen,paramv.Output_File,"data"  );
    if( strcmp(ext,"data") ) plot_name = paramv.Output_File;

    if( (data_file=fopen(data_name,"w")) == NULL )
        fatal(F,"Cannot write %s:  %m",data_name);
    fprintf( data_file, "%s%012d#tests=%d",CYCLE_HEADER,0,paramv.Tests);
    FOREACH_HEALTH_STATE(hs)
	fprintf( data_file, "#%s", health_state_strings[hs] );
    fprintf( data_file, "#R\n" );
    }

/************************************************************************/
/*	Finish out the data file.					*/
/************************************************************************/
void output_finish()
    {
    int is3d = ( paramv.Tests > 1 );

    fflush( data_file );
    rewind( data_file );
    fprintf( data_file, "%s%012d", CYCLE_HEADER, max_cycle );
    fflush( data_file );
    fclose( data_file );

    /* system( bprintf("chmod +x %s; ./%s",script_name,script_name) ); */
    }

/************************************************************************/
/*	Main								*/
/************************************************************************/
int main( int argc, const char *argv[] )
    {
    progname = get_progname( argv[0] );

    params = CALLOC( p_end+1, sizeof(struct params_struct) );
    PARAMS( PARAMS_SETUP )
    parse_arguments( argc, argv );
    if( errs != errlist )
	{
	*errs = NULL;
	for( errs=errlist; *errs; errs++ )
	    fprintf(stderr,"%s\n",*errs);
	fprintf(stderr,"\nUsage:  %s { <arg> }\n",progname);
	fprintf(stderr," Where <arg> is one or more of:\n");
	dump_params(stderr,NULL);
	exit(1);
	}

    if( ! paramv.Seed ) srand( time(NULL) );

    if( paramv.Output_File )
	output_setup();
    else if( paramv.Iterations > 1 )
        /* temp_name = mprintf("/tmp/%s.%d",progname,getpid()); */
        temp_name = mprintf("/tmp/%s.temp",progname);

    run_all_tests();

    if( paramv.Output_File ) output_finish();

    if( temp_name ) unlink( temp_name );

    if(plot_name) system(bprintf("pandaplot -i%s -o%s",data_name,plot_name));
    exit(0);
    }
