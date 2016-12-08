/*
 * Copyright (C) 2016 NXP Semiconductors
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

#define RECV_MSG_SIZE 64
#define RED  "\x1B[31m"
#define YEL  "\x1B[33m"
#define RESET "\x1B[0m"

#define MAX_FDS 64
#define WAIT_FOR_EVER -1

#define DISCONNECT 1

/*
 * Server communication with a client specified by his
 * index number from the client_sock list
 */
static int server_communication(int client_sock)
{
	int n;
	static char buff_recv[RECV_MSG_SIZE];

	/* Read data from client */
	n = read(client_sock, buff_recv, RECV_MSG_SIZE);

	/* If client disconnected */
	if (n == 0)
		return DISCONNECT;

	if (n == -1) {
		perror("read");
		return -errno;
	}

	buff_recv[n] = '\0';
	printf(RED "Client%d: " RESET "%s\n", client_sock, buff_recv);

	/* Send data to client */
	n = wwrite(client_sock, buff_recv, strlen(buff_recv));
	if (n < 0) {
		perror("write");
		return -errno;
	}

	return 0;
}

/* Usage function for server side */
static void usage(char *executable_name)
{
	fprintf(stderr, "Usage: %s port_no\n"
			"For more details see the readme file!\n",
			executable_name);
}

int main(int argc, char *argv[])
{
	int listen_sock, i, j, err;
	unsigned int port_no;
	struct pollfd fds[MAX_FDS];
	int poll_size, temp_fd;
	char *temp;
	int fds_nr = 0;
	int ret = EXIT_SUCCESS;

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

	listen_sock = create_listen_sock(port_no);
	if (listen_sock < 0)
		return -errno;

	/* Add listen_sock */
	fds[fds_nr].fd = listen_sock;
	fds[fds_nr].events = POLLIN;
	fds_nr++;

	/* Server */
	while (1) {
		poll_size = fds_nr;

		/*
		 * Wait for available data on any of the file descriptors
		 * from fds.
		 */
		ret = poll(fds, poll_size, WAIT_FOR_EVER);
		if (ret < 0) {
			perror("poll");
			ret = -errno;
			goto close_sockets;
		}

		/* Iterate through current file descriptors so use poll_size
		 * instead of fds_nr which can be modified when a new client
		 * appears.
		 */
		for (i = 0; i < poll_size; i++) {
			if (fds[i].revents == 0)
				/* No event on this fd */
				continue;

			if (fds[i].revents != POLLIN) {
				/* Error */
				fprintf(stderr, "revents = %x for socket %d\n",
						fds[i].revents, i);
				ret = -EINVAL;
				goto close_sockets;
			}

			if (fds[i].fd == listen_sock && fds_nr < MAX_FDS) {
				/* Accept new client */
				temp_fd = accept(listen_sock, 0, 0);
				if (temp_fd < 0) {
					perror("accept");
					ret = -errno;
					goto close_sockets;
				}

				/* New client accepted */
				printf("Client%d connected\n", temp_fd);
				fds[fds_nr].fd = temp_fd;
				fds[fds_nr].events = POLLIN;
				fds_nr++;
			} else if (fds[i].fd != listen_sock) {
				/* Message from this client */
				err = server_communication(fds[i].fd);
				if (err == DISCONNECT) {
					printf("Client%d disconnected!\n", fds[i].fd);

					/* Remove client */
					for (j = i; j < fds_nr - 1; j++)
						fds[j] = fds[j + 1];
					fds_nr--;
					poll_size--;
					i--;

				} else if (err < 0) {
					ret = err;
					goto close_sockets;
				}
			}
		}
	}

close_sockets:
	for (i = 0; i < fds_nr; i++)
		if (fds[i].fd > 0)
			close(fds[i].fd);
	return ret;
}
