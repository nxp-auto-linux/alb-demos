/*
 * Copyright 2018, 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * EndPoint address definitions.
 */

/* FIXME remove hardcoding of addresses */
#define S32V_PCIE_BASE_ADDR	0x72000000

#ifndef EP_BAR
#define EP_BAR 2
#endif

int pcie_init_inbound(unsigned long int ep_local_ddr_addr, int fd);

int pcie_init_outbound(unsigned long long int targ_addr, unsigned int buff_size, int fd);
