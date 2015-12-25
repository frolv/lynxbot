#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

class TwitchBot {

	public:
		TwitchBot(const std::string name, const std::string channel, const std::string password = "");
		~TwitchBot();
		inline bool isConnected() { return m_connected; };
		void disconnect();
		bool sendData(const std::string &data);
		bool sendMsg(const std::string &msg);
		void serverLoop();

	private:
		SOCKET m_socket;
		WSADATA m_wsa;
		bool m_connected;
		std::string m_nick;
		std::string m_channelName;

};
