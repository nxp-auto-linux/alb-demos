/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

#define SIZE 64
#define RED  "\x1B[31m"
#define YEL  "\x1B[33m"
#define RESET "\x1B[0m"

#define PENDING_CONNECTIONS	1

static int server_communication(int client_sock, char **line, size_t *line_len)
{
	int n, len;

	/*
	 * Reading data from the socket
	 */
	n = read(client_sock, *line, SIZE);
	if (n == 0) {
		printf("Client closed the connection\n");
		return -1;
	}
	if (n < 0) {
		perror("read");
		return -errno;
	}

	(*line)[n] = '\0';

	printf(RED "Client: %s%s\n", RESET, *line);

	do {
		printf(YEL "Server: " RESET);

		/* Reading from the standard input. */
		len = getline(line, line_len, stdin);
		if (len < 0) {
			printf("Error reading from stdin!");
			return -1;
		}

	} while (len == 1);

	/* Remove endline. */
	(*line)[len - 1] = '\0';

	/* Don't send more than SIZE characters */
	if (len >= SIZE) {
		(*line)[SIZE - 1] = '\0';
		len = SIZE;
	}

	/*
	 * Sending data to the client
	 */
	n = wwrite(client_sock, *line, len);
	if (n < 0) {
		perror("write!");
		return -1;
	}

	return 0;
}

static void usage(char *executable_name)
{
	fprintf(stderr, "Usage:	%s port_no\n"
		"For more details see the readme file!\n",
		executable_name);
}

int main(int argc, char *argv[])
{
	int client_sock, sock, port_no;
	char *temp;
	int ret = EXIT_SUCCESS;
	char *line = NULL;
	size_t line_len = SIZE;

	/* Validating the number of arguments */
	if (argc < 2) {
		usage(argv[0]);
		return -EINVAL;
	}

	/* Convert port number to an integer value */
	port_no = strtoul(argv[1], &temp, 0);
	if (port_no == 0 || *temp != '\0') {
		usage(argv[0]);
		return -EINVAL;
	}

	sock = create_listen_sock(port_no);
	if (sock < 0)
		return -errno;

	client_sock = accept(sock, NULL, NULL);
	if (client_sock < 0) {
		perror("accept");
		goto close_socket;
	}

	line = malloc(sizeof(*line) * line_len);
	if (!line) {
		ret = -ENOMEM;
		fprintf(stderr, "Out of memory !\n");
		goto close_socket;
	}

	while (!server_communication(client_sock, &line, &line_len));

	if (line)
		free(line);

	/* Free used resources */
	close(client_sock);
close_socket:
	close(sock);
	return ret;
}
