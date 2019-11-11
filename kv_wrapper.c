#include "iocheck.h"

#ifdef SHANNONDB

#include <shannon_db.h>

struct shannon_db_handle {
	struct shannon_db *db;
	struct db_options *db_opt;
};

db_t *shannondb_kv_open(const char *dev_name, const char *db_name, char **err)
{
	struct shannon_db_handle *db = malloc(sizeof(*db));

	assert(db != NULL);
	db->db_opt = shannon_db_options_create();
	assert(db->db_opt != NULL);
	shannon_db_options_set_create_if_missing(db->db_opt, true);
	*err = NULL;
	db->db = shannon_db_open(db->db_opt, dev_name, db_name, err);
	return db;
}

void shannondb_kv_close(db_t *db_handle)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;

	if (db->db_opt) {
		shannon_db_options_destroy(db->db_opt);
	}
	if (db->db) {
		shannon_db_close(db->db);
	}
	free(db);
}

int shannondb_kv_get(db_t *db_handle, const kv_readoptions_t *opt, const char *key, unsigned int key_len,
			char *val_buf, unsigned int buf_size, unsigned int *val_len,
			char **err)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;
	return shannon_db_get(db->db, opt, key, key_len, val_buf, buf_size, val_len, err);
}

void shannondb_kv_put(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len,
			const char *val, unsigned int val_len, char **err)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;
	return shannon_db_put(db->db, opt, key, key_len, val, val_len, err);
}

int shannondb_kv_delete(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len, char **err)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;
	return shannon_db_delete(db->db, opt, key, key_len, err);
}

void shannondb_kv_readbatch_submit(db_t *db_handle, const kv_readoptions_t *opt, kv_readbatch_t *batch,
							unsigned int *failed_cmd_count, char **err)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;
	return shannon_db_read(db->db, opt, batch, failed_cmd_count, err);
}

void shannondb_kv_writebatch_submit(db_t *db_handle, const kv_writeoptions_t *opt, kv_writebatch_t *batch, char **err)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;
	return shannon_db_write(db->db, opt, batch, err);
}

struct db_ops shannon_db_ops = {
	.kv_open	= shannondb_kv_open,
	.kv_close	= shannondb_kv_close,

	.kv_readoptions_create 		= (void *)shannon_db_readoptions_create,
	.kv_writeoptions_create		= (void *)shannon_db_writeoptions_create,
	.kv_readoptions_destroy		= (void *)shannon_db_readoptions_destroy,
	.kv_writeoptions_destroy	= (void *)shannon_db_writeoptions_destroy,

	.kv_readoptions_set_fill_cache = (void *)shannon_db_readoptions_set_fill_cache,
	.kv_writeoptions_set_sync	= (void *)shannon_db_writeoptions_set_sync,

	.kv_readbatch_create		= (void *)shannon_db_readbatch_create,
	.kv_readbatch_add			= (void *)shannon_db_readbatch_get,
	.kv_readbatch_submit		= shannondb_kv_readbatch_submit,
	.kv_readbatch_clear			= (void *)shannon_db_readbatch_clear,
	.kv_readbatch_destroy		= (void *)shannon_db_readbatch_destroy,

	.kv_writebatch_create		= (void *)shannon_db_writebatch_create,
	.kv_writebatch_add			= (void *)shannon_db_writebatch_put,
	.kv_writebatch_submit		= shannondb_kv_writebatch_submit,
	.kv_writebatch_clear		= (void *)shannon_db_writebatch_clear,
	.kv_writebatch_destroy		= (void *)shannon_db_writebatch_destroy,

	.kv_get		= shannondb_kv_get,
	.kv_put		= shannondb_kv_put,
	.kv_delete	= shannondb_kv_delete,
};

struct db_ops *get_shannon_db_ops()
{
	return &shannon_db_ops;
}

#else

struct db_ops *get_shannon_db_ops()
{
	return NULL;
}

#endif

#ifdef LEVELDB
#include <leveldb/c.h>

