#pragma once

#include "GEReader.h"
#include "TimerManager.h"
#include "cmdmodules\SelectionWheel.h"

class GEReader;
class TimerManager;
class SelectionWheel;

class CommandHandler {

	public:
		CommandHandler();
		~CommandHandler();
		std::string processCommand(const std::string &nick, const std::string &fullCmd, bool privileges);
		std::string processResponse(const std::string &message);
	private:
		std::map<std::string, std::string(CommandHandler::*)(const std::string &)> m_defaultCmds;
		GEReader m_GEReader;
		TimerManager m_timerManager;
		SelectionWheel m_wheel;
		Json::Value m_responses;
		bool m_responding;
		const std::string CML_HOST = "crystalmathlabs.com";
		const std::string CML_EHP_AHI = "/tracker/api.php?type=virtualhiscoresatplayer&page=timeplayed&player=";
		const std::string RS_HOST = "services.runescape.com";
		const std::string RS_HS_API = "/m=hiscore_oldschool/index_lite.ws?player=";
		const std::string EXCHANGE_HOST = "api.rsbuddy.com";
		const std::string EXCHANGE_API = "/grandExchange?a=guidePrice&i=";
		std::string ehpFunc(const std::string &fullCmd);
		std::string levelFunc(const std::string &fullCmd);
		std::string geFunc(const std::string &fullCmd);
		std::string calcFunc(const std::string &fullCmd);
		std::string cmlFunc(const std::string &fullCmd);
		std::string wheelFunc(const std::string &fullCmd, const std::string &nick);
		std::string extractCMLData(const std::string &httpResp, const std::string &rsn);
		std::string extractHSData(const std::string &httpResp, uint8_t skillID);
		std::string extractGEData(const std::string &httpResp);

};