#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <hash.h>
#include <ctype.h>
#include <time.h>
#include <util.h>
#include <sched_kw.h>
#include <sched_util.h>
#include <sched_kw_gruptree.h>
#include <sched_kw_tstep.h>
#include <sched_kw_dates.h>
#include <sched_kw_wconhist.h>
#include <sched_kw_wconinjh.h>
#include <sched_kw_welspecs.h>
#include <sched_kw_wconprod.h>
#include <sched_kw_wconinj.h>
#include <sched_kw_wconinje.h>
#include <sched_kw_compdat.h>
#include <sched_kw_untyped.h>
#include <sched_kw_include.h>
#include <sched_macros.h>
#include <stringlist.h>

/*
  The structure sched_kw_type is used for internalization
  of arbitrary keywords in an ECLIPSE schedule file/section.

  Two structs are defined in this file:

    1. The sched_kw_type, which can be accessed externaly
       through various interface functions.
    2. The data_handlers_type, which provides an abstraction
       for the data_handling of the various keywords. This
       is for internal use only.

  Keywords from the ECLIPSE schedule are divided into three
  different groups:

    1. Fully internalized keywords, e.g. GRUPTREE.
       Functions implementing the data_handlers and
       more for these keywords are found in separate
       files, e.g. sched_kw_gruptree.c.

    2. Keywords which are known to have a fixed number
       of records, but not having a full internal
       representation. The number of records for these
       keywords are specified in the function
       get_fixed_record_length  in the file
       sched_kw_untyped.c

    3. Keywords which are not implemented and have a
       variable record length. These are handled 
       automatically by sched_kw_untyped.c.

*/

typedef void * (data_token_alloc_proto)  ( const stringlist_type * , int * );
typedef void   (data_free_proto)         ( void *);
typedef void   (data_fprintf_proto)      ( const void *, FILE *);
typedef void * (alloc_copy_proto)        ( const void *);


struct data_handlers_struct {
  data_token_alloc_proto  * token_alloc;
  data_free_proto         * free;
  data_fprintf_proto      * fprintf;
  alloc_copy_proto        * copyc;
};


struct sched_kw_struct {
  char                    * kw_name; 
  sched_type_enum           type;
  int                       restart_nr;  /* The block nr owning this instance. */
  

  /* Function pointers to work on the data pointer. */
  data_token_alloc_proto  * alloc;
  data_free_proto         * free;
  data_fprintf_proto      * fprintf;
  alloc_copy_proto        * copyc;
  
  void                    * data;        /* A void point pointer to a detailed implementation - i.e. sched_kw_wconhist. */
};




/**
   This function does a direct translation of a string name to
   implementation type - i.e. an enum instance. Observe that
   (currently) no case-normalization is performed.
*/

static sched_type_enum get_sched_type_from_string(const char * kw_name)
{
  sched_type_enum kw_type = UNTYPED;

  if     ( strcmp(kw_name, GRUPTREE_STRING ) == 0) kw_type = GRUPTREE ;
  else if( strcmp(kw_name, TSTEP_STRING    ) == 0) kw_type = TSTEP    ;
  else if( strcmp(kw_name, INCLUDE_STRING  ) == 0) kw_type = INCLUDE  ;
  else if( strcmp(kw_name, TIME_STRING     ) == 0) kw_type = TIME     ;
  else if( strcmp(kw_name, DATES_STRING    ) == 0) kw_type = DATES    ;
  else if( strcmp(kw_name, WCONHIST_STRING ) == 0) kw_type = WCONHIST ;
  else if( strcmp(kw_name, WELSPECS_STRING ) == 0) kw_type = WELSPECS ;
  else if( strcmp(kw_name, WCONINJ_STRING  ) == 0) kw_type = WCONINJ  ;
  else if( strcmp(kw_name, WCONINJE_STRING ) == 0) kw_type = WCONINJE ;
  else if( strcmp(kw_name, WCONINJH_STRING ) == 0) kw_type = WCONINJH ;
  else if( strcmp(kw_name, WCONPROD_STRING ) == 0) kw_type = WCONPROD ;
  else if( strcmp(kw_name, COMPDAT_STRING  ) == 0) kw_type = COMPDAT  ;   
  
  return kw_type;
}



