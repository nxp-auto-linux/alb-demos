#
# Copyright 2016-2018 NXP
#

LDFLAGS	+= -lpthread

CFLAGS	+= -g

OBJ	+= ../../pcie_common/pcie_ep_addr.o ../../pcie_common/pcie_ops.o test_ep.o
DEPS	:= pcie_ops.h pcie_ep_addr.h pcie_handshake.h
EXE	:= test_ep

# this one should be last on the include list
include ../../common.mk
