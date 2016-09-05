#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

LDFLAGS += -lpthread
include ../common.mk

# Temporarily override the target order from common.mk. This will be removed
# after we factor out the 'clean' and 'install' targets.
.DEFAULT_GOAL = multicore

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