static const char * get_name_from_type(sched_type_enum kw_type) {
  if      ( kw_type == GRUPTREE ) return GRUPTREE_STRING ;
  else if ( kw_type == TSTEP    ) return TSTEP_STRING    ;
  else if ( kw_type == INCLUDE  ) return INCLUDE_STRING  ;
  else if ( kw_type == TIME     ) return TIME_STRING     ;
  else if ( kw_type == DATES    ) return DATES_STRING    ;
  else if ( kw_type == WCONHIST ) return WCONHIST_STRING ;
  else if ( kw_type == WELSPECS ) return WELSPECS_STRING ;
  else if ( kw_type == WCONINJ  ) return WCONINJ_STRING  ;
  else if ( kw_type == WCONINJE ) return WCONINJE_STRING ;
  else if ( kw_type == WCONINJH ) return WCONINJH_STRING ;
  else if ( kw_type == WCONPROD ) return WCONPROD_STRING ;
  else if ( kw_type == COMPDAT  ) return COMPDAT_STRING  ;   

  return UNTYPED_STRING; /* Unknown type */
}



/*****************************************************************/

/**
   Nothing like a little manual inheritance....
*/

static sched_kw_type * sched_kw_alloc_empty( const char * kw_name ) {
  sched_kw_type * kw = util_malloc(sizeof * kw, __func__);
  kw->kw_name = util_alloc_string_copy( kw_name );
  kw->type    = get_sched_type_from_string( kw_name );
  
  switch( kw->type ) {
  case(WCONHIST):
    kw->alloc   = sched_kw_wconhist_alloc__;
    kw->free    = sched_kw_wconhist_free__;
    kw->fprintf = sched_kw_wconhist_fprintf__;
    kw->copyc   = sched_kw_wconhist_copyc__;
    break;
  case(DATES):
    kw->alloc   = sched_kw_dates_alloc__;
    kw->free    = sched_kw_dates_free__;
    kw->fprintf = sched_kw_dates_fprintf__;
    kw->copyc   = sched_kw_dates_copyc__;
    break;
  case(TSTEP):
    kw->alloc   = sched_kw_tstep_alloc__;
    kw->free    = sched_kw_tstep_free__;
    kw->fprintf = sched_kw_tstep_fprintf__;
    kw->copyc   = sched_kw_tstep_copyc__;
    break;
  case(COMPDAT):
    kw->alloc   = sched_kw_compdat_alloc__;
    kw->free    = sched_kw_compdat_free__;
    kw->fprintf = sched_kw_compdat_fprintf__;
    kw->copyc   = sched_kw_compdat_copyc__;
    break;
  case(WELSPECS):
    kw->alloc   = sched_kw_welspecs_alloc__;
    kw->free    = sched_kw_welspecs_free__;
    kw->fprintf = sched_kw_welspecs_fprintf__;
    kw->copyc   = sched_kw_welspecs_copyc__;
    break;
  case(GRUPTREE):
    kw->alloc   = sched_kw_gruptree_alloc__;
    kw->free    = sched_kw_gruptree_free__;
    kw->fprintf = sched_kw_gruptree_fprintf__;
    kw->copyc   = sched_kw_gruptree_copyc__;
    break;
  case(INCLUDE):
    kw->alloc   = sched_kw_include_alloc__;
    kw->free    = sched_kw_include_free__;
    kw->fprintf = sched_kw_include_fprintf__;
    kw->copyc   = sched_kw_include_copyc__;
    break;
  case(UNTYPED):
    /** 
        Observe that the untyped keyword uses a custom allocator
        function, because it needs to get the keyword length as extra
        input. 
    */
    kw->alloc   = NULL;    
    kw->free    = sched_kw_untyped_free__;
    kw->fprintf = sched_kw_untyped_fprintf__;
    kw->copyc   = sched_kw_untyped_copyc__;
    break;
  case(WCONINJ):
    kw->alloc   = sched_kw_wconinj_alloc__;
    kw->free    = sched_kw_wconinj_free__;
    kw->fprintf = sched_kw_wconinj_fprintf__;
    kw->copyc   = sched_kw_wconinj_copyc__;
    break;
  case(WCONINJE):
    kw->alloc   = sched_kw_wconinje_alloc__;
    kw->free    = sched_kw_wconinje_free__;
    kw->fprintf = sched_kw_wconinje_fprintf__;
    kw->copyc   = sched_kw_wconinje_copyc__;
    break;
  case(WCONINJH):
    kw->alloc   = sched_kw_wconinjh_alloc__;
    kw->free    = sched_kw_wconinjh_free__;
    kw->fprintf = sched_kw_wconinjh_fprintf__;
    kw->copyc   = sched_kw_wconinjh_copyc__;
    break;
  case(WCONPROD):
    kw->alloc   = sched_kw_wconprod_alloc__;
    kw->free    = sched_kw_wconprod_free__;
    kw->fprintf = sched_kw_wconprod_fprintf__;
    kw->copyc   = sched_kw_wconprod_copyc__;
    break;
  default:
    util_abort("%s: unrecognized type:%d \n",__func__ , kw->type );
  }
  return kw;
}




