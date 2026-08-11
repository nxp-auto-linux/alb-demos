// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * NXP S32N79 CDMA - zero-source/discard-sink performance test
 *
 * Measures CDMA userspace throughput without pattern generation,
 * verification, or filesystem I/O in the timed path. TX repeatedly
 * writes pre-zeroed aligned buffers to the CDMA pipe; RX reads into
 * aligned buffers and immediately reuses them, discarding the data.
 *
 * Build:
 *   gcc -O2 -Wall -o cdma_perf_test cdma_perf_test.c -luring
 *
 * Copyright 2026 NXP
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <liburing.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cdma_test_common.h"

#define CDMA_PERF_DEFAULT_DEPTH 16

enum cdma_dir { DIR_NONE, DIR_TX, DIR_RX };
enum cdma_api { API_SYNC, API_URING };

struct xfer_buf {
	void *ptr;
	int in_flight;
};

static ssize_t full_write(int fd, const void *buf, size_t len)
{
	size_t done = 0;

	while (done < len) {
		ssize_t ret = write(fd, (const char *)buf + done, len - done);

		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (!ret) {
			errno = EIO;
			return -1;
		}
		done += ret;
	}

	return (ssize_t)done;
}

static ssize_t full_read(int fd, void *buf, size_t len)
{
	size_t done = 0;

	while (done < len) {
		ssize_t ret = read(fd, (char *)buf + done, len - done);

		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (!ret) {
			errno = EIO;
			return -1;
		}
		done += ret;
	}

	return (ssize_t)done;
}

static int run_sync(int fd, enum cdma_dir dir, void *buf, size_t len,
			unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		ssize_t ret;

		if (dir == DIR_TX)
			ret = full_write(fd, buf, len);
		else
			ret = full_read(fd, buf, len);

		if (ret < 0) {
			perror(dir == DIR_TX ? "write" : "read");
			return -1;
		}
		if ((size_t)ret != len) {
			fprintf(stderr, "short transfer: %zd/%zu\n", ret, len);
			return -1;
		}
	}

	return 0;
}

static int find_free_buf(struct xfer_buf *bufs, unsigned int nr_bufs)
{
	unsigned int i;

	for (i = 0; i < nr_bufs; i++) {
		if (!bufs[i].in_flight)
			return (int)i;
	}

	return -1;
}

static int run_uring(struct io_uring *ring, int fd, enum cdma_dir dir,
		     struct xfer_buf *bufs, size_t len, unsigned int count,
		     unsigned int max_inflight)
{
	unsigned int submitted = 0, completed = 0, in_flight = 0;
	int ret;

	while (completed < count) {
		unsigned int prepared = 0;

		while (submitted < count && in_flight < max_inflight) {
			struct io_uring_sqe *sqe;
			int idx = find_free_buf(bufs, max_inflight);

			if (idx < 0)
				break;

			sqe = io_uring_get_sqe(ring);
			if (!sqe)
				break;

			if (dir == DIR_TX)
				io_uring_prep_write(sqe, fd, bufs[idx].ptr, len, 0);
			else
				io_uring_prep_read(sqe, fd, bufs[idx].ptr, len, 0);

			io_uring_sqe_set_data64(sqe, idx + 1ULL);
			bufs[idx].in_flight = 1;
			submitted++;
			in_flight++;
			prepared++;
		}

		if (prepared) {
			ret = io_uring_submit(ring);
			if (ret < 0) {
				fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
				goto err;
			}
			if ((unsigned int)ret != prepared) {
				fprintf(stderr, "short io_uring submit: %d/%u\n",
					ret, prepared);
				goto err;
			}
		}

		if (in_flight) {
			struct io_uring_cqe *cqe;
			unsigned long long data;
			unsigned int idx;

			ret = io_uring_wait_cqe(ring, &cqe);
			if (ret < 0) {
				fprintf(stderr, "io_uring_wait_cqe: %s\n",
					strerror(-ret));
				goto err;
			}

			data = io_uring_cqe_get_data64(cqe);
			if (!data || data > max_inflight) {
				fprintf(stderr, "bad cqe data: %llu\n", data);
				io_uring_cqe_seen(ring, cqe);
				goto err;
			}

			idx = (unsigned int)data - 1;
			bufs[idx].in_flight = 0;
			in_flight--;
			completed++;

			if (cqe->res < 0) {
				fprintf(stderr, "cqe: %s\n", strerror(-cqe->res));
				io_uring_cqe_seen(ring, cqe);
				goto err;
			}
			if ((size_t)cqe->res != len) {
				fprintf(stderr, "short transfer: %d/%zu\n",
					cqe->res, len);
				io_uring_cqe_seen(ring, cqe);
				goto err;
			}

			io_uring_cqe_seen(ring, cqe);
		}
	}

	return 0;

err:
	return -1;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s {-t|-r} DEVPATH [-a sync|uring] [-s SIZE] [-n COUNT] [-d DEPTH] [-i INFLIGHT] [-w WARMUP]\n"
		"  -t DEVPATH   TX mode, write zero buffers to CDMA TX device\n"
		"  -r DEVPATH   RX mode, read CDMA RX device and discard data\n"
		"  -a API       sync or uring (default sync)\n"
		"  -s SIZE      transfer size, bytes or K/M/G suffix (default 262144)\n"
		"  -n COUNT     timed transfer count (default 4096)\n"
		"  -d DEPTH     io_uring depth (default %u, uring only)\n"
		"  -i INFLIGHT  max submitted CDMA ops at once (default 16)\n"
		"  -w WARMUP    untimed warmup transfer count (default 16)\n",
		prog, CDMA_PERF_DEFAULT_DEPTH);
}

