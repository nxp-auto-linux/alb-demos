#
# Copyright 2017-2018 NXP
#
# SPDX-License-Identifier:     GPL-2.0+
#

include ../../pcie_common/pcie_shmem_common.mk

CFLAGS	+= -g -O3
CFLAGS	+= -D$(S32V_BOARD_SETUP)
LDFLAGS	+= -lpthread
OBJ	:= net_rc.o
EXE	:= net_rc

# this one should be last on the include list
include ../../common.mk