/*
  This tries to check if kw_name is a valid keyword in an ECLIPSE
  schedule file. It is essentially based on checking that there are
  not more argeuments on the line:

   OK:
   -------------------
   RPTSCHED
    arg1 arg2 arg2 ....

   
   Invalid:
   ------------------- 
   RPTSCHED arg1 arg2 arg3 ...

   Quite naive .... 
*/
static void sched_kw_name_assert(const char * kw_name , FILE * stream)
{
  if(kw_name == NULL)
  {
    fprintf(stderr,"** Parsing SCHEDULE file line-nr: %d \n",util_get_current_linenr(stream));
    util_abort("%s: Internal error - trying to dereference NULL pointer.\n",__func__);
  }
  
  {
    bool valid_kw = true;
    for (int i = 0; i < strlen(kw_name); i++)
      if (isspace(kw_name[i])) 
        valid_kw = false;

    if (!valid_kw) {
      if (stream != NULL)
        fprintf(stderr,"** Parsing SCHEDULE file line-nr: %d \n",util_get_current_linenr(stream));
      util_abort("%s: \"%s\" is not a valid schedule kw - aborting.\n",__func__ , kw_name);
    }
  }
}




static sched_kw_type ** sched_kw_tstep_split_alloc(const sched_kw_type * sched_kw, int * num_steps)
{
  *num_steps = sched_kw_tstep_get_size(sched_kw->data);
  sched_kw_type ** sched_kw_tsteps = util_malloc(*num_steps * sizeof * sched_kw_tsteps, __func__);
  
  for(int i=0; i<*num_steps; i++) {
    sched_kw_tsteps[i] = sched_kw_alloc_empty( "TSTEP" );
    double step = sched_kw_tstep_iget_step((const sched_kw_tstep_type *) sched_kw->data, i);
    sched_kw_tsteps[i]->data = sched_kw_tstep_alloc_from_double(step);
  }
  
  return sched_kw_tsteps;
}



static sched_kw_type ** sched_kw_dates_split_alloc(const sched_kw_type * sched_kw, int * num_steps) 
{
  *num_steps = sched_kw_dates_get_size(sched_kw->data);
  sched_kw_type ** sched_kw_dates = util_malloc(*num_steps * sizeof * sched_kw_dates, __func__);
  
  for(int i=0; i<*num_steps; i++) {
    sched_kw_dates[i] = sched_kw_alloc_empty( "DATES" );
    time_t date = sched_kw_dates_iget_time_t((const sched_kw_dates_type *) sched_kw->data, i);
    sched_kw_dates[i]->data = sched_kw_dates_alloc_from_time_t(date);
  }
  return sched_kw_dates;
}
/*****************************************************************/





