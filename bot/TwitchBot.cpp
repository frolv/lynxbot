#include "stdafx.h"

// library for winsock2
#pragma comment(lib, "ws2_32.lib")

TwitchBot::TwitchBot(const std::string nick, const std::string address, const std::string port, const std::string channel, const std::string password)
	: m_active(false), m_nick(nick), m_channelName(channel), m_socket(NULL) {

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (int32_t error = WSAStartup(MAKEWORD(2, 2), &m_wsa)) {
		std::cerr << "WSAStartup failed. Error: " << error << std::endl;
	}
	else {
		
		std::clog << "WSAStartup successful." << std::endl;

		// set up the server info
		if (uint32_t error = getaddrinfo(address.c_str(), port.c_str(), &hints, &servinfo)) {
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
				}
				else {

					std::clog << "Socket connection successful." << std::endl;
					freeaddrinfo(servinfo);
					
					int32_t bytes, count = 0;
					char buf[100];

					for (;;) {

						std::clog << ++count << std::endl;
						
						if (count == 1) {
							// send required IRC data: PASS, NICK, USER
							sendMsg("PASS " + password);
							sendMsg("NICK " + nick);
							sendMsg("USER " + nick + " 0 * :" + nick);
						}
						else if (count == 2) {
							// connect to channel
							sendMsg("JOIN " + channel);
						}

						bytes = recv(m_socket, buf, 99, 0);
						buf[bytes] = '\0';
						std::clog << buf;

						if (bytes == 0) {
							std::cout << "No data recieved. Exiting." << std::endl;
							break;
						}

					}

				}

			}
		
		}
	
	}

}

TwitchBot::~TwitchBot() {
	closesocket(m_socket);
	WSACleanup();
}

bool TwitchBot::sendMsg(const std::string &msg) {

	// add win style newline and covert to char
	const char *formatted = (msg + "\r\n").c_str();

	// if no data is sent
	if (send(m_socket, formatted, strlen(formatted), NULL) == SOCKET_ERROR) {
		std::cerr << "Failed to send: " << msg << std::endl;
		return false;
	}

	std::clog << "Sent: " << msg << std::endl;
	return true;

}