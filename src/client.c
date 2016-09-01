#ifdef __linux__
# include <netdb.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
#endif

#ifdef _WIN32
# include <sdkddkver.h>
# include <ws2tcpip.h>
#endif

#include "client.h"
#include "lynxbot.h"

#ifdef __linux__
static int connect_unix(struct client *cl, const char *serv, const char *port);
#endif
#ifdef _WIN32
static int connect_win(struct client *cl, const char *serv, const char *port);
#endif

/* cconnect: connect cl to server serv on port port */
int cconnect(struct client *cl, const char *serv, const char *port)
{
#ifdef __linux__
	return connect_unix(cl, serv, port);
#endif
#ifdef _WIN32
	return connect_win(cl, serv, port);
#endif
}

/* cdisconnect: disconnect cl from current server */
int cdisconnect(struct client *cl)
{
	if (cl->connected) {
		cl->connected = 0;
#ifdef __linux__
		if (close(cl->fd) == -1) {
			perror("close");
			return 1;
		}
#endif
#ifdef _WIN32
		closesocket(cl->sock);
		WSACleanup();
#endif
	}
	return 0;
}

/* cwrite: write data to cl server */
int cwrite(struct client *cl, const char *msg)
{
	if (!cl->connected)
		return -1;
#ifdef __linux__
	return write(cl->fd, msg, strlen(msg));
#endif
#ifdef _WIN32
	return send(cl->sock, msg, strlen(msg), 0);
#endif
}

/* cread: read data from cl server */
int cread(struct client *cl, char *buf, size_t sz)
{
	int bytes;

	if (!cl->connected)
		return -1;

#ifdef __linux__
	bytes = read(cl->fd, buf, sz - 1);
#endif
#ifdef _WIN32
	bytes = recv(cl->sock, buf, sz - 1, 0);
#endif
	if (bytes >= 0)
		buf[bytes] = '\0';
	return bytes;
}

/* send_raw: send a raw network message to cl server */
int send_raw(struct client *cl, char *data)
{
	size_t end;
	int bytes;

	if ((end = strlen(data)) > MAX_MSG - 3) {
		fprintf(stderr, "Message too long: %s\n", data);
		return 0;
	}

	if (data[end - 1] != '\n' && data[end - 2] != '\r')
		strcpy(data + end, "\r\n");

	/* send formatted data */
	bytes = cwrite(cl, data);
	printf("%s %s\n", bytes > 0 ? "[SENT]" : "Failed to send:", data);

	/* return true iff data was sent succesfully */
	return bytes > 0;
}

/* send_msg: send a privmsg to channel on cl server */
int send_msg(struct client *cl, const char *channel, const char *msg)
{
	char buf[MAX_MSG + 64];

	_sprintf(buf, MAX_MSG + 64, "PRIVMSG %s :%s", channel, msg);
	return send_raw(cl, buf);
}

/* send_pong: respond to ping with a pong */
int pong(struct client *cl, char *ping)
{
	/* first six chars are "PING :", server name starts after */
	strcpy(++ping, "PONG");
	ping[4] = ' ';
	return send_raw(cl, ping);
}

#ifdef __linux__
static int connect_unix(struct client *cl, const char *serv, const char *port)
{
	struct hostent *hp;
	struct sockaddr_in s;

	cl->connected = 0;
	if (!(hp = gethostbyname(serv))) {
		fprintf(stderr, "could not find host %s\n", serv);
		return 1;
	}

	if ((cl->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}

	memset(&s, '\0', sizeof(s));
	s.sin_family = AF_INET;
	memcpy(&s.sin_addr, hp->h_addr_list[0], hp->h_length);
	s.sin_port = htons(atoi(port));

	if (connect(cl->fd, (struct sockaddr *)&s, sizeof(s)) < 0) {
		perror("connect");
		return 1;
	}
	cl->connected = 1;
	return 0;
}
#endif

#ifdef _WIN32
static int connect_win(struct client *cl, const char *serv, const char *port)
{
	struct addrinfo hints, *servinfo;
	int error;

	cl->connected = 0;
	if ((error = WSAStartup(MAKEWORD(2, 2), &cl->wsa))) {
		fprintf(stderr, "WSAStartup failed. Error %d: %d\n",
				error, WSAGetLastError());
		return 1;
	}

	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if ((error = getaddrinfo(serv, port, &hints, &servinfo))) {
		fprintf(stderr, "getaddrinfo failed. Error %d: %d\n",
				error, WSAGetLastError());
		return 1;
	}

	if ((cl->sock = socket(servinfo->ai_family, servinfo->ai_socktype,
		servinfo->ai_protocol)) == INVALID_SOCKET) {
		fprintf(stderr, "Could not open socket. Error: %d\n",
				WSAGetLastError());
		return 1;
	}

	if (connect(cl->sock, servinfo->ai_addr, servinfo->ai_addrlen)
		== SOCKET_ERROR) {
		fprintf(stderr, "Could not connect to socket. Error: %d\n",
				WSAGetLastError());
		closesocket(cl->sock);
		return 1;
	}

	freeaddrinfo(servinfo);
	cl->connected = 1;
	return 0;
}
#endif
