#ifndef _DAO_OAL_H
#define _DAO_OAL_H

void dao_read_create_config_file(char *file, void *root_config);
void *dao_load_default_config_file_singular(char *file,const char *parent_element,const char *element,const char *string,int val);
const char *dao_load_config_file_singular(char *file,const char *parent_element,const char *element);
const char *dao_load_config_file_array(char *file,const char *array,const char *name,const char *value,const char *element);

#endif

