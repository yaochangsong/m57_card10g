#ifndef _SPM_HASH_H
#define _SPM_HASH_H

/* 最大hash节点数 */
#define MAX_SPM_RANDOM_ENTRIES  8192
/* 一个hash节点最大数据数组长度 */
#define MAX_SPM_HASH_DATA  16384


void spm_test_main(void);


extern int spm_hash_create(void **hash);
extern int spm_hash_delete(void *hash);
extern int spm_hash_delete_item(void *hash, int key);
extern int spm_hash_insert(void *hash, int key, struct iovec *data);
extern int spm_hash_renew(void *hash, int key, struct iovec *data);
extern int spm_hash_dump(void *hash);
extern int  spm_clear_all_hash(void *hash);
extern void spm_hash_clear_vec_data(void *hash);
extern ssize_t spm_hash_do_for_each_vec(void *hash, int key, ssize_t (*callback) (void *, void *), void *args);

#endif

