#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

LDFLAGS ?= -lpthread
include ../common.mk

DEPS   := functions.h
OBJ    := multicore.o functions.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
multicore: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) multicore
