/*
 * client.h: definition of client struct and functions to use client
 * for connecting, sending and recieving data through a socket
 */

#ifndef _CLIENT_H
#define _CLIENT_H

#ifdef _WIN32
# include <Winsock2.h>
#endif

/* a simple networking client */
struct client {
	int connected;
#ifdef __linux__
	int fd;
#endif
#ifdef _WIN32
	SOCKET sock;
	WSADATA wsa;
#endif
};

/* cconnect: connect cl to server serv on port port */
int cconnect(struct client *cl, const char *serv, const char *port);

/* cdisconnect: disconnect cl from current server */
int cdisconnect(struct client *cl);

/* cwrite: write data to cl server */
int cwrite(struct client *cl, const char *msg);

/* cread: read data from cl server */
int cread(struct client *cl, char *buf, size_t sz);

#endif
