#include <stdio.h>
#include <stdlib.h>

#include "util.h"

static void simple_stat_reset(struct simple_stat *stat)
{
	gettimeofday(&stat->start, NULL);
	gettimeofday(&stat->stop, NULL);
	stat->count = 0;
	stat->total = 0.0;
	stat->real_rate = 0.0;
	stat->avg_rate = 0.0;
}

static void simple_stat_add(struct simple_stat *stat, double v)
{
	struct timeval current;
	gettimeofday(&current, NULL);
	double real_epoch = (current.tv_sec - stat->stop.tv_sec) + (current.tv_usec - stat->stop.tv_usec) / 1e6;
	double total_epoch = (current.tv_sec - stat->start.tv_sec) + (current.tv_usec - stat->start.tv_usec) / 1e6;
	stat->count++;
	stat->real_rate = (v - stat->total) / real_epoch;
	stat->avg_rate = v / total_epoch;
	stat->total = v;
	stat->stop = current;
}

struct simple_stat *new_simple_stat()
{
	struct simple_stat *stat = malloc(sizeof(*stat));
	if (stat == NULL) {
		fprintf(stderr, "malloc simple_stat failed\n");
		return NULL;
	}
	stat->reset = simple_stat_reset;
	stat->add = simple_stat_add;
	stat->reset(stat);
	return stat;
}
