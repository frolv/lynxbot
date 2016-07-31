#pragma once

#include <unordered_map>
#include <random>
#include <string>
#include <tw/authenticator.h>
#include "Moderator.h"
#include "GEReader.h"
#include "TimerManager.h"
#include "EventManager.h"
#include "SelectionWheel.h"
#include "URLParser.h"
#include "Giveaway.h"
#include "RSNList.h"
#include "config.h"
#include "permissions.h"

#define DEFAULT 1
#define CUSTOM  2

#define MAX_LEN 128

class Moderator;
class CustomCommandHandler;
class GEReader;
class TimerManager;
class EventManager;
class SelectionWheel;
class URLParser;
class Giveaway;
class RSNList;
class ConfigReader;

struct command {
	char *nick;		/* name of command user */
	int argc;		/* number of arguments */
	char **argv;		/* array of arguments */
	perm_t privileges;	/* user privileges */
	std::string cmd;
	std::string fullCmd;
};

class CommandHandler {

	public:
		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(char *,
			struct command *)> commandMap;

		CommandHandler(const char *name, const char *channel,
				const char *token, Moderator *mod,
				URLParser *urlp, EventManager *evtp,
				Giveaway *givp, ConfigReader *cfgr,
				tw::Authenticator *auth);
		~CommandHandler();

		void process_cmd(char *out, char *nick, char *cmdstr, perm_t p);
		void process_resp(char *out, char *msg, char *nick);
		bool isCounting() const;
		void count(const std::string &nick, const std::string &message);

	private:
		const char *m_name;
		const char *m_channel;
		const char *m_token;
		Moderator *m_modp;
		URLParser *m_parsep;
		commandMap m_defaultCmds;
		GEReader m_GEReader;
		TimerManager m_cooldowns;
		SelectionWheel m_wheel;
		CustomCommandHandler *m_customCmds;
		EventManager *m_evtp;
		Giveaway *m_givp;
		RSNList m_rsns;
		ConfigReader *m_cfgr;
		tw::Authenticator *m_auth;
		Json::Value m_responses;
		Json::Value m_fashion;
		bool m_responding;
		bool m_counting;
		std::vector<std::string> m_usersCounted;
		std::unordered_map<std::string, uint16_t> m_messageCounts;
		std::unordered_map<std::string, std::string> m_help;
		std::random_device m_rd;
		std::mt19937 m_gen;
		char m_poll[MAX_LEN];
		std::vector<std::string> m_eightball = {
			"It is certain",
			"It is decidedly so",
			"Without a doubt",
			"Yes, definitely",
			"You may rely on it",
			"As I see it, yes",
			"Most likely",
			"Outlook good",
			"Yes",
			"Signs point to yes",
			"Reply hazy try again",
			"Ask again later",
			"Better not tell you now",
			"Cannot predict now",
			"Concentrate and ask again",
			"Don't count on it", "My reply is no",
			"My sources say no",
			"Outlook not so good",
			"Very doubtful"
		};

		void process_default(char *out, struct command *c);
		void process_custom(char *out, struct command *c);

		/* default bot commands */
		std::string about(char *out, struct command *c);
		std::string active(char *out, struct command *c);
		std::string age(char *out, struct command *c);
		std::string calc(char *out, struct command *c);
		std::string cgrep(char *out, struct command *c);
		std::string cmdinfo(char *out, struct command *c);
		std::string cml(char *out, struct command *c);
		std::string duck(char *out, struct command *c);
		std::string ehp(char *out, struct command *c);
		std::string eightball(char *out, struct command *c);
		std::string fashiongen(char *out, struct command *c);
		std::string ge(char *out, struct command *c);
		std::string level(char *out, struct command *c);
		std::string man(char *out, struct command *c);
		std::string manual(char *out, struct command *c);
		std::string pokemon(char *out, struct command *c);
		std::string rsn(char *out, struct command *c);
		std::string submit(char *out, struct command *c);
		std::string twitter(char *out, struct command *c);
		std::string uptime(char *out, struct command *c);
		std::string wheel(char *out, struct command *c);
		std::string xp(char *out, struct command *c);

		/* moderator only commands */
		std::string addcom(char *out, struct command *c);
		std::string addrec(char *out, struct command *c);
		std::string count(char *out, struct command *c);
		std::string delcom(char *out, struct command *c);
		std::string delrec(char *out, struct command *c);
		std::string editcom(char *out, struct command *c);
		std::string permit(char *out, struct command *c);
		std::string setgiv(char *out, struct command *c);
		std::string setrec(char *out, struct command *c);
		std::string showrec(char *out, struct command *c);
		std::string status(char *out, struct command *c);
		std::string strawpoll(char *out, struct command *c);
		std::string whitelist(char *out, struct command *c);

		/* helpers */
		uint8_t source(const std::string &cmd);
		int getrsn(char *out, size_t len, const char *text,
				const char *nick, int username = 0);
		void populateCmds();
		void populateHelp();
};

class CustomCommandHandler {

	public:
		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(char *,
			struct command *)> commandMap;

		CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm,
				const std::string &wheelCmd,
				const std::string &name,
				const std::string &channel);
		~CustomCommandHandler();
		bool isActive();
		bool addcom(const std::string &cmd, std::string response,
				const std::string &nick, time_t cooldown);
		bool delcom(const std::string &cmd);
		bool editcom(const std::string &cmd,
				std::string newResp = "",
				time_t newcd = -1);
		bool activate(const std::string &cmd);
		bool deactivate(const std::string &cmd);
		bool rename(const std::string &cmd, const std::string &newcmd);
		Json::Value *getcom(const std::string &cmd);
		const Json::Value *commands();
		size_t size();
		bool validName(const std::string &cmd, bool loading = false);
		void write();
		std::string error() const;
		std::string format(const Json::Value *cmd,
				const std::string &nick) const;

	private:
		bool m_active;
		commandMap *m_cmp;
		TimerManager *m_tmp;
		const std::string m_wheelCmd;
		const std::string m_name;
		const std::string m_channel;
		Json::Value m_commands;
		Json::Value m_emptyVal;
		std::string m_error;
		bool cmdcheck();

};
