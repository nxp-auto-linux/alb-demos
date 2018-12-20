/*
 * Copyright (C) 2016 NXP Semiconductors
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:		BSD-3-Clause
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "gpio_func.h"

#define GPIO_EXPORT_PATH	"/sys/class/gpio/export"
#define GPIO_UNEXPORT_PATH	"/sys/class/gpio/unexport"

#define SYS_GPIO_FOLDER		"/sys/class/gpio/"
#define GPIO			"gpio"
#define GPIO_VALUE		"/value"
#define GPIO_DIRECTION		"/direction"

#define GPIO_DIR_IN		"in"
#define GPIO_DIR_OUT		"out"

#define READ			"r"
#define WRITE			"w"
#define ERROR			-1

static unsigned get_ndigits(unsigned n)
{
	int digits = 0;

	while (n) {
		digits++;
		n /= 10;
	}
	return digits;
}

int export_gpio(unsigned pin_no)
{
	FILE *export_file = NULL;

	/* Open for exporting the new pin value */
	export_file = fopen(GPIO_EXPORT_PATH, WRITE);
	if (!export_file) {
		printf("Failed to open `" GPIO_EXPORT_PATH "`\n");
		return ERROR;
	}

	/* Export pin */
	fprintf(export_file, "%d", pin_no);

	/* Closing the file descriptor */
	fclose(export_file);
	return 0;
}

int unexport_gpio(unsigned pin_no)
{
	FILE *unexport_file = NULL;

	/* Open for unexporting the pin with a given value  */
	unexport_file = fopen(GPIO_UNEXPORT_PATH, WRITE);
	if (!unexport_file) {
		printf("Failed to open `" GPIO_UNEXPORT_PATH "`\n");
		return ERROR;
	}

	/* Unexport pin */
	fprintf(unexport_file, "%d", pin_no);

	/* Closing the file descriptor */
	fclose(unexport_file);
	return 0;
}

int set_direction(unsigned pin_no, enum gpio_dir dir)
{
	FILE *set_dir = NULL;
	unsigned pin_chars = get_ndigits(pin_no);
	size_t dir_path_len =
	    strlen(SYS_GPIO_FOLDER) + strlen(GPIO) + pin_chars +
	    strlen(GPIO_DIRECTION) + 1;

	char *dir_path = calloc(dir_path_len, sizeof(*dir_path));

	/* Build a path to the GPIO pin wanted */
	snprintf(dir_path, dir_path_len,
		 SYS_GPIO_FOLDER GPIO "%d" GPIO_DIRECTION,
		 pin_no);

	/* Open for write into the file descriptor */
	set_dir = fopen(dir_path, WRITE);
	free(dir_path);
	if (!set_dir) {
		printf("Failed to open `%s`\n", dir_path);
		return ERROR;
	}

	/* Writing into the direction file of the gpio */
	fprintf(set_dir, "%s", dir == IN ? GPIO_DIR_IN : GPIO_DIR_OUT);

	/* Closing the file descriptor */
	fclose(set_dir);
	return 0;
}

static int gpio_value(unsigned pin_no, int val)
{
	FILE *gpio_file = NULL;
	unsigned pin_chars = get_ndigits(pin_no);
	const char *file_mod;

	size_t pin_length =
	    strlen(SYS_GPIO_FOLDER) + strlen(GPIO) + pin_chars +
	    strlen(GPIO_VALUE) + 1;

	char *pin_path = calloc(pin_length, sizeof(*pin_path));

	snprintf(pin_path, pin_length, SYS_GPIO_FOLDER GPIO "%d" GPIO_VALUE,
		 pin_no);

	if (val == -EINVAL)
		file_mod = READ;
	else
		file_mod = WRITE;

	gpio_file = fopen(pin_path, file_mod);
	free(pin_path);
	if (!gpio_file) {
		printf("Failed to open `%s`\n", pin_path);
		return ERROR;
	}

	/* Writing into the value file of the gpio */
	if (val == -EINVAL)
		fscanf(gpio_file, "%d", &val);
	else
		fprintf(gpio_file, "%d", val);

	/* Closing the file descriptor */
	fclose(gpio_file);
	return val;
}

int set_value(unsigned pin_no, unsigned val)
{
	return gpio_value(pin_no, val & 0x1) == ERROR ? ERROR : 0;
}

int get_value(unsigned pin_no)
{
	return gpio_value(pin_no, -EINVAL);
}
