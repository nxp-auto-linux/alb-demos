/*
 * Copyright (C) 2016 NXP Semiconductor
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>


#define CMD1_PATTERN	0x42
#define CMD3_PATTERN	0xC8
#define CMD6_PATTERN	CMD1_PATTERN

#define MAP_DDR_SIZE	1024 * 1024 * 1

/* bhamciu1 FIXME remove hardcoding of addresses */
/* EP_BAR2_ADDR is an address in PCIE mem space.
 * It is the value in the BAR2 register of the device(EP).
 * EP, on its side will match accesses on that address to its DDR.
 */
#define EP_BAR2_ADDR	0x1440100000ll
/* Physical memory mapped by the RC CPU */
#define RC_DDR_ADDR	0x8350000000

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
	unsigned int *mapDDR;
	unsigned int *mapPCIe;
	unsigned int *src_buff;
	unsigned int *dest_buff;
	unsigned int mapsize = MAP_DDR_SIZE;/* Use a default 1M value if no arg */
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int cmd = 0;
	unsigned long int nanoSec1Start;
	unsigned long int nanoSec1Stop;
	unsigned long int nanoSec2Start;
	unsigned long int nanoSec2Stop;
	unsigned long int nanoSecDiff1;
	unsigned long int nanoSecDiff2;
	char c;
	unsigned int first_cmd = 0;
	unsigned int go_exit = 0;
	unsigned short int go1 = 0;
	unsigned short int go2 = 0;
	char word[256];
	
	struct timespec tps;
	
start :
	go1 = 0;
	go2 = 0;
	cmd = 0xFF;
	printf("\nHello LS2_RC PCIe  mem test app");
	printf("\n Test cases :\
	\n 1. Single 1M Write transfer from local buffer to S32V_EP mem (pattern = 0x%x)\
	\n 2. Single 1M Read  transfer from S32V_EP mem to local buffer\
	\n 3. Variable size throughput test Write(pattern = 0x%x) + Read to/from S32V_EP mem\
	\n 4. Fill local DDR_BASE + 1M with DW pattern 0x55AA55AA\
	\n 5. Read and print first and last 32DW(128bytes) in local DDR\
	\n 6. Multiple 1M Write transfers from local buffer to S32V_EP mem (pattern = 0x%x)\
	\n    This is essentially the same as #1, only looped and multithreaded.\
	\n    Usable for performance tests and some errata validation.\
	\n 7. Exit app\
	\n Select test : ",
	CMD1_PATTERN,
	CMD3_PATTERN,
	CMD6_PATTERN);

	/* Stop for command */
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
		default :
			/* Omit enter key */
			if (c != '\n')
				printf("\n Invalid input\n");
			break;
		}
	} while (!go1);

	printf("\n Command code  = %d \n", cmd);
	printf("\n mapsize = %x", mapsize);

	if (!first_cmd){
		first_cmd = 1;
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
			printf("%s %d\n", "Errors opening /dev/mem file ", errno);
			goto err;
		} else {
			printf("\n  /dev/mem file opened successfully");
		}
		
		/* MAP PCIe area */
		printf("\n EP_BAR2_ADDR = %llx", EP_BAR2_ADDR);
		mapPCIe = (unsigned int *)mmap(NULL, mapsize,
				PROT_READ | PROT_WRITE,
				MAP_SHARED, fd1, EP_BAR2_ADDR);
		if (!mapPCIe) {
			printf("\n /dev/mem PCIe area mapping  FAILED");
			goto err;
		} else {
			printf("\n /dev/mem PCIe area mapping  OK");
		}	
		
		/* MAP DDR free 1M area. This was reserved at boot time */
		mapDDR = (unsigned int *)mmap(NULL, MAP_DDR_SIZE,
				PROT_READ | PROT_WRITE,
				MAP_SHARED, fd1, RC_DDR_ADDR);
		if (!mapDDR) {
			printf("\n /dev/mem DDR area mapping FAILED");
			goto err;
		} else {
			printf("\n /dev/mem DDR area mapping OK");
		}
	}
	switch (cmd) {
	case 1: /* Write to PCIe area */
		memset(src_buff, CMD1_PATTERN, mapsize);
		memcpy((unsigned int *)mapPCIe, (unsigned int *)src_buff, mapsize);
		break;
	case 2 : /* Read from PCIe area */
		/* Clear local buffer*/
		memset(dest_buff, 0x0, mapsize);
		/* Copy from EP to local buffer */
		memcpy((unsigned int *)dest_buff, (unsigned int *)mapPCIe, mapsize);

		printf("\n First 32 DWORDS(1DW = 4bytes) copied from S32V_EP");
		for (i = 0 ; i < 32; i++) {
			printf("\n *(dest_buff + 0x%8x) = 0x%08x", i, *(dest_buff + i));
		}
		/* Resize for last 32 DWs */
		j = (mapsize / 4) - 32;
		printf("\n Last 32 DWORDS(1DW = 4bytes) copied from S32V_EP");
		for (i = j ; i < mapsize / 4; i++) {
			printf("\n *(dest_buff + 0x%8x) = 0x%08x", i, *(dest_buff + i));
		}
		break;
	case 3:
		memset(src_buff, CMD3_PATTERN, mapsize);
		memset(dest_buff, 0, mapsize);
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
	case 4 : 
		/* Fill local DDR_BASE + 1M with DW pattern 0x55AA55AA */
		for (i = 0 ; i < mapsize / 4 ; i++) {
			*(mapDDR + i) = (unsigned int)0x55AA55AA;
		}		
		break;		
	case 5 : 
		/* Read DDR area(minimal check). Can verify what EP has written */
		printf("\n First 32 DWORDS(1DW = 4 bytes) from local mapped DDR");
		for (i = 0 ; i < 32; i++) {
			printf("\n *(mapDDR + 0x%8x) = 0x%08x", i, *(mapDDR + i));
		}
		j = (mapsize / 4) - 32;
		printf("\n Last 32 DWORDS(1DW = 4bytes) from local mapped DDR");
		for (i = j ; i < mapsize / 4; i++) {
			printf("\n *(mapDDR + 0x%8x) = 0x%08x", i, *(mapDDR + i));
		}
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
