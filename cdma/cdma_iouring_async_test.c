// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * NXP S32N79 CDMA - io_uring async test
 *
 * Exercises the async paths of the cdev (read_iter/write_iter +
 * IOCB_NOWAIT) against the cross-SoC pipe matcher architecture.
 *
 * Usage:
 *   # Sender (board A):
 *   ./cdma_iouring_async_test -t /dev/cdma-pipe0-tx -s 4096 -n 100
 *
 *   # Receiver (board B):
 *   ./cdma_iouring_async_test -r /dev/cdma-pipe0-rx -s 4096 -n 100
 *
 * The two sides do N async iterations of a fixed-size transfer. TX
 * sends a known pattern (per-iter seed); RX verifies the same
 * pattern. Wall-clock time for N iterations is printed at the end.
 *
 * Build:
 *   gcc -O2 -Wall -o cdma_iouring_async_test \
 *       cdma_iouring_async_test.c -luring
 *
 * Copyright 2026 NXP
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <liburing.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "cdma_test_common.h"

#define CDMA_ASYNC_QUEUE_DEPTH 8

enum mode { MODE_NONE, MODE_TX, MODE_RX };

static int verify_pattern(const uint8_t *buf, size_t len, uint32_t seed)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if (buf[i] != (uint8_t)(seed + i)) {
			fprintf(stderr,
				"verify: mismatch at offset %zu: expected %02x got %02x (seed=%u)\n",
				i, (uint8_t)(seed + i), buf[i], seed);
			return -1;
		}
	}

	return 0;
}

static int do_one_iter(struct io_uring *ring, int fd, void *buf, size_t len,
		       enum mode mode)
{
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	int ret;

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "get_sqe failed\n");
		return -1;
	}

	if (mode == MODE_TX)
		io_uring_prep_write(sqe, fd, buf, len, 0);
	else
		io_uring_prep_read(sqe, fd, buf, len, 0);

	ret = io_uring_submit(ring);
	if (ret < 0) {
		fprintf(stderr, "submit: %s\n", strerror(-ret));
		return -1;
	}

	ret = io_uring_wait_cqe(ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
		return -1;
	}

	if (cqe->res < 0) {
		fprintf(stderr, "cqe res: %s\n", strerror(-cqe->res));
		io_uring_cqe_seen(ring, cqe);
		return -1;
	}

	if ((size_t)cqe->res != len) {
		fprintf(stderr, "short transfer: %d < %zu\n", cqe->res, len);
		io_uring_cqe_seen(ring, cqe);
		return -1;
	}

	io_uring_cqe_seen(ring, cqe);

	return 0;
}

static void usage(const char *p)
{
	fprintf(stderr,
		"usage: %s {-t|-r} DEVPATH [-s SIZE] [-n COUNT]\n"
		"  -t DEVPATH   send mode, open DEVPATH as TX device\n"
		"  -r DEVPATH   receive mode, open DEVPATH as RX device\n"
		"  -s SIZE      transfer size, bytes or K/M/G suffix (default 4096)\n"
		"  -n COUNT     number of iterations (default 100)\n",
		p);
}

int main(int argc, char **argv)
{
	int i, opt, ret = 1, ring_ready = 0;
	const char *devpath = NULL;
	enum mode mode = MODE_NONE;
	int count = 100, fd = -1;
	struct timespec t0, t1;
	struct io_uring ring;
	size_t xfer = 4096;
	void *buf = NULL;
	double us, mbps;

	while ((opt = getopt(argc, argv, "t:r:s:n:h")) != -1) {
		switch (opt) {
		case 't':
			mode = MODE_TX;
			devpath = optarg;
			break;
		case 'r':
			mode = MODE_RX;
			devpath = optarg;
			break;
		case 's':
			if (cdma_parse_size(optarg, &xfer)) {
				usage(argv[0]);
				return 1;
			}
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'h':
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 1;
		}
	}
	if (mode == MODE_NONE || !devpath || !xfer || count <= 0) {
		usage(argv[0]);
		return 1;
	}

	if (posix_memalign(&buf, sysconf(_SC_PAGESIZE), xfer)) {
		perror("posix_memalign");
		return 1;
	}

	fd = open(devpath,
		  (mode == MODE_TX ? O_WRONLY : O_RDONLY) | O_NONBLOCK);
	if (fd < 0) {
		perror(devpath);
		goto out;
	}

	if (io_uring_queue_init(CDMA_ASYNC_QUEUE_DEPTH, &ring, 0) < 0) {
		perror("io_uring_queue_init");
		goto out;
	}
	ring_ready = 1;

	clock_gettime(CLOCK_MONOTONIC, &t0);

	for (i = 0; i < count; i++) {
		uint32_t seed = (uint32_t)i;

		if (mode == MODE_TX)
			cdma_fill_pattern(buf, xfer, seed);
		else
			memset(buf, 0, xfer);

		if (do_one_iter(&ring, fd, buf, xfer, mode))
			goto out;

		if (mode == MODE_RX && verify_pattern(buf, xfer, seed))
			goto out;
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);

	us = cdma_ts_diff_us(&t0, &t1);
	mbps = (double)count * xfer / us;

	printf("%s: %d x %zu B in %.1f ms  (%.1f MB/s)\n",
		   mode == MODE_TX ? "TX" : "RX",
		   count, xfer, us / 1000.0, mbps);

	ret = 0;
out:
	if (fd >= 0)
		close(fd);
	if (buf)
		free(buf);
	if (ring_ready)
		io_uring_queue_exit(&ring);

	return ret;
}
