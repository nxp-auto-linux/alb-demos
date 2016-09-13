/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SIZE 512
#define RED  "\x1B[31m"
#define YEL  "\x1B[33m"
#define RESET "\x1B[0m"

#define PENDING_CONNECTIONS	1

static void server_communication(int new_sock)
{
	int n, bytes_written, buff_len;
	char buff[SIZE];

	while (1) {
		/*
		 * Reading data from the socket
		 */
		n = read(new_sock, buff, SIZE - 1);
		if (n == 0) {
			printf("Client closed the connection\n");
			return;
		}
		if (n < 0)
			perror("Error reading from socket");
		buff[n] = '\0';

		printf(RED "Client: %s%s", RESET, buff);
		printf(YEL "Server: " RESET);

		/*
		 * Reading from the standard input
		 */
		if (fgets(buff, SIZE, stdin) == NULL) {
			printf("Error reading from stdin!");
			exit(1);
		}
		/*
		 * Sending data to the client
		 */
		buff_len = strlen(buff);
		bytes_written = 0;
		while (bytes_written < buff_len) {
			n = write(new_sock, buff + bytes_written,
					buff_len - bytes_written);
			if (n < 0)
				perror("Error writing to socket!");
			else
				bytes_written += n;
		}
	}
}

static void usage()
{
	fprintf(stderr, "Usage:\n\
		./server port_no\n\n\
		For more details see the readme file!\n\n");
}

int main(int argc, char *argv[])
{
	int new_sock, sock, port_no;
	struct sockaddr_in server;

	/*
	 * Validating the number of arguments
	 */
	if (argc < 2) {
		usage();
		exit(1);
	}

	/*
	 * Convert port number to an integer value
	 */
	if (sscanf(argv[1], "%u", &port_no) <= 0) {
		usage();
		exit(1);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error opening socket!");
		exit(1);
	}

	memset(&server, '\0', sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port_no);

	if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Error on binding");
		exit(1);
	}

	if (listen(sock, PENDING_CONNECTIONS) != 0) {
		perror("Error on listening the socket!");
		exit(1);
	}

	new_sock = accept(sock, NULL, NULL);

	if (new_sock < 0) {
		perror("Error on accept!");
		exit(1);
	}

	server_communication(new_sock);

	close(new_sock);
	close(sock);
	return 0;
}
