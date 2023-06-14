/*
 * Copyright 2016, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "pcie_ops.h"
#include "pcie_ep_addr.h"
#include "pcie_handshake.h"
#include "pcie_benchmark.h"

#define CMD1_PATTERN	0x12
#define CMD3_PATTERN	0x67
#define CMD8_PATTERN	CMD1_PATTERN

#define EP_DBGFS_FILE		"/sys/kernel/debug/ep_dbgfs/ep_file"

volatile sig_atomic_t dma_flag = 0;
volatile sig_atomic_t cntSignalHandler = 0;
struct sigaction action;

void signal_handler(int signum) {
	if (signum == SIGUSR1) {
		printf ("\n DMA transfer completed");
		dma_flag = 1;
		cntSignalHandler++;
	}
	return;
}

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
	int fd2;
	int ret = 0;
	void *mapDDR_base = NULL;
	void *mapPCIe_base = NULL;
	unsigned int *mapDDR = NULL;
	unsigned int *mapPCIe = NULL;
	unsigned long int rc_ddr_addr = UNDEFINED_DATA;
	int i = 0;
	void *src_buff ;
	void *dest_buff ;
	unsigned char cmd = 0xff;
	struct timespec ts;

	unsigned int totalsize = MAP_DDR_SIZE; /* 1M default */
	unsigned int mapsize = MAP_DDR_SIZE - HEADER_SIZE;
	char c;
	unsigned short int go1 = 0;
	unsigned short int go2 = 0;
	unsigned int go_exit = 0;
	char word[256];
	int pid;
	int batch_idx = 0;
	unsigned long int ep_pcie_base_address = 0;
	unsigned long int ep_local_ddr_addr = 0;
	unsigned int bar_number = 0; /* BAR0 by default */
	unsigned int show_count = SHOW_COUNT;
	unsigned int skip_handshake = 0;

	/* Struct for DMA ioctl */
	struct dma_data_elem dma_single = {0,0,0,0,0,0};
	struct s32_common_args args = {MAP_DDR_SIZE, SHOW_COUNT, 0,};

	if (pcie_parse_ep_command_arguments(argc, argv,
			&ep_pcie_base_address, &ep_local_ddr_addr, &bar_number, &args)) {
		printf("\nUsage:\n%s -b <pcie_base_address> -a <local_ddr_addr_hex> [-n <BAR index>][-m <memsize>][-w <count>][-s][-c <commands>]\n\n", argv[0]);
		printf("E.g. for S32G2 (PCIe1, BAR0):\n %s -a 0xC0000000 -b 0x4800000000\n", argv[0]);
		printf("By default, BAR0 is used.\n\n");
		exit(1);
	}

	totalsize = args.map_size;
	mapsize = totalsize - HEADER_SIZE;
	show_count = args.show_count;
	skip_handshake = args.skip_handshake;

	printf ("EP local PCIe base address = 0x%lX\n", ep_pcie_base_address);
	printf ("EP local DDR address = 0x%lX\n", ep_local_ddr_addr);
	printf ("Total size = %d bytes\nUsing BAR%u\n\n", totalsize, bar_number);

	/* Set handler for SIGUSR */
	memset(&action, 0, sizeof (action));	/* clean variable */
	action.sa_handler = signal_handler;	/* specify signal handler */
	// action.sa_flags = 0;	/* operation flags setting */
	action.sa_flags = SA_NODEFER;		/* do not block SIGUSR1 within sig_handler_int */
	sigaction(SIGUSR1, &action, NULL);	/* attach action with SIGIO */

	src_buff = malloc(mapsize);
	dest_buff = malloc(mapsize);
	if (!src_buff || !dest_buff) {
		perror("Cannot allocate mem for buffers");
		goto err;
	}

	fd1 = open(EP_DBGFS_FILE, O_RDWR);
	if (fd1 < 0) {
		perror("Error while opening debug file");
		goto err;
	}
	printf("\n Ep debug file opened successfully");

	fd2 = open("/dev/mem", O_RDWR);
	if (fd2 < 0) {
		perror("Error while opening /dev/mem file");
		goto err;
	}
	printf("\n Mem opened successfully");

	/* MAP DDR free 1M area. This was reserved at boot time */
	mapDDR_base = mmap(NULL, totalsize,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd2, ep_local_ddr_addr);
	if (!mapDDR_base) {
		perror("/dev/mem DDR area mapping FAILED");
		goto err;
	}
	printf("\n /dev/mem DDR area mapping OK");

	/* Map PCIe area */
	mapPCIe_base = mmap(NULL, totalsize,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd2, ep_pcie_base_address);
	if (!mapPCIe_base) {
		perror("/dev/mem PCIe area mapping FAILED");
		goto err;
	}
	printf("\n /dev/mem PCIe area mapping OK\n");

	mapDDR = mapDDR_base + HEADER_SIZE;
	mapPCIe = mapPCIe_base + HEADER_SIZE;
	printf ("Local DDR base: 0x%lX; DDR buffer = 0x%lX\n",
		(uintptr_t)mapDDR_base, (uintptr_t)mapDDR);

	if (!skip_handshake) {
		/* Setup inbound window for receiving data into local shared buffer */
		ret = pcie_init_inbound(ep_local_ddr_addr, bar_number, fd1);
		if (ret < 0) {
		    perror("Error while setting inbound region");
		    goto err;
		}
		printf("\n Inbound region setup successfully");

		printf("\n Connecting to RC...\n");
		rc_ddr_addr = pcie_wait_for_rc((struct s32_handshake *)mapDDR_base);
		printf(" RC_DDR_ADDR = %lx", rc_ddr_addr);

		/* Setup outbound window for accessing RC mem */
		ret = pcie_init_outbound(ep_pcie_base_address,
			rc_ddr_addr, totalsize, fd1);
		if (ret < 0) {
			perror("Error while setting outbound region");
			goto err;
		}
		printf("\n Outbound region setup successfully");
	}

	/* Sending pid by ioctl. This sets up signaling from kernel */
	pid = getpid();
	printf("\n LOG USspace getpid() = %d", pid);
	ret = ioctl(fd1, STORE_PID, &pid);
	if (ret < 0) {
		perror("Error while sending pid to driver, DMA may not work");
	} else {
		printf("\n Pid sent successfully to driver");
	}

	printf("\nHello PCIe EP mem test app\n");

