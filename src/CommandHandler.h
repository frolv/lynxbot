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
		struct command {
			std::string nick;
			int argc;
			char **argv;
			std::string cmd;
			std::string fullCmd;
			perm_t privileges;
		};

		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(CommandHandler::command *)> commandMap;

		CommandHandler(const std::string &name, const std::string &channel,
				const std::string &token, Moderator *mod,
				URLParser *urlp, EventManager *evtp,
				Giveaway *givp, ConfigReader *cfgr,
				tw::Authenticator *auth);
		~CommandHandler();

		std::string process_cmd(char *nick, char *cmdstr, perm_t p);
		std::string process_resp(const std::string &message);
		bool isCounting() const;
		void count(const std::string &nick, const std::string &message);

	private:
		const std::string m_name;
		const std::string m_channel;
		const std::string m_token;
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
		std::string m_activePoll;
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

		/* default bot commands */
		std::string about(struct command *c);
		std::string active(struct command *c);
		std::string age(struct command *c);
		std::string calc(struct command *c);
		std::string cgrep(struct command *c);
		std::string cmdinfo(struct command *c);
		std::string cml(struct command *c);
		std::string duck(struct command *c);
		std::string ehp(struct command *c);
		std::string eightball(struct command *c);
		std::string fashiongen(struct command *c);
		std::string ge(struct command *c);
		std::string level(struct command *c);
		std::string man(struct command *c);
		std::string manual(struct command *c);
		std::string pokemon(struct command *c);
		std::string rsn(struct command *c);
		std::string submit(struct command *c);
		std::string twitter(struct command *c);
		std::string uptime(struct command *c);
		std::string wheel(struct command *c);
		std::string xp(struct command *c);

		/* moderator only commands */
		std::string addcom(struct command *c);
		std::string addrec(struct command *c);
		std::string count(struct command *c);
		std::string delcom(struct command *c);
		std::string delrec(struct command *c);
		std::string editcom(struct command *c);
		std::string permit(struct command *c);
		std::string setgiv(struct command *c);
		std::string setrec(struct command *c);
		std::string showrec(struct command *c);
		std::string status(struct command *c);
		std::string strawpoll(struct command *c);
		std::string whitelist(struct command *c);

		/* helpers */
		uint8_t source(const std::string &cmd);
		std::string getRSN(const std::string &text,
			const std::string &nick, std::string &err,
			bool username = false);
		void populateCmds();
		void populateHelp();
};

class CustomCommandHandler {

	public:
		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(CommandHandler::command *)> commandMap;
		CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm,
				const std::string &wheelCmd,
				const std::string &name,
				const std::string &channel);
		~CustomCommandHandler();
		bool isActive();
		bool addcom(const std::string &cmd, const std::string &response,
				const std::string &nick, time_t cooldown);
		bool delcom(const std::string &cmd);
		bool editcom(const std::string &cmd,
				const std::string &newResp = "",
				time_t newcd = -1);
		bool activate(const std::string &cmd);
		bool deactivate(const std::string &cmd);
		bool rename(const std::string &cmd, const std::string &newcmd);
		Json::Value *getcom(const std::string &cmd);
		const Json::Value *commands();
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
