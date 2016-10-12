#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#
# Convenience definitions of variables and targets for the make subsystems.
# This file also defines environment variables for Yocto.
#
# The following variables must be defined by the Makefiles in the leaves of the
# dependency tree:
#  - $(EXE)		- Name of the final executable
#  - $(OBJ)		- List of object files
#  - $(DEPS)		- Optional, list of additional build dependencies
#  - $(INSTALLEXTRA)	- Optional, files to be installed in addition to $(EXE)
#  - $(CLEANEXTRA)	- Optional, files to be cleaned in addition to $(EXE) and $(OBJ)
#
# Standard variables such as $(CFLAGS) and $(LDFLAGS) can also be overridden.
#
# Makefiles that need these definitions should include this file
# *on the last line*, to make sure all the locally-defined variables are
# properly expanded.

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

ifeq ($(words $(EXE)),1)
ifeq ($(OBJ_$(EXE)),)
OBJ_$(EXE) = OBJ
endif
else


# initalizing OBJ
OBJ :=

define objadd
# when multiple EXE files exist, there must be multiple OBJ_*
OBJ += $$(OBJ_$(1))
# creating dep rules for each exe in $(EXE)
$(1): $$(OBJ_$(1))
endef

$(foreach exe,$(EXE),$(eval $(call objadd,$(exe))))

endif

all: $(EXE)

$(EXE): $($(OBJ_$(EXE)))
	$(CC) -o $@ $^ $(LDFLAGS)

# Convenience rule to make individual objects
%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

# $(INSTALLDIR) should have been prepared by the parent Makefile; do nothing
# in case it hasn't, to avoid silly errors in case 'make' is executed directly
# from the lower-dir level.
ifeq ($(INSTALLDIR),)
install:
	$(info >> INSTALLDIR not set, skipping install.)
else
install: all
# Livin' on the edge: if $(INSTALLDIR) is a file, remove it; otherwise,
# we'll just add to a pre-existing folder.
	if [ -f $(INSTALLDIR) ]; then rm -f $(INSTALLDIR); fi
	mkdir -p $(INSTALLDIR)
	install -v $(EXE) $(INSTALLEXTRA) $(INSTALLDIR)
endif

clean:
	rm -rf $(OBJ) $(EXE) $(CLEANEXTRA)

# Force default target. Individual Makefiles can override this as needed.
.DEFAULT_GOAL = all
