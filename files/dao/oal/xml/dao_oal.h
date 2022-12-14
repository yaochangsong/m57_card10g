#ifndef _DAO_OAL_H
#define _DAO_OAL_H

#define ARRAY        0
#define ARRAY_PARENT 1
#define ARRAY_ARRAY  2

#define  ONE_LEVEL   0
#define  TWO_LEVEL   1
#define  THREE_LEVEL   2


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

typedef struct {

    char  series;
    int   val;
    char  *file;
    const char *string;
    const char *parent_element;
    const char *element;
    const char *s_element;
    const char *seed_element;
} spectrum_single;

typedef enum dao_data_type_e		/**** The XML node type. ****/
{
  DDATA_I8,
  DDATA_U8,
  DDATA_I16,
  DDATA_U16,
  DDATA_U32,              /* unsigned Integer value */
  DDATA_I32,              /* Integer value */
  DDATA_FLOAT,            /* Float*/
  DDATA_U64,              /* longlong int */
  DDATA_TEXT,               /* text*/
} dao_data_type_t;


void *write_config_file_single(char *file,const char *parent_element,const char *element,const char *string,int val);
void *write_config_file_array(char *file,const char *array,
const char *name,const char *value,const char *element,const char *seed_element,
const char *seed_array,const char *seed_name,const char *seed_value,
int val,char series);




void dao_read_create_config_file(char *file,void *root_config);
void *write_struc_config_file_single(spectrum_single           spectrum_fre);
void *write_struc_config_file_array(spectrum           spectrum_fre);
const char *read_config_file_single(const char *parent_element,const char *element);
const char *read_config_file_array(mxml_node_t *root, const char *array,const char *name,const char *value,const char *element);


#endif

