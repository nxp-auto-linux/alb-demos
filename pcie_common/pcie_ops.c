/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * PCIe initialization and HandShake between EndPoint and RootComplex.
 */

#include <sys/ioctl.h> /* ioctl() */

#include "include/pcie_ops.h"
#include "include/pcie_ep_addr.h"
#include "include/pcie_handshake.h"

#include <unistd.h>

/* Outbound region structure */
struct s32v_outbound_region outb1 = {
    UNDEFINED_DATA,  	  	/* target_addr, to be filled from handshake data */
    S32V_PCIE_BASE_ADDR,    /* base_addr */
    UNDEFINED_DATA, 		/* size >= 64K(min for PCIE on S32V), to be filled afterwards */
    0,						/* region number */
    0						/* region type = mem */
};

/* Inbound region structure */
struct s32v_inbound_region inb1 = {
    EP_BAR,			  	/* BAR2 by default */
    EP_LOCAL_DDR_ADDR,  /* locally-mapped DDR on EP (target addr) */
    0			  		/* region 0 */
};

int pcie_init_inbound(int fd)
{
	int ret = 0;

	ret = ioctl(fd, SETUP_INBOUND, &inb1);
	return ret;
}

int pcie_init_outbound(unsigned long long int targ_addr, unsigned int buff_size, int fd)
{
    int ret = 0;

    outb1.target_addr = targ_addr;
    outb1.size = buff_size;

    ret = ioctl(fd, SETUP_OUTBOUND, &outb1);
    return ret;
}
