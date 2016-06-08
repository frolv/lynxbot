#include <cpr/cpr.h>
#include <fstream>
#include <regex>
#include <tw/reader.h>
#include <utils.h>
#include "TwitchBot.h"
#include "version.h"

#define MAX_BUFFER_SIZE 2048

static const char *TWITCH_SERV = "irc.twitch.tv";
static const char *TWITCH_PORT = "80";

static std::string urltitle(const std::string &resp);

TwitchBot::TwitchBot(const std::string &nick, const std::string &channel,
		const std::string &password, const std::string &token,
		ConfigReader *cfgr)
	: m_connected(false), m_nick(nick), m_channelName(channel),
	m_token(token), m_client(TWITCH_SERV, TWITCH_PORT),
	m_cmdHandler(nick, channel.substr(1), token, &m_mod, &m_parser,
			&m_eventManager, &m_giveaway, cfgr), m_cfgr(cfgr),
	m_giveaway(channel.substr(1), time(nullptr), cfgr),
	m_mod(&m_parser, cfgr)
{
	if ((m_connected = m_client.cconnect())) {
		/* send required IRC data: PASS, NICK, USER */
		sendData("PASS " + password);
		sendData("NICK " + nick);
		sendData("USER " + nick);

		/* enable tags in PRIVMSGs */
		sendData("CAP REQ :twitch.tv/tags");

		/* join channel */
		sendData("JOIN " + channel);

		m_tick = std::thread(&TwitchBot::tick, this);

		/* create giveaway checking event */
		m_eventManager.add("checkgiveaway", 10, time(nullptr));

		/* read the subscriber message */
		m_subMsg = m_cfgr->get("submessage");
	}
}

TwitchBot::~TwitchBot()
{
	m_client.cdisconnect();
}

bool TwitchBot::isConnected() const
{
	return m_connected;
}

void TwitchBot::disconnect()
{
	m_client.cdisconnect();
	m_connected = false;
	m_tick.join();
}

void TwitchBot::serverLoop()
{
	std::string msg;
	/* continously receive data from server */
	while (true) {
		if (m_client.cread(msg) <= 0) {
			std::cerr << "No data received. Exiting." << std::endl;
			disconnect();
			break;
		}
		std::cout << "[RECV] " << msg << std::endl;
		processData(msg);
		if (!m_connected)
			break;
	}
}

bool TwitchBot::sendData(const std::string &data)
{
	/* format string by adding CRLF */
	std::string formatted = data
		+ (utils::endsWith(data, "\r\n") ? "" : "\r\n");
	/* send formatted data */
	int32_t bytes = m_client.cwrite(formatted);
	std::cout << (bytes > 0 ? "[SENT] " : "Failed to send: ")
		<< formatted << std::endl;

	/* return true iff data was sent succesfully */
	return bytes > 0;
}

bool TwitchBot::sendMsg(const std::string &msg)
{
	return sendData("PRIVMSG " + m_channelName + " :" + msg);
}

bool TwitchBot::sendPong(const std::string &ping)
{
	/* first six chars are "PING :", server name starts after */
	return sendData("PONG " + ping.substr(6));
}

void TwitchBot::processData(const std::string &data)
{
	if (data.find("Error logging in") != std::string::npos
			|| data.find("Login unsuccessful")
			!= std::string::npos) {
		disconnect();
		std::cerr << "\nCould not log in to Twitch IRC.\nMake sure "
			<< utils::configdir() << utils::config("config")
			<< " is configured correctly." << std::endl;
		std::cin.get();
	} else if (utils::startsWith(data, "PING")) {
		sendPong(data);
	} else if (data.find("PRIVMSG") != std::string::npos) {
		processPRIVMSG(data);
	}
}

