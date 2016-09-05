#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

LDFLAGS += -lpthread
include ../common.mk

DEPS   := functions.h
OBJ    := multicore.o functions.o

multicore: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) multicore

install:multicore
	if [ -f $(INSTALLDIR) ]; then rm -rf $(INSTALLDIR); fi
	mkdir -p $(INSTALLDIR)
	install multicore $(INSTALLDIR)
	install *.in $(INSTALLDIR)

