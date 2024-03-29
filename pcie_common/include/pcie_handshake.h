/*
 * Copyright 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * Handshake between EndPoint and RootComplex.
 */

#define UNDEFINED_DATA 	0x0
#define DEFAULT_TIMEOUT_US 10000

#define COMMON_COMMAND_ARGUMENTS "a:b:e:c:n:w:m:s"
#define MAX_BATCH_COMMANDS 10

#define HEADER_SIZE	sizeof(struct s32_handshake)
/* Default 1MB shared mem */
#define MAP_DDR_SIZE	(1024 * 1024 * 1)
/* MAx 8MB shared mem, as this is the amount reserved in the device tree */
#define MAP_DDR_SIZE_MAX	(1024 * 1024 * 8)

#define SHOW_COUNT	2
#define SHOW_COUNT_MAX	8

/* A small buffer used for getting the RootComplex's DDR base address */
struct s32_handshake {
    unsigned long long int rc_ddr_addr;
};

struct s32_common_args {
	unsigned int map_size;
	unsigned int show_count;
	unsigned int skip_handshake;

	char batch_commands[MAX_BATCH_COMMANDS + 1];
};

/* Handshake function which waits until RC sends its DDR base address
   To be used on the EP side
 */
unsigned long long int pcie_wait_for_rc(struct s32_handshake *phandshake);

/* Handshake function which sends the RC DDR base address to the EP
   To be called from the RC side
 */
int pcie_notify_ep(struct s32_handshake *phandshake,
	unsigned long int rc_local_ddr_addr);

/* Helper functions for parsing command line arguments and setting
   the internal address variables needed for the handshake
 */
int pcie_parse_rc_command_arguments(int argc, char *argv[],
	unsigned long int *rc_local_ddr_addr,
	unsigned long int *ep_bar_addr,
	struct s32_common_args *common
);
int pcie_parse_ep_command_arguments(int argc, char *argv[],
	unsigned long int *ep_pcie_base_address,
	unsigned long int *ep_local_ddr_addr,
	unsigned int *bar_number,
	struct s32_common_args *common
);
int pcie_parse_common_arguments(int opt, char *optarg,
	struct s32_common_args *args
);
