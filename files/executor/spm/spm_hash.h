#ifndef _SPM_HASH_H
#define _SPM_HASH_H


#define MAX_SPM_RANDOM_ENTRIES  1
#define MAX_SPM_HASH_DATA  16384


void spm_test_main(void);


extern int spm_hash_create(void **hash);
extern int spm_hash_delete(void *hash);
extern int spm_hash_delete_item(void *hash, int key);
extern int spm_hash_insert(void *hash, int key, struct iovec *data);
extern int spm_hash_dump(void *hash);
#endif

