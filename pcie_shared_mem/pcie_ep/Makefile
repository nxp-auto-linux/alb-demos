#
# Copyright 2016, 2021 NXP
#

LDFLAGS	+= -lpthread

CFLAGS	+= -g -I../../pcie_common/include

OBJ	+= ../../pcie_common/pcie_ep_addr.o ../../pcie_common/pcie_benchmark.o \
		test_ep.o
DEPS	:= pcie_ops.h pcie_ep_addr.h pcie_handshake.h
EXE	:= test_ep

# this one should be last on the include list
include ../../common.mk
