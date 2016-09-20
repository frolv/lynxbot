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
		bool bot_connected;
		/* twitch.tv information */
		const char *bot_password;
		const char *bot_name;
		const char *bot_channel;
		const char *bot_token;
		const char *CID = "\x6b\x6b\x6a\x68\x6d\x65\x6b\x6b"
				  "\x7a\x62\x65\x70\x71\x30\x70\x67"
				  "\x6e\x33\x34\x67\x36\x37\x31\x79"
				  "\x35\x6e\x65\x78\x61\x70\x38\x00";

		tw::Authenticator auth;
		struct client client;
		CmdHandler cmdhnd;
		ConfigReader *cfg;
		EventManager evtman;
		Giveaway giveaway;
		Moderator mod;
		URLParser parser;

		std::string submsg;
		std::string resubmsg;
		std::thread tick_thread;
		bool url_titles;
		bool familiarity_mode;
		bool auto_disable;
		std::unordered_map<std::string, int> active_users;

		void process_data(char *data);
		bool process_privmsg(char *privmsg);
		bool process_url(char *out);
		bool parse_privmsg(char *privmsg, char **nick,
				char **msg, perm_t *p);
		bool process_submsg(char *out, char *msgbuf);
		bool process_resub(char *msgbuf);
		void extract_names_list(char *data);
		void process_user(char *data);
		void read_names(char *names);
		bool moderate(const std::string &nick, const std::string &msg);
		void tick();
		void parse_submsg(std::string &fmt, const std::string &which);
		std::string format_submsg(const std::string &format,
				const std::string &n, const std::string &m);

};

#endif /* TWITCHBOT_H */
