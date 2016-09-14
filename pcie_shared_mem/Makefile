#
# Copyright (C) 2016 NXP Semiconductor
#

apps := pcie_rc pcie_ep

# Accept any target, up to and including none
ifeq ($(MAKECMDGOALS),)
.PHONY: default
default:
else
$(MAKECMDGOALS):
endif
# if SETUP has not been declared on the command-line, it will receive a default
# value in pcie_shmem_common.mk
	$(foreach app, $(apps), make S32V_BOARD_SETUP=$(S32V_BOARD_SETUP) \
		-C $(app) $(MAKECMDGOALS);)
