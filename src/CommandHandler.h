#pragma once

#include <unordered_map>
#include <string>
#include <random>
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
		struct cmdinfo {
			std::string nick;
			std::string cmd;
			std::string fullCmd;
			perm_t privileges;
		};

		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(CommandHandler::cmdinfo *)> commandMap;

		CommandHandler(const std::string &name, const std::string &channel,
				const std::string &token, Moderator *mod,
				URLParser *urlp, EventManager *evtp,
				Giveaway *givp, ConfigReader *cfgr);
		~CommandHandler();

		std::string processCommand(const std::string &nick,
				const std::string &fullCmd, perm_t p);
		std::string processResponse(const std::string &message);
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
		Json::Value m_responses;
		bool m_responding;
		bool m_counting;
		std::vector<std::string> m_usersCounted;
		std::unordered_map<std::string, uint16_t> m_messageCounts;
		std::unordered_map<std::string, std::string> m_help;
		std::random_device m_rd;
		std::mt19937 m_gen;
		const std::string STRAWPOLL_HOST = "strawpoll.me";
		const std::string STRAWPOLL_API = "/api/v2/polls";
		const std::string TWITCH_API = "https://api.twitch.tv/kraken";
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
		std::string ehp(struct cmdinfo *c);
		std::string level(struct cmdinfo *c);
		std::string ge(struct cmdinfo *c);
		std::string calc(struct cmdinfo *c);
		std::string cml(struct cmdinfo *c);
		std::string wheel(struct cmdinfo *c);
		std::string eightball(struct cmdinfo *c);
		std::string active(struct cmdinfo *c);
		std::string manual(struct cmdinfo *c);
		std::string help(struct cmdinfo *c);
		std::string about(struct cmdinfo *c);
		std::string uptime(struct cmdinfo *c);
		std::string rsn(struct cmdinfo *c);
		std::string submit(struct cmdinfo *c);
		std::string duck(struct cmdinfo *c);

		/* moderator only commands */
		std::string strawpoll(struct cmdinfo *c);
		std::string count(struct cmdinfo *c);
		std::string whitelist(struct cmdinfo *c);
		std::string permit(struct cmdinfo *c);
		std::string makecom(struct cmdinfo *c);
		std::string delcom(struct cmdinfo *c);
		std::string addrec(struct cmdinfo *c);
		std::string delrec(struct cmdinfo *c);
		std::string listrec(struct cmdinfo *c);
		std::string setrec(struct cmdinfo *c);
		std::string setgiv(struct cmdinfo *c);
		std::string status(struct cmdinfo *c);

		/* helpers */
		uint8_t source(const std::string &cmd);
		std::string extractHSData(const std::string &httpResp,
			uint8_t skillID) const;
		std::string getRSN(const std::string &text,
			const std::string &nick, std::string &err,
			bool username = false);
		void populateCmds();
		void populateHelp();
};

class CustomCommandHandler {

	public:
		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(CommandHandler::cmdinfo *)> commandMap;
		CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm,
				const std::string &wheelCmd);
		~CustomCommandHandler();
		bool isActive();
		bool addCom(const std::string &cmd,
				const std::string &response, time_t cooldown);
		bool delCom(const std::string &cmd);
		bool editCom(const std::string &cmd,
				const std::string &newResp = "", time_t newcd = -1);
		Json::Value *getCom(const std::string &cmd);
		bool validName(const std::string &cmd, bool loading = false);

	private:
		bool m_active;
		commandMap *m_cmp;
		TimerManager *m_tmp;
		const std::string m_wheelCmd;
		Json::Value m_commands;
		Json::Value m_emptyVal;
		void writeToFile() const;

};
