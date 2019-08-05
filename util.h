#ifndef __UTIL_H
#define __UTIL_H

#include <sys/time.h>

struct simple_stat {
	struct timeval start;
	struct timeval stop;

	long long count;
	double total;
	double real_rate;
	double avg_rate;

	void (*reset)(struct simple_stat *stat);
	void (*add)(struct simple_stat *stat, double v);
};

struct simple_stat *new_simple_stat();

#endif
