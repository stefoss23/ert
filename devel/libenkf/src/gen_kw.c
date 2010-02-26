#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <enkf_types.h>
#include <util.h>

#include <enkf_util.h>
#include <math.h>
#include <scalar.h>
#include <enkf_macros.h>
#include <subst_list.h>
#include <buffer.h>
#include <matrix.h>
#include <gen_kw_common.h>
#include <gen_kw_config.h>
#include <gen_kw.h>
#include <log.h>

GET_DATA_SIZE_HEADER(gen_kw);


struct gen_kw_struct {
  int                        __type_id;
  const gen_kw_config_type * config;
  scalar_type              * scalar;
  subst_list_type          * subst_list;
};

/*****************************************************************/



void gen_kw_free(gen_kw_type *gen_kw) {
  scalar_free(gen_kw->scalar);
  subst_list_free(gen_kw->subst_list);
  free(gen_kw);
}




void gen_kw_output_transform(const gen_kw_type * gen_kw) {
  scalar_transform(gen_kw->scalar);
}

void gen_kw_set_data(gen_kw_type * gen_kw , const double * data) {
  scalar_set_data(gen_kw->scalar , data);
}



gen_kw_type * gen_kw_alloc(const gen_kw_config_type * config) {
  gen_kw_type * gen_kw  = util_malloc(sizeof *gen_kw , __func__);
  gen_kw->config = config;
  gen_kw->scalar = scalar_alloc(gen_kw_config_get_scalar_config( config ));
  gen_kw->__type_id          = GEN_KW;
  gen_kw->subst_list         = subst_list_alloc( NULL );
  return gen_kw;
}


void gen_kw_clear(gen_kw_type * gen_kw) {
  scalar_clear(gen_kw->scalar);
}



void gen_kw_copy(const gen_kw_type * src , gen_kw_type * target) {
  if (src->config == target->config) 
    scalar_memcpy(target->scalar , src->scalar);
  else
    util_abort("%s: two elements do not share config object \n",__func__);
}




bool gen_kw_store(const gen_kw_type *gen_kw , buffer_type * buffer,  int report_step , bool internal_state) {
  buffer_fwrite_int( buffer , GEN_KW );
  scalar_buffer_fsave(gen_kw->scalar , buffer , internal_state);
  return true;
}



/**
   As of 17/03/09 (svn 1811) MULTFLT has been depreceated, and GEN_KW
   has been inserted as a 'drop-in-replacement'. This implies that
   existing storage labeled with implemantation type 'MULTFLT' should
   be silently 'upgraded' to 'GEN_KW'.

*/


#define MULTFLT 102
void gen_kw_load(gen_kw_type * gen_kw , buffer_type * buffer, int report_step) {
  enkf_impl_type file_type;
  file_type = buffer_fread_int(buffer);
  if ((file_type == GEN_KW) || (file_type == MULTFLT))
    scalar_buffer_fload( gen_kw->scalar , buffer);
}
#undef MULTFLT


void gen_kw_upgrade_103( const char * filename ) {
  FILE * stream            = util_fopen( filename , "r");
  enkf_impl_type impl_type = util_fread_int( stream );
  int size                 = util_fread_int( stream );
  double * data            = util_malloc( size * sizeof * data , __func__ ); 
  util_fread( data , sizeof * data , size , stream , __func__);
  fclose( stream );
  
  {
    buffer_type * buffer = buffer_alloc( 100 );
    buffer_fwrite_time_t( buffer , time(NULL));
    buffer_fwrite_int( buffer , impl_type );
    buffer_fwrite(buffer , data , sizeof * data    ,size);
    buffer_store( buffer , filename);
    buffer_free( buffer );
  }
  free( data );
}


void gen_kw_truncate(gen_kw_type * gen_kw) {
  scalar_truncate( gen_kw->scalar );  
}



bool gen_kw_initialize(gen_kw_type *gen_kw, int iens) {
  char * init_file = gen_kw_config_alloc_initfile( gen_kw->config , iens);

  if (init_file != NULL) {
    gen_kw_fload(gen_kw , init_file );
    free( init_file );
  } else 
    scalar_sample(gen_kw->scalar);  

  return true;
}





