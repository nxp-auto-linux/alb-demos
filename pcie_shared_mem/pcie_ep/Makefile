#
# Copyright 2016-2018 NXP
#
include ../../pcie_common/pcie_shmem_common.mk

LDFLAGS	+= -lpthread

CFLAGS	+= -g
# $(SETUP) should have been properly set by the parent Makefile
CFLAGS	+= -D$(S32V_BOARD_SETUP)

OBJ	+= test_ep.o
DEPS	:= pcie_ops.h
EXE	:= test_ep

# this one should be last on the include list
include ../../common.mk
