#pragma once

#include "CommandHandler.h"
#include "TimerManager.h"

class CommandHandler;
class TimerManager;

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
		TimerManager m_timerManager;
		bool m_connected;
		const std::string m_nick;
		const std::string m_channelName;
		const std::vector<std::string> m_keys;
		bool sendData(const std::string &data);
		bool sendMsg(const std::string &msg);
		bool sendPong(const std::string &ping);
		void processData(const std::string &data);
		bool processPRIVMSG(const std::string &PRIVMSG);

};
