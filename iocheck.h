#ifndef __IOCHECK_H
#define __IOCHECK_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <getopt.h>
#include <linux/fs.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>
#include <unistd.h>

#define __USE_GNU 1
#include <linux/types.h>

#include "util.h"

#define bool int
#define true 1
#define false 0
#define random(x) (rand() % x)

#define MAX_KEY_LENGTH            128
#define MAX_VALUE_LENGTH            4194304
#define MAX_BATCH_COUNT             20000
#define DATA_PAGE_KEY_OFFSET            13
#define LOGICB_SIZE            4096
#define KEY_LEN 8

#define move_up(n)      do { printf("\033[%dA", n); } while(0)  // move cursor up n lines
#define move_down(n)    do { printf("\033[%dB", n); } while(0)  // move cursor down n lines
#define move_right(n)   do { printf("\033[%dC", n); } while(0)  // move cursor right n column
#define move_left(n)    do { printf("\033[%dD", n); } while(0)  // move cursor left n column
#define moveto_head()   do { printf("\r"); } while(0)           // move cursor to line head
#define clear_line()    do { printf("\033[K"); } while(0)       // clear display from cursor to line tail

#define save_cursor()   do { printf("\033[s"); } while(0)       // save current cursor location
#define load_cursor()   do { printf("\033[u"); } while(0)       // restore cursor location saved by save_cursor()

typedef void db_t;
typedef void kv_readoptions_t;
typedef void kv_writeoptions_t;
typedef void kv_readbatch_t;
typedef void kv_writebatch_t;

#define	IOCHECK_WRITE	0
#define	IOCHECK_READ	1

#define	IOCHECK_RANDOM	0
#define	IOCHECK_SEQ	1

struct io_thread {
	int id;
	pthread_t tid;
	struct io_check *check;
	volatile int terminate;

	volatile __u64 write_count;
	volatile __u64 read_count;
	volatile __u64 write_length;
	volatile __u64 read_length;

	char *val_buf;
	int value_size;
	int key_size;

	int curr_batch_count;

	kv_readoptions_t *read_options;
	kv_readbatch_t *read_batch;
	char **batch_val_buf_base;
	unsigned int *batch_val_len_base;
	unsigned int *batch_status_base;

	kv_writeoptions_t *write_options;
	kv_writebatch_t *write_batch;
};

struct db_ops {
	db_t *(*kv_open)(const char *dev_name, const char *db_name, char **err);
	void (*kv_close)();

	/* read/write options */
	kv_readoptions_t *(*kv_readoptions_create)();
	kv_writeoptions_t *(*kv_writeoptions_create)();
	void (*kv_readoptions_destroy)(kv_readoptions_t *opt);
	void (*kv_writeoptions_destroy)(kv_writeoptions_t *opt);

	void (*kv_readoptions_set_fill_cache)(kv_readoptions_t *opt, unsigned char v);
	void (*kv_writeoptions_set_sync)(kv_writeoptions_t *opt, unsigned char v);

	/* read batch */
	void *(*kv_readbatch_create)();
	int (*kv_readbatch_add)(kv_readbatch_t *batch, const char *key, unsigned int key_len,
				const char *val_buf, unsigned int buf_size, unsigned int *value_len,
				unsigned int *status, char **err);
	void (*kv_readbatch_submit)(db_t *db_handle, const kv_readoptions_t *opt, kv_readbatch_t *batch,
								unsigned int *failed_cmd_count, char **err);
	void (*kv_readbatch_clear)(kv_readbatch_t *batch);
	void (*kv_readbatch_destroy)(kv_readbatch_t *batch);

	/* write batch */
	void *(*kv_writebatch_create)();
	int (*kv_writebatch_add)(kv_writebatch_t *batch, const char *key, unsigned int key_len,
				const char *val, unsigned int val_len, char **err);
	void (*kv_writebatch_submit)(db_t *db_handle, const kv_writeoptions_t *opt, kv_writebatch_t *batch, char **err);
	void (*kv_writebatch_clear)(kv_writebatch_t *batch);
	void (*kv_writebatch_destroy)(kv_writebatch_t *batch);

	/* get/put/delete */
	int (*kv_get)(db_t *db_handle, const kv_readoptions_t *opt, const char *key, unsigned int key_len,
				char *val_buf, unsigned int buf_size, unsigned int *val_len,
				char **err);
	void (*kv_put)(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len,
				const char *val, unsigned int val_len, char **err);
	int (*kv_delete)(db_t *db_handle, const kv_writeoptions_t *opt, const char *key, unsigned int key_len, char **err);
};

struct io_check {
	struct db_ops *ops;
	db_t *target_db;

	char target[PATH_MAX];
	char target_path[PATH_MAX];
	char db_name[PATH_MAX];
	char loginfo[PATH_MAX];

	int nthread;
	struct io_thread *thread;
	volatile int stop;

	__u64 num;  // number of operations to stop
	int batch_count;
	__u64 max_seq;
	int interval;
	__u64 total_time;
	char rw[16];
	int rw_state;
	int seq_state;
	int check_state;
	int mb_memory;
	int value_size;
	int keysize;
	int reporting_perline;
	FILE *log_fp;
	int ready;
	int complete_nthread;
	pthread_cond_t ready_cond;
	pthread_mutex_t ready_mutex;

	/* stats */
	struct simple_stat **stat_rband;
	struct simple_stat **stat_wband;
	struct simple_stat **stat_riops;
	struct simple_stat **stat_wiops;

	double total_read_band_rt_rate;
	double total_read_band_avg_rate;
	double total_write_band_rt_rate;
	double total_write_band_avg_rate;

	double total_read_iops_rt_rate;
	double total_read_iops_avg_rate;
	double total_write_iops_rt_rate;
	double total_write_iops_avg_rate;

	double total_read_amount;
	double total_write_amount;
	double total_read_count;
	double total_write_count;
};

struct db_ops *get_shannon_db_ops();
struct db_ops *get_leveldb_ops();
struct db_ops *get_rocksdb_ops();

#endif
