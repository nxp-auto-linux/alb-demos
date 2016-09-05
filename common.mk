#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

.PHONY: all install clean

ifeq ($(CC)_$(CROSS_COMPILE),_)
$(warning CROSS_COMPILE is not set!)
endif

ifneq ($(CROSS_COMPILE),)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS += -Wall

ifneq ($(SYSROOT),)
CFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT)
endif

# $(EXE), $(OBJ) and (optionally) $(DEPS) must be defined by the Makefiles
# from the leaves of the dependency tree. Standard variables such as $CFLAGS
# and $(LDFLAGS) can/should also be overridden.

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $(EXE) $^ $(LDFLAGS)

# Convenience rule to make individual objects
%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

# Force default target. Individual Makefiles can override this as needed.
.DEFAULT_GOAL = all
