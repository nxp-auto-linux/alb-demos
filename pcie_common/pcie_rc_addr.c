/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * PCIe HandShake between EndPoint and RootComplex,
 * RootComplex side.
 */

#include "include/pcie_rc_addr.h"
#include "include/pcie_handshake.h"

#include <unistd.h>

int pcie_notify_ep(struct s32v_handshake *phandshake)
{
	int ret = 0; /* -1 if something is invalid; 0 if all done; 1 if already set */
	if (!phandshake)
		return -1;

	if (phandshake->rc_ddr_addr == RC_DDR_ADDR)
		return 1; /* already set */

	while (phandshake->rc_ddr_addr != UNDEFINED_DATA)
        usleep(DEFAULT_TIMEOUT_US);

    phandshake->rc_ddr_addr = RC_DDR_ADDR;
	return 0;
}
