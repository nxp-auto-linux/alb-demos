/*
 * Copyright 2016, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>

#include "pcie_handshake.h"
#include "pcie_benchmark.h"

#define CMD1_PATTERN	0x42
#define CMD3_PATTERN	0xC8
#define CMD6_PATTERN	CMD1_PATTERN

#define MAP_DDR_SIZE	1024 * 1024 * 1

struct test_write_args {
	uint32_t count;
	void *dst;
	void *src;
	ssize_t size;
};

static void *loop_pcie_write(void *va)
{
	int i;
	struct timespec ts;
	const struct test_write_args *args = (const struct test_write_args *)va;

	pcie_test_start(&ts);

	for (i = 0; i < args->count; i++) {
		memcpy(args->dst, args->src, args->size);
	}

	pcie_test_stop(&ts, "8 : MultiWrite", args->size, 0);

	return NULL;
}

int main(int argc, char *argv[])
{
	int fd1;
	unsigned int *mapDDR;
	unsigned int *mapPCIe;
	unsigned int *src_buff;
	unsigned int *dest_buff;

	/* Use a default 1M value if no arg */
	unsigned int mapsize = MAP_DDR_SIZE;

	unsigned int i = 0;
	unsigned char cmd = 0;
	char c;
	unsigned int go_exit = 0;
	unsigned short int go1 = 0;
	unsigned short int go2 = 0;
	char word[256];
	unsigned long int ep_bar2_addr = 0;
	unsigned long int rc_local_ddr_addr = 0;
	char batch_commands[MAX_BATCH_COMMANDS + 1] = {0,};
	int batch_idx = 0;

	struct timespec ts;

	if (pcie_parse_rc_command_arguments(argc, argv,
			&rc_local_ddr_addr, &ep_bar2_addr, batch_commands)) {
		printf("\nUsage:\n%s -a <rc_local_ddr_addr_hex> -e <ep_bar_addr_hex> [-c <commands>]\n\n", argv[0]);
		printf("E.g. for S32G2 (PCIe0, EP using BAR0):\n %s -a 0xC0000000 -e 0x4800100000\n\n", argv[0]);
		printf("Make sure <ep_bar_addr_hex> matches the EP BAR for the correct RC PCIe controller.\n");
		printf("By default, the EPs are using BAR0.\n\n");
		exit(1);
	}

	printf ("RC local DDR address = 0x%lx\n", rc_local_ddr_addr);
	printf ("EP BAR2 address = 0x%lx\n", ep_bar2_addr);

	src_buff = (unsigned int *)malloc(mapsize);
	if (!src_buff) {
		printf("\n Cannot allocate mem for source buffer");
	}

	dest_buff = (unsigned int *)malloc(mapsize);
	if (!dest_buff) {
		printf("\n Cannot allocate heap for dest buffer");
	}

	fd1 = open("/dev/mem", O_RDWR);
	if (fd1 < 0) {
		perror("Errors opening /dev/mem file");
		goto err;
	} else {
		printf("\n /dev/mem file opened successfully");
	}

	/* MAP PCIe area */
	mapPCIe = (unsigned int *)mmap(NULL, mapsize,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd1, ep_bar2_addr);
	if (!mapPCIe) {
		perror("/dev/mem PCIe area mapping FAILED");
		goto err;
	} else {
		printf("\n /dev/mem PCIe area mapping  OK");
	}

	/* MAP DDR free 1M area. This was reserved at boot time */
	mapDDR = (unsigned int *)mmap(NULL, MAP_DDR_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd1, rc_local_ddr_addr);
	if (!mapDDR) {
		perror("/dev/mem DDR area mapping FAILED");
		goto err;
	} else {
		printf("\n /dev/mem DDR area mapping OK");
	}

	/* Connect to EP and send RC_DDR_ADDR */
	printf("\n Connecting to EP\n");
	if (pcie_notify_ep((struct s32_handshake *)mapPCIe,
			rc_local_ddr_addr) < 0) {
	    perror("Unable to send RC local DDR address to EP");
	    goto err;
	}

	printf("Hello PCIe RC mem test app\n");

