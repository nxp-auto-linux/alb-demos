/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

/**
 * Read from a socket
 * @param fd: socket to read from
 * @param buf: Pointer to a buffer to store the read data
 * @param count: Number of bytes to read
 * @return Number of bytes read. Is equal to count on success, a negative value otherwise
 */
int rread(int fd, void *buf, size_t count);

/**
 * Write to a socket
 * @param fd: socket to write to
 * @param buf: Pointer to data to be written
 * @param count: Number of bytes to write
 * @return Number of bytes written. Is equal to count on success, a negative value otherwise
 */
int wwrite(int fd, const void *buf, size_t count);

/**
 * Create and bind a socket to the specified port number, and start listening on it.
 * @param port: The port number to bind the socket to
 * @return Returns the created socket or a negative value on failure
 */
int create_listen_sock(unsigned int port);
