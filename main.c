// Test for poll() and select() system calls
//
// Copyright (C) 2018  Ricardo Biehl Pasquali
//
// License: See LICENSE file at the root of this repository.

// 2017-11-10
//
// polling for send timestamp on socket's error queue

#include <getopt.h>  // getopt_long()
#include <poll.h>    // POLL*
#include <pthread.h> // pthread_*()
#include <stdio.h>   // fputs()
#include <string.h>  // strcmp()
#include <unistd.h>  // sleep(), getopt_long()

// open_socket() do_send() do_poll() do_recv()
#include "common.h"

static int send_in_other_thread = 0;
static int use_select = 0;
static short request_mask = 0;

// Sender thread used when send_in_other_thread is true
static void*
sender(void *data)
{
	int sfd = *((int*) data);

	for (;;) {
		sleep(1);
		do_send(sfd);
	}

	return NULL;
}

const char help_text[] =
"--pollpri  Request POLLPRI event.\n"
"--pollin   Request POLLIN event.\n"
"--pollerr  Request POLLERR event (ignored because this\n"
"           event is always requested).\n"
"--multi-thread   Send packets in another thread.\n"
"--use-select     Use select() system call instead of\n"
"                 poll().\n"
"--mask-pollpri   Mask POLLPRI in wake up. This set\n"
"                 SO_SELECT_ERR_QUEUE socket option. The\n"
"                 idea is to allow \"instantaneous wake\n"
"                 up\" on error queue.\n"
"--bind-socket    Bind socket to receive packets.\n"
"--tx-timestamp``: Enable transmit timestamping.\n"
"\nE.g.: $ ./main --pollpri --mask-pollpri\n";

int
main(int argc, char **argv)
{
	pthread_t sender_thread;
	int sfd;
	int flags = 0;

	while (1) {
		int c;
		struct option long_options[] = {
			{ "help",          no_argument, 0, 'h' },
			{ "pollpri",       no_argument, 0, 'p' },
			{ "pollin",        no_argument, 0, 'i' },
			{ "pollerr",       no_argument, 0, 'e' },
			{ "multi-thread",  no_argument, 0, 's' },
			{ "use-select",    no_argument, 0, 'S' },
			{ "mask-pollpri",  no_argument, 0, 'm' },
			{ "bind-socket",   no_argument, 0, 'b' },
			{ "tx-timestamp",  no_argument, 0, 't' },
			{ 0,               0,           0, 0   },
		};

		c = getopt_long(argc, argv, "", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		default:
		case 'h':
			fputs(help_text, stdout);
			return 0;
		case 'p': request_mask |= POLLPRI; break;
		case 'i': request_mask |= POLLIN;  break;
		case 'e': request_mask |= POLLERR; break;
		case 's': send_in_other_thread = 1; break;
		case 'S': use_select = 1;           break;
		case 'm': flags |= POLLPRI_WAKEUP_ON_ERROR_QUEUE; break;
		case 'b': flags |= BIND_SOCKET;                   break;
		case 't': flags |= ENABLE_TX_TIMESTAMP;           break;
		}
	}

	if ((sfd = open_socket(flags)) == -1)
		return 1;

	// create thread and wait for it to terminate
	if (send_in_other_thread)
		pthread_create(&sender_thread, NULL, sender, &sfd);

	for (;;) {
		if (!send_in_other_thread) {
			sleep(1);
			do_send(sfd);
		}

		if (use_select)
			do_select(sfd, request_mask);
		else
			do_poll(sfd, request_mask);
		do_recv(sfd);
	}

	if (send_in_other_thread)
		pthread_join(sender_thread, NULL);

	return 0;
}
