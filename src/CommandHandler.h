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

class Moderator;
class CustomCommandHandler;
class GEReader;
class TimerManager;
class EventManager;
class SelectionWheel;
class URLParser;

class CommandHandler {
	public:
		struct cmdinfo {
			std::string nick;
			std::string fullCmd;
			bool privileges;
		};
		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(CommandHandler::cmdinfo *)> commandMap;
		CommandHandler(const std::string &name, const std::string &channel,
			Moderator *mod, URLParser *urlp, EventManager *evtp);
		~CommandHandler();
		std::string processCommand(const std::string &nick,
			const std::string &fullCmd, bool privileges);
		std::string processResponse(const std::string &message);
		bool isCounting() const;
		void count(const std::string &nick, const std::string &message);
	private:
		const std::string m_name;
		const std::string m_channel;
		Moderator *m_modp;
		URLParser *m_parsep;
		commandMap m_defaultCmds;
		GEReader m_GEReader;
		TimerManager m_cooldowns;
		SelectionWheel m_wheel;
		CustomCommandHandler *m_customCmds;
		EventManager *m_evtp;
		Json::Value m_responses;
		bool m_responding;
		bool m_counting;
		std::vector<std::string> m_usersCounted;
		std::unordered_map<std::string, uint16_t> m_messageCounts;
		std::random_device m_rd;
		std::mt19937 m_gen;
		const std::string CML_HOST = "crystalmathlabs.com";
		const std::string CML_EHP_API =
			"/tracker/api.php?type=virtualhiscoresatplayer&page=timeplayed&player=";
		const std::string CML_UPDATE_API =
			"/tracker/api.php?type=update&player=";
		const std::string CML_USER = "/tracker/track.php?player=";
		const std::string RS_HOST = "services.runescape.com";
		const std::string RS_HS_API = "/m=hiscore_oldschool/index_lite.ws?player=";
		const std::string EXCHANGE_HOST = "api.rsbuddy.com";
		const std::string EXCHANGE_API = "/grandExchange?a=guidePrice&i=";
		const std::string STRAWPOLL_HOST = "strawpoll.me";
		const std::string STRAWPOLL_API = "/api/v2/polls";
		const std::string SOURCE = "https://github.com/frolv/lynxbot";
		std::string m_activePoll;
		std::vector<std::string> m_eightballResponses = {
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
		std::string ehpFunc(struct cmdinfo *c);
		std::string levelFunc(struct cmdinfo *c);
		std::string geFunc(struct cmdinfo *c);
		std::string calcFunc(struct cmdinfo *c);
		std::string cmlFunc(struct cmdinfo *c);
		std::string wheelFunc(struct cmdinfo *c);
		std::string eightballFunc(struct cmdinfo *c);
		std::string strawpollFunc(struct cmdinfo *c);
		std::string activeFunc(struct cmdinfo *c);
		std::string commandsFunc(struct cmdinfo *c);
		std::string aboutFunc(struct cmdinfo *c);
		std::string countFunc(struct cmdinfo *c);
		std::string uptimeFunc(struct cmdinfo *c);
		std::string whitelistFunc(struct cmdinfo *c);
		std::string permitFunc(struct cmdinfo *c);
		std::string makecomFunc(struct cmdinfo *c);
		std::string delcomFunc(struct cmdinfo *c);
		std::string addrecFunc(struct cmdinfo *c);
		std::string delrecFunc(struct cmdinfo *c);
		std::string listrecFunc(struct cmdinfo *c);
		std::string setrecFunc(struct cmdinfo *c);

		/* helpers */
		std::string extractCMLData(const std::string &httpResp) const;
		std::string extractHSData(const std::string &httpResp,
			uint8_t skillID) const;
		std::string extractGEData(const std::string &httpResp) const;
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