int main(int argc, char **argv)
{
	unsigned int count = 4096, depth = CDMA_PERF_DEFAULT_DEPTH;
	size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
	unsigned int max_inflight = 16, warmup = 16;
	int ring_ready = 0, fd = -1, opt, ret = 1;
	enum cdma_dir dir = DIR_NONE;
	enum cdma_api api = API_SYNC;
	struct xfer_buf *bufs = NULL;
	const char *devpath = NULL;
	unsigned int num_bufs, i;
	struct timespec t0, t1;
	struct io_uring ring;
	size_t xfer = 262144;
	double us, mbps;

	while ((opt = getopt(argc, argv, "t:r:a:s:n:d:i:w:h")) != -1) {
		switch (opt) {
		case 't':
			dir = DIR_TX;
			devpath = optarg;
			break;
		case 'r':
			dir = DIR_RX;
			devpath = optarg;
			break;
		case 'a':
			if (!strcmp(optarg, "sync"))
				api = API_SYNC;
			else if (!strcmp(optarg, "uring"))
				api = API_URING;
			else {
				usage(argv[0]);
				return 1;
			}
			break;
		case 's':
			if (cdma_parse_size(optarg, &xfer)) {
				usage(argv[0]);
				return 1;
			}
			break;
		case 'n':
			count = (unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'd':
			depth = (unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'i':
			max_inflight = (unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'w':
			warmup = (unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'h':
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 1;
		}
	}

	if (dir == DIR_NONE || !devpath || !xfer || !count || !depth ||
	    !max_inflight || max_inflight > depth) {
		usage(argv[0]);
		return 1;
	}

	num_bufs = api == API_URING ? max_inflight : 1;
	bufs = calloc(num_bufs, sizeof(*bufs));
	if (!bufs) {
		perror("calloc");
		return 1;
	}

	for (i = 0; i < num_bufs; i++) {
		if (posix_memalign(&bufs[i].ptr, page_size, xfer)) {
			perror("posix_memalign");
			goto out;
		}
		memset(bufs[i].ptr, 0, xfer);
	}

	fd = open(devpath, dir == DIR_TX ? O_WRONLY : O_RDONLY);
	if (fd < 0) {
		perror(devpath);
		goto out;
	}

	if (api == API_URING) {
		ret = io_uring_queue_init(depth, &ring, 0);
		if (ret < 0) {
			fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
			goto out;
		}
		ring_ready = 1;
	}

	if (warmup) {
		if (api == API_SYNC)
			ret = run_sync(fd, dir, bufs[0].ptr, xfer, warmup);
		else
			ret = run_uring(&ring, fd, dir, bufs, xfer, warmup,
					max_inflight);
		if (ret)
			goto out;
	}

	clock_gettime(CLOCK_MONOTONIC, &t0);
	if (api == API_SYNC)
		ret = run_sync(fd, dir, bufs[0].ptr, xfer, count);
	else
		ret = run_uring(&ring, fd, dir, bufs, xfer, count, max_inflight);
	clock_gettime(CLOCK_MONOTONIC, &t1);
	if (ret)
		goto out;

	us = cdma_ts_diff_us(&t0, &t1);
	mbps = (double)count * xfer / us;
	unsigned long long total = (unsigned long long)count * xfer;

	printf("%s %s perf: %u x %zu B = %llu B in %.1f ms  "
		   "(%.1f MB/s, depth=%u, inflight=%u, warmup=%u)\n",
		   dir == DIR_TX ? "TX" : "RX",
		   api == API_SYNC ? "sync" : "uring",
		   count, xfer, total, us / 1000.0, mbps,
		   api == API_URING ? depth : 1,
		   api == API_URING ? max_inflight : 1, warmup);

	ret = 0;
out:
	if (ring_ready)
		io_uring_queue_exit(&ring);
	if (fd >= 0)
		close(fd);
	if (bufs) {
		for (i = 0; i < num_bufs; i++)
			free(bufs[i].ptr);
		free(bufs);
	}

	return ret;
}
