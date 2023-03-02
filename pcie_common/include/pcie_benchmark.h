/*
 * Benchmarking functions
 * 
 * Copyright 2021, 2023 NXP
 * 
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define SHOW_COUNT	2

void pcie_test_start(struct timespec *start);

void pcie_test_stop(struct timespec *start,
		const char *ops, uint64_t size,
		int dma);

void pcie_show_mem(unsigned int *buff,
		unsigned int size,
		char *mem_name, unsigned int count);
