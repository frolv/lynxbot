#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

class TwitchBot {

	public:
		TwitchBot(const std::string name, const std::string user, const std::string address, const std::string port, const std::string channel, const std::string password = "");
		~TwitchBot();

	private:
		SOCKET m_socket;
		WSADATA m_wsa;
		std::string m_channelName;

};
