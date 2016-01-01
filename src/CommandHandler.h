#pragma once

#include "GEReader.h"

class GEReader;

class CommandHandler {

	public:
		CommandHandler();
		~CommandHandler();
		std::string processCommand(const std::string &nick, const std::string &fullCmd);
	private:
		GEReader m_GEReader;
		const std::string CML_HOST = "crystalmathlabs.com";
		const std::string CML_EHP_AHI = "/tracker/api.php?type=virtualhiscoresatplayer&page=timeplayed&player=";
		const std::string RS_HOST = "services.runescape.com";
		const std::string RS_HS_API = "/m=hiscore_oldschool/index_lite.ws?player=";
		const std::string EXCHANGE_HOST = "api.rsbuddy.com";
		const std::string EXCHANGE_API = "/grandExchange?a=guidePrice&i=";
		std::string handleCalc(const std::string &fullCmd);
		std::string extractCMLData(const std::string &httpResp, const std::string &rsn);
		std::string extractHSData(const std::string &httpResp, uint8_t skillID);
		std::string extractGEData(const std::string &httpResp);

};