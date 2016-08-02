/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <errno.h>

#define LOOP_NUMBER 1923 
#define MAX_SIZE 20
int core_thread1, core_thread2, core_thread3, core_thread4, core_thread5;

int size;
int matrix[MAX_SIZE][MAX_SIZE], matrix1[MAX_SIZE][MAX_SIZE], 
	matrix2[MAX_SIZE][MAX_SIZE], matrix3[MAX_SIZE][MAX_SIZE], 
	matrix4[MAX_SIZE][MAX_SIZE], matrix5[MAX_SIZE][MAX_SIZE];


struct parameter{

	int size, id_thread;
	int matrix[MAX_SIZE][MAX_SIZE], matrix1[MAX_SIZE][MAX_SIZE], 
		matrix2[MAX_SIZE][MAX_SIZE], matrix3[MAX_SIZE][MAX_SIZE], 
		matrix4[MAX_SIZE][MAX_SIZE], matrix5[MAX_SIZE][MAX_SIZE];
};


/*
 * Prototypes for the functions 
 */
void make_matrix(char path[], int matrix[MAX_SIZE][MAX_SIZE]);
void* thread_multi_matrix(void *params);
void display_matrix(int matrix[MAX_SIZE][MAX_SIZE], int size);
void multi_matrix(int matrix[MAX_SIZE][MAX_SIZE], int size);


#endif
