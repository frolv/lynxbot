#pragma once

#include <string>
#include <thread>
#include <tw/authenticator.h>
#include "client.h"
#include "Moderator.h"
#include "CommandHandler.h"
#include "EventManager.h"
#include "URLParser.h"
#include "Giveaway.h"

class Client;
class Moderator;
class CommandHandler;
class EventManager;
class URLParser;
class Giveaway;
class tw::Authenticator;

class TwitchBot {

	public:
		TwitchBot(const std::string name, const std::string channel,
			const std::string password = "");
		~TwitchBot();
		bool isConnected() const;
		void disconnect();
		void serverLoop();

	private:
		bool m_connected;
		const std::string m_nick;
		const std::string m_channelName;
		Client m_client;
		Moderator m_mod;
		CommandHandler m_cmdHandler;
		EventManager m_eventManager;
		URLParser m_parser;
		Giveaway m_giveaway;
		tw::Authenticator m_auth;
		std::string m_subMsg;
		std::thread m_tick;
		bool sendData(const std::string &data);
		bool sendMsg(const std::string &msg);
		bool sendPong(const std::string &ping);
		void processData(const std::string &data);
		bool processPRIVMSG(const std::string &PRIVMSG);
		bool moderate(const std::string &nick, const std::string &msg);
		void tick();

};