bool TwitchBot::processPRIVMSG(const std::string &PRIVMSG)
{
	/* regex to extract all necessary data from message */
	static const std::regex privmsgRegex("subscriber=(\\d).*user-type=(.*) "
			":(\\w+)!\\3@\\3.* PRIVMSG (#\\w+) :(.+)");
	static const std::regex subRegex(":twitchnotify.* PRIVMSG (#\\w+) "
			":(.+) just subscribed!");
	std::smatch match;

	if (std::regex_search(PRIVMSG.begin(), PRIVMSG.end(),
			match, privmsgRegex)) {

		const bool subscriber = match[1].str() == "1";
		const std::string type = match[2].str();
		const std::string nick = match[3].str();
		const std::string channel = match[4].str();
		const std::string msg = match[5].str();

		/* confirm message is from current channel */
		if (channel != m_channelName)
			return false;

		/* channel owner or mod */
		const bool privileges = nick == channel.substr(1)
			|| !type.empty() || nick == "brainsoldier";

		/* check if the message contains a URL */
		m_parser.parse(msg);

		/* check if message is valid */
		if (!privileges && !subscriber && m_mod.active()
				&& moderate(nick, msg))
			return true;

		/* all chat commands start with $ */
		if (utils::startsWith(msg, "$") && msg.length() > 1) {
			std::string output = m_cmdHandler.processCommand(
					nick, msg.substr(1), privileges);
			if (!output.empty())
				sendMsg(output);
			return true;
		}

		/* count */
		if (m_cmdHandler.isCounting() && utils::startsWith(msg, "+")
				&& msg.length() > 1) {
			m_cmdHandler.count(nick, msg.substr(1));
			return true;
		}

		/* link information */
		if (m_parser.wasModified()) {
			URLParser::URL *url = m_parser.getLast();
			/* print info about twitter statuses */
			if (url->twitter && !url->tweetID.empty()) {
				tw::Reader twr(&m_auth);
				if (twr.read(url->tweetID)) {
					sendMsg(twr.result());
					return true;
				} else {
					std::cout << "Could not read tweet"
						<< std::endl;
					return false;
				}
			}
			/* get the title of the url otherwise */
			cpr::Response resp = cpr::Get(cpr::Url(url->full),
					cpr::Header{{ "Connection", "close" }});
			std::string title;
			std::string s = url->subdomain + url->domain;
			if (!(title = urltitle(resp.text)).empty()) {
				sendMsg("[URL] " + title + " (at " + s + ")");
				return true;
			}
			return false;
		}

		/* check for responses */
		std::string output = m_cmdHandler.processResponse(msg);
		if (!output.empty())
			sendMsg("@" + nick + ", " + output);

		return true;

	} else if (std::regex_search(PRIVMSG.begin(), PRIVMSG.end(),
			match, subRegex)) {
		const std::string nick = match[2].str();
		sendMsg("@" + nick + ", " + m_subMsg);
		return true;
	} else {
		std::cerr << "Could not extract data" << std::endl;
		return false;
	}
	return false;
}

bool TwitchBot::moderate(const std::string &nick, const std::string &msg)
{
	std::string reason;
	if (!m_mod.isValidMsg(msg, nick, reason)) {
		uint8_t offenses = m_mod.getOffenses(nick);
		static const std::string warnings[5] = { "first", "second",
			"third", "fourth", "FINAL" };
		std::string warning;
		if (offenses < 6) {
			/* timeout for 2^(offenses - 1) minutes */
			uint16_t t = 60 * (uint16_t)pow(2, offenses - 1);
			sendMsg("/timeout " + nick + " " + std::to_string(t));
			warning = warnings[offenses - 1] + " warning";
		} else {
			sendMsg("/ban " + nick);
			warning = "Permanently banned";
		}
		sendMsg(nick + " - " + reason + " (" + warning + ")");
		return true;
	}

	return false;
}

void TwitchBot::tick()
{
	while (m_connected) {
		/* check a set of variables every second and perform */
		/* actions if certain conditions are met */
		for (std::vector<std::string>::size_type i = 0;
				i < m_eventManager.messages()->size(); ++i) {
			if (m_eventManager.ready("msg" + std::to_string(i))) {
				if (m_eventManager.messagesActive())
					sendMsg(((*m_eventManager.messages())[i]).first);
				m_eventManager.setUsed("msg" + std::to_string(i));
				break;
			}
		}
		if (m_giveaway.active() && m_eventManager.ready("checkgiveaway")) {
			if (m_giveaway.checkConditions(time(nullptr)))
				sendMsg(m_giveaway.giveaway());
			m_eventManager.setUsed("checkgiveaway");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

static std::string urltitle(const std::string &resp)
{
	size_t start;
	if ((start = resp.find("<title>")) != std::string::npos) {
		std::string title;
		for (start += 7; resp[start] != '<'; ++start)
			title += resp[start] == '\n' ? ' ' : resp[start];
		return title;
	}
	return "";
}
