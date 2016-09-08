#
# Copyright (C) 2016 NXP Semiconductor
#

LDFLAGS	+= -lpthread

CFLAGS	+= -g
ifeq ($(SETUP), PCIE_SHMEM_BLUEBOX)
	CFLAGS += -DPCIE_SHMEM_BLUEBOX
else
	CFLAGS += -DPCIE_SHMEM_EVB
endif

OBJ	+= test_ep.o
DEPS	:= pcie_ops.h
EXE	:= test_ep

include ../../common.mk
