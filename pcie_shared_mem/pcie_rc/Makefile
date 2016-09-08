#
# Copyright (C) 2016 NXP Semiconductor
#

ifeq ($(SETUP), PCIE_SHMEM_BLUEBOX)
	CFLAGS += -DPCIE_SHMEM_BLUEBOX
else
	CFLAGS += -DPCIE_SHMEM_EVB
endif

LDFLAGS	+= -lpthread
OBJ	:= test_rc.o
EXE	:= test_rc

include ../../common.mk
