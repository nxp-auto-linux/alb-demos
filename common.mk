#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

ifeq ($(CC)_$(CROSS_COMPILE),_)
$(warning CROSS_COMPILE is not set!)
endif

ifneq ($(CROSS_COMPILE),)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -Wall

ifneq ($(SYSROOT),)
CFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT)
endif

# Convenience rule to make individual objects
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
