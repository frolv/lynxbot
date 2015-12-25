#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

class TwitchBot {

	public:
		TwitchBot(const std::string name, const std::string address, const std::string port, const std::string channel, const std::string password = "");
		~TwitchBot();
		inline bool isActive() { return m_active; };
		bool sendMsg(const std::string &msg);

	private:
		SOCKET m_socket;
		WSADATA m_wsa;
		bool m_active;
		std::string m_nick;
		std::string m_channelName;

};
