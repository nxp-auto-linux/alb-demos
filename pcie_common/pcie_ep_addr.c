/*
 * Copyright 2018, 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * PCIe HandShake between EndPoint and RootComplex,
 * EndPoint side.
 */

#include "include/pcie_ep_addr.h"
#include "include/pcie_ops.h"
#include "include/pcie_handshake.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Inbound region structure */
struct s32v_inbound_region inb1 = {
    EP_BAR,			  	/* BAR2 by default */
    UNDEFINED_DATA,		/* locally-mapped DDR on EP (target addr) */
    0			  		/* region 0 */
};

/* Outbound region structure */
struct s32v_outbound_region outb1 = {
    UNDEFINED_DATA,  	  	/* target_addr, to be filled from handshake data */
    S32V_PCIE_BASE_ADDR,    /* base_addr */
    UNDEFINED_DATA, 		/* size >= 64K(min for PCIE on S32V), to be filled afterwards */
    0,						/* region number */
    0						/* region type = mem */
};

int pcie_init_inbound(unsigned long int ep_local_ddr_addr, int fd)
{
	int ret = 0;

	inb1.target_addr = ep_local_ddr_addr;
	ret = ioctl(fd, SETUP_INBOUND, &inb1);
	return ret;
}

int pcie_init_outbound(unsigned long long int targ_addr,
	unsigned int buff_size, int fd)
{
    int ret = 0;

    outb1.target_addr = targ_addr;
    outb1.size = buff_size;

    ret = ioctl(fd, SETUP_OUTBOUND, &outb1);
    return ret;
}

unsigned long long int pcie_wait_for_rc(struct s32v_handshake *phandshake)
{
	if (!phandshake)
		return UNDEFINED_DATA;

    phandshake->rc_ddr_addr = UNDEFINED_DATA;
	while (phandshake->rc_ddr_addr == UNDEFINED_DATA)
        usleep(DEFAULT_TIMEOUT_US);

    return phandshake->rc_ddr_addr;
}

int pcie_parse_ep_command_arguments(int argc, char *argv[],
	unsigned long int *ep_local_ddr_addr,
	char *batch_commands)
{
	char *ep_local_ddr_addr_str = NULL;
	int c;

	if (!ep_local_ddr_addr) {
		fprintf(stderr, "Invalid arguments\n");
		return -1;
	}

	while ((c = getopt (argc, argv, COMMON_COMMAND_ARGUMENTS)) != -1)
		switch (c) {
		  case 'a':
			ep_local_ddr_addr_str = optarg;
			*ep_local_ddr_addr = strtoul(ep_local_ddr_addr_str, NULL, 16);
			break;
		  case 'e':
			printf("Argument \"-e\" does not apply to EndPoint\n");
			break;
		  case 'c':
			if (batch_commands)
				strncpy(batch_commands, optarg, MAX_BATCH_COMMANDS);
			else
				fprintf(stderr, "Unsupported option '-c'\n");
			break;
		}

	batch_commands[MAX_BATCH_COMMANDS] = 0;
	if (*ep_local_ddr_addr)
		return 0;

	return 1;
}
