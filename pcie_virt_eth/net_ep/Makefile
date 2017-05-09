#
# Copyright 2017 NXP
#
# SPDX-License-Identifier:     GPL-2.0+
#

include ../pcie_shmem_common.mk

# LDFLAGS	+= -lpthread

CFLAGS	+= -g -O3
# $(SETUP) should have been properly set by the parent Makefile
CFLAGS	+= -D$(S32V_BOARD_SETUP)

OBJ	+= net_ep.o
DEPS	:= pcie_ops.h
EXE	:= net_ep

# this one should be last on the include list
include ../../common.mk
