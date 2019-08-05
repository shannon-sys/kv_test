#include "iocheck.h"

#ifdef SHANNONDB

#include <iostream>
#include <sstream>
#include <string>
#include <swift/shannon_db.h>

using namespace std;
using shannon::DB;
using shannon::Status;
using shannon::Options;
using shannon::Slice;
using shannon::WriteOptions;
using shannon::WriteBatch;
using shannon::WriteBatchNonatomic;
using shannon::ReadOptions;
using shannon::ReadBatch;
using shannon::Snapshot;
using shannon::Iterator;

char *g_kv_get_err = (char *)"shannondb_kv_get() failed";
char *g_kv_put_err = (char *)"shannondb_kv_put() failed";
char *g_kv_delete_err = (char *)"shannondb_kv_delete() failed";
char *g_kv_writebatch_add_err = (char *)"shannondb_kv_writebatch_add() failed";
char *g_kv_writebatch_submit_err = (char *)"shannondb_kv_writebatch_submit() failed";
char *g_kv_readbatch_add_err = (char *)"shannondb_kv_readbatch_add() failed";
char *g_kv_readbatch_submit_err = (char *)"shannondb_kv_readbatch_submit() failed";

struct shannon_db_handle {
	DB *db;
	// Options *db_opt;
};

class ReadBatchEntry {
public:
	ReadBatchEntry(char *val_buf, int buf_size, unsigned int *val_len, unsigned int *status_ptr):
		val_buf(val_buf), buf_size(buf_size), val_len(val_len), status_ptr(status_ptr) {}

	char *val_buf;
	int buf_size;
	unsigned int *val_len;
	unsigned int *status_ptr;
};

class shannon_readbatch {
public:
	shannon_readbatch() {
		batch = new ReadBatch();
	}

	ReadBatch *batch;
	vector<ReadBatchEntry> values;

	~shannon_readbatch() {
		delete batch;
	}
};

db_t *shannondb_kv_open(const char *dev_name, const char *db_name, char **err)
{
	struct shannon_db_handle *db_handle = new struct shannon_db_handle();
	shannon::DB* db;
	shannon::Options options;
	std::string dbpath = dev_name;
	string dbname = db_name;
	options.create_if_missing = true;
	shannon::Status status = shannon::DB::Open (options, dbname, dbpath, &db);
	assert(status.ok());

	db_handle->db = db;
	return db_handle;
}

void shannondb_kv_close(db_t *db_handle)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;

	delete db->db;
	delete db;
}

kv_readoptions_t *shannondb_kv_readoptions_create()
{
	return new ReadOptions();
}

kv_writeoptions_t *shannondb_kv_writeoptions_create()
{
	return new WriteOptions();
}

void shannondb_kv_readoptions_destroy(kv_readoptions_t *opt)
{
	ReadOptions *options = (ReadOptions *) opt;
	delete options;
}

void shannondb_kv_writeoptions_destroy(kv_writeoptions_t *opt)
{
	WriteOptions *options = (WriteOptions *) opt;
	delete options;
}

void shannondb_kv_readoptions_set_fill_cache(kv_readoptions_t *opt, unsigned char v)
{
	ReadOptions *options = (ReadOptions *) opt;
	options->fill_cache = v;
}

void shannondb_kv_writeoptions_set_sync(kv_writeoptions_t *opt, unsigned char v)
{
	WriteOptions *options = (WriteOptions *) opt;
	options->sync = v;
}

int shannondb_kv_get(db_t *db_handle, const kv_readoptions_t *opt, const char *key, unsigned int key_len,
			char *val_buf, unsigned int buf_size, unsigned int *val_len,
			char **err)
{
	struct shannon_db_handle *db_h = (struct shannon_db_handle *)db_handle;
	DB *db = db_h->db;
	ReadOptions *options = (ReadOptions *)opt;
	Slice slice_key(key, key_len);
	string value;
	Status status = db->Get(*options, slice_key, &value);

	*err = NULL;

	if (status.ok()) {
		*val_len = value.size();
		memcpy(val_buf, value.c_str(), min(buf_size, *val_len));
		return 0;
	} else {
		cerr << status.ToString() << endl;
		*err = g_kv_get_err;
		return -1;
	}
}

void shannondb_kv_put(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len,
			const char *val, unsigned int val_len, char **err)
{
	struct shannon_db_handle *db_h = (struct shannon_db_handle *)db_handle;
	DB *db = db_h->db;
	WriteOptions *options = (WriteOptions *)opt;
	Slice slice_key(key, key_len);
	Slice slice_val(val, val_len);
	Status status = db->Put(*options, slice_key, slice_val);

	*err = NULL;

	if (!status.ok()) {
		cerr << status.ToString() << endl;
		*err = g_kv_put_err;
	}
}

int shannondb_kv_delete(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len, char **err)
{
	struct shannon_db_handle *db_h = (struct shannon_db_handle *)db_handle;
	DB *db = db_h->db;
	WriteOptions *options = (WriteOptions *)opt;
	Slice slice_key(key, key_len);
	Status status = db->Delete(*options, slice_key);

	*err = NULL;

	if (status.ok()) {
		return 0;
	} else {
		cerr << status.ToString() << endl;
		*err = g_kv_delete_err;
		return -1;
	}
}

kv_readbatch_t *shannondb_kv_readbatch_create()
{
	return new shannon_readbatch;
}

