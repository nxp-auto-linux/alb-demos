#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     GPL-2.0+
#
# Common definitions to the PCIe SHMEM demos

# In case S32V_BOARD_SETUP was not passed on the command-line or it has
# an invalid value, choose the Bluebox as a sane default.
ifeq (, $(filter $(S32V_BOARD_SETUP), PCIE_SHMEM_EVB PCIE_SHMEM_BLUEBOX))
ifneq (, $(S32V_BOARD_SETUP))
$(warning Invalid board setup: $(S32V_BOARD_SETUP); \
	supported values: PCIE_SHMEM_BLUEBOX (default), PCIE_SHMEM_EVB)
endif
	# Must use the override directive, because variable re-assignment
	# will not take place in the case of command-line variables.
	override S32V_BOARD_SETUP = PCIE_SHMEM_BLUEBOX
endif
