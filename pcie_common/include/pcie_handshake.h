/*
 * Copyright 2018-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * Handshake between EndPoint and RootComplex.
 */

#define UNDEFINED_DATA 	0x0
#define DEFAULT_TIMEOUT_US 10000

/* A small buffer used for getting the RootComplex's DDR base address */
struct s32v_handshake {
    unsigned long long int rc_ddr_addr;
};

/* Handshake function which waits until RC sends its DDR base address
   To be used on the EP side
 */
unsigned long long int pcie_wait_for_rc(struct s32v_handshake *phandshake);

/* Handshake function which sends the RC DDR base address to the EP
   To be called from the RC side
 */
int pcie_notify_ep(struct s32v_handshake *phandshake);

/* Helper function for parsing command line arguments and setting
   the internal address variables needed for the handshake
 */
int pcie_parse_command_arguments(int argc, char *argv[]);
