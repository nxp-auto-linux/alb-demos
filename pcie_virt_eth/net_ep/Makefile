#
# Copyright 2017-2018 NXP
#
# SPDX-License-Identifier:     GPL-2.0+
#

# LDFLAGS	+= -lpthread

CFLAGS	+= -g -O3

OBJ	+= ../../pcie_common/pcie_ep_addr.o ../../pcie_common/pcie_ops.o net_ep.o
DEPS	:= pcie_ops.h pcie_ep_addr.h
EXE	:= net_ep

# this one should be last on the include list
include ../../common.mk
