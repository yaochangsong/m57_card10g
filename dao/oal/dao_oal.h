#ifndef _DAO_OAL_H
#define _DAO_OAL_H

void dao_read_create_config_file(char *file, void *root_config);
void *dao_load_default_config_file_singular(char *file,char *parent_element,char *element,char *string,int val);


#endif

