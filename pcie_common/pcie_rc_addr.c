/*
 * Copyright 2018, 2021 NXP
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
#include <string.h>

int pcie_notify_ep(struct s32_handshake *phandshake,
	unsigned long int rc_local_ddr_addr)
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

int pcie_parse_rc_command_arguments(int argc, char *argv[],
	unsigned long int *rc_local_ddr_addr,
	unsigned long int *ep_bar_addr,
	char *batch_commands)
{
	char *ep_bar_addr_str = NULL;
	char *rc_local_ddr_addr_str = NULL;
	int c;

	if (!ep_bar_addr || !rc_local_ddr_addr) {
		fprintf(stderr, "Invalid arguments\n");
		return -1;
	}

	while ((c = getopt (argc, argv, COMMON_COMMAND_ARGUMENTS)) != -1)
		switch (c) {
		  case 'a':
			rc_local_ddr_addr_str = optarg;
			*rc_local_ddr_addr = strtoul(rc_local_ddr_addr_str, NULL, 16);
			break;
		  case 'b':
			printf("Argument \"-b\" does not apply to RootComplex\n");
			break;
		  case 'e':
			ep_bar_addr_str = optarg;
			*ep_bar_addr = strtoul(ep_bar_addr_str, NULL, 16);
			break;
		  case 'i':
			printf("Argument \"-i\" does not apply to RootComplex\n");
			break;
		  case 'c':
			if (batch_commands)
				strncpy(batch_commands, optarg, MAX_BATCH_COMMANDS);
			else
				fprintf(stderr, "Unsupported option '-c'\n");
			break;
		}

	batch_commands[MAX_BATCH_COMMANDS] = 0;
	if (*ep_bar_addr && *rc_local_ddr_addr)
		return 0;

	return 1;
}
