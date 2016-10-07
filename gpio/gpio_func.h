/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:		BSD-3-Clause
 */

#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

enum gpio_dir {
	IN, OUT
};

/**
 * Export a GPIO pin
 * @param pin_no: The ID of the GPIO pin
 * @return 0 for success or an error code otherwise
 */
int export_gpio(unsigned pin_no);

/**
 * Unexport a GPIO pin
 * @param pin_no: The ID of the GPIO pin
 * @return 0 for success or an error code otherwise
 */
int unexport_gpio(unsigned pin_no);

/**
 * Set the direction of a GPIO pin
 * @param pin_no: The ID of the GPIO pin
 * @param dir: The direction : IN / OUT
 * @return 0 for success or an error code otherwise
 */
int set_direction(unsigned pin_no, enum gpio_dir dir);

/**
 * Set a valuei (0 or 1) to a GPIO pin.
 * The specified pin must be exported before.
 * @param pin_no: The ID of the GPIO pin
 * @param val: The value to be written
 * @return 0 for success or an error code otherwise
 */
int set_value(unsigned pin_no, unsigned val);

/**
 * Return the value of a GPIO pin.
 * The specified pin must be exported before.
 * @param pin_no: The ID of the GPIO pin
 * @return 0, 1 or an error code
 */
int get_value(unsigned pin_no);

#endif
