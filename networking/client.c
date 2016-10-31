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

#define SIZE 512
#define RED  "\x1B[31m"
#define YEL  "\x1B[33m"
#define RESET "\x1B[0m"

static void client_communication(int sock)
{
	int n, bytes_written, buff_len;
	char buff[SIZE];

	while (1) {
		printf(RED "Client: " RESET);
		/* Reading from the standard input. */
		if (fgets(buff, SIZE, stdin) == NULL) {
			printf("Error reading from stdin!");
			exit(1);
		}

		/* Sending data to the server. */
		buff_len = strlen(buff);
		bytes_written = 0;
		while (bytes_written < buff_len) {
			n = write(sock, buff + bytes_written,
					buff_len - bytes_written);
			if (n < 0)
				perror("Error writing to socket!");
			else
				bytes_written += n;
		}

		/* Reading from the socket. */
		n = read(sock, buff, SIZE - 1);
		if (n == 0) {
			printf("Server closed the connection\n");
			return;
		}
		if (n < 0)
			perror("Error reading from socket!");

		buff[n] = '\0';
		printf(YEL "Server: %s%s", RESET, buff);
	}
}

static void usage()
{
	fprintf(stderr, "Usage:\n\
		./client server_ip port_no\n\n\
		For more details see the readme file!\n\n");

}

int main(int argc, char *argv[])
{

	int sock, port_no, ret;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	/*
	 * Make a validation depending on the number of arguments
	 * and explain to the user the meaning of those arguments
	 */
	if (argc < 3) {
		usage();
		exit(1);
	}

	/*
	 * Convert port number to an integer value
	 */
	if (sscanf(argv[2], "%u", &port_no) <= 0) {
		usage();
		exit(1);
	}

	/*
	 * Create an endpoint for communication
	 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Couldn't open the socket!");
		exit(1);
	}

	/*
	 *  The gethostbyname() function returns a structure
	 * of type hostent for the given host name. Here 'name'
	 * is either a hostname, or an IPv4 address in standard
	 * dot notation
	 *  For more details about this function, please check
	 * into the Linux Programmer's Manual --
	 * https://www.cl.cam.ac.uk/cgi-bin/manpage?3+gethostbyname
	 */
	server = gethostbyname(argv[1]);

	if (server == NULL) {
		fprintf(stderr, "Invalid host!\n");
		exit(1);
	}

	memset(&serv_addr, '\0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;

	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

	serv_addr.sin_port = htons(port_no);
	ret = connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if ( ret < 0) {
		perror("The connection has been refused!");
		exit(1);
	}

	client_communication(sock);

	close(sock);
	return 0;
}
