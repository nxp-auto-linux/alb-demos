// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * NXP S32N79 CDMA - io_uring pipeline test
 *
 * Submits N async transfers back-to-back into io_uring's SQ without
 * waiting between them, then drains all CQEs. Exercises:
 *
 *  - Concurrent SQE submission under io_uring's IOCB_NOWAIT discipline.
 *  - Multiple outstanding async requests on one CDMA BD ring. If the
 *    ring fills, -EAGAIN CQEs are retried for the same iteration.
 *  - Order-independent integrity verification using a transfer ID embedded
 *    in each payload. Every ID must arrive exactly once.
 *  - Future multi-ring deployments can reuse the same -i depth sweep
 *    while distributing requests across rings in the driver.
 *
 * Usage (run a pair, one per board for cross-SoC pipe 10):
 *
 *   # Board A (sender):
 *   ./cdma_iouring_pipeline_test -t /dev/cdma-pipe0-tx -s 4096 -n 64
 *
 *   # Board B (receiver):
 *   ./cdma_iouring_pipeline_test -r /dev/cdma-pipe0-rx -s 4096 -n 64
 *
 * Build (requires liburing-dev):
 *   gcc -O2 -Wall -o cdma_iouring_pipeline_test \
 *       cdma_iouring_pipeline_test.c -luring
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
#include <time.h>
#include <unistd.h>

#include "cdma_test_common.h"

enum mode { MODE_NONE, MODE_TX, MODE_RX };

#define CDMA_PIPELINE_DEFAULT_DEPTH 16
#define CDMA_TRANSFER_ID_SIZE 4

struct iter_buf {
	void *ptr;
	size_t len;
	uint32_t index;
	int in_flight;
	int done;
};

static void fill_transfer_pattern(uint8_t *buf, size_t len, uint32_t id)
{
	size_t i;

	buf[0] = (uint8_t)id;
	buf[1] = (uint8_t)(id >> 8);
	buf[2] = (uint8_t)(id >> 16);
	buf[3] = (uint8_t)(id >> 24);

	for (i = CDMA_TRANSFER_ID_SIZE; i < len; i++)
		buf[i] = (uint8_t)(id + i);
}

static uint32_t get_transfer_id(const uint8_t *buf)
{
	return (uint32_t)buf[0] |
	       (uint32_t)buf[1] << 8 |
	       (uint32_t)buf[2] << 16 |
	       (uint32_t)buf[3] << 24;
}

static int verify_transfer_pattern(const uint8_t *buf, size_t len, int count,
				   uint8_t *seen)
{
	uint32_t id = get_transfer_id(buf);
	size_t i;

	if (id >= (uint32_t)count) {
		fprintf(stderr, "verify: transfer ID %u out of range [0, %d)\n",
			id, count);
		return -1;
	}

	for (i = CDMA_TRANSFER_ID_SIZE; i < len; i++) {
		if (buf[i] != (uint8_t)(id + i)) {
			fprintf(stderr,
				"verify: ID %u byte %zu got %02x expected %02x\n",
				id, i, buf[i], (uint8_t)(id + i));
			return -1;
		}
	}

	if (seen[id]) {
		fprintf(stderr, "verify: duplicate transfer ID %u\n", id);
		return -1;
	}

	seen[id] = 1;
	return 0;
}

static void usage(const char *p)
{
	fprintf(stderr,
		"usage: %s {-t|-r} DEVPATH [-s SIZE] [-n COUNT] [-d DEPTH] [-i INFLIGHT]\n"
		"  -t DEVPATH   send mode\n"
		"  -r DEVPATH   receive mode\n"
		"  -s SIZE      transfer size, bytes or K/M/G suffix (default 4096)\n"
		"  -n COUNT     total iterations (default 64)\n"
		"  -d DEPTH     io_uring SQ depth (default %d)\n"
		"  -i INFLIGHT  max submitted CDMA ops at once (default 16)\n",
		p, CDMA_PIPELINE_DEFAULT_DEPTH);
}

static int find_next_iter(struct iter_buf *iters, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (!iters[i].done && !iters[i].in_flight)
			return i;
	}

	return -1;
}