start :
	go1 = 0;
	go2 = 0;
	cmd = 0xFF;

	printf("\n Test cases :\
	\n 1. Single 1M Write transfer from local buffer to EP mem (pattern = %#x)\
	\n 2. Single 1M Read  transfer from EP mem to local buffer\
	\n 3. Variable size throughput test Write(pattern = %#x) + Read to/from EP mem\
	\n 4. Fill local DDR address + 1M with DW pattern 0x55AA55AA\
	\n 5. Read and print first and last 8DW(32bytes) in local DDR\
	\n 6. Multiple 1M Write transfers from local buffer to EP mem (pattern = %#x)\
	\n    This is essentially the same as #1, only looped and multithreaded.\
	\n    Usable for performance tests and some errata validation.\
	\n 7. Exit app\
	\n Select test : ",
	CMD1_PATTERN,
	CMD3_PATTERN,
	CMD6_PATTERN);

	/* Check if we have batch commands; if not, go to interactive mode */
	if (batch_commands[0]) {
		cmd = batch_commands[batch_idx++];
		if (cmd)
			cmd -= '0';
		else
			cmd = 7;
		printf("%c\n", cmd + '0');
		if (cmd == 3) {
			printf("Command 3 can be executed only in interactive mode\n");
			goto exit;
		}
	} else {

		/* Stop for command */
		do {
			c = getchar();
			switch (c) {
			case '1':
			case '2':
			case '4':
			case '5':
			case '6':
			case '7':
				cmd = c - '0';
				go1 = 1;
				break;
			case '3':
				printf("\n Enter bytes transfer size in hex (max %x, 128bytes multiple): ",
					MAP_DDR_SIZE);
				do {
					scanf("%s" , word);
					if (!sscanf(word, "%x", &mapsize) || (mapsize > MAP_DDR_SIZE))
						printf ("\n ERR, invalid input '%s'", word);
					else {
						cmd = 3;
						go1 = 1;
						go2 = 1;
						printf ("\n mapsize = %x",mapsize);
						printf ("\n OK, going to transfer");
					}
				} while (!go2);
				break;
			default :
				/* Omit enter key */
				if (c != '\n')
					printf("\n Invalid input\n");
				break;
			}
		} while (!go1);
	}

	printf("\n Command code  = %d \n", cmd);

	switch (cmd) {
	case 0: /* Only for batch mode: 5s sleep */
		sleep(5);
		break;
	case 1: /* Write to PCIe area */
		memset(src_buff, CMD1_PATTERN, mapsize);
		pcie_test_start(&ts);
		memcpy((unsigned int *)mapPCIe, (unsigned int *)src_buff, mapsize);

		pcie_test_stop(&ts, "1 : 1MB Write", mapsize, 0);
		break;
	case 2 : /* Read from PCIe area */
		/* Clear local buffer*/
		memset(dest_buff, 0x0, mapsize);
		pcie_test_start(&ts);
		/* Copy from EP to local buffer */
		memcpy((unsigned int *)dest_buff, (unsigned int *)mapPCIe, mapsize);

		pcie_test_stop(&ts, "2 : 1MB Read", mapsize, 0);

		pcie_show_mem(dest_buff, mapsize, "copied from EP remote DDR");
		break;
	case 3:
		/* Fill local src and dest buffer with different patterns */
		memset(src_buff, CMD3_PATTERN, mapsize);
		memset(dest_buff, 0, mapsize);

		pcie_test_start(&ts);
		/* Start write transaction  */
		memcpy(mapPCIe, src_buff, mapsize);
		pcie_test_stop(&ts, "3 : Write", mapsize, 0);

		pcie_test_start(&ts);

		/* Start read transaction  */
		memcpy(dest_buff, mapPCIe, mapsize);
		pcie_test_stop(&ts, "3 : Read", mapsize, 0);

		if (memcmp(dest_buff, src_buff, mapsize) == 0) {
			printf("\n W/R successful");
		} else {
			printf("\n Transfer failed!");
		}

		mapsize = MAP_DDR_SIZE;
		break;
	case 4 : 
		/* Fill local DDR_BASE + 1M with DW pattern 0x55AA55AA */
		for (i = 0 ; i < mapsize / 4 ; i++) {
			*(mapDDR + i) = (unsigned int)0x55AA55AA;
		}
		break;
	case 5 : 
		/* Read DDR area(minimal check). Can verify what EP has written */
		pcie_show_mem(mapDDR, mapsize, "from local mapped DDR");
		break;
	case 6:
	{
		const int num_threads = 10;

		pthread_t threads[num_threads];
		struct test_write_args args = {
			.count = 100,
			.dst = mapPCIe,
			.src = src_buff,
			.size = mapsize,
		};
		int err;

		memset(src_buff, CMD6_PATTERN, mapsize);
		printf("\n Starting %dx1M transfers from %d threads...\n",
			args.count, num_threads);

		for (i = 0; i < num_threads; i++) {
			err = pthread_create(&threads[i], NULL, loop_pcie_write, &args);
			if (err) {
				perror("pthread_create");
				break;
			}
		}
		for (i = 0; i < num_threads; i++) {
			err = pthread_join(threads[i], NULL);
			if (err) {
				perror("pthread_join");
				break;
			}
		}

		break;
	}
	case 7:
		go_exit = 1;
		break;
	default :
		printf("No args or out of bounds cmd val");
		break;

	}
	if (go_exit)
		goto exit;
	else 
		goto start;

err :
	printf("\n Too many errors");

exit : 
	close(fd1);
	printf("\n Gonna exit now\n");
	exit(0);
}
