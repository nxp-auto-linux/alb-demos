/*
 * Copyright 2018-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * PCIe HandShake between EndPoint and RootComplex,
 * EndPoint side.
 */

#include "include/pcie_ep_addr.h"
#include "include/pcie_handshake.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

unsigned long int ep_bar2_addr = 0;
unsigned long int ep_local_ddr_addr = 0;
char batch_commands[MAX_BATCH_COMMANDS + 1] = {0,};

unsigned long long int pcie_wait_for_rc(struct s32v_handshake *phandshake)
{
	if (!phandshake)
		return UNDEFINED_DATA;

    phandshake->rc_ddr_addr = UNDEFINED_DATA;
	while (phandshake->rc_ddr_addr == UNDEFINED_DATA)
        usleep(DEFAULT_TIMEOUT_US);

    return phandshake->rc_ddr_addr;
}

int pcie_parse_command_arguments(int argc, char *argv[])
{
	char *ep_local_ddr_addr_str = NULL;
	int c;

	while ((c = getopt (argc, argv, COMMON_COMMAND_ARGUMENTS)) != -1)
		switch (c) {
		  case 'a':
			ep_local_ddr_addr_str = optarg;
			ep_local_ddr_addr = strtoul(ep_local_ddr_addr_str, NULL, 16);
			break;
		  case 'e':
			printf("Argument \"-e\" does not apply to EndPoint\n");
			break;
		  case 'c':
			strncpy(batch_commands, optarg, MAX_BATCH_COMMANDS);
			break;
		}

	batch_commands[MAX_BATCH_COMMANDS] = 0;
	if (ep_local_ddr_addr)
		return 0;

	return 1;
}