#
# Copyright 2016-2017 NXP
#
# SPDX-License-Identifier:     GPL-2.0+
#

apps := net_rc net_ep

# Accept any target, up to and including none
ifeq ($(MAKECMDGOALS),)
.PHONY: default
default:
else
$(MAKECMDGOALS):
endif
# if SETUP has not been declared on the command-line, it will receive a default
# value in pcie_shmem_common.mk
	$(foreach app, $(apps), make -C $(app) $(MAKECMDGOALS);)