void gen_kw_matrix_serialize(const gen_kw_type *gen_kw , const active_list_type * active_list , matrix_type * A , int row_offset , int column) {
  scalar_matrix_serialize(gen_kw->scalar , active_list , A , row_offset , column);
}


void gen_kw_matrix_deserialize(gen_kw_type *gen_kw , const active_list_type * active_list , const matrix_type * A , int row_offset , int column) {
  scalar_matrix_deserialize(gen_kw->scalar , active_list , A , row_offset , column);
}





void gen_kw_filter_file(const gen_kw_type * gen_kw , const char * target_file) {
  const char * template_file = gen_kw_config_get_template_ref(gen_kw->config);
  if (template_file != NULL) {
    const int size               = gen_kw_config_get_data_size(gen_kw->config );
    const double * output_data   = scalar_get_output_ref(gen_kw->scalar);

    int ikw;
    
    gen_kw_output_transform(gen_kw);
    for (ikw = 0; ikw < size; ikw++) {
      const char * key = gen_kw_config_get_tagged_name(gen_kw->config , ikw);      
      subst_list_insert_owned_ref(gen_kw->subst_list , key , util_alloc_sprintf("%g" , output_data[ikw]) , NULL);
    }
    
    subst_list_filter_file( gen_kw->subst_list  , template_file  , target_file);
  } else 
    util_abort("%s: internal error - tried to filter gen_kw instance without template file.\n",__func__);
}


void gen_kw_ecl_write(const gen_kw_type * gen_kw , const char * run_path , const char * base_file , fortio_type * fortio) {
  char * target_file = util_alloc_filename( run_path , base_file  , NULL);
  gen_kw_filter_file(gen_kw , target_file);
  free( target_file );
}



void gen_kw_export(const gen_kw_type * gen_kw , int * _size , char ***_kw_list , double **_output_values) {
  gen_kw_output_transform(gen_kw);

  *_kw_list       = (char **) gen_kw_config_get_name_list(gen_kw->config);
  *_size          = gen_kw_config_get_data_size(gen_kw->config );
  *_output_values = (double *) scalar_get_output_ref(gen_kw->scalar);

}




const char * gen_kw_get_name(const gen_kw_type * gen_kw, int kw_nr) {
  return  gen_kw_config_iget_name(gen_kw->config , kw_nr);
}


/**
   This function will load values for gen_kw instance from file. The
   file should be formatted as either:
   
   -------
   Value1
   Value2
   Value3
   ....
   ValueN
   -------
   
   Or

   ------------
   Key3  Value3  
   Key5  Value5
   Key1  Value1
   .....
   ------------

   I.e. you can either just dump in all the numbers in one long
   vector, or you can interlace numbers and keys. In the latter case
   the ordering is arbitrary.

   Observe the following:

    1. All values must be specified.
    2. The values are in the N(0,1) domain, i.e. the untransformed variables.
    
*/

void gen_kw_fload(gen_kw_type * gen_kw , const char * filename) {
  const int size = gen_kw_config_get_data_size(gen_kw->config );
  FILE * stream  = util_fopen( filename , "r");
  bool   readOK = true;

  /* First try reading all the data as one long vector. */
  {
    int index = 0;
    while ((index < size) && readOK) {
      double value;
      if (fscanf(stream,"%lg" , &value) == 1) 
        scalar_iset( gen_kw->scalar , index , value);
      else
        readOK = false;
      index++;
    }
  }

  /* 
     OK - rewind and try again with interlaced key + value
     pairs. Observe that we still require that ALL the elements in the
     gen_kw instance are set, i.e. it is not allowed to read only some
     of the keywords; but the ordering is not relevant.
     
     The code will be fooled (and give undefined erronous results) if
     the same key appears several times. Be polite!
  */
  
  if (!readOK) {
    int counter = 0;
    readOK = true;
    fseek( stream , 0 , SEEK_SET );
    
    while ((counter < size) && readOK) {
      char key[128];
      double value;
      int    fscanf_return = fscanf(stream , "%s %lg" , key , &value);

      if (fscanf_return == 2) {
        int index = gen_kw_config_get_index(gen_kw->config , key);
        if (index >= 0) 
          scalar_iset( gen_kw->scalar , index , value);
        else
          util_abort("%s: key:%s not recognized as part of GEN_KW instance - error when reading file:%s \n",__func__ , key , filename);
        counter++;
      } else {
        util_abort("%s: failed to read (key,value) pair at line:%d in file:%s \n",__func__ , util_get_current_linenr( stream ) , filename);
        readOK = false;
      }
    }
  }
  
  if (!readOK)
    util_abort("%s: failed loading from file:%s \n",__func__ , filename);

  fclose(stream);
}



