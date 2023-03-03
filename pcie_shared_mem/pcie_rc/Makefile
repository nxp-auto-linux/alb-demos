#
# Copyright 2016, 2018 NXP
#

LDFLAGS	+= -lpthread

CFLAGS	+= -g -I../../pcie_common/include

OBJ	:= ../../pcie_common/pcie_rc_addr.o ../../pcie_common/pcie_benchmark.o \
		../../pcie_common/pcie_common.o test_rc.o
EXE	:= test_rc

# this one should be last on the include list
include ../../common.mk
