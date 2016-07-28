#ifdef __linux__
# include <cstdio>
# include <cstdlib>
# include <cstring>
# include <netdb.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#ifdef _WIN32
# include <WS2tcpip.h>
# include <sdkddkver.h>
# pragma comment(lib, "ws2_32.lib")
#endif

#include "client.h"

Client::Client(const char *serv, const char *port)
	:m_serv(serv), m_port(port)
{
}

Client::~Client()
{
	cdisconnect();
}

bool Client::cconnect()
{
#ifdef __linux__
	m_connected = connect_unix();
#endif
#ifdef _WIN32
	m_connected = connect_win();
#endif
	return m_connected;
}

void Client::cdisconnect()
{
	if (m_connected) {
		m_connected = false;
#ifdef __linux__
		if (close(m_fd) == -1)
			perror("close");
#endif
#ifdef _WIN32
		closesocket(m_socket);
		WSACleanup();
#endif
	}
}

int32_t Client::cwrite(const char *msg)
{
	if (!m_connected)
		return -1;
#ifdef __linux__
	return write(m_fd, msg, strlen(msg));
#endif
#ifdef _WIN32
	return send(m_socket, msg, strlen(msg), NULL);
#endif
}

int32_t Client::cread(char *buf, size_t sz)
{
	int32_t bytes;

	if (!m_connected)
		return -1;

#ifdef __linux__
	bytes = read(m_fd, buf, sz - 1);
#endif
#ifdef _WIN32
	bytes = recv(m_socket, buf, sz - 1, 0);
#endif
	if (bytes >= 0)
		buf[bytes] = '\0';
	return bytes;
}

#ifdef __linux__
bool Client::connect_unix()
{
	struct hostent *hp;
	struct sockaddr_in s;

	if (!(hp = gethostbyname(m_serv))) {
		std::cerr << "could not find host " << m_serv << std::endl;
		return false;
	}

	if ((m_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return false;
	}

	memset(&s, '\0', sizeof(s));
	s.sin_family = AF_INET;
	memcpy(&s.sin_addr, hp->h_addr_list[0], hp->h_length);
	s.sin_port = htons(atoi(m_port));

	if (connect(m_fd, (struct sockaddr *)&s, sizeof(s)) < 0) {
		perror("connect");
		return false;
	}
	return true;
}
#endif

#ifdef _WIN32
bool Client::connect_win()
{
	if (int32_t error = WSAStartup(MAKEWORD(2, 2), &m_wsa)) {
		std::cerr << "WSAStartup failed. Error " << error << ": "
			<< WSAGetLastError() << std::endl;
		return false;
	}

	struct addrinfo hints, *servinfo;
	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (int32_t error = getaddrinfo(m_serv, m_port, &hints, &servinfo)) {
		std::cerr << "getaddrinfo failed. Error " << error << ": "
			<< WSAGetLastError() << std::endl;
		return false;
	}

	if ((m_socket = socket(servinfo->ai_family, servinfo->ai_socktype,
		servinfo->ai_protocol)) == INVALID_SOCKET) {
		std::cerr << "Socket initializiation failed. Error: "
			<< WSAGetLastError() << std::endl;
		return false;
	}

	if (connect(m_socket, servinfo->ai_addr, servinfo->ai_addrlen)
		== SOCKET_ERROR) {
		std::cerr << "Socket connection failed. Error: "
			<< WSAGetLastError() << std::endl;
		closesocket(m_socket);
		return false;
	}

	freeaddrinfo(servinfo);
	return true;
}
#endif