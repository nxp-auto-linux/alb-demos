.. SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

NXP CDMA tests
==============

cdma_perf_test
--------------

Zero-source/discard-sink performance benchmark for comparing the old
synchronous userspace path with the io_uring path. Unlike the
integrity tests, it does not generate or verify patterns in the
timed loop. TX writes pre-zeroed aligned buffers to the CDMA pipe; RX
reads into aligned buffers and immediately reuses them, discarding the
data. This is the C equivalent of a ``/dev/zero`` to CDMA TX and CDMA
RX to ``/dev/null`` test, without filesystem I/O in the measured path.

Build (requires liburing-dev)::

    gcc -O2 -Wall -o cdma_perf_test cdma_perf_test.c -luring

Run sync baseline, receiver first::

    # Board B — receiver:
    ./cdma_perf_test -r /dev/cdma-pipe0-rx -a sync -s 256K -n 4096 -w 16

    # Board A — sender:
    ./cdma_perf_test -t /dev/cdma-pipe0-tx -a sync -s 256K -n 4096 -w 16

Run io_uring, receiver first::

    # Board B — receiver:
    ./cdma_perf_test -r /dev/cdma-pipe0-rx -a uring -s 256K -n 4096 -d 16 -i 16 -w 16

    # Board A — sender:
    ./cdma_perf_test -t /dev/cdma-pipe0-tx -a uring -s 256K -n 4096 -d 16 -i 16 -w 16

CDMA supports multiple outstanding async requests per TX/RX ring.
Use ``-i`` to sweep in-flight depth, for example ``-i 1``, ``-i 2``,
``-i 4``, ``-i 8``, and ``-i 16``. Future multi-ring support can use
the same userspace depth while the driver distributes requests across
multiple rings.

Options::

    -t DEVPATH    TX mode, write zero buffers to CDMA TX device
    -r DEVPATH    RX mode, read CDMA RX device and discard data
    -a API        sync or uring (default sync)
    -s SIZE       transfer size, bytes or K/M/G suffix (default 256K)
    -n COUNT      timed transfer count (default 4096)
    -d DEPTH      io_uring queue depth (default 16, uring only)
    -i INFLIGHT   max submitted CDMA ops at once (default 16)
    -w WARMUP     untimed warmup transfer count (default 16)

cdma_iouring_async_test
-----------------------

CDMA io_uring async-path exerciser. Runs as either the sender
(TX) or the receiver (RX) of a CDMA pipe and submits N asynchronous
transfers of a fixed size, verifying data integrity with a per-
iteration pattern seed.

Build (requires liburing-dev)::

    gcc -O2 -Wall -o cdma_iouring_async_test \
        cdma_iouring_async_test.c -luring

Run pairs of instances on the two endpoints of a pipe.

Cross-SoC pipe 10 (S32G #1 / S32G #2 paired through S32N79)::

    # Board A — sender (SI 4 side on S32N79):
    ./cdma_iouring_async_test -t /dev/cdma-pipe0-tx -s 4096 -n 1000

    # Board B — receiver (SI 21 side on S32N79):
    ./cdma_iouring_async_test -r /dev/cdma-pipe0-rx -s 4096 -n 1000

Intra-SoC pipe (both endpoints on the same Linux instance, e.g. on
S32N79 with the appropriate FSS-authorised SI pair)::

    # Two terminals on the same board:
    ./cdma_iouring_async_test -t /dev/cdma-pipe0-tx -s 4096 -n 1000 &
    ./cdma_iouring_async_test -r /dev/cdma-pipe1-rx -s 4096 -n 1000

The two sides print their measured wall-clock time and effective
throughput at the end. Errors are reported on stderr with the kernel
errno that came back through io_uring's cqe->res.

Options::

    -t DEVPATH    send mode, opens DEVPATH for write
    -r DEVPATH    receive mode, opens DEVPATH for read
    -s SIZE       transfer size, bytes or K/M/G suffix (default 4096)
    -n COUNT      number of iterations (default 100)

cdma_iouring_pipeline_test
--------------------------

Submits async transfers into io_uring's SQ and drains CQEs while
verifying that every transfer arrives exactly once with an intact payload.
Each payload contains a 32-bit transfer ID, so completion and pipe matching
may occur out of order without producing false integrity errors. The ``-i``
option controls how many CDMA operations may be submitted at once.
For the current CDMA driver, ``-i`` maps to multiple outstanding
requests on one BD ring. Future multi-ring support can reuse the same
test while the driver distributes requests across rings. ``-EAGAIN``
CQEs are retried by the test and reported in the final ``eagains=``
counter.

Build::

    gcc -O2 -Wall -o cdma_iouring_pipeline_test \
        cdma_iouring_pipeline_test.c -luring

Run (paired with the other endpoint on the partner board)::

    # Board A — sender:
    ./cdma_iouring_pipeline_test -t /dev/cdma-pipe0-tx -s 4096 -n 256 -d 16 -i 16

    # Board B — receiver:
    ./cdma_iouring_pipeline_test -r /dev/cdma-pipe0-rx -s 4096 -n 256 -d 16 -i 16

Options::

    -t DEVPATH    TX device
    -r DEVPATH    RX device
    -s SIZE       transfer size, bytes or K/M/G suffix (default 4096)
    -n COUNT      total iterations (default 64)
    -d DEPTH      io_uring SQ depth (default 16)
    -i INFLIGHT   max submitted CDMA ops at once (default 16)

cdma_epoll_test
---------------

Verifies the cdev's .poll handler signals EPOLLOUT/EPOLLIN correctly
across the async-submit/complete cycle.

Test steps (printed as it runs)::

    step1: idle ring -> ready (immediate)
    step2: submit one async transfer
    step3: in flight -> ready if another async slot is available
    step4: drain cqe
    step5: slot freed -> ready again

Build::

    gcc -O2 -Wall -o cdma_epoll_test \
        cdma_epoll_test.c -luring

Run::

    ./cdma_epoll_test -t /dev/cdma-pipe0-tx
    ./cdma_epoll_test -r /dev/cdma-pipe0-rx

Options::

    -t DEVPATH    TX device
    -r DEVPATH    RX device
    -s SIZE       transfer size, bytes or K/M/G suffix (default 4096)