start:
	go1 = 0;
	go2 = 0;
	cmd = 0xFF;

	printf("\n Select test (press 'h' to show all tests): ");

	/* Check if we have batch commands; if not, go to interactive mode */
	if (args.batch_commands[0]) {
		cmd = args.batch_commands[batch_idx++];
		if (cmd)
			cmd -= '0';
		else
			cmd = 9;
		printf("%c\n", cmd + '0');
		if (cmd == 3) {
			printf("Command 3 can be executed only in interactive mode\n");
			goto exit;
		}
	} else {

		/* Stop for char input. Sync with debugger */
		do {
			c = getchar();
			switch (c) {
			case '1':
			case '2':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				cmd = c - '0';
				go1 = 1;
				break;
			case '3':
				printf("\n Enter bytes transfer size in hex (max %x, 128bytes multiple): ",
					mapsize);
				do {
					scanf("%s" , word);
					if (!sscanf(word, "%x", &mapsize) || (mapsize > totalsize - HEADER_SIZE))
						printf ("\n ERR, invalid input '%s'", word);
					else {
						cmd = 3;
						go1 = 1;
						go2 = 1;
						printf ("\n size = %x", mapsize);
						printf ("\n OK, going to transfer");
					}
				} while (!go2);
				break;
			case 'h':
				printf("\n Test cases : \
				\n 1. Single 1M Write transfer from local buffer to RC DDR mem (pattern=%#x)\
				\n 2. Single 1M Read transfer from RC DDR buffer to local DDR buffer\
				\n 3. Variable size throughput test Write(pattern = %#x) + Read to/from RC DDR mem\
				\n 4. Fill local DDR buffer (1M) with pattern 0xdeadbeef\
				\n 5. Read and print first and last %d DW(32bytes) bytes in local DDR\
				\n 6. Write 1M from local DDR to RC DDR mem through DMA single transfer\
				\n 7. Read 1M from RC DDR mem to local DDR through DMA single transfer\
				\n 8. Multiple 1M Write transfers from local buffer to RC DDR mem (pattern=%#x)\
				\n    This is essentially the same as #1, only looped and multithreaded.\
				\n    Usable for performance tests and some errata validation.\
				\n 9. Exit app\n",
				CMD1_PATTERN,
				CMD3_PATTERN, show_count,
				CMD8_PATTERN);
				printf("\n Select test (press 'h' to show all tests): ");
				break;
			default :
				/* Omit enter key */
				if (c != '\n')
					printf("\n Invalid command %c\n", c);
				break;
			}
		} while (!go1);
	}

	printf("\n Command code  = %d", cmd);

	switch (cmd) {
	/* special command, only for batch mode: 5s sleep */
	case 0:
		sleep(5);
		break;
	/* Single 1M bytes Write to LS_RC mem */
	case 1:
		memset(src_buff, CMD1_PATTERN, mapsize);
		pcie_test_start(&ts);
		memcpy(mapPCIe, src_buff, mapsize);
		pcie_test_stop(&ts, "1 : 1MB Write", mapsize, 0);
		break;
	/* Single 1M bytes Read from LS_RC mem */
	case 2:
		/* Clear local DDR_BASE + 1M */
		pcie_fill_dev_mem(mapDDR, mapsize, 0x0U);
		pcie_test_start(&ts);
		memcpy(mapDDR, mapPCIe, mapsize);
		pcie_test_stop(&ts, "2 : 1MB Read", mapsize, 0);
		break;
	/* R/W thoughput test */
	case 3:
		/* Fill local src and dest buffer with different patterns */
		memset(dest_buff, 0, mapsize);
		memset(src_buff, CMD3_PATTERN, mapsize);

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

		mapsize = totalsize - HEADER_SIZE;
		break;
	/* Clear local mapped DDR */
	case 4:
		pcie_fill_dev_mem(mapDDR, mapsize, 0xDEADBEEFU);
		break;
	/* read some data from ddr , starting with base addr */
	case 5:
		pcie_show_mem((unsigned int *)mapDDR, mapsize,
			"from local mapped DDR", show_count);
		break;
	/* SEND DMA single  block */
	case 6:
		dma_single.flags = 0;
		dma_flag = 0;
		dma_single.size = mapsize;
		dma_single.sar = ep_local_ddr_addr + HEADER_SIZE;
		dma_single.dar = ep_pcie_base_address + HEADER_SIZE;
		dma_single.ch_num = 0;
		dma_single.flags = (DMA_FLAG_WRITE_ELEM | DMA_FLAG_EN_DONE_INT | DMA_FLAG_LIE);

		pcie_test_start(&ts);
		ret = ioctl(fd1, SEND_SINGLE_DMA, &dma_single);
		while (!dma_flag) { ; }

		pcie_test_stop(&ts, "6 : 1MB Write", mapsize, 1);

		printf("\n DMA Signal Handler count = %d", cntSignalHandler);
		break;
	/* GET DMA single  block */
	case 7:
		dma_single.flags = 0;
		dma_flag = 0;
		dma_single.size = mapsize;
		dma_single.sar = ep_pcie_base_address + HEADER_SIZE;
		dma_single.dar = ep_local_ddr_addr + HEADER_SIZE;
		dma_single.ch_num = 0;
		dma_single.flags = (DMA_FLAG_READ_ELEM | DMA_FLAG_EN_DONE_INT  | DMA_FLAG_LIE);

		pcie_test_start(&ts);
		ret = ioctl(fd1, SEND_SINGLE_DMA, &dma_single);
		/* Wait for transfer done interrupt */
		while (!dma_flag) { ; }

		pcie_test_stop(&ts, "7 : 1MB Read", mapsize, 1);
		break;
	case 8:
	{
		const int num_threads = 100;

		pthread_t threads[num_threads];
		struct test_write_args args = {
			.count = 100,
			.dst = mapPCIe,
			.src = src_buff,
			.size = mapsize,
		};
		int err;

		memset(src_buff, CMD8_PATTERN, mapsize);
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

		printf("\n ...done.\n");

		break;
	}
	case 9:
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
	printf("\n too many errors");

exit :
	close(fd1);
	close(fd2);
	printf("\n Gonna exit now\n");
	exit(0);
}
