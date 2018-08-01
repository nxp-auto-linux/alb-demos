#
# Copyright 2016-2018 NXP
#
include ../../pcie_common/pcie_shmem_common.mk

CFLAGS	+= -D$(S32V_BOARD_SETUP)
LDFLAGS	+= -lpthread
OBJ	:= test_rc.o
EXE	:= test_rc

# this one should be last on the include list
include ../../common.mk
