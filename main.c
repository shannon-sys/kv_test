#include"iocheck.h"

# define unlikely(x)    __builtin_expect(!!(x), 0)
#define __USE_GNU 1

#define	OPT_BATCH_COUNT		'b'
#define	OPT_DATABASE		'd'
#define	OPT_HELP			'h'
#define	OPT_LOG				'l'
#define	OPT_NUM				'm'
#define	OPT_NUM_THREADS		'n'
#define	OPT_TARGET_PATH		'p'
#define	OPT_DATA_RW			'r'
#define	OPT_MAXSEQ			's'
#define	OPT_TARGET			't'
#define	OPT_VALUE_SIZE		'v'

struct io_check *gic = NULL;

static struct option lopts[] = {
	{"target", required_argument, NULL, OPT_TARGET},
	{"path", required_argument, NULL, OPT_TARGET_PATH},
	{"rw", required_argument, NULL, OPT_DATA_RW},
	{"value-size", required_argument, NULL, OPT_VALUE_SIZE},
	{"num-threads", required_argument, NULL, OPT_NUM_THREADS},
	{"num", required_argument, NULL, OPT_NUM},
	{"loginfo", required_argument, NULL, OPT_LOG},
	{"maxseq", required_argument, NULL, OPT_MAXSEQ},
	{"batch_count", required_argument, NULL, OPT_BATCH_COUNT},
	{"dbname", required_argument, NULL, OPT_DATABASE},
	{"help", no_argument, NULL, OPT_HELP},
	{0, 0, 0, 0},
};

void *run_db_thread(void *arg);

void get_optstr(struct option *long_opt, char *optstr, int optstr_size)
{
	int c = 0;
	struct option *opt;
	memset(optstr, 0x00, optstr_size);
	optstr[c++] = ':';
	for (opt = long_opt; NULL != opt->name; opt++) {
		optstr[c++] = opt->val;
		if (required_argument == opt->has_arg) {
			optstr[c++] = ':';
		} else if (optional_argument == opt->has_arg) {
			optstr[c++] = ':';
			optstr[c++] = ':';
		}
		assert(c < optstr_size);
	}
}

void sigint_handler(int signo)
{
	if (NULL == gic)
		exit(EXIT_SUCCESS);
	gic->stop = 1;
}

static void usage(void)
{
	printf("Description:\n");
	printf("\tTest performance of shannon_db/leveldb/rocksdb with 'read/write/randread/randwrite'.\n");
	printf("Usage: ./test <target> <path> <rw> <value-size> <maxseq> [num-threads] [num] [batch_count]\n");
	printf("Options:\n");
	printf("\t--target=shannon_db/leveldb/rocksdb\n"
		   "\t\ttest target\n");
	printf("\t--path=/mnt/leveldb or /dev/kvdev0\n"
		   "\t\tif the target is leveldb, you should give the path of ssd mount which is like /mnt/leveldb.\n "
		   "\t\tif the target is rocksdb, you should give the path of ssd mount which is like /mnt/rocksdb.\n "
		   "\t\tif the target is shannon_db, you shoule give the path of kvssd which is like /dev/kvdev0.\n");
	printf("\t--rw=read/write/randread/randwrite\n");
	printf("\t--value-size\n"
		   "\t\tvalue size can be provided with unit like 4K, 1M\n");
	printf("\t--num-threads\n"
		   "\t\tnumber of test threads\n");
	printf("\t--num\n"
		   "\t\tnumber of operations to perform, 0 is endless\n");
	printf("\t--maxseq\n"
		   "\t\tthe max range of random key when you randread/randwrite\n");
	printf("\t--batch_count\n"
		   "\t\tnumber of kv in one batch\n");
	printf("\t--dbname\n"
		   "\t\tthe database name, used by shannon_db\n");
	printf("\t--loginfo=name\n"
		   "\t\tif you want to see the history log, you should use this options and give the log file name\n");
}

