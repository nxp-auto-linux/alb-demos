/*
 * Copyright (C) 2016 NXP Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#define CMD1_PATTERN	0x12
#define CMD3_PATTERN	0x67
#define CMD8_PATTERN	CMD1_PATTERN

#define MAP_DDR_SIZE	1024 * 1024 * 1 /* 1MB  */

/* bhamciu1 FIXME remove address hardcoding */
#define PCIE_ADDR	0x72000000
#define CPU_ADDR	0x8FF00000
#define RC_ADDR		0x8350000000
#define EP_DBGFS_FILE	"/sys/kernel/debug/ep_dbgfs/ep_file"

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
	const struct test_write_args *args = (const struct test_write_args *)va;

	for (i = 0; i < args->count; i++) {
		memcpy(args->dst, args->src, args->size);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	int fd1;
	int fd2;
	int ret = 0;
	unsigned int *mapDDR = NULL;
	unsigned int *mapPCIe = NULL;
	int i = 0;
	int j = 0;
	unsigned int *src_buff ;
	unsigned int *dest_buff ;
	unsigned int cmd = 0xff;
	struct timespec tps;
	unsigned long int nanoSec1Start;
	unsigned long int nanoSec1Stop;
	unsigned long int nanoSec2Start;
	unsigned long int nanoSec2Stop;
	unsigned long int nanoSecDiff1;
	unsigned long int nanoSecDiff2;
	unsigned int mapsize = MAP_DDR_SIZE; /* 1M default */
	char c;
	unsigned short int go1 = 0;
	unsigned short int go2 = 0;
	unsigned int first_cmd = 0;
	unsigned int go_exit = 0;
	char word[256];
	int pid;
	
	/* Struct for DMA ioctl */
	struct dma_data_elem dma_single = {0,0,0,0,0,0};
	
	/* Outbound region structure */
	struct s32v_outbound_region outb1 = {
		RC_ADDR,	/* target_addr = CPU_ADDR */
		PCIE_ADDR,	/* base_addr */
		MAP_DDR_SIZE,	/* size >= 64K(min for PCIE on S32V) */
		0,		/* region number */
		0		/* region type = mem */
		};

	struct s32v_inbound_region inb1 = {
		2,		/* BAR2 */
		CPU_ADDR,	/* locally-mapped DDR on EP */
		0		/* region 0 */
	};
start:
	go1 = 0;
	go2 = 0;
	cmd = 0xFF;
	printf("\nHello, S32V_EP PCIe mem test app #1\n");
	printf("\n Test cases : \
	\n 1. Single 1M Write transfer from local buffer to LS_RC DDR mem (pattern=0x%x)\
	\n 2. Single 1M Read transfer from LS_RC DDR mem to local buffer\
	\n 3. Variable size throughput test Write(pattern = 0x%x) + Read to/from LS_RC DDR mem\
	\n 4. Fill local DDR_BASE + 1M with pattern 0xdeadbeef\
	\n 5. Read and print first and last 32DW(128bytes) bytes in local DDR\
	\n 6. Write 1M from local DDR to LS_RC DDR mem through DMA single transfer\
	\n 7. Read 1M from LS_RC DDR mem to local DDR through DMA single transfer\
	\n 8. Multiple 1M Write transfers from local buffer to LS_RC DDR mem (pattern=0x%x)\
	\n    This is essentially the same as #1, only looped and multithreaded.\
	\n    Usable for performance tests and some errata validation.\
	\n 9. Exit app\
	\n Select test : ",
	CMD1_PATTERN,
	CMD3_PATTERN,
	CMD8_PATTERN);
	
	/* Stop for char input. Sync with debugger */
	do {
		c = getchar();
		switch (c) {
		case '1':
			cmd = 1;
			go1 = 1;
			break;
		case '2':
			cmd = 2;
			go1 = 1;
			break;
		case '3':
			printf("\n Enter bytes transfer size in hexa(max 1M, 128bytes multiple): ");
			do {
				scanf("%s" , word);
				if (!sscanf(word, "%x", &mapsize))
					printf ("\n ERR, invalid input");
				else {
					cmd = 3;
					go1 = 1;
					go2 = 1;
					first_cmd = 0;
					printf ("\n mapsize = %x",mapsize);
					printf ("\n OK, going to transfer");
				}
			} while (!go2);
			break;
		case '4':
			cmd = 4;
			go1 = 1;
			break;
		case '5':
			cmd = 5;
			go1 = 1;
			break;
		case '6':
			cmd = 6;
			go1 = 1;
			break;
		case '7':
			cmd = 7;
			go1 = 1;
			break;
		case '8':
			cmd = 8;
			go1 = 1;
			break;
		case '9':
			cmd = 9;
			go1 = 1;
			break;
		default :
			/* Omit enter key */
			if (c != '\n')
				printf("\n Invalid input\n");
			break;
		}
	} while (!go1);
	
	if (!first_cmd) {
		first_cmd = 1;
		memset(&action, 0, sizeof (action));	/* clean variable */
		action.sa_handler = signal_handler;	/* specify signal handler */
		// action.sa_flags = 0;	/* operation flags setting */
		action.sa_flags = SA_NODEFER;		/* do not block SIGUSR1 within sig_handler_int */
		sigaction(SIGUSR1, &action, NULL);	/* attach action with SIGIO */

		printf("\n Command code  = %d", cmd);
		
		src_buff = (unsigned int *)malloc(MAP_DDR_SIZE);
		dest_buff = (unsigned int *)malloc(MAP_DDR_SIZE);

		/* bhamciu1 FIXME remove hardcoding of filename */
		fd1 = open(EP_DBGFS_FILE, O_RDWR);
		if (fd1 < 0) {
			printf("%s %d\n", "Error while opening debug file ", errno);
			goto err;
		} else {
			printf("\n Ep debug file opened successfully");
		}

		fd2 = open("/dev/mem", O_RDWR);
		if (fd2 < 0) {
			printf("%s %d\n", "Error while opening /dev/mem file", errno);
			goto err;
		} else {
			printf("\n Mem opened successfully");
		}
		
		/* MAP DDR free 1M area. This was reserved at boot time */
		mapDDR = mmap(NULL, MAP_DDR_SIZE,
				PROT_READ | PROT_WRITE,
				MAP_SHARED, fd2, CPU_ADDR);
		if (!mapDDR) {
			printf("\n /dev/mem DDR area mapping FAILED");
			goto err;
		} else {
			printf("\n /dev/mem DDR area mapping OK");
		}
		
		/* Map PCIe area */
		mapPCIe = mmap(NULL, MAP_DDR_SIZE,
				PROT_READ | PROT_WRITE,
				MAP_SHARED, fd2, PCIE_ADDR);
		if (!mapPCIe) {
			printf("\n /dev/mem PCIe area mapping FAILED");
			goto err;
		} else {
			printf("\n /dev/mem PCIe area mapping OK");
		}
		
		/* Setup outbound window for accessing RC mem */
		ret = ioctl(fd1, SETUP_OUTBOUND, &outb1);
		if (ret < 0) {
			printf("\n Error while setting outbound1 region");
			goto err;
		} else {
			printf("\n Outbound1 region setup successfully");
		}

		/* Same thing for inbound window for transactions from RC */
		ret = ioctl(fd1, SETUP_INBOUND, &inb1);
		if (ret < 0) {
			printf("\nError while setting inbound1 region");
			goto err;
		} else {
			printf("\n Inbound1 region setup successfully");
		}

		/* Sending pid by ioctl. This sets up signaling from kernel */
		pid = getpid();
		printf("\n LOG USspace getpid() = %d", pid);
		ret = ioctl(fd1, STORE_PID, &pid);	
		if (ret < 0) {
			printf("\n Error while sending pid to driver, DMA may not work");
		} else {
			printf("\n Pid sent successfully to driver");
		}
	}
	printf("\n");

	switch (cmd) {
	/* Single 1M bytes Write to LS_RC mem */
	case 1:
		memset(src_buff, CMD1_PATTERN, mapsize);
		memcpy((unsigned int *)mapPCIe, (unsigned int *) src_buff, mapsize);
		break;
	/* Single 1M bytes Read from LS_RC mem */
	case 2:
		memcpy((unsigned int *)dest_buff,(unsigned int *)mapPCIe, mapsize);
		printf("\n First 32 DWORDS(1DW = 4bytes) copied from RC");
		for (i = 0; i < 32; i++) {
			printf("\n *(dest_buff + 0x%8x) = 0x%08x", i, *(dest_buff + i));
		}
		/* Resize for last 32 DWs */
		j = (mapsize / 4) - 32;
		printf("\n Last 32 DWORDS(1DW = 4bytes) copied from RC");
		for (i = j ; i < mapsize / 4; i++) {
			printf("\n *(dest_buff + 0x%8x) = 0x%08x", i, *(dest_buff + i));
		}
		break;
	/* R/W thoughput test */
	case 3:
		/* Fill local src and dest buffer with different patterns */
		memset(dest_buff, 0, mapsize);
		memset(src_buff, CMD3_PATTERN, mapsize);
		
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec1Start = tps.tv_nsec;
		/* Start write transaction  */
		memcpy(mapPCIe, src_buff, mapsize);
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec1Stop = tps.tv_nsec;
		nanoSecDiff1 = nanoSec1Stop - nanoSec1Start;
		
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec2Start = tps.tv_nsec;
		/* Start read transaction  */
		memcpy(dest_buff, mapPCIe, mapsize);

		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec2Stop = tps.tv_nsec;		
		nanoSecDiff2 = nanoSec2Stop - nanoSec2Start;
		
		if (memcmp(dest_buff, src_buff, mapsize) == 0) {
			printf("\n W/R successful");
		} else {
			printf("\n Transfer failed!");
		}

		printf("\n nanoSecDiff1(Write Transaction) = %ld	\
			\n nanoSecDiff2(Read Transaction) = %ld",
			nanoSecDiff1, nanoSecDiff2);
		printf("\n tps.tv_sec = %ld", tps.tv_sec);
		printf("\n tps.tv_nsec = %ld", tps.tv_nsec);
		printf("\n Size of transfer in bytes = %d", mapsize);
		/* (bytes / Period_nanosec) * 10^9 / (1024 * 1024)  */
		printf ("\n /* (bytes / Period_nanosec) * 10^9 / (1024 * 1024)  */");
		printf("\n Throughput write =%3.3f MB/sec", ((float)mapsize/nanoSecDiff1)*(float)953.67);
		printf("\n Throughput read =%3.3f MB/sec", ((float)mapsize/nanoSecDiff2)*(float)953.67);
		break;
	/* Clear local mapped DDR */
	case 4:
		for (i = 0 ; i < mapsize / 4; i++) {
			*(mapDDR + i) = 0xdeadbeef;
		}
		break;
	/* read some data from ddr , starting with base addr */
	case 5:
		printf("\n First 32 DWORDS(1DW = 4bytes) from local mapped DDR");
		for (i = 0 ; i < 32; i++) {
			printf("\n *(mapDDR + 0x%8x) = 0x%08x", i, *(mapDDR + i));
		}
		j = (mapsize / 4) - 32;
		printf("\n Last 32 DWORDS(1DW = 4bytes) from local mapped DDR");
		for (i = j ; i < mapsize/4; i++) {
			printf("\n *(mapDDR + 0x%8x) = 0x%08x", i, *(mapDDR + i));
		}
		break;
	/* SEND DMA single  block */
	case 6:
		dma_single.flags = 0;
		dma_flag = 0;
		dma_single.size = mapsize;
		dma_single.sar = CPU_ADDR; /* bhamciu1 FIXME remove hardcoding */
		dma_single.dar = PCIE_ADDR; /* bhamciu1 FIXME remove hardcoding */
		dma_single.ch_num = 0;
		dma_single.flags = (DMA_FLAG_WRITE_ELEM | DMA_FLAG_EN_DONE_INT | DMA_FLAG_LIE);
		
		/* Get timestamp before initiating transfer */
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec1Start = tps.tv_nsec;
		ret = ioctl(fd1, SEND_SINGLE_DMA, &dma_single);
		while (!dma_flag) { ; }
		
		/* Get timestamp after transfer */
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec1Stop = tps.tv_nsec;
		nanoSecDiff1 = nanoSec1Stop - nanoSec1Start;
		printf("\n Throughput write =%3.3f MB/sec", ((float)mapsize/nanoSecDiff1)*(float)953.67);		
		
		printf("\n cntSignalHandler = %d", cntSignalHandler);
		break;
	/* GET DMA single  block */
	case 7:
		dma_single.flags = 0;
		dma_flag = 0;
		dma_single.size = mapsize;
		dma_single.sar = PCIE_ADDR; /* bhamciu1 FIXME remove hardcoding */
		dma_single.dar = CPU_ADDR; /* bhamciu1 FIXME remove hardcoding */
		dma_single.ch_num = 0;
		dma_single.flags = (DMA_FLAG_READ_ELEM | DMA_FLAG_EN_DONE_INT  | DMA_FLAG_LIE);
		
		/* Get timestamp before initiating transfer */
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec2Start = tps.tv_nsec;
		ret = ioctl(fd1, SEND_SINGLE_DMA, &dma_single);
		/* Wait for transfer done interrupt */
		while (!dma_flag) { ; }
		/* Get timestamp after transfer */		
		clock_gettime(CLOCK_REALTIME, &tps);
		nanoSec2Stop = tps.tv_nsec;		
		nanoSecDiff2 = nanoSec2Stop - nanoSec2Start;
		printf("\n Throughput read =%3.3f MB/sec", ((float)mapsize/nanoSecDiff2)*(float)953.67);		
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
	exit(0);
}
