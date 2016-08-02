#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

ifeq ($(CC)_$(CROSS_COMPILE),_)
$(warning CROSS_COMPILE is not set!)
endif

# built in GNU Make variable CC=cc makes ?= not work
# but 'cc' will map to gcc on host native build
# on cross compile, CROSS_COMPILE is set and we override CC
# In Yocto CC is set, but CROSS_COMPILE is not, so all is ok
ifneq ($(CROSS_COMPILE),)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -Wall
LDFLAGS ?= -lpthread
ifneq ($(SYSROOT),)
CFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT)
endif

DEPS   := functions.h
OBJ    := multicore.o functions.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
multicore: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) multicore
