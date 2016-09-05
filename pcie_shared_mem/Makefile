#
# Copyright (C) 2016 NXP Semiconductor
#

apps := pcie_rc pcie_ep
.PHONY: all install clean

all:
	$(foreach app, $(apps), make -C $(app);)

ifeq ($(INSTALLDIR),)
install:
	$(info >>> INSTALLDIR not set, skipping install.)
else
install: all
# if $INSTALLDIR is a file, remove it; otherwise, we'll just add to a pre-existing folder
	if [ -f ${INSTALLDIR} ]; then rm -f ${INSTALLDIR}; fi
	mkdir -p ${INSTALLDIR}
	$(foreach app, $(apps), make INSTALLDIR=$(INSTALLDIR) -C $(app) install;)
endif

clean:
	$(foreach app, $(apps), make -C $(app) clean;)
