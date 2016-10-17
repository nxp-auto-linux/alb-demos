/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <errno.h>
#include "functions.h"

#define MAX_SIZE 20
#define PATH_MAX 100
#define RED  "\x1B[31m"
#define RESET "\x1B[0m"

#define SYSTEM_CPU_ONLINE	"/sys/devices/system/cpu/online"
#define SYSTEM_CPU_OFFLINE	"/sys/devices/system/cpu/offline"

#define READ		"r"
#define WRITE		"w"
#define ERROR		-1

int main(int argc, char *argv[])
{
	int count, loop_no = atoi(argv[1]);
	char str[MAX_SIZE];
	struct timeval start, end;
	long long unsigned time_without_threads, time_with_threads;
	FILE *cpu_available = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage:\n"
			"%s loop_number\n\n"
			"For more details see the readme file!\n\n", argv[0]);
		exit(1);
	}

	pthread_t thread1, thread2, thread3, thread4, thread5;
	struct parameter thread1_args, thread2_args, thread3_args, thread4_args,
	    thread5_args;

	cpu_set_t set;
	if (sched_getaffinity(0, sizeof(set), &set) == 0) {
#ifdef CPU_COUNT
		count = CPU_COUNT(&set);
#else
		size_t i;
		count = 0;
		for (i = 0; i < CPU_COUNT; i++)
			if (CPU_ISSET(i, &set))
				count++;
#endif
	}

	printf("\nThe number of cores: %d", count);
	printf("\nThe online cores : \n");

	/* Open for reading the online cores */
	cpu_available = fopen(SYSTEM_CPU_ONLINE, READ);
	if (!cpu_available) {
		printf("Failed to open `" SYSTEM_CPU_ONLINE "`\n");
		return ERROR;
	}
	if (fgets(str, MAX_SIZE, cpu_available) != NULL)
		puts(str);
	fclose(cpu_available);

	printf("The offline cores : \n");

	/* Open for writing the online cores */
	cpu_available = fopen(SYSTEM_CPU_OFFLINE, READ);
	if (!cpu_available) {
		printf("Failed to open `" SYSTEM_CPU_OFFLINE "`\n");
		return ERROR;
	}
	if (fgets(str, MAX_SIZE, cpu_available) != NULL)
		puts(str);
	fclose(cpu_available);

	if (make_matrix("matrix1.in", thread1_args.matrix, &thread1_args.size)
	    != 0)
		printf("Could not read from matrix1.in files!");
	thread1_args.id_thread = 1;
	thread1_args.loop_no = loop_no;
	if (make_matrix("matrix2.in", thread2_args.matrix, &thread2_args.size)
	    != 0)
		printf("Could not read from matrix2.in files!");
	thread2_args.id_thread = 2;
	thread2_args.loop_no = loop_no;
	if (make_matrix("matrix3.in", thread3_args.matrix, &thread3_args.size)
	    != 0)
		printf("Could not read from matrix3.in files!");
	thread3_args.id_thread = 0;
	thread3_args.loop_no = loop_no;
	if (make_matrix("matrix4.in", thread4_args.matrix, &thread4_args.size)
	    != 0)
		printf("Could not read from matrix4.in files!");
	thread4_args.id_thread = 4;
	thread4_args.loop_no = loop_no;
	if (make_matrix("matrix5.in", thread5_args.matrix, &thread5_args.size)
	    != 0)
		printf("Could not read from matrix5.in files!");
	thread5_args.id_thread = 3;
	thread5_args.loop_no = loop_no;

	/* start timestamp */
	gettimeofday(&start, NULL);

	multi_matrix(thread1_args.matrix, thread1_args.size,
		     thread1_args.loop_no);
	multi_matrix(thread2_args.matrix, thread2_args.size,
		     thread2_args.loop_no);
	multi_matrix(thread3_args.matrix, thread3_args.size,
		     thread3_args.loop_no);
	multi_matrix(thread4_args.matrix, thread4_args.size,
		     thread4_args.loop_no);
	multi_matrix(thread5_args.matrix, thread5_args.size,
		     thread5_args.loop_no);

	/* end timestamp */
	gettimeofday(&end, NULL);

	/**
	 * We make the difference between
	 * the end timestamp and the start timestamp
	 */
	time_without_threads = (end.tv_sec - start.tv_sec) * 1000 +
	    (end.tv_usec - start.tv_usec) / 1000;

	/* It displays some red content into the linux console */
	printf(RED "\nWithout threads- - - > %llums\n", time_without_threads);
	printf(RESET);

	/* Threads implementation */

	/* start timestamp */
	gettimeofday(&start, NULL);

	if (pthread_create(&thread1, NULL, &thread_multi_matrix, &thread1_args)) {
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread2, NULL, &thread_multi_matrix, &thread2_args)) {
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread3, NULL, &thread_multi_matrix, &thread3_args)) {
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread4, NULL, &thread_multi_matrix, &thread4_args)) {
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread5, NULL, &thread_multi_matrix, &thread5_args)) {
		perror("pthread_create");
		exit(1);
	}

	/* We'll join all threads in order to finish our purpose */
	if (pthread_join(thread1, NULL))
		perror("pthread_join");

	if (pthread_join(thread2, NULL))
		perror("pthread_join");

	if (pthread_join(thread3, NULL))
		perror("pthread_join");

	if (pthread_join(thread4, NULL))
		perror("pthread_join");

	if (pthread_join(thread5, NULL))
		perror("pthread_join");

	/* end timestamp */
	gettimeofday(&end, NULL);

	time_with_threads = (end.tv_sec - start.tv_sec) * 1000 +
	    (end.tv_usec - start.tv_usec) / 1000;

	printf(RED "\nWith threads- - - > %llums\n\n", time_with_threads);
	printf(RESET);

	return 0;
}
