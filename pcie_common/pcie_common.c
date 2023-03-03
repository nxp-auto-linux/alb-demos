/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * PCIe HandShake between EndPoint and RootComplex,
 * common stuff.
 */

#include "include/pcie_ops.h"
#include "include/pcie_handshake.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int pcie_parse_common_arguments(int opt, char *optarg,
	struct s32_common_args *args)
{
	if (!args) {
		fprintf(stderr, "Unsupported option '-%c'\n", (char)opt);
		return -1;
	}

	if (!optarg) {
		fprintf(stderr, "Invalid arguments list\n");
		return -1;
	}
	switch (opt) {
		case 'w':
			args->show_count = strtoul(optarg, NULL, 10);
			if (args->show_count < 1 || args->show_count > SHOW_COUNT_MAX) {
				fprintf(stderr, "Invalid value %u for option '-%c'\n",
					args->show_count, (char)opt);
				return -1;
			}
			break;
		case 'm':
			args->map_size = strtoul(optarg, NULL, 10);
			if (args->map_size < MAP_DDR_SIZE ||
				args->map_size > MAP_DDR_SIZE_MAX) {
				fprintf(stderr, "Invalid value %u for option '-%c'\n",
					args->map_size, (char)opt);
				return -1;
			}
			break;
		case 's':
			args->skip_handshake = 1;
			break;
		case 'c':
			strncpy(args->batch_commands, optarg, MAX_BATCH_COMMANDS + 1);
			args->batch_commands[MAX_BATCH_COMMANDS] = 0;
			break;
	}

	return 0;
}
