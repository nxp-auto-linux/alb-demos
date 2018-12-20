/*
 * Copyright (C) 2016 NXP Semiconductors
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:		BSD-3-Clause
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gpio_func.h"

#define RED "\x1B[31m"
#define RESET "\x1B[0m"

static unsigned pin;

static void usage()
{
	fprintf(stderr, "Usage:\
		./gpio pin_no value --- Set pin to output a specified value)\n\
		./gpio pin_no       --- Set pin to input and read status\n");
}

static void sigintHandler(int signum)
{
	unexport_gpio(pin);
	exit(signum);
}

int main(int argc, char *argv[])
{
	int value;
	unsigned pin_val;
	int status;

	if (argc >= 2)
		if (sscanf(argv[1], "%u", &pin) <= 0) {
			usage();
			return -EINVAL;
		}

	if (argc == 2) {
		/* Just an input pin */
		status = export_gpio(pin);
		if (status)
			return status;

		status = set_direction(pin, IN);
		if (status)
			return status;

		/* Unregister the pin after a CTR+C */
		signal(SIGINT, sigintHandler);

		while (1) {
			pin_val = get_value(pin);
			if (pin_val >= 0)
				printf(RED "GPIO %d : %d" RESET "\r",
					pin, pin_val);
			fflush(stdout);

			sleep(1);
		}
	} else if (argc == 3) {
		if (sscanf(argv[2], "%d", &value) <= 0) {
			usage();
			return -EINVAL;
		}
		status = export_gpio(pin);
		if (status)
			return status;

		status = set_direction(pin, OUT);
		if (status)
			return status;

		status = set_value(pin, value);
		if (status)
			return status;

		unexport_gpio(pin);
	} else {
		usage();
		return -EINVAL;
	}

	return 0;
}
