/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#define _GNU_SOURCE
#include "functions.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <errno.h>

/* 
 * It will make a bidimensional array of integer values with 
 * the number of columns equal with the number of lines 
 */
void make_matrix(char path[], int matrix[MAX_SIZE][MAX_SIZE]){
	int i, j;
	FILE *f;

	f = fopen(path, "r+");
	if(!f){
		printf("Could not read from matrix.in files!");
	}

	fscanf(f, "%d", &size);
	for(i = 0; i < size; i++)
		for(j = 0; j<size; j++){
			fscanf(f, "%d", &matrix[i][j]);
		}
}

/*
 * This is the function to displays a  
 * matrix with a specified size as input
 */
void display_matrix(int matrix[MAX_SIZE][MAX_SIZE], int size){
	int i, j;
	
	for(i = 0; i < size; i++){
		for(j = 0; j < size; j++)
			printf(" %d ", matrix[i][j]);
		printf("\n");
	}

}

/*
 * This is the algorithm for the square of 
 * the bidimensional array given as an argument
 */
void multi_matrix(int matrix[MAX_SIZE][MAX_SIZE], int size){
	int i, j, k;
	int matrix_result[MAX_SIZE][MAX_SIZE];

	for(i = 0; i < size; i++)
		for(j = 0; j < size; j++){
			matrix_result[i][j]=0;
			for(k = 0; k < size; k++)
				matrix_result[i][j] += (matrix[i][k] * matrix[k][j]);
		}

	for(i = 0; i < size; i++)
		for(j = 0; j < size; j++){
			matrix[i][j] = matrix_result[i][j];
		}
	//display_matrix(matrix_result, size);
}

/*
 * Square function for threads implementation
 */
void* thread_multi_matrix(void *params)
{
	struct parameter* p = (struct parameter*) params;
	int i,j,k;
	int matrix_result[MAX_SIZE][MAX_SIZE];

	cpu_set_t cpuset; //the CPU we want to use
	CPU_ZERO(&cpuset); //clears the cpuset
	CPU_SET( p->id_thread , &cpuset); //set CPU 
	/*
	 * cpu affinity for the calling thread 
	 * first parameter is the pid, 0 = calling thread
	 * second parameter is the size of your cpuset
	 * third param is the cpuset in which your thread will be
	 * placed. Each bit represents a CPU
	 */
	sched_setaffinity(0, sizeof(cpuset), &cpuset);
    /*
     *  This is a piece of code for debugging
	 *	if (rc != 0) {
	 *	   printf("GET == %d", sched_getaffinity(0, sizeof(cpuset), &cpuset));
	 *		  for(i=0; i<=3; ++i){
	 *  			if(CPU_ISSET(i, &cpuset))
	 *					printf("Can run on %d", i);
	 *		  }
	 *	}	
	 */

	/*
	 * This is the classical algorithm for multiplication 
	 */
	for(i = 0; i < p->size; i++)
		for(j = 0; j < p->size; j++){
			matrix_result[i][j]=0;
			for(k = 0; k < p->size; k++)
				matrix_result[i][j] += (p->matrix[i][k] * p->matrix[k][j]);
		}

	/*
	 * Because we use an alternative  bidimensional array 'matrix_result',
	 * we must copy into the 'matrix' the result of our multiplication
	 */
	for(i = 0; i < p->size; i++)
		for(j = 0; j < p->size; j++){
			p->matrix[i][j] = matrix_result[i][j];
		}
	/*
	 * Let's print the core
	 */	
	printf("---Thread %d---\nThe number of the core  - %d\n", 
		p->id_thread, sched_getcpu());

	return NULL;
}
