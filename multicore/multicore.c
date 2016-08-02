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

#define LOOP_NUMBER 1923 
#define MAX_SIZE 20
#define PATH_MAX 100

int core_thread1, core_thread2, core_thread3, core_thread4, core_thread5;

int size;
int matrix[MAX_SIZE][MAX_SIZE], matrix1[MAX_SIZE][MAX_SIZE], 
	matrix2[MAX_SIZE][MAX_SIZE], matrix3[MAX_SIZE][MAX_SIZE], 
	matrix4[MAX_SIZE][MAX_SIZE], matrix5[MAX_SIZE][MAX_SIZE];


int main(){
	int k, count;
	struct timeval start, end;
	long unsigned time_without_threads, time_with_threads;

	pthread_t thread1, thread2, thread3, thread4, thread5;
	struct parameter thread1_args, thread2_args, thread3_args, thread4_args, thread5_args;

	
	cpu_set_t set;
	if(sched_getaffinity(0,sizeof(set), &set)==0){
		#ifdef CPU_COUNT
			count = CPU_COUNT(&set);
		#else
			size_t i;
			count = 0;
			for(i = 0; i < CPU_COUNT; i++)
				if(CPU_ISSET(i, &set))
					count++;
		#endif
	}
	printf("\nThe number of cores: %d", count);



	
	printf("\nThe online cores : ");
	printf("\n");
	system("cat /sys/devices/system/cpu/online");

	printf("The offline cores : ");
	printf("\n");
	system("cat /sys/devices/system/cpu/offline");

	gettimeofday(&start, NULL);//start timestamp

	make_matrix("matrix1.in", thread1_args.matrix1);
	thread1_args.id_thread = 1;
	thread1_args.size = size;
	make_matrix("matrix2.in", thread2_args.matrix2);
	thread2_args.id_thread = 2;
	thread2_args.size = size;
	make_matrix("matrix3.in", thread3_args.matrix3);
	thread3_args.id_thread = 0;
	thread3_args.size = size;
	make_matrix("matrix4.in", thread4_args.matrix4);
	thread4_args.id_thread = 4;
	thread4_args.size = size;
	make_matrix("matrix5.in", thread5_args.matrix5);
	thread5_args.id_thread = 3;
	thread5_args.size = size;

	for(k=0; k<LOOP_NUMBER; k++){
		multi_matrix(matrix1, thread1_args.size);
		multi_matrix(matrix2, thread2_args.size);
		multi_matrix(matrix3, thread3_args.size);
		multi_matrix(matrix4, thread4_args.size);
		multi_matrix(matrix5, thread5_args.size);
	}
			
	gettimeofday(&end, NULL);//end timestamp
	/*
	 * We make the difference between
	 * the end timestamp and the start timestamp
	 */ 
	time_without_threads = end.tv_usec - start.tv_usec;	

	/* 
	 * This isn't black magic! 
	 * It displays some red content into the linux console
	 */
	printf("\nWithout threads:\033[1;31m- - - > %lums\n\033[0m\n", time_without_threads); 

	/*
	 * Threads implementation
	 */
	gettimeofday(&start, NULL);//start timestamp

	if (pthread_create(&thread1, NULL, &thread_multi_matrix, &thread1_args)){
		perror("pthread_create");
		exit(1);
	}
			
 	if (pthread_create(&thread2, NULL, &thread_multi_matrix, &thread2_args)){
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread3, NULL, &thread_multi_matrix, &thread3_args)){
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread4, NULL, &thread_multi_matrix, &thread4_args)){
		perror("pthread_create");
		exit(1);
	}

	if (pthread_create(&thread5, NULL, &thread_multi_matrix, &thread5_args)){
		perror("pthread_create");
		exit(1);
	}

	/*
	 * We'll join all threads in order to finish our purpose
	 */
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

	gettimeofday(&end, NULL);//end timestamp
	
	time_with_threads = end.tv_usec - start.tv_usec;

	printf("\nWith threads: \033[1;31m - - - > %lums\n\033[0m\n", time_with_threads);

	return 0;
}
