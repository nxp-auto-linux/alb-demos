# SPDX-License-Identifier: BSD-3-Clause

LDFLAGS += -luring
DEPS := cdma_test_common.h
EXE := cdma_epoll_test cdma_iouring_async_test \
	cdma_iouring_pipeline_test cdma_perf_test

OBJ_cdma_epoll_test := cdma_epoll_test.o
OBJ_cdma_iouring_async_test := cdma_iouring_async_test.o
OBJ_cdma_iouring_pipeline_test := cdma_iouring_pipeline_test.o
OBJ_cdma_perf_test := cdma_perf_test.o

include ../common.mk
