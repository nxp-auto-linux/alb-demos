#
# Copyright 2017-2018 NXP
#
# SPDX-License-Identifier:     GPL-2.0+
#

CFLAGS	+= -g -O3 -I../../pcie_common/include

LDFLAGS	+= -lpthread
OBJ	:= ../../pcie_common/pcie_rc_addr.o net_rc.o
EXE	:= net_rc

# this one should be last on the include list
include ../../common.mk
