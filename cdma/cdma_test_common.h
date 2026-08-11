/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright 2026 NXP
 */

#ifndef CDMA_TEST_COMMON_H
#define CDMA_TEST_COMMON_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static inline int cdma_parse_size(const char *arg, size_t *size)
{
	unsigned long long value;
	char *end;

	errno = 0;
	value = strtoull(arg, &end, 0);

	if (errno || end == arg)
		return -1;

	if (*end) {
		switch (*end) {
		case 'k':
		case 'K':
			value *= 1024ULL;
			end++;
			break;
		case 'm':
		case 'M':
			value *= 1024ULL * 1024ULL;
			end++;
			break;
		case 'g':
		case 'G':
			value *= 1024ULL * 1024ULL * 1024ULL;
			end++;
			break;
		default:
			return -1;
		}

		if (*end == 'b' || *end == 'B')
			end++;
		if (*end)
			return -1;
	}

	if (!value || value > (unsigned long long)SIZE_MAX)
		return -1;

	*size = (size_t)value;

	return 0;
}

static inline double cdma_ts_diff_us(const struct timespec *start,
				     const struct timespec *end)
{
	return (end->tv_sec - start->tv_sec) * 1e6 +
	       (end->tv_nsec - start->tv_nsec) / 1e3;
}

static inline void cdma_fill_pattern(uint8_t *buf, size_t len, uint32_t seed)
{
	size_t i;

	for (i = 0; i < len; i++)
		buf[i] = (uint8_t)(seed + i);
}

#endif /* CDMA_TEST_COMMON_H */