void shannondb_kv_readbatch_destroy(kv_readbatch_t *batch)
{
	delete (shannon_readbatch *)batch;
}

int shannondb_kv_readbatch_add(kv_readbatch_t *batch, const char *key, unsigned int key_len,
			char *val_buf, unsigned int buf_size, unsigned int *value_len,
			unsigned int *status, char **err)
{
	struct shannon_readbatch *bat = (struct shannon_readbatch *)batch;
	Slice slice_key(key, key_len);
	Status ret = bat->batch->Get(slice_key);

	*err = NULL;

	if (ret.ok()) {
		bat->values.push_back(ReadBatchEntry(val_buf, buf_size, value_len, status));
		return 0;
	} else {
		cerr << ret.ToString() << endl;
		*err = g_kv_writebatch_add_err;
		return -1;
	}
}

void shannondb_kv_readbatch_clear(kv_readbatch_t *batch)
{
	struct shannon_readbatch *bat = (struct shannon_readbatch *)batch;
	bat->batch->Clear();
	bat->values.clear();
}

void shannondb_kv_readbatch_submit(db_t *db_handle, const kv_readoptions_t *opt, kv_readbatch_t *batch,
							unsigned int *failed_cmd_count, char **err)
{
	struct shannon_db_handle *db = (struct shannon_db_handle *)db_handle;
	shannon_readbatch *readbatch = (shannon_readbatch *)batch;
	vector<string> values;
	Status status = db->db->Read(*(ReadOptions *)opt, readbatch->batch, &values);

	*err = NULL;

	if (!status.ok()) {
		cerr << status.ToString() << endl;
		*err = g_kv_readbatch_submit_err;
		return;
	}

	if (values.size() != readbatch->values.size()) {
		cerr << "batch size mismatch, expect = " << readbatch->values.size()
			 << ", fact = " << values.size() << endl;
		*err = g_kv_readbatch_submit_err;
		return;
	}

	for (size_t i = 0; i < values.size(); i++) {
		string s = values[i];
		ReadBatchEntry entry = readbatch->values[i];
		*entry.val_len = s.size();
		if (s.size() == 0) {
			*entry.status_ptr = 1;  // NOT_FOUND
		} else {
			*entry.status_ptr = 0;  // SUCC
			memcpy(entry.val_buf, s.c_str(), MIN(entry.buf_size, s.size()));
		}
	}
}

kv_writebatch_t *shannondb_kv_writebatch_create()
{
	return new WriteBatch();
}

int shannondb_kv_writebatch_add(kv_writebatch_t *batch, const char *key, unsigned int key_len,
			const char *val, unsigned int val_len, char **err)
{
	WriteBatch *bat = (WriteBatch *)batch;
	Slice slice_key(key, key_len);
	Slice slice_val(val, val_len);
	Status status = bat->Put(slice_key, slice_val);

	*err = NULL;

	if (status.ok()) {
		return 0;
	} else {
		cerr << status.ToString() << endl;
		*err = g_kv_writebatch_add_err;
		return -1;
	}
}

void shannondb_kv_writebatch_clear(kv_writebatch_t *batch)
{
	WriteBatch *bat = (WriteBatch *)batch;
	bat->Clear();
}

void shannondb_kv_writebatch_destroy(kv_writebatch_t *batch)
{
	delete (WriteBatch *)batch;
}

void shannondb_kv_writebatch_submit(db_t *db_handle, const kv_writeoptions_t *opt, kv_writebatch_t *batch, char **err)
{
	DB *db = ((struct shannon_db_handle *)db_handle)->db;
	WriteOptions *wopt = (WriteOptions *)opt;
	WriteBatch *bat = (WriteBatch *)batch;
	Status status  = db->Write(*wopt, bat);

	*err = NULL;

	if (!status.ok()) {
		cerr << status.ToString() << endl;
		*err = g_kv_writebatch_submit_err;
	}
}

struct db_ops shannon_db_ops = {
	.kv_open	= shannondb_kv_open,
	.kv_close	= shannondb_kv_close,

	.kv_readoptions_create 		= shannondb_kv_readoptions_create,
	.kv_writeoptions_create		= shannondb_kv_writeoptions_create,
	.kv_readoptions_destroy		= shannondb_kv_readoptions_destroy,
	.kv_writeoptions_destroy	= shannondb_kv_writeoptions_destroy,

	.kv_readoptions_set_fill_cache = shannondb_kv_readoptions_set_fill_cache,
	.kv_writeoptions_set_sync	= shannondb_kv_writeoptions_set_sync,

	.kv_readbatch_create		= shannondb_kv_readbatch_create,
	.kv_readbatch_add			= shannondb_kv_readbatch_add,
	.kv_readbatch_submit		= shannondb_kv_readbatch_submit,
	.kv_readbatch_clear			= shannondb_kv_readbatch_clear,
	.kv_readbatch_destroy		= shannondb_kv_readbatch_destroy,

	.kv_writebatch_create		= shannondb_kv_writebatch_create,
	.kv_writebatch_add			= shannondb_kv_writebatch_add,
	.kv_writebatch_submit		= shannondb_kv_writebatch_submit,
	.kv_writebatch_clear		= shannondb_kv_writebatch_clear,
	.kv_writebatch_destroy		= shannondb_kv_writebatch_destroy,

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

struct db_ops *get_leveldb_ops()
{
	return NULL;
}

struct db_ops *get_rocksdb_ops()
{
	return NULL;
}
