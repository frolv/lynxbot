#ifndef TWITCHBOT_H
#define TWITCHBOT_H

#include <string>
#include <thread>
#include <tw/authenticator.h>
#include <unordered_map>
#include "client.h"
#include "CmdHandler.h"
#include "config.h"
#include "EventManager.h"
#include "Giveaway.h"
#include "Moderator.h"
#include "permissions.h"
#include "URLParser.h"

class Authenticator;
class CmdHandler;
class ConfigReader;
class EventManager;
class Giveaway;
class Moderator;
class URLParser;

class TwitchBot {

	public:
		TwitchBot(const char *name, const char *channel,
			const char *password, const char *token,
			ConfigReader *cfgr);
		~TwitchBot();
		bool connected() const;
		bool connect();
		void disconnect();
		void server_loop();

	private:
		bool m_connected;
		/* twitch.tv information */
		const char *m_password;
		const char *m_nick;
		const char *m_channel;
		const char *m_token;

		tw::Authenticator m_auth;
		struct client m_client;
		CmdHandler m_cmdhnd;
		ConfigReader *m_cfgr;
		EventManager m_event;
		Giveaway m_giveaway;
		Moderator m_mod;
		URLParser m_parser;

		std::string m_subMsg;
		std::string m_resubMsg;
		std::thread m_tick;
		bool m_urltitles;
		bool m_familiarity;
		bool m_disable;
		std::unordered_map<std::string, int> m_names;

		void process_data(char *data);
		bool process_privmsg(char *privmsg);
		bool process_url(char *out);
		bool parse_privmsg(char *privmsg, char **nick,
				char **msg, perm_t *p);
		bool process_submsg(char *out, char *submsg);
		bool process_resub(char *resubmsg);
		void extract_names_list(char *data);
		void process_user(char *data);
		void read_names(char *names);
		bool moderate(const std::string &nick, const std::string &msg);
		void tick();
		void parseSubMsg(std::string &fmt, const std::string &which);
		std::string formatSubMsg(const std::string &format,
				const std::string &n, const std::string &m);

};

#endif /* TWITCHBOT_H */
