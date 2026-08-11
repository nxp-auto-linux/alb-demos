// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * NXP S32N79 CDMA - epoll readiness test
 *
 * Verifies the cdev's .poll handler signals EPOLLOUT/EPOLLIN correctly
 * across the async-submit/complete cycle.
 *
 * Test sequence / Implementation:
 *
 *   1. Open /dev/cdma-pipeN-{tx,rx} and add it to an epoll set.
 *   2. epoll_wait(0) — the ring is idle, so the fd must report ready
 *      immediately (EPOLLOUT for TX, EPOLLIN for RX).
 *   3. Submit one async io_uring transfer.
 *   4. epoll_wait(short timeout) — with multi-outstanding support the
 *      fd may remain ready if the ring has room for another request.
 *   5. Wait for the cqe. The work queue unmaps the request and wakes
 *      the wait queue.
 *   6. epoll_wait(0) — the fd must report ready again.
 *
 * Usage (run on a board that has the chosen endpoint of the pipe):
 *
 *   ./cdma_epoll_test -t /dev/cdma-pipe0-tx
 *   ./cdma_epoll_test -r /dev/cdma-pipe0-rx
 *
 * Build (requires liburing-dev):
 *   gcc -O2 -Wall -o cdma_epoll_test \
 *       cdma_epoll_test.c -luring
 *
 * Copyright 2026 NXP
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cdma_test_common.h"

#define CDMA_EPOLL_QUEUE_DEPTH 4

enum mode { MODE_NONE, MODE_TX, MODE_RX };

static const char *mode_name(enum mode m)
{
	return m == MODE_TX ? "TX" : "RX";
}

static uint32_t expected_event(enum mode m)
{
	return m == MODE_TX ? EPOLLOUT : EPOLLIN;
}

static int check_ready(int ep, int timeout_ms, enum mode m, int want_ready,
		       const char *step)
{
	struct epoll_event ev = {};
	int n;

	n = epoll_wait(ep, &ev, 1, timeout_ms);
	if (n < 0) {
		fprintf(stderr, "%s: epoll_wait: %s\n", step, strerror(errno));
		return -1;
	}

	if (want_ready) {
		if (n != 1 || !(ev.events & expected_event(m))) {
			fprintf(stderr,
				"%s: expected ready (%s), got n=%d events=0x%x\n",
				step, m == MODE_TX ? "EPOLLOUT" : "EPOLLIN",
				n, n ? ev.events : 0);
			return -1;
		}
	} else {
		if (n != 0) {
			fprintf(stderr,
				"%s: expected NOT ready, got events=0x%x\n",
				step, ev.events);
			return -1;
		}
	}
	return 0;
}

static int wait_ready(int ep, int timeout_ms, enum mode m, const char *step)
{
	struct epoll_event ev = {};
	int n;

	n = epoll_wait(ep, &ev, 1, timeout_ms);
	if (n < 0) {
		fprintf(stderr, "%s: epoll_wait: %s\n", step, strerror(errno));
		return -1;
	}
	if (n > 0 && !(ev.events & expected_event(m))) {
		fprintf(stderr,
			"%s: unexpected events=0x%x\n", step, ev.events);
		return -1;
	}

	return n;
}

static void usage(const char *p)
{
	fprintf(stderr,
		"usage: %s {-t|-r} DEVPATH [-s SIZE]\n"
		"  -t DEVPATH   TX device\n"
		"  -r DEVPATH   RX device\n"
		"  -s SIZE      transfer size, bytes or K/M/G suffix (default 4096)\n",
		p);
}

int main(int argc, char **argv)
{
	int opt, uring_ready = 0, ready;
	int fd = -1, ep = -1, ret = 1;
	enum mode mode = MODE_NONE;
	struct epoll_event ev = {};
	const char *devpath = NULL;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	struct io_uring ring;
	size_t xfer = 4096;
	void *buf = NULL;

	while ((opt = getopt(argc, argv, "t:r:s:h")) != -1) {
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
		case 'h':
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 1;
		}
	}
	if (mode == MODE_NONE || !devpath) {
		usage(argv[0]);
		return 1;
	}

	if (posix_memalign(&buf, sysconf(_SC_PAGESIZE), xfer)) {
		perror("posix_memalign");
		return 1;
	}
	memset(buf, 0xaa, xfer);

	fd = open(devpath, mode == MODE_TX ? O_WRONLY : O_RDONLY);
	if (fd < 0) {
		perror(devpath);
		goto out;
	}

	ep = epoll_create1(EPOLL_CLOEXEC);
	if (ep < 0) {
		perror("epoll_create1");
		goto out;
	}

	ev.events = expected_event(mode);
	ev.data.fd = fd;
	if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev) < 0) {
		perror("epoll_ctl");
		goto out;
	}

	if (io_uring_queue_init(CDMA_EPOLL_QUEUE_DEPTH, &ring, 0) < 0) {
		perror("io_uring_queue_init");
		goto out;
	}
	uring_ready = 1;

	/* Step 1: idle ring should report ready immediately */
	if (check_ready(ep, 0, mode, 1, "step1 (idle)"))
		goto out;
	printf("step1 (idle): ready as expected\n");

	/* Step 2: submit one async transfer */
	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "step2: get_sqe failed\n");
		goto out;
	}

	if (mode == MODE_TX)
		io_uring_prep_write(sqe, fd, buf, xfer, 0);
	else
		io_uring_prep_read(sqe, fd, buf, xfer, 0);

	if (io_uring_submit(&ring) < 0) {
		perror("submit");
		goto out;
	}

	/* Step 3: while one request is in flight, multi-outstanding kernels
	 * may still report ready because the ring has room for another async
	 * request. One-deep kernels report not-ready until the cqe is drained.
	 */
	ready = wait_ready(ep, 5, mode, "step3 (in flight)");
	if (ready < 0)
		goto out;
	if (ready)
		printf("step3 (in flight): ready for another request\n");
	else
		printf("step3 (in flight): no additional async slot available\n");

	/* Step 4: drain the cqe (the transfer will succeed if a peer is
	 * present)
	 */
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
		goto out;
	}
	printf("step4 (cqe drained): res=%d (%s)\n",
	       cqe->res,
	       cqe->res < 0 ? strerror(-cqe->res)
			    : "ok / peer matched");
	io_uring_cqe_seen(&ring, cqe);

	/* Step 5: slot freed, must be ready again */
	if (check_ready(ep, 0, mode, 1, "step5 (idle again)")) {
		ret = 1;
		goto out;
	}
	printf("step5 (idle again): ready as expected\n");

	printf("%s: epoll readiness test PASS\n", mode_name(mode));
	ret = 0;
out:
	if (uring_ready)
		io_uring_queue_exit(&ring);
	if (ep >= 0)
		close(ep);
	if (fd >= 0)
		close(fd);
	free(buf);

	return ret;
}
