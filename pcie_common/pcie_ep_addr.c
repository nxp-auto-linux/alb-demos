/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * PCIe HandShake between EndPoint and RootComplex,
 * EndPoint side.
 */

#include "include/pcie_ep_addr.h"
#include "include/pcie_handshake.h"

#include <unistd.h>

unsigned long long int pcie_wait_for_rc(struct s32v_handshake *phandshake)
{
	if (!phandshake)
		return UNDEFINED_DATA;

    phandshake->rc_ddr_addr = UNDEFINED_DATA;
	while (phandshake->rc_ddr_addr == UNDEFINED_DATA)
        usleep(DEFAULT_TIMEOUT_US);

    return phandshake->rc_ddr_addr;
}