struct leveldb_handle {
	leveldb_t *db;
	leveldb_options_t *db_opt;
};

db_t *leveldb_kv_open(const char *dev_name, const char *db_name, char **err)
{
	struct leveldb_handle *db = malloc(sizeof(*db));

	assert(db != NULL);
	db->db_opt = leveldb_options_create();
	assert(db->db_opt != NULL);
	leveldb_options_set_create_if_missing(db->db_opt, true);
	leveldb_options_set_compression(db->db_opt, false);
	*err = NULL;
	db->db = leveldb_open(db->db_opt, dev_name, err);
	return db;
}

void leveldb_kv_close(db_t *db_handle)
{
	struct leveldb_handle *db = (struct leveldb_handle *)db_handle;
	if (db->db_opt) {
		leveldb_options_destroy(db->db_opt);
	}
	if (db->db) {
		leveldb_close(db->db);
	}
	free(db);
}

int leveldb_kv_get(db_t *db_handle, const kv_readoptions_t *opt, const char *key, unsigned int key_len,
			char *val_buf, unsigned int buf_size, unsigned int *val_len,
			char **err)
{
	struct leveldb_handle *db = (struct leveldb_handle *)db_handle;
	char *value = leveldb_get(db->db, opt, key, key_len, (size_t *)val_len, err);
	if (*err == NULL) {
		int copy_size = MIN(buf_size, *val_len);
		memcpy(val_buf, value, copy_size);
		free(value);
		return 0;
	}
	return -1;
}

void leveldb_kv_put(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len,
			const char *val, unsigned int val_len, char **err)
{
	struct leveldb_handle *db = (struct leveldb_handle *)db_handle;
	return leveldb_put(db->db, opt, key, key_len, val, val_len, err);
}

int leveldb_kv_delete(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len, char **err)
{
	struct leveldb_handle *db = (struct leveldb_handle *)db_handle;
	leveldb_delete(db->db, opt, key, key_len, err);
	return *err != NULL;
}

int leveldb_kv_writebatch_add(kv_writebatch_t *batch, const char *key, unsigned int key_len,
		const char *val, unsigned int val_len, char **err)
{
	leveldb_writebatch_put(batch, key, key_len, val, val_len);
	*err = NULL;
	return 0;
}

void leveldb_kv_writebatch_submit(db_t *db_handle, const kv_writeoptions_t *opt, kv_writebatch_t *batch, char **err)
{
	struct leveldb_handle *db = (struct leveldb_handle *)db_handle;
	return leveldb_write(db->db, opt, batch, err);
}

struct db_ops leveldb_ops = {
	.kv_open	= leveldb_kv_open,
	.kv_close	= leveldb_kv_close,

	.kv_readoptions_create 		= (void *)leveldb_readoptions_create,
	.kv_writeoptions_create		= (void *)leveldb_writeoptions_create,
	.kv_readoptions_destroy		= (void *)leveldb_readoptions_destroy,
	.kv_writeoptions_destroy	= (void *)leveldb_writeoptions_destroy,

	.kv_readoptions_set_fill_cache = (void *)leveldb_readoptions_set_fill_cache,
	.kv_writeoptions_set_sync	= (void *)leveldb_writeoptions_set_sync,

	.kv_writebatch_create		= (void *)leveldb_writebatch_create,
	.kv_writebatch_add			= leveldb_kv_writebatch_add,
	.kv_writebatch_submit		= leveldb_kv_writebatch_submit,
	.kv_writebatch_clear		= (void *)leveldb_writebatch_clear,
	.kv_writebatch_destroy		= (void *)leveldb_writebatch_destroy,

	.kv_get		= leveldb_kv_get,
	.kv_put		= leveldb_kv_put,
	.kv_delete	= leveldb_kv_delete,
};

struct db_ops *get_leveldb_ops()
{
	return &leveldb_ops;
}

#else

struct db_ops *get_leveldb_ops()
{
	return NULL;
}

#endif

#ifdef ROCKSDB
#include <rocksdb/c.h>

struct rocksdb_handle {
	rocksdb_t *db;
	rocksdb_options_t *db_opt;
};

