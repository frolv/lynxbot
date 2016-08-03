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

class CommandHandler {

	public:
		typedef int(CommandHandler::*cmdfun)(char *, struct command *);
		typedef std::unordered_map<std::string, cmdfun> commandMap;

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
		int m_status;
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
		int about(char *out, struct command *c);
		int active(char *out, struct command *c);
		int age(char *out, struct command *c);
		int calc(char *out, struct command *c);
		int cgrep(char *out, struct command *c);
		int cmdinfo(char *out, struct command *c);
		int cml(char *out, struct command *c);
		int duck(char *out, struct command *c);
		int ehp(char *out, struct command *c);
		int eightball(char *out, struct command *c);
		int fashiongen(char *out, struct command *c);
		int ge(char *out, struct command *c);
		int level(char *out, struct command *c);
		int man(char *out, struct command *c);
		int manual(char *out, struct command *c);
		int rsn(char *out, struct command *c);
		int submit(char *out, struct command *c);
		int twitter(char *out, struct command *c);
		int uptime(char *out, struct command *c);
		int wheel(char *out, struct command *c);
		int xp(char *out, struct command *c);

		/* moderator only commands */
		int addcom(char *out, struct command *c);
		int addrec(char *out, struct command *c);
		int count(char *out, struct command *c);
		int delcom(char *out, struct command *c);
		int delrec(char *out, struct command *c);
		int editcom(char *out, struct command *c);
		int permit(char *out, struct command *c);
		int setgiv(char *out, struct command *c);
		int setrec(char *out, struct command *c);
		int showrec(char *out, struct command *c);
		int status(char *out, struct command *c);
		int strawpoll(char *out, struct command *c);
		int whitelist(char *out, struct command *c);

		/* helpers */
		uint8_t source(const std::string &cmd);
		int getrsn(char *out, size_t len, const char *text,
				const char *nick, int username = 0);
		void populateCmds();
		void populateHelp();
};

class CustomCommandHandler {

	public:
		CustomCommandHandler(CommandHandler::commandMap *defaultCmds,
				TimerManager *tm,
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
		CommandHandler::commandMap *m_cmp;
		TimerManager *m_tmp;
		const std::string m_wheelCmd;
		const std::string m_name;
		const std::string m_channel;
		Json::Value m_commands;
		Json::Value m_emptyVal;
		std::string m_error;
		bool cmdcheck();

};