int parse_cmdline(struct io_check *check, int argc, char **argv)
{
	char optstr[256];
	int opt;
	opterr = 1;
	int len = 0;
	if (argc <= 1) {
		usage();
		exit(EXIT_FAILURE);
	}
	get_optstr(lopts, optstr, sizeof(optstr));
	while ((opt = getopt_long_only(argc, argv, optstr, lopts, NULL)) != -1) {
		char *endptr = NULL;
		switch (opt) {
		case OPT_TARGET:
			strcpy(check->target, optarg);
			break;
		case OPT_TARGET_PATH:
			strcpy(check->target_path, optarg);
			if ('/' == check->target_path[strlen(check->target_path) - 1])
				check->target_path[strlen(check->target_path) - 1] = 0;
			break;
		case OPT_NUM_THREADS:
			check->nthread = strtoul(optarg, NULL, 10);
			break;
		case OPT_NUM:
			check->num = strtoul(optarg, NULL, 10);
			break;
		case OPT_MAXSEQ:
			check->max_seq = strtoul(optarg, NULL, 10);
			break;
		case OPT_LOG:
			strcpy(check->loginfo, optarg);
			break;
		case OPT_DATA_RW:
			strncpy(check->rw, optarg, 16);
			break;
		case OPT_VALUE_SIZE:
			len = strtol(optarg, &endptr, 10);
			switch (*endptr) {
			case 'm':
			case 'M':
				len <<= 10;
			case 'k':
			case 'K':
				len <<= 10;
			}
			check->value_size = len;
			break;
		case OPT_BATCH_COUNT:
			check->batch_count = strtoul(optarg, NULL, 10);
			break;
		case OPT_HELP:
			usage();
			exit(EXIT_FAILURE);
		case OPT_DATABASE:
			strcpy(check->db_name, optarg);
			break;
		case '?':
			printf("the arguments is error,please input again!!!\n");
			exit(EXIT_FAILURE);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}

int check_args(struct io_check *check)
{
	if (!strlen(check->target)) {
		fprintf(stderr, "target name is missing !!!\n");
		return -EINVAL;
	}
	if (!strlen(check->target_path)) {
		fprintf(stderr, "target_path is missing\n");
		return -EINVAL;
	}
	if (check->nthread == 0) {
		printf("num-threads is missing, default num-threads 1\n");
		check->nthread = 1;
	}
	if (check->value_size == 0) {
		fprintf(stderr, "value-size is missing, please input again!!!\n");
		return -EINVAL;
	}
	if (check->value_size > MAX_VALUE_LENGTH) {
		fprintf(stderr, "the value size is bigger than 4M, please input again !!!\n");
		return -EINVAL;
	}
	if (check->max_seq == 0) {
		fprintf(stderr, "max_seq is missing, please input again!!!\n");
		return -EINVAL;
	}
	if (check->batch_count > MAX_BATCH_COUNT) {
		fprintf(stderr, "unsupported the batch_count, please input [0,%d] again!!!\n", MAX_BATCH_COUNT);
		return -EINVAL;
	}

	if (strlen(check->rw) == 0) {
		fprintf(stderr, "target rw  is missing, please input again\n");
		return -EINVAL;
	} else {
		if (!strcmp(check->rw, "read")) {
			check->rw_state = IOCHECK_READ;
			check->seq_state = IOCHECK_SEQ;
		} else if (!strcmp(check->rw, "write")) {
			check->rw_state = IOCHECK_WRITE;
			check->seq_state = IOCHECK_SEQ;
		} else if (!strcmp(check->rw, "randread")) {
			check->rw_state = IOCHECK_READ;
			check->seq_state = IOCHECK_RANDOM;
		} else if (!strcmp(check->rw, "randwrite")) {
			check->rw_state = IOCHECK_WRITE;
			check->seq_state = IOCHECK_RANDOM;
		} else {
			fprintf(stderr, "target rw is illegal, please input again\n");
			return -EINVAL;
		}
	}

	return 0;
}

char *human_size(off_t size, char *buf, int buf_size)
{
	int n = 0;
	char p[2] = {0, 0};
	char size_buf[256];

	if (buf_size <= 0) {
		fprintf(stderr, "invalid buf_size: %d\n", buf_size);
		return NULL;
	}

	memset(buf, 0, buf_size);

	if (size >= (1024 * 1024 * 1024)) {
		n += sprintf(size_buf + n, "%ldG", size / (1024 * 1024 * 1024));
		size %= (1024 * 1024 * 1024);
		p[0] = '.';
	}

	if (size >= (1024 * 1024)) {
		n += sprintf(size_buf + n, "%s%ldM", p, size / (1024 * 1024));
		size %= (1024 * 1024);
		p[0] = '.';
	}

	if (size >= 1024) {
		n += sprintf(size_buf + n, "%s%ldK", p, size / 1024);
		size %= 1024;
		p[0] = '.';
	}

	n += sprintf(size_buf + n, "%s%ld", p, size);
	if (n < buf_size) {
		strcpy(buf, size_buf);
	} else {
		fprintf(stderr, "buf_size(%d) is too small, need %d\n", buf_size, n + 1);
		return NULL;
	}
	return buf;
}

int open_db(struct io_check *check)
{
	char *err = NULL;

	if (strcmp("shannon_db", check->target) == 0) {
		if (!strlen(check->db_name)) {
			fprintf(stderr, "database_name is missing\n");
			return -EINVAL;
		}
		check->ops = get_shannon_db_ops();
	} else if (strcmp("leveldb", check->target) == 0) {
		check->ops = get_leveldb_ops();
	} else if (strcmp("rocksdb", check->target) == 0) {
		check->ops = get_rocksdb_ops();
	} else {
		fprintf(stderr, "please input leveldb/rocksdb/shannon_db\n");
		return -EINVAL;
	}

	if (check->ops == NULL) {
		fprintf(stderr, "ops is NULL, check your compile FLAG\n");
		return -1;
	}

	check->target_db = check->ops->kv_open(check->target_path, check->db_name, &err);

	if (check->target_db == NULL) {
		fprintf(stderr, "open db failed: db_name=%s\n", check->db_name);
		return -EINVAL;
	}

	return 0;
}

int create_thread_data(struct io_check *check)
{
	struct io_thread *thread;
	int i;

	check->thread = calloc(check->nthread, sizeof(*check->thread));
	if (NULL == check->thread) {
		printf("Allocate check->thread failed\n");
		return -1;
	}
	for (i = 0; i < check->nthread; i++) {
		thread = check->thread + i;
		thread->id = i;
		thread->check = check;
		thread->value_size = check->value_size;
	}

	return 0;
}

int create_threads(struct io_check *check)
{
	pthread_attr_t attr;
	int status;
	int i;

	status = pthread_attr_init(&attr);
	if (status) {
		printf("pthread_attr_init failed: %s\n", strerror(status));
		return -1;
	}
	status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (status) {
		printf("pthread_attr_setdetachstate failed: %s\n", strerror(status));
		return -1;
	}

	if (create_thread_data(check))
		return -1;

	// create threads
	for (i = 0; i < check->nthread; i++) {
		if (!strcmp("leveldb", check->target)) {
			status = pthread_create(&check->thread[i].tid, &attr, &run_db_thread, (void *)(check->thread + i));
		} else if (!strcmp("rocksdb", check->target)) {
			status = pthread_create(&check->thread[i].tid, &attr, &run_db_thread, (void *)(check->thread + i));
		} else if (!strcmp("shannon_db", check->target)) {
			status = pthread_create(&check->thread[i].tid, &attr, &run_db_thread, (void *)(check->thread + i));
		} else {
			printf("the target is not supported, please input leveldb/rocksdb/shannon_db again !!!\n");
			return -1;
		}
		if (status) {
			printf("create thread %d failed: %s\n", i, strerror(status));
			return -1;
		}
	}

	status = pthread_attr_destroy(&attr);
	if (status) {
		printf("pthread_attr_destroy failed: %s\n", strerror(status));
		return -1;
	}

	return 0;
}

void print_info(struct io_check *check)
{
	time_t rawtime;

	fprintf(check->log_fp, "---------------------------------------------------------------------\n");
	fprintf(check->log_fp, "%-40s%s\n", "Target:", check->target);
	fprintf(check->log_fp, "%-40s%s\n", "Func rw:", check->rw);
	fprintf(check->log_fp, "%-40s%d\n", "Value size:", check->value_size);
	fprintf(check->log_fp, "%-40s%d\n", "Num of threads:", check->nthread);
	fprintf(check->log_fp, "%-40s%lld\n", "Num of kv read/write:", check->num);
	fprintf(check->log_fp, "%-40s%d\n", "Loginfo (0 off, 1 on):", (check->log_fp == NULL) ? 0 : 1);
	fprintf(check->log_fp, "%-40s%d\n", "Batch size:", check->batch_count);
	if (strlen(check->db_name))
		fprintf(check->log_fp, "%-40s%s\n", "Database name:", check->db_name);
	time(&rawtime);
	fprintf(check->log_fp, "%-40s%s\n", "Start time:", ctime(&rawtime));
	fprintf(check->log_fp, "---------------------------------------------------------------------\n");
}

void init_stats(struct io_check *check)
{
	int i;

	check->stat_rband = malloc(sizeof(*check->stat_rband) * check->nthread);
	check->stat_wband = malloc(sizeof(*check->stat_wband) * check->nthread);
	check->stat_riops = malloc(sizeof(*check->stat_riops) * check->nthread);
	check->stat_wiops = malloc(sizeof(*check->stat_wiops) * check->nthread);
	assert(check->stat_rband && check->stat_wband && check->stat_riops && check->stat_wiops);
	for (i = 0; i < check->nthread; i++) {
		check->stat_rband[i] = new_simple_stat();
		check->stat_wband[i] = new_simple_stat();
		check->stat_riops[i] = new_simple_stat();
		check->stat_wiops[i] = new_simple_stat();
		assert(check->stat_rband[i] && check->stat_wband[i] && check->stat_riops[i] && check->stat_wiops[i]);
	}
}

void free_stats(struct io_check *check)
{
	int i;

	for (i = 0; i < check->nthread; i++) {
		free(check->stat_rband[i]);
		free(check->stat_wband[i]);
		free(check->stat_riops[i]);
		free(check->stat_wiops[i]);
	}
	free(check->stat_rband);
	free(check->stat_wband);
	free(check->stat_riops);
	free(check->stat_wiops);
}

void update_stats(struct io_check *check)
{
	int i;

	check->total_read_band_rt_rate = 0;
	check->total_read_band_avg_rate = 0;
	check->total_write_band_rt_rate = 0;
	check->total_write_band_avg_rate = 0;

	check->total_read_iops_rt_rate = 0;
	check->total_read_iops_avg_rate = 0;
	check->total_write_iops_rt_rate = 0;
	check->total_write_iops_avg_rate = 0;

	check->total_read_amount = 0;
	check->total_write_amount = 0;
	check->total_read_count = 0;
	check->total_write_count = 0;


	for (i = 0; i < check->nthread; i++) {
		check->stat_rband[i]->add(check->stat_rband[i], check->thread[i].read_length);
		check->total_read_band_rt_rate += check->stat_rband[i]->real_rate;
		check->total_read_band_avg_rate += check->stat_rband[i]->avg_rate;
		check->total_read_amount += check->stat_rband[i]->total;

		check->stat_wband[i]->add(check->stat_wband[i], check->thread[i].write_length);
		check->total_write_band_rt_rate += check->stat_wband[i]->real_rate;
		check->total_write_band_avg_rate += check->stat_wband[i]->avg_rate;
		check->total_write_amount += check->stat_wband[i]->total;

		check->stat_riops[i]->add(check->stat_riops[i], check->thread[i].read_count);
		check->total_read_iops_rt_rate += check->stat_riops[i]->real_rate;
		check->total_read_iops_avg_rate += check->stat_riops[i]->avg_rate;
		check->total_read_count += check->stat_riops[i]->total;

		check->stat_wiops[i]->add(check->stat_wiops[i], check->thread[i].write_count);
		check->total_write_iops_rt_rate += check->stat_wiops[i]->real_rate;
		check->total_write_iops_avg_rate += check->stat_wiops[i]->avg_rate;
		check->total_write_count += check->stat_wiops[i]->total;
	}
}

void print_stats(struct io_check *check)
{
	char total_read_str[256];
	char total_write_str[256];

	if (!check->reporting_perline || check->log_fp) {
		moveto_head();
		clear_line();
	}

	fprintf(check->log_fp, "[%llu | %s | %lld | %s | %lld]\n", check->total_time,
			human_size((long long)check->total_write_amount, total_write_str, 256), (long long)check->total_write_count,
			human_size((long long)check->total_read_amount, total_read_str, 256), (long long)check->total_read_count);
	fprintf(check->log_fp, "BAND(RT): write: %.2fMB/s, read: %.2fMB/s | BAND(AVG): write %.2fMB/s, read: %.2fMB/s\n",
			check->total_write_band_rt_rate / (1024.0 * 1024),
			check->total_read_band_rt_rate / (1024.0 * 1024),
			check->total_write_band_avg_rate / (1024.0 * 1024),
			check->total_read_band_avg_rate / (1024.0 * 1024));
	fprintf(check->log_fp, "IOPS(RT): write: %.0f/s, read: %.0f/s | IOPS(AVG): write: %.0f/s, read: %.0f/s\n",
			check->total_write_iops_rt_rate,
			check->total_read_iops_rt_rate,
			check->total_write_iops_avg_rate,
			check->total_read_iops_avg_rate);
}

void print_summary_stats(struct io_check *check)
{
	time_t rawtime;
	int i;

	fprintf(check->log_fp, "\n\n");
	for (i = 0; i < check->nthread; i++) {
		fprintf(check->log_fp, "Thread %3d: write %.2fGB (%llu) read %.2fGB (%llu) write_count (%lld) read_count (%lld)\n", i,
				check->stat_wband[i]->total / (1024.0 * 1024 * 1024), (long long)check->stat_wband[i]->total,
				check->stat_rband[i]->total / (1024.0 * 1024 * 1024), (long long)check->stat_rband[i]->total,
				(long long)check->stat_wiops[i]->total, (long long)check->stat_riops[i]->total);
	}
	fprintf(check->log_fp, "\n\n");
	fprintf(check->log_fp, "All thread: write %.2fGB (%lld) read %.2fGB (%lld)\n",
			check->total_write_amount / (1024.0 * 1024 * 1024), (long long)check->total_write_amount,
			check->total_read_amount / (1024.0 * 1024 * 1024), (long long)check->total_read_amount);
	fprintf(check->log_fp, "\nAverage: write bw: %.2fMB/s  read %.2fMB/s\n",
			check->total_write_band_avg_rate / (1024.0 * 1024),
			check->total_read_band_avg_rate / (1024.0 * 1024));
	fprintf(check->log_fp, "Average: write iops: %.2f k/s  read iops %.2f k/s\n",
			check->total_write_iops_avg_rate / 1000.0,
			check->total_read_iops_avg_rate / 1000.0);

	time(&rawtime);
	fprintf(check->log_fp, "\nEnd time: %s", ctime(&rawtime));
}

int main(int argc, char *argv[])
{
	struct io_check *check = NULL;
	int status = 0;
	int i;

	srand48(time(NULL));
	srandom(time(NULL) + 13);
	srand(time(NULL) + 17);

	check = malloc(sizeof(*check));
	if (NULL == check) {
		printf("Allocate check failed\n");
		exit(EXIT_FAILURE);
	}
	memset(check, 0x00, sizeof(*check));
	check->interval = 1;
	check->seq_state = IOCHECK_RANDOM;

	if (parse_cmdline(check, argc, argv))
		exit(EXIT_FAILURE);

	if (check_args(check))
		exit(EXIT_FAILURE);

	check->log_fp = stdout;

	if (strlen(check->loginfo)) {
		check->log_fp = fopen(check->loginfo, "w");
		if (!check->log_fp) {
			printf("open perline file %s failed\n", optarg);
			exit(EXIT_FAILURE);
		}
	}

	status = pthread_mutex_init(&check->ready_mutex, NULL);
	if (status) {
		printf("pthread_mutex_init failed: %s\n", strerror(status));
		exit(EXIT_FAILURE);
	}
	status = pthread_cond_init(&check->ready_cond, NULL);
	if (status) {
		printf("pthread_cond_init failed: %s\n", strerror(status));
		exit(EXIT_FAILURE);
	}

	if (open_db(check))
		exit(EXIT_FAILURE);

	if (create_threads(check))
		exit(EXIT_FAILURE);

	gic = check;

	// register signal handler
	if (SIG_ERR == signal(SIGINT, sigint_handler)) {
		printf("set signal handler failed\n");
		exit(EXIT_FAILURE);
	}

	pthread_mutex_lock(&check->ready_mutex);
	while (check->ready < check->nthread)
		pthread_cond_wait(&check->ready_cond, &check->ready_mutex);
	pthread_mutex_unlock(&check->ready_mutex);

	print_info(check);

	init_stats(check);

	while (check->complete_nthread < check->nthread) {
		sleep(check->interval);
		check->total_time += check->interval;

		update_stats(check);
		print_stats(check);

		pthread_mutex_lock(&check->ready_mutex);
		if (check->complete_nthread >= check->nthread) {
			check->stop = 1;
		}
		pthread_mutex_unlock(&check->ready_mutex);
	}

	for (i = 0; i < check->nthread; i++) {
		status = pthread_join(check->thread[i].tid, NULL);
		if (status) {
			printf("pthread_join failed: %s\n", strerror(status));
			exit(EXIT_FAILURE);
		}
	}

	print_summary_stats(check);

	check->ops->kv_close(check->target_db);

	free_stats(check);

	if (check->log_fp)
		fclose(check->log_fp);
	if (check->thread)
		free(check->thread);
	if (check)
		free(check);
	return 0;
}

static __u64 key_to_integer(const char *key)
{
	return *(__u64 *)key;
}

int create_read_options(struct io_thread *thread)
{
	struct io_check *check = thread->check;
	char **batch_val_buf_base = NULL;
	unsigned int *batch_val_len_base = NULL;
	unsigned int *batch_status_base = NULL;
	int i;

	thread->read_options = check->ops->kv_readoptions_create();
	check->ops->kv_readoptions_set_fill_cache(thread->read_options, false);
	if (check->batch_count > 0) {
		if (check->ops->kv_readbatch_create == NULL) {
			fprintf(stderr, "read batch mode is not supported, use normal read instead.\n");
			check->batch_count = 0;
		} else {
			batch_val_buf_base = malloc(check->batch_count * sizeof(*batch_val_buf_base));
			if (batch_val_buf_base == NULL) {
				fprintf(stderr, "malloc batch_val_buf_base failed\n");
				return -1;
			}
			for (i = 0; i < check->batch_count; i++) {
				batch_val_buf_base[i] = malloc(thread->value_size);
				if (batch_val_buf_base[i] == NULL) {
					fprintf(stderr, "malloc batch_val_buf_base[%d] failed\n", i);
					return -1;
				}
			}
			batch_val_len_base = malloc(check->batch_count * sizeof(*batch_val_len_base));
			if (batch_val_len_base == NULL) {
				fprintf(stderr, "malloc batch_val_len_base failed\n");
				return -1;
			}
			batch_status_base = malloc(check->batch_count * sizeof(*batch_status_base));
			if (batch_status_base == NULL) {
				fprintf(stderr, "malloc batch_status_base failed\n");
				return -1;
			}
			thread->read_batch = check->ops->kv_readbatch_create();
			thread->batch_val_buf_base = batch_val_buf_base;
			thread->batch_val_len_base = batch_val_len_base;
			thread->batch_status_base = batch_status_base;
		}
	}

	return 0;
}

int create_write_options(struct io_thread *thread)
{
	struct io_check *check = thread->check;

	thread->write_options = check->ops->kv_writeoptions_create();
	check->ops->kv_writeoptions_set_sync(thread->write_options, true);
	if (check->batch_count > 0) {
		if (check->ops->kv_writebatch_create == NULL) {
			fprintf(stderr, "write batch mode is not supported, use normal write instead.\n");
			check->batch_count = 0;
		} else {
			thread->write_batch = check->ops->kv_writebatch_create();
		}
	}
	return 0;
}

int do_single_read(struct io_thread *thread, const char *key, unsigned int key_len,
					char *buffer, unsigned int buf_size)
{
	struct io_check *check = thread->check;
	kv_readoptions_t *option = thread->read_options;
	unsigned int val_len;
	char *err = NULL;

	check->ops->kv_get(check->target_db, option, key, key_len, buffer, buf_size, &val_len, &err);
	if (err) {
		fprintf(stderr, "kv_get() failed: can not read key=%lld\n", key_to_integer(key));
		return -1;
	} else {
		thread->read_count++;
		thread->read_length += key_len + val_len;
	}
	return 0;
}

int do_batch_read(struct io_thread *thread, const char *key, unsigned int key_len, unsigned int buf_size)
{
	struct io_check *check = thread->check;
	kv_readoptions_t *option = thread->read_options;
	kv_readbatch_t *batch = thread->read_batch;
	int batch_index = thread->curr_batch_count;
	int i;
	char *err = NULL;

	check->ops->kv_readbatch_add(batch, key, key_len, thread->batch_val_buf_base[batch_index], buf_size,
				thread->batch_val_len_base + batch_index, thread->batch_status_base + batch_index, &err);
	if (err) {
		fprintf(stderr, "kv_readbatch_add() failed: can not add key=%llu\n", key_to_integer(key));
		return -1;
	}
	thread->curr_batch_count++;
	if (thread->curr_batch_count == check->batch_count) {
		unsigned int failed_cmd_count = 0;
		err = NULL;
		check->ops->kv_readbatch_submit(check->target_db, option, batch, &failed_cmd_count, &err);
		if (err) {
			fprintf(stderr, "kv_readbatch_submit() failed: %s\n", err);
			return -1;
		}
		thread->read_count += thread->curr_batch_count;
		for (i = 0; i < thread->curr_batch_count; i++) {
			thread->read_length += thread->batch_val_len_base[i];
		}
		check->ops->kv_readbatch_clear(batch);
		thread->curr_batch_count = 0;
	}
	return 0;
}

int do_single_write(struct io_thread *thread, const char *key, unsigned int key_len, const char *val_buf, unsigned int val_len)
{
	struct io_check *check = thread->check;
	char *err = NULL;

	check->ops->kv_put(check->target_db, thread->write_options, key, key_len, val_buf, val_len, &err);
	if (err) {
		fprintf(stderr, "kv_put() failed: can not put key=%lld, err=%s.\n", key_to_integer(key), err);
		return -1;
	} else {
		thread->write_count++;
		thread->write_length += key_len + val_len;
	}
	return 0;
}

int do_batch_write(struct io_thread *thread, const char *key, unsigned int key_len, const char *val_buf, unsigned int val_len)
{
	struct io_check *check = thread->check;
	char *err = NULL;

	check->ops->kv_writebatch_add(thread->write_batch, key, key_len, val_buf, val_len, &err);
	if (err) {
		fprintf(stderr, "kv_writebatch_add() failed: can not add key=%lld\n", key_to_integer(key));
		return -1;
	}
	thread->curr_batch_count++;
	thread->write_count++;
	thread->write_length += key_len + val_len;
	if (thread->curr_batch_count == check->batch_count) {
		err = NULL;
		check->ops->kv_writebatch_submit(check->target_db, thread->write_options, thread->write_batch, &err);
		if (err) {
			fprintf(stderr, "kv_writebatch_submit() failed\n");
			return -1;
		}
		check->ops->kv_writebatch_clear(thread->write_batch);
		thread->curr_batch_count = 0;
	}
	return 0;
}

void free_thread_resource(struct io_thread *thread)
{
	struct io_check *check = thread->check;
	struct db_ops *ops = check->ops;
	int i;

	if (thread->write_options)
		ops->kv_writeoptions_destroy(thread->write_options);
	if (thread->read_options)
		ops->kv_readoptions_destroy(thread->read_options);
	if (thread->write_batch) {
		if (thread->curr_batch_count) {
			ops->kv_writebatch_clear(thread->write_batch);
		}
		ops->kv_writebatch_destroy(thread->write_batch);
	}
	if (thread->read_batch) {
		if (thread->curr_batch_count) {
			ops->kv_readbatch_clear(thread->read_batch);
		}
		ops->kv_readbatch_destroy(thread->read_batch);
	}
	free(thread->val_buf);
	if (thread->batch_val_buf_base) {
		for (i = 0; i < check->batch_count; i++) {
			free(thread->batch_val_buf_base[i]);
		}
		free(thread->batch_val_buf_base);
	}
	free(thread->batch_val_len_base);
	free(thread->batch_status_base);
}

void *run_db_thread(void *arg)
{
	struct io_thread *thread = (struct io_thread *)arg;
	struct io_check *check = thread->check;

	__u64 seq_key = 0;
	__u64 tmp_key = 0;
	char *key = NULL;
	int key_len = sizeof(__u64);

	int i;

	pthread_mutex_lock(&check->ready_mutex);
	check->ready++;
	if (check->ready >= check->nthread)
		pthread_cond_signal(&check->ready_cond);
	pthread_mutex_unlock(&check->ready_mutex);

	// for single value buffer (read/write)
	thread->val_buf = malloc(thread->value_size);
	if (thread->val_buf == NULL) {
		fprintf(stderr, "malloc thread->val_buf failed\n");
		thread->terminate = 1;
		goto out;
	}
	for (i = 0; i < thread->value_size; i++) {
		thread->val_buf[i] = random(0x100);
	}

	// construct readbatch/writebatch
	if (check->rw_state == IOCHECK_READ) {
		if (create_read_options(thread)) {
			thread->terminate = 1;
			goto out;
		}
	} else {
		if (create_write_options(thread)) {
			thread->terminate = 1;
			goto out;
		}
	}

	while (!thread->terminate && !check->stop) {
		// gen key
		if (check->seq_state == IOCHECK_SEQ) {
			tmp_key = seq_key++;
		} else {
			tmp_key = random(check->max_seq);
		}
		key = (char *)&tmp_key;

		if (check->rw_state == IOCHECK_READ) {
			if (check->batch_count > 0) {
				thread->terminate |= do_batch_read(thread, key, key_len, thread->value_size);
			} else {
				thread->terminate |= do_single_read(thread, key, key_len, thread->val_buf, thread->value_size);
			}
		} else {
			if (check->batch_count > 0) {
				thread->terminate |= do_batch_write(thread, key, key_len, thread->val_buf, thread->value_size);
			} else {
				thread->terminate |= do_single_write(thread, key, key_len, thread->val_buf, thread->value_size);
			}
		}

		// check exit condition
		if (check->num && ((thread->read_count >= check->num) || (thread->write_count >= check->num))) {
			thread->terminate = 1;
		}
	}

out:
	free_thread_resource(thread);

	pthread_mutex_lock(&check->ready_mutex);
	check->complete_nthread++;
	pthread_mutex_unlock(&check->ready_mutex);

	return (void *)0;
}