db_t *rocksdb_kv_open(const char *dev_name, const char *db_name, char **err)
{
	struct rocksdb_handle *db = malloc(sizeof(*db));

	assert(db != NULL);
	db->db_opt = rocksdb_options_create();
	assert(db->db_opt != NULL);
	rocksdb_options_set_create_if_missing(db->db_opt, true);
	rocksdb_options_set_compression(db->db_opt, false);
	*err = NULL;
	db->db = rocksdb_open(db->db_opt, dev_name, err);
	return db;
}

void rocksdb_kv_close(db_t *db_handle)
{
	struct rocksdb_handle *db = (struct rocksdb_handle *)db_handle;
	if (db->db_opt) {
		rocksdb_options_destroy(db->db_opt);
	}
	if (db->db) {
		rocksdb_close(db->db);
	}
	free(db);
}

int rocksdb_kv_get(db_t *db_handle, const kv_readoptions_t *opt, const char *key, unsigned int key_len,
			char *val_buf, unsigned int buf_size, unsigned int *val_len,
			char **err)
{
	struct rocksdb_handle *db = (struct rocksdb_handle *)db_handle;
	char *value = rocksdb_get(db->db, opt, key, key_len, (size_t *)val_len, err);
	if (*err == NULL) {
		int copy_size = MIN(buf_size, *val_len);
		memcpy(val_buf, value, copy_size);
		free(value);
		return 0;
	}
	return -1;
}

void rocksdb_kv_put(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len,
			const char *val, unsigned int val_len, char **err)
{
	struct rocksdb_handle *db = (struct rocksdb_handle *)db_handle;
	return rocksdb_put(db->db, opt, key, key_len, val, val_len, err);
}

int rocksdb_kv_delete(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len, char **err)
{
	struct rocksdb_handle *db = (struct rocksdb_handle *)db_handle;
	rocksdb_delete(db->db, opt, key, key_len, err);
	return *err != NULL;
}

int rocksdb_kv_writebatch_add(kv_writebatch_t *batch, const char *key, unsigned int key_len,
		const char *val, unsigned int val_len, char **err)
{
	rocksdb_writebatch_put(batch, key, key_len, val, val_len);
	*err = NULL;
	return 0;
}

void rocksdb_kv_writebatch_submit(db_t *db_handle, const kv_writeoptions_t *opt, kv_writebatch_t *batch, char **err)
{
	struct rocksdb_handle *db = (struct rocksdb_handle *)db_handle;
	return rocksdb_write(db->db, opt, batch, err);
}

struct db_ops rocksdb_ops = {
	.kv_open	= rocksdb_kv_open,
	.kv_close	= rocksdb_kv_close,

	.kv_readoptions_create 		= (void *)rocksdb_readoptions_create,
	.kv_writeoptions_create		= (void *)rocksdb_writeoptions_create,
	.kv_readoptions_destroy		= (void *)rocksdb_readoptions_destroy,
	.kv_writeoptions_destroy	= (void *)rocksdb_writeoptions_destroy,

	.kv_readoptions_set_fill_cache = (void *)rocksdb_readoptions_set_fill_cache,
	.kv_writeoptions_set_sync	= (void *)rocksdb_writeoptions_set_sync,

	.kv_writebatch_create		= (void *)rocksdb_writebatch_create,
	.kv_writebatch_add			= rocksdb_kv_writebatch_add,
	.kv_writebatch_submit		= rocksdb_kv_writebatch_submit,
	.kv_writebatch_clear		= (void *)rocksdb_writebatch_clear,
	.kv_writebatch_destroy		= (void *)rocksdb_writebatch_destroy,

	.kv_get		= rocksdb_kv_get,
	.kv_put		= rocksdb_kv_put,
	.kv_delete	= rocksdb_kv_delete,
};

struct db_ops *get_rocksdb_ops()
{
	return &rocksdb_ops;
}

#else

struct db_ops *get_rocksdb_ops()
{
	return NULL;
}

#endif
