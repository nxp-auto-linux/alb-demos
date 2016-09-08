#
# Copyright (C) 2016 NXP Semiconductor
#

apps := pcie_rc pcie_ep

#SETUP=PCIE_SHMEM_BLUEBOX
SETUP=PCIE_SHMEM_EVB

# Accept any target, up to and including none
ifeq ($(MAKECMDGOALS),)
.PHONY: default
default:
else
$(MAKECMDGOALS):
endif
	$(foreach app, $(apps), SETUP=$(SETUP) make -C $(app) $(MAKECMDGOALS);)
