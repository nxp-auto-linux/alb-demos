/*
 * Copyright 2018, 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * EndPoint address definitions.
 */

#ifndef EP_NUM_BARS
/* TODO: validate inside the driver */
#define EP_NUM_BARS 6
#endif

int pcie_init_inbound(unsigned long int ep_local_ddr_addr,
	unsigned int bar_number, int fd);

int pcie_init_outbound(unsigned long long int base_address,
	unsigned long long int targ_addr,
	unsigned int buff_size, int fd);