const char * sched_kw_get_type_name( const sched_kw_type * sched_kw ) {
  return get_name_from_type( sched_kw->type );
}

sched_type_enum sched_kw_get_type(const sched_kw_type * sched_kw)
{
  return sched_kw->type;  
}


static void sched_kw_alloc_data( sched_kw_type * kw , const stringlist_type * token_list , int * token_index , hash_type * fixed_length_table) {
  if (kw->type == UNTYPED) {
    int rec_len = -1;
    if (hash_has_key( fixed_length_table , kw->kw_name ))
      rec_len = hash_get_int( fixed_length_table , kw->kw_name );
    kw->data = sched_kw_untyped_alloc( token_list , token_index , rec_len );
  } else
    kw->data = kw->alloc( token_list , token_index );
}


sched_kw_type * sched_kw_token_alloc(const stringlist_type * token_list, int * token_index, hash_type * fixed_length_table) {
  if (*token_index >= stringlist_get_size( token_list ))
    return NULL;
  else {
    const char * kw_name  = stringlist_iget( token_list , *token_index );
    (*token_index) += 1;
    sched_kw_name_assert(kw_name , NULL);
    if (strcmp(kw_name,"END") == 0)
      return NULL;
    else {
      sched_kw_type * sched_kw = sched_kw_alloc_empty( kw_name );
      sched_kw->restart_nr     = -1;
      
      sched_util_skip_newline( token_list , token_index );
      sched_kw_alloc_data( sched_kw , token_list , token_index , fixed_length_table);
      
      return sched_kw;
    }
  }
}


const char * sched_kw_get_name( const sched_kw_type * kw) { return kw->kw_name; }



void sched_kw_set_restart_nr( sched_kw_type * kw , int restart_nr) {
  kw->restart_nr = restart_nr;
}



void sched_kw_free(sched_kw_type * sched_kw)
{
  sched_kw->free(sched_kw->data);
  free(sched_kw->kw_name);
  free(sched_kw);
}



void sched_kw_free__(void * sched_kw_void)
{
  sched_kw_type * sched_kw = (sched_kw_type *) sched_kw_void;
  sched_kw_free(sched_kw);
}



/*
  This will print the kw in ECLIPSE style formating.
*/
void sched_kw_fprintf(const sched_kw_type * sched_kw, FILE * stream)
{
  sched_kw->fprintf(sched_kw->data, stream);
}







/*
  This function takes a kw related to timing, such as DATES or TSTEP
  and converts it into a series of kw's with one timing event in each kw.

  Note that TIME (ECL300 only) is not supported yet.
*/
sched_kw_type ** sched_kw_restart_file_split_alloc(const sched_kw_type * sched_kw,  int * num_steps)
{
  switch(sched_kw_get_type(sched_kw))
  {
    case(TSTEP):
      return sched_kw_tstep_split_alloc(sched_kw, num_steps);

    case(DATES):
      return sched_kw_dates_split_alloc(sched_kw, num_steps);

     case(TIME):
       util_abort("%s: Sorry - no support for TIME kw yet. Please use TSTEP.\n", __func__);
       return NULL;
     default:
       util_abort("%s: Internal error - aborting.\n", __func__);
       return NULL;
  }
}



time_t sched_kw_get_new_time(const sched_kw_type * sched_kw, time_t curr_time)
{
  time_t new_time = -1;
  switch(sched_kw_get_type(sched_kw))
  {
    case(TSTEP):
      new_time = sched_kw_tstep_get_new_time((const sched_kw_tstep_type *) sched_kw->data, curr_time);
      break;
    case(DATES):
      new_time = sched_kw_dates_get_time_t((const sched_kw_dates_type *) sched_kw->data);
      break;
    case(TIME):
      util_abort("%s: Sorry - no support for TIME kw. Please use TSTEP.\n", __func__);
      break;
    default:
      util_abort("%s: Internal error - trying to get time from non-timing kw - aborting.\n", __func__);
      break;
  }

  return new_time;
}



