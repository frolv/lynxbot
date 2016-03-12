#pragma once

#include "Moderator.h"
#include "CustomCommandHandler.h"
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
		typedef std::map<std::string, std::string(CommandHandler::*)(const std::string &, const std::string &, bool)> commandMap;
		CommandHandler(const std::string &name, Moderator *mod, URLParser *urlp, EventManager *evtp);
		~CommandHandler();
		std::string processCommand(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string processResponse(const std::string &message);
		bool isCounting() const;
		void count(const std::string &nick, std::string &message);
	private:
		std::string m_name;
		Moderator *m_modp;
		commandMap m_defaultCmds;
		GEReader m_GEReader;
		TimerManager m_cooldowns;
		SelectionWheel m_wheel;
		CustomCommandHandler m_customCmds;
		URLParser *m_parsep;
		EventManager *m_evtp;
		Json::Value m_responses;
		bool m_responding;
		bool m_counting;
		std::vector<std::string> m_usersCounted;
		std::unordered_map<std::string, uint16_t> m_messageCounts;
		std::random_device m_rd;
		std::mt19937 m_gen;
		const std::string CML_HOST = "crystalmathlabs.com";
		const std::string CML_EHP_API = "/tracker/api.php?type=virtualhiscoresatplayer&page=timeplayed&player=";
		const std::string RS_HOST = "services.runescape.com";
		const std::string RS_HS_API = "/m=hiscore_oldschool/index_lite.ws?player=";
		const std::string EXCHANGE_HOST = "api.rsbuddy.com";
		const std::string EXCHANGE_API = "/grandExchange?a=guidePrice&i=";
		const std::string STRAWPOLL_HOST = "strawpoll.me";
		const std::string STRAWPOLL_API = "/api/v2/polls";
		const std::string SOURCE = "https://github.com/frolv/lynxbot";
		std::string m_activePoll;
		std::vector<std::string> m_eightballResponses = { "It is certain", "It is decidedly so", "Without a doubt", "Yes, definitely",
			"You may rely on it", "As I see it, yes", "Most likely", "Outlook good", "Yes", "Signs point to yes",
			"Reply hazy try again", "Ask again later", "Better not tell you now", "Cannot predict now", "Concentrate and ask again",
			"Don't count on it", "My reply is no", "My sources say no", "Outlook not so good", "Very doubtful" };

		/* default bot commands */
		std::string ehpFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string levelFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string geFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string calcFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string cmlFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string wheelFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string eightballFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string strawpollFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string activeFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string commandsFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string aboutFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string countFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string whitelistFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string permitFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string makecomFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string delcomFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string addrecFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string delrecFunc(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string listrecFunc(const std::string &nick, const std::string &fullCmd, bool privileges);

		/* helpers */
		std::string extractCMLData(const std::string &httpResp, const std::string &rsn) const;
		std::string extractHSData(const std::string &httpResp, uint8_t skillID) const;
		std::string extractGEData(const std::string &httpResp) const;
};