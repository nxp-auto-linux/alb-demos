#
# Copyright (C) 2016 NXP Semiconductor
#

# Standard variables (defined or overridden from common.mk)
LDFLAGS	+= -lpthread
# Define or override global variables from common.mk
OBJ	:= test_rc.o
EXE	:= test_rc

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

# Common variables and rule definitions
include ../../common.mk