char ** sched_kw_alloc_well_list(const sched_kw_type * sched_kw, int * num_wells)
{
  switch(sched_kw_get_type(sched_kw))
  {
  case(WCONPROD):
    return sched_kw_wconprod_alloc_wells_copy( (const sched_kw_wconprod_type *) sched_kw->data , num_wells);
    break;
  case(WCONINJE):
    return sched_kw_wconinje_alloc_wells_copy( (const sched_kw_wconinje_type *) sched_kw->data , num_wells);
    break;
  case(WCONINJ):
    return sched_kw_wconinj_alloc_wells_copy( (const sched_kw_wconinj_type *) sched_kw->data , num_wells);
    break;
  case(WCONHIST):
    {
      hash_type * well_obs = sched_kw_wconhist_alloc_well_obs_hash( (sched_kw_wconhist_type *) sched_kw->data);
      *num_wells = hash_get_size(well_obs);
      char ** well_list = hash_alloc_keylist(well_obs);
      hash_free(well_obs);
      return well_list;
    }
    break;
  case(WCONINJH):
    {
      hash_type * well_obs = sched_kw_wconinjh_alloc_well_obs_hash( (sched_kw_wconinjh_type *) sched_kw->data);
      *num_wells = hash_get_size(well_obs);
      char ** well_list = hash_alloc_keylist(well_obs);
      hash_free(well_obs);
      return well_list;
    }
    break;
  default:
       util_abort("%s: Internal error - trying to get well list from non-well kw - aborting.\n", __func__);
       return NULL;
  }
}



hash_type * sched_kw_alloc_well_obs_hash(const sched_kw_type * sched_kw)
{
  switch(sched_kw_get_type(sched_kw))
  {
    case(WCONHIST):
    {
      return sched_kw_wconhist_alloc_well_obs_hash( (sched_kw_wconhist_type *) sched_kw->data);
    }
    case(WCONINJH):
    {
      return sched_kw_wconinjh_alloc_well_obs_hash( (sched_kw_wconinjh_type *) sched_kw->data);
    }
    default:
    {
       util_abort("%s: Internal error - trying to get well observations from non-history kw - aborting.\n", __func__);
       return NULL;
    }
  }
}



void sched_kw_alloc_child_parent_list(const sched_kw_type * sched_kw, char *** children, char *** parents, int * num_pairs)
{
  switch(sched_kw_get_type(sched_kw))
  {
    case(GRUPTREE):
    {
      sched_kw_gruptree_alloc_child_parent_list((sched_kw_gruptree_type *) sched_kw->data, children, parents, num_pairs);
      break;
    }
    case(WELSPECS):
    {
      sched_kw_welspecs_alloc_child_parent_list((sched_kw_welspecs_type *) sched_kw->data, children, parents, num_pairs);
      break;
    }
    default:
    {
       util_abort("%s: Internal error - trying to get GRUPTREE from non-gruptre kw - aborting.\n", __func__);
    }
  }
}


/** 
    Only WCONHIST 
*/

bool sched_kw_has_well( const sched_kw_type * sched_kw , const char * well ) {
  if (sched_kw_get_type( sched_kw ) == WCONHIST)
    return sched_kw_wconhist_has_well( sched_kw->data , well);
  else
    return false;
}

/** 
    Only WCONHIST 
*/

bool sched_kw_well_open( const sched_kw_type * sched_kw , const char * well ) {
  if (sched_kw_get_type( sched_kw ) == WCONHIST)
    return sched_kw_wconhist_well_open( sched_kw->data , well);
  else
    return false;
}



sched_kw_type * sched_kw_alloc_copy(const sched_kw_type * src) {
  sched_kw_type * target = NULL;
  
  return target;
}


/*
  Returns an untyped poiniter to the spesific implementation. Used by
  the sched_file_update system. A bit careful with this one...
*/

void * sched_kw_get_data( sched_kw_type * kw) {
  return kw->data;
}


