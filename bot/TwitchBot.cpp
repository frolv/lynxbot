#include "stdafx.h"

// library for winsock2
#pragma comment(lib, "ws2_32.lib")

TwitchBot::TwitchBot(const std::string name, const std::string user, const std::string address, const std::string port, const std::string channel, const std::string password)
	: m_channelName(channel), m_socket(NULL) {

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
				std::cerr << "Socket function failed with error " << WSAGetLastError() << std::endl;
			}
			else {

				std::clog << "Socket function succeeded." << std::endl;

				// connect to socket
				if (int32_t error = connect(m_socket, servinfo->ai_addr, servinfo->ai_addrlen)) {
					std::cerr << "Socket connection failed with error " << WSAGetLastError() << std::endl;
				}
				else {
					std::clog << "Socket connection successful." << std::endl;
				}

			}
		
		}
	
	}

}

TwitchBot::~TwitchBot()
{
}
