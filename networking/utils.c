/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define PENDING_CONNECTIONS	10

int rread(int fd, void *buf, size_t count) {
	ssize_t n;
	ssize_t bytes_read = 0;

	if (!buf) {
		errno = EINVAL;
		return -errno;
	}

	while (bytes_read < count) {
		n = read(fd, buf + bytes_read,
				count - bytes_read);
		if (n == 0) {
			/* EOF */
			return count;
		}
		if (n < 0)
			return -1;

		bytes_read += n;
	}
	return count;
}

int wwrite(int fd, const void *buf, size_t count) {
	ssize_t n;
	ssize_t bytes_written = 0;

	if (!buf) {
		errno = EINVAL;
		return -errno;
	}

	while (bytes_written < count) {
		n = write(fd, buf + bytes_written,
				count - bytes_written);
		if (n < 0)
			return -1;

		bytes_written += n;
	}
	return count;
}

int create_listen_sock(unsigned int port)
{
	int listen_sock;
	struct sockaddr_in server;

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock < 0) {
		perror("socket");
		return -errno;
	}

	memset(&server, 0, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	/* Bind */
	if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("bind");
		close(listen_sock);
		return -1;
	}

	/* Listen */
	if (listen(listen_sock, PENDING_CONNECTIONS) != 0) {
		perror("listen");
		close(listen_sock);
		return -1;
	}

	return listen_sock;
}