/**
   Will return 0.0 on invalid input, and set valid -> false. It is the
   responsibility of the calling scope to check valid.
*/
double gen_kw_user_get(const gen_kw_type * gen_kw, const char * key , bool * valid) {
  const bool internal_value = false;
  int index = gen_kw_config_get_index(gen_kw->config , key);
  if (index >= 0) {
    *valid = true;
    return scalar_iget_double(gen_kw->scalar , internal_value , index);
  } else {
    *valid = false;
    fprintf(stderr,"** Warning:could not lookup key:%s in gen_kw instance \n",key);
    return 0.0;
  }
}


void gen_kw_set_subst_parent(gen_kw_type * gen_kw , const subst_list_type * subst_parent) {
  subst_list_set_parent( gen_kw->subst_list , subst_parent );
}


void gen_kw_set_inflation(gen_kw_type * inflation , const gen_kw_type * std , const gen_kw_type * min_std) {
  const int log_level           = 3;
  const int data_size           = gen_kw_config_get_data_size(std->config );
  const double * std_data       = scalar_get_data_ref( std->scalar );
  const double * min_std_data   = scalar_get_data_ref( min_std->scalar );
  double       * inflation_data = scalar_get_data_ref( inflation->scalar );

  {
    for (int i=0; i < data_size; i++) {
      if (std_data[i] > 0)
        inflation_data[i] = util_double_max( 1.0 , min_std_data[i] / std_data[i]);   
      else 
        inflation_data[i] = 1;
    }
  }
}


void gen_kw_iadd( gen_kw_type * gen_kw , const gen_kw_type * delta) {
  scalar_iadd( gen_kw->scalar , delta->scalar );
}

void gen_kw_iaddsqr( gen_kw_type * gen_kw , const gen_kw_type * delta) {
  scalar_iaddsqr( gen_kw->scalar , delta->scalar );
}

void gen_kw_imul( gen_kw_type * gen_kw , const gen_kw_type * delta) {
  scalar_imul( gen_kw->scalar , delta->scalar );
}

void gen_kw_scale( gen_kw_type * gen_kw , double scale_factor) {
  scalar_scale( gen_kw->scalar , scale_factor );
}

void gen_kw_isqrt( gen_kw_type * gen_kw ) {
  scalar_isqrt( gen_kw->scalar );
}


/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/
SAFE_CAST(gen_kw , GEN_KW);
SAFE_CONST_CAST(gen_kw , GEN_KW);
VOID_ALLOC(gen_kw);
VOID_INITIALIZE(gen_kw);
VOID_COPY(gen_kw)
VOID_FREE   (gen_kw)
VOID_ECL_WRITE(gen_kw)
VOID_USER_GET(gen_kw)
VOID_STORE(gen_kw)
VOID_LOAD(gen_kw)
VOID_MATRIX_SERIALIZE(gen_kw)
VOID_MATRIX_DESERIALIZE(gen_kw)
VOID_SET_INFLATION(gen_kw)
VOID_CLEAR(gen_kw)
VOID_IADD(gen_kw)
VOID_SCALE(gen_kw)
VOID_IMUL(gen_kw)
VOID_IADDSQR(gen_kw)
VOID_ISQRT(gen_kw)
     
