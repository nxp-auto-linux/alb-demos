#
# Copyright (C) 2016 NXP Semiconductor
#

include ../../common.mk
.PHONY: all install clean

LDFLAGS	+= -lpthread
CFLAGS	+= -g
EXE = test_ep

OBJ	:= test_ep.o
DEPS	:= pcie_ops.h

all: $(OBJ)
	$(CC) -o $(EXE) $^ $(LDFLAGS)

# $(INSTALLDIR) should have been prepared by the parent Makefile; do
# nothing in case it hasn't, to avoid silly errors in case 'make' is
# executed directly from this dirlevel
ifeq ($(INSTALLDIR),)
install:
	$(info >> INSTALLDIR not set, skipping install.)
else
install: all
	install -v ${EXE} ${INSTALLDIR}
endif

clean:
	rm -rf $(OBJ) $(EXE)
