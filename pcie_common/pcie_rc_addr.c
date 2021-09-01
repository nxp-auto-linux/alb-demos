/*
 * Copyright 2018-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * PCIe HandShake between EndPoint and RootComplex,
 * RootComplex side.
 */

#include "include/pcie_handshake.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

unsigned long int ep_bar2_addr = 0;
unsigned long int rc_local_ddr_addr = 0;

int pcie_notify_ep(struct s32v_handshake *phandshake)
{
	if (!phandshake)
		return -1;

	if (phandshake->rc_ddr_addr == rc_local_ddr_addr)
		return 1; /* already set */

	while (phandshake->rc_ddr_addr != UNDEFINED_DATA)
        usleep(DEFAULT_TIMEOUT_US);

    phandshake->rc_ddr_addr = rc_local_ddr_addr;
	return 0;
}

int pcie_parse_command_arguments(int argc, char *argv[])
{
	char *ep_bar2_addr_str = NULL;
	char *rc_local_ddr_addr_str = NULL;
	int c;

	while ((c = getopt (argc, argv, "a:e:")) != -1)
		switch (c) {
		  case 'a':
			rc_local_ddr_addr_str = optarg;
			rc_local_ddr_addr = strtoul(rc_local_ddr_addr_str, NULL, 16);
			break;
		  case 'e':
			ep_bar2_addr_str = optarg;
			ep_bar2_addr = strtoul(ep_bar2_addr_str, NULL, 16);
			break;
		}

	if (ep_bar2_addr && rc_local_ddr_addr)
		return 0;

	return 1;
}
