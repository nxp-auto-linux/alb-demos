/**
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#define MAX_SIZE		20

struct parameter {
	int loop_no;
	int size, id_thread;
	int matrix[MAX_SIZE][MAX_SIZE];
};

/* Prototypes for the functions */

/**
 * It will make a bidimensional array of integer values with
 * the number of columns equal with the number of lines
 * @param path[] is the name of the file which contains the matrix
 * @param matrix[][] is the bidimentional array created
 * @param size is the size of the matrix
 * return 0 for success or -1 on failure
 */
int make_matrix(char path[], int matrix[MAX_SIZE][MAX_SIZE], int *size);

/**
 * Square function for thread implementation 
 * @param loop_no is the number of loops given as an argument
 * @param size is the size of the matrix
 * @param id_thread is the thread id which is used in threads distribution
 * @param matrix[][] is the bidimentional array created
 */
void *thread_multi_matrix(void *params);

/**
 * This is the function to displays a
 * matrix with a specified size as input
 * @param matrix[][] is the bidimentional array that will be displayed
 * @param size is the size of it
 */
void display_matrix(int matrix[MAX_SIZE][MAX_SIZE], int size);

/**
 * This is the algorithm for the square of
 * the bidimensional array given as an argument
 * @param matrix[][] is the bidimentional array that will be multiplied
 * @param size is the size of it
 * @param loop_no is the number of the loops for the multiplication operation
 * return 0 on success or -1 on failure
 */
void multi_matrix(int matrix[MAX_SIZE][MAX_SIZE], int size, int loop_no);

#endif
