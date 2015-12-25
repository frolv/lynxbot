#include "stdafx.h"

// library for winsock2
#pragma comment(lib, "ws2_32.lib")

#define MAXBUFFERSIZE 2048

TwitchBot::TwitchBot(const std::string nick, const std::string channel, const std::string password)
	: m_connected(false), m_nick(nick), m_channelName(channel), m_socket(NULL) {

	const char *serv = "irc.twitch.tv", *port = "6667";

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (int32_t error = WSAStartup(MAKEWORD(2, 2), &m_wsa)) {
		std::cerr << "WSAStartup failed. Error: " << error << std::endl;
	}
	else {
		
		std::clog << "WSAStartup successful." << std::endl;

		// set up the server info
		if (int32_t error = getaddrinfo(serv, port, &hints, &servinfo)) {
			std::cerr << "Getaddrinfo failed. Error: " << gai_strerror(error);
		}
		else {

			// create the socket
			m_socket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
			if (m_socket == INVALID_SOCKET) {
				std::cerr << "Socket creation failed with error " << WSAGetLastError() << std::endl;
			}
			else {

				std::clog << "Socket creation succeeded." << std::endl;

				// connect to socket
				if (connect(m_socket, servinfo->ai_addr, servinfo->ai_addrlen) == SOCKET_ERROR) {
					std::cerr << "Socket connection failed with error " << WSAGetLastError() << std::endl;
					disconnect();
				}
				else {

					std::clog << "Connected to " << serv << std::endl;
					m_connected = true;
					freeaddrinfo(servinfo);
					
					// send required IRC data: PASS, NICK, USER
					sendData("PASS " + password);
					sendData("NICK " + nick);
					sendData("USER " + nick);

					sendData("JOIN " + channel);

				}

			}
		
		}
	
	}

}

TwitchBot::~TwitchBot() {
	closesocket(m_socket);
	WSACleanup();
}

void TwitchBot::disconnect() {
	m_connected = false;
	closesocket(m_socket);
	WSACleanup();
}

bool TwitchBot::sendData(const std::string &data) {

	// add win newline and covert to char array
	std::string rn = data + "\r\n";
	const char *formatted = rn.c_str();

	// send formatted data
	int32_t bytes = send(m_socket, formatted, strlen(formatted), NULL);
	std::clog << (bytes > 0 ? "[SENT] " : "Failed to send: ") << formatted << std::endl;

	// return true iff data was sent succesfully
	return bytes > 0;

}

bool TwitchBot::sendMsg(const std::string &msg) {
	return sendData("PRIVMSG " + m_channelName + " :" + msg);
}

void TwitchBot::serverLoop() {

	int32_t bytes;
	char buf[MAXBUFFERSIZE];

	for (;;) {

		// recieve data from server
		bytes = recv(m_socket, buf, MAXBUFFERSIZE - 1, 0);
		buf[bytes] = '\0';

		std::clog << "[RECV] " << buf << std::endl;

		// quit program if no data is recieved
		if (bytes <= 0) {
			std::cerr << "No data received. Exiting." << std::endl;
			disconnect();
			break;
		}

	}

}