/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <sys/socket.h>
#include <netdb.h>
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

#define QUIT_CMD "quit"

static int client_communication(int sock, char **line, size_t *line_len)
{
	int n, len;

	printf(RED "Client: " RESET);

	/* Reading from the standard input. */
	len = getline(line, line_len, stdin);
	if (len < 0) {
		printf("Error reading from stdin!");
		return -1;
	}

	/* Empty line */
	if (len == 1)
		return 0;

	/* Remove endline. */
	(*line)[len - 1] = '\0';

	/* Check for quit command. */
	if (strcmp(*line, QUIT_CMD) == 0)
		return -1;

	/* Don't send more than SIZE characters */
	if (len >= SIZE) {
		(*line)[SIZE - 1] = '\0';
		len = SIZE;
	}

	/* Sending data to the server. */
	n = wwrite(sock, *line, len);
	if (n < 0) {
		perror("Error writing to socket!");
		return -1;
	}

	/* Reading from the socket. */
	n = read(sock, *line, SIZE);
	if (n == 0) {
		printf("Server closed the connection\n");
		return -1;
	}

	if (n < 0) {
		perror("read");
		return -1;
	}

	(*line)[n] = '\0';
	printf(YEL "Server: " RESET "%s\n", *line);
	return 0;
}

static void usage(char *executable_name)
{
	fprintf(stderr, "Usage: %s server_ip port_no\n"
			"For more details see the readme file!\n",
			executable_name);

}

int main(int argc, char *argv[])
{
	int sock, ret;
	struct sockaddr_in serv_addr;
	struct addrinfo hints, *res;
	char *temp;
	unsigned int port_no;
	char *line = NULL;
	size_t line_len = SIZE;

	/* Make a validation depending on the number of arguments */
	if (argc < 3) {
		usage(argv[0]);
		return -EINVAL;
	}

	/* Convert port number to an integer value */
	port_no = strtoul(argv[2], &temp, 0);
	if (port_no == 0 || *temp != '\0') {
		usage(argv[0]);
		return -EINVAL;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	ret = getaddrinfo(argv[1], NULL, &hints, &res);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -EINVAL;
	}

	serv_addr = *((struct sockaddr_in *)res->ai_addr);
	freeaddrinfo(res);

	/*
	 * Create an endpoint for communication
	 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return -errno;
	}

	serv_addr.sin_port = htons(port_no);
	ret = connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (ret < 0) {
		perror("connect");
		ret = -errno;
		goto close_socket;
	}

	line = malloc(sizeof(*line) * line_len);
	if (!line) {
		ret = -ENOMEM;
		fprintf(stderr, "Out of memory !\n");
		goto close_socket;
	}

	ret = EXIT_SUCCESS;
	while (!client_communication(sock, &line, &line_len));

	if (line)
		free(line);

close_socket:
	/* Free used resources */
	close(sock);
	return ret;
}
