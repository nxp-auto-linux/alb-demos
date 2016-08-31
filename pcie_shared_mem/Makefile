#
# Copyright (C) 2016 NXP Semiconductor
#

apps := pcie_rc pcie_ep

default:
	$(foreach app, $(apps), make -C $(app);)
clean:
	$(foreach app, $(apps), make -C $(app) clean;)
