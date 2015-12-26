#pragma once

#include "CommandHandler.h"

class CommandHandler;

class TwitchBot {

	public:
		TwitchBot(const std::string name, const std::string channel, const std::string password = "");
		~TwitchBot();
		inline bool isConnected() { return m_connected; };
		void disconnect();
		void serverLoop();

	private:
		SOCKET m_socket;
		WSADATA m_wsa;
		CommandHandler m_cmdHandler;
		bool m_connected;
		std::string m_nick;
		std::string m_channelName;
		bool sendData(const std::string &data);
		bool sendMsg(const std::string &msg);
		bool sendPong(const std::string &ping);
		void processData(const std::string &data);
		bool processPRIVMSG(const std::string &PRIVMSG);

};
