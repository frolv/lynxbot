#pragma once

#include <string>
#include <thread>
#include <tw/authenticator.h>
#include "client.h"
#include "CommandHandler.h"
#include "config.h"
#include "EventManager.h"
#include "Giveaway.h"
#include "Moderator.h"
#include "URLParser.h"

class Authenticator;
class Client;
class CommandHandler;
class ConfigReader;
class EventManager;
class Giveaway;
class Moderator;
class URLParser;

class TwitchBot {

	public:
		TwitchBot(const std::string &name, const std::string &channel,
			const std::string &password, const std::string &token,
			ConfigReader *cfgr);
		~TwitchBot();
		bool isConnected() const;
		void disconnect();
		void serverLoop();

	private:
		bool m_connected;
		/* twitch.tv information */
		const std::string m_nick;
		const std::string m_channelName;
		const std::string m_token;

		tw::Authenticator m_auth;
		Client m_client;
		CommandHandler m_cmdHandler;
		ConfigReader *m_cfgr;
		EventManager m_event;
		Giveaway m_giveaway;
		Moderator m_mod;
		URLParser m_parser;

		std::string m_subMsg;
		std::string m_resubMsg;
		std::thread m_tick;

		bool sendData(const std::string &data);
		bool sendMsg(const std::string &msg);
		bool sendPong(const std::string &ping);
		void processData(const std::string &data);
		bool processPRIVMSG(const std::string &PRIVMSG);
		bool moderate(const std::string &nick, const std::string &msg);
		void tick();

};
