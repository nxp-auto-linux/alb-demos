/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier:		BSD-3-Clause
 */

#include <errno.h>
#include <gpiod.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define CONSUMER    "GPIO-Demo"

static struct gpiod_chip *chip;
static struct gpiod_line *line;

static void sigintHandler(int signum)
{
	gpiod_line_release(line);
	gpiod_chip_close(chip);
	exit(signum);
}

static void usage(char *name)
{
	fprintf(stderr, "Usage:\
		%s <chip> <pin> <value>   --- Set pin to output with an initial value (0 or 1)\n\
		%s <chip> <pin>           --- Set pin to input and read status\n\
		e.g.: %s gpiochip0 26 1\n", name, name, name);
}

int main(int argc, char *argv[])
{
	unsigned int pin_offset;
	int ret = 0, pin_val;
	bool is_input = true;

	if (argc != 3 && argc != 4) {
		usage(argv[0]);
		return -EINVAL;
	}

	if (sscanf(argv[2], "%u", &pin_offset) <= 0) {
		fprintf(stderr, "Invalid pin offset provided: %s\n", argv[2]);
		usage(argv[0]);
		return -EINVAL;
	}

	if (argc == 4) {
		is_input = false;
		if (sscanf(argv[3], "%d", &pin_val) <= 0) {
			fprintf(stderr, "Invalid pin value provided: %s\n", argv[3]);
			usage(argv[0]);
			return -EINVAL;
		}

		if (pin_val != 0 && pin_val != 1) {
			fprintf(stderr, "Pin value must be 0 or 1\n");
			return -EINVAL;
		}
	}

	chip = gpiod_chip_open_by_name(argv[1]);
	if (!chip) {
		fprintf(stderr, "Could not open %s\n", argv[1]);
		return -ENODEV;
	}

	line = gpiod_chip_get_line(chip, pin_offset);
	if (!line) {
		fprintf(stderr, "Could not get line for pin %u\n", pin_offset);
		ret = -EINVAL;
		goto err_chip;
	}

	if (is_input) {
		/* input pin */
		ret = gpiod_line_request_input(line, CONSUMER);
		if (ret)
			goto exit;

		/* Unregister the pin after a CTRL+C */
		signal(SIGINT, sigintHandler);

		while (1) {
			pin_val = gpiod_line_get_value(line);
			if (pin_val >= 0)
				printf("GPIO %d : %d\r", pin_offset, pin_val);
			fflush(stdout);

			sleep(1);
		}
	} else {
		/* output pin */
		ret = gpiod_line_request_output(line, CONSUMER, 0);
		if (ret)
			goto exit;

		for (int i = 0; i < 50; i++) {
			ret = gpiod_line_set_value(line, pin_val);
			if (ret)
				goto exit;

			sleep(1);
			pin_val ^= 1;
		}
	}

exit:
	gpiod_line_release(line);
err_chip:
	gpiod_chip_close(chip);
	return ret;
}