int main(int argc, char **argv)
{
	int count = 64, depth = CDMA_PIPELINE_DEFAULT_DEPTH, max_inflight = 16;
	int opt, completed = 0, in_flight = 0, errors = 0, eagains = 0;
	size_t pagesz = sysconf(_SC_PAGESIZE);
	int ring_ready = 0, fd = -1, ret = 1;
	struct iter_buf *iters = NULL;
	const char *devpath = NULL;
	enum mode mode = MODE_NONE;
	struct timespec t0, t1;
	struct io_uring ring;
	uint8_t *seen = NULL;
	size_t xfer = 4096;
	double us, mbps;

	while ((opt = getopt(argc, argv, "t:r:s:n:d:i:h")) != -1) {
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
		case 'd':
			depth = atoi(optarg);
			break;
		case 'i':
			max_inflight = atoi(optarg);
			break;
		case 'h':
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 1;
		}
	}

	if (mode == MODE_NONE || !devpath || xfer < CDMA_TRANSFER_ID_SIZE ||
	    count <= 0 || depth <= 0 ||
	    max_inflight <= 0 || max_inflight > depth) {
		usage(argv[0]);
		return 1;
	}

	iters = calloc(count, sizeof(*iters));
	if (!iters) {
		perror("calloc");
		return 1;
	}
	if (mode == MODE_RX) {
		seen = calloc(count, sizeof(*seen));
		if (!seen) {
			perror("calloc");
			goto out;
		}
	}

	for (int i = 0; i < count; i++) {
		if (posix_memalign(&iters[i].ptr, pagesz, xfer)) {
			perror("posix_memalign");
			goto out;
		}
		iters[i].len = xfer;
		iters[i].index = (uint32_t)i;
		if (mode == MODE_TX)
			fill_transfer_pattern(iters[i].ptr, xfer, iters[i].index);
		else
			memset(iters[i].ptr, 0, xfer);
	}

	fd = open(devpath, mode == MODE_TX ? O_WRONLY : O_RDONLY);
	if (fd < 0) {
		perror(devpath);
		goto out;
	}

	if (io_uring_queue_init(depth, &ring, 0) < 0) {
		perror("io_uring_queue_init");
		goto out;
	}
	ring_ready = 1;

	clock_gettime(CLOCK_MONOTONIC, &t0);

	/* Keep the SQ topped up until all N iterations are completed. */
	while (completed < count) {
		struct io_uring_cqe *cqe;
		int to_submit = 0;

		while (in_flight < max_inflight) {
			struct io_uring_sqe *sqe;
			int idx = find_next_iter(iters, count);

			if (idx < 0)
				break;

			sqe = io_uring_get_sqe(&ring);
			if (!sqe)
				break;	/* SQ full, drain CQEs */

			if (mode == MODE_TX)
				io_uring_prep_write(sqe, fd,
						    iters[idx].ptr,
						    iters[idx].len, 0);
			else
				io_uring_prep_read(sqe, fd,
						   iters[idx].ptr,
						   iters[idx].len, 0);

			io_uring_sqe_set_data(sqe, &iters[idx]);
			iters[idx].in_flight = 1;
			in_flight++;
			to_submit++;
		}

		if (to_submit) {
			int s = io_uring_submit(&ring);

			if (s < 0) {
				fprintf(stderr, "submit: %s\n", strerror(-s));
				ret = 1;
				goto out;
			}
			if (s != to_submit) {
				fprintf(stderr, "short io_uring submit: %d/%d\n",
					s, to_submit);
				ret = 1;
				goto out;
			}
		}

		ret = io_uring_wait_cqe(&ring, &cqe);
		if (ret < 0) {
			fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
			goto out;
		}

		struct iter_buf *it = io_uring_cqe_get_data(cqe);

		it->in_flight = 0;
		in_flight--;

		if (cqe->res == -EAGAIN) {
			eagains++;
		} else if (cqe->res < 0) {
			fprintf(stderr,
				"request %u: %s\n",
				it->index, strerror(-cqe->res));
			errors++;
			it->done = 1;
			completed++;
		} else if ((size_t)cqe->res != it->len) {
			fprintf(stderr,
				"request %u: short %d/%zu\n",
				it->index, cqe->res, it->len);
			errors++;
			it->done = 1;
			completed++;
		} else if (mode == MODE_RX &&
			   verify_transfer_pattern(it->ptr, it->len,
						   count, seen)) {
			errors++;
			it->done = 1;
			completed++;
		} else {
			it->done = 1;
			completed++;
		}

		io_uring_cqe_seen(&ring, cqe);
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);
	if (mode == MODE_RX) {
		int missing = 0;

		for (int i = 0; i < count; i++) {
			if (!seen[i]) {
				if (missing < 16)
					fprintf(stderr,
						"verify: missing transfer ID %d\n", i);
				missing++;
			}
		}
		if (missing > 16)
			fprintf(stderr, "verify: %d additional transfer IDs missing\n",
				missing - 16);
		errors += missing;
	}

	us = cdma_ts_diff_us(&t0, &t1);
	mbps = (double)count * xfer / us;

	printf("%s pipeline: %d x %zu B (depth=%d, inflight=%d) in %.1f ms  "
		   "(%.1f MB/s) errors=%d eagains=%d\n",
		   mode == MODE_TX ? "TX" : "RX",
		   count, xfer, depth, max_inflight, us / 1000.0, mbps,
		   errors, eagains);

	ret = errors ? 1 : 0;
out:
	if (fd >= 0)
		close(fd);
	if (iters) {
		for (int i = 0; i < count; i++)
			free(iters[i].ptr);
		free(iters);
	}
	free(seen);
	if (ring_ready)
		io_uring_queue_exit(&ring);

	return ret;
}
