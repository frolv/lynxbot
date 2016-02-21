#pragma once

#include "Moderator.h"
#include "CommandHandler.h"
#include "TimerManager.h"
#include "URLParser.h"

class Moderator;
class CommandHandler;
class TimerManager;
class URLParser;

class TwitchBot {

	public:
		TwitchBot(const std::string name, const std::string channel, const std::string password = "");
		~TwitchBot();
		bool isConnected() const;
		void disconnect();
		void serverLoop();

	private:
		SOCKET m_socket;
		WSADATA m_wsa;
		Moderator m_mod;
		CommandHandler m_cmdHandler;
		TimerManager m_eventManager;
		URLParser m_parser;
		bool m_connected;
		const std::string m_nick;
		const std::string m_channelName;
		std::thread m_tick;
		bool sendData(const std::string &data) const;
		bool sendMsg(const std::string &msg) const;
		bool sendPong(const std::string &ping) const;
		void processData(const std::string &data);
		bool processPRIVMSG(const std::string &PRIVMSG);
		bool moderate(const std::string &nick, const std::string &msg);
		void tick();

};
