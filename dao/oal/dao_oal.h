#ifndef _DAO_OAL_H
#define _DAO_OAL_H


typedef struct {

char  series;
int   val;
char  *file;
const char *array;
const char *name;
const char *value;
const char *element;
const char *seed_element;
const char *seed_array;
const char *seed_name;
const char *seed_value;
} spectrum;




void dao_read_create_config_file(char *file,void *root_config);

void *write_config_file_single(char *file,const char *parent_element,const char *element,
const char *s_element,const char *seed_element,
const char *string,int val,char series);


void *write_config_file_array(spectrum         spectrum_fre);



const char *read_config_file_single(char *file,const char *parent_element,const char *element);
const char *read_config_file_array(char *file,const char *array,const char *name,const char *value,const char *element);


#endif

