/*
 * Benchmarking functions inspired from the Linux kernel code and the
 * PCIe EPF testing framework, commit:
 * fb9b1f85326abc95bb469ecd22796b92cf1e815e
 * 
 * Copyright (C) 2017 Texas Instruments
 * Copyright 2021, 2023 NXP
 * 
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <limits.h>
#include "pcie_benchmark.h"

/**
 * do_div - returns 2 values: calculate remainder and update new dividend
 * @n: uint64_t dividend (will be updated)
 * @base: uint32_t divisor
 *
 * Summary:
 * ``uint32_t remainder = n % base;``
 * ``n = n / base;``
 *
 * Return: (uint32_t)remainder
 *
 * NOTE: macro parameter @n is evaluated multiple times,
 * beware of side effects!
 * 
 * NOTE: this macro is taken from the linux kernel and works for 64bit integers.
 */
# define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })

#define NSEC_PER_SEC	1000000000L

/**
 * timespec_to_ns - Convert timespec to nanoseconds
 * @ts:		pointer to the timespec variable to be converted
 *
 * Returns the scalar nanosecond representation of the timespec
 * parameter.
 * 
 * NOTE: this function is taken from the linux kernel.
 */
static inline int64_t timespec_to_ns(const struct timespec *ts)
{
	return ((int64_t) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

/* Subtract the ‘struct timespec’ values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 * 
 * Inspired from: 
 * https://www.gnu.org/software/libc/manual/html_node/Calculating-Elapsed-Time.html
 */

static int timespec_substract (struct timespec *result,
		struct timespec *x,
		struct timespec *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_nsec < y->tv_nsec) {
		int n_sec = (y->tv_nsec - x->tv_nsec) / NSEC_PER_SEC + 1;
		y->tv_nsec -= NSEC_PER_SEC * n_sec;
		y->tv_sec += n_sec;
	}
	if (x->tv_nsec - y->tv_nsec > NSEC_PER_SEC) {
		int n_sec = (x->tv_nsec - y->tv_nsec) / NSEC_PER_SEC;
		y->tv_nsec += NSEC_PER_SEC * n_sec;
		y->tv_sec -= n_sec;
	}

	/* Compute the time remaining to wait.
	 tv_nsec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

void pcie_test_start(struct timespec *start)
{
	if (!start) {
		printf("Invalid time reference\n");
		return;
	}
	
	clock_gettime(CLOCK_REALTIME, start);
}

void pcie_test_stop(struct timespec *start,
		const char *ops, uint64_t size,
		int dma)
{
	struct timespec ts, end;
	uint64_t rate, ns;
	int kb;

	clock_gettime(CLOCK_REALTIME, &end);
	timespec_substract(&ts, &end, start);

	/* convert both size (stored in 'rate') and time in terms of 'ns' */
	ns = timespec_to_ns(&ts);
	rate = size * NSEC_PER_SEC;

	/* Divide both size (stored in 'rate') and ns by a common factor */
	while (ns > UINT_MAX) {
		   rate >>= 1;
		   ns >>= 1;
	}

	if (!ns)
		   return;

	/* calculate the rate */
	do_div(rate, (uint32_t)ns);
	kb = (rate >= 1024);

	printf("\n%s => Size: %lu bytes\t DMA: %s\t Time: %lu.%09u seconds\t"
		   "Rate: %lu %s/s\n", ops, size, dma ? "YES" : "NO",
		   (uint64_t)ts.tv_sec, (uint32_t)ts.tv_nsec,
		   kb ? rate/1024 : rate, kb ? "KB" : "B");
}

void pcie_show_mem(unsigned int *buff,
		unsigned int size,
		char *mem_name, unsigned int count)
{
	int i;

	printf("\n First %u DWORDS(1DW = 4bytes) %s", count, mem_name);
	for (i = 0; i < count; i++) {
		printf("\n *(buf + %#8x) = %#08x", i, *(buff + i));
	}
	/* Resize for last DWs */
	printf("\n Last %u DWORDS(1DW = 4bytes) %s", count, mem_name);
	for (i = (size / 4) - count; i < size / 4; i++) {
		printf("\n *(buff + %#8x) = %#08x", i, *(buff + i));
	}
}
