#include <fstream>
#include <regex>
#include "TwitchBot.h"
#include "utils.h"
#include "version.h"

#define MAX_BUFFER_SIZE 2048

static const char *TWITCH_SERV = "irc.twitch.tv";
static const char *TWITCH_PORT = "80";

TwitchBot::TwitchBot(const std::string nick, const std::string channel,
	const std::string password)
	: m_connected(false), m_nick(nick), m_channelName(channel),
	m_client(TWITCH_SERV, TWITCH_PORT), m_mod(&m_parser),
	m_cmdHandler(nick, channel.substr(1), &m_mod, &m_parser, &m_eventManager),
	m_giveaway(channel.substr(1), time(nullptr))
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

		/* create giveaway checking event if active */
		if (m_giveaway.active())
			m_eventManager.add("checkgiveaway", 10, time(nullptr));

		/* read the subscriber message */
		std::string path = utils::configdir() + utils::config("submessage");
		std::ifstream reader(path);
		if (reader.is_open())
			std::getline(reader, m_subMsg);
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
	if (data.find("Error logging in") != std::string::npos) {
		disconnect();
		std::cerr << "\nCould not log in to Twitch IRC.\nMake sure "
			<< utils::configdir() << utils::config("settings")
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
		if (!privileges && !subscriber && moderate(nick, msg))
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
			if (url->twitter && !url->tweetID.empty()) {
				/* stuff */
			}
			return true;
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
		std::cerr << "Could not extract data." << std::endl;
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
	uint8_t reason;
	while (m_connected) {
		/* check a set of variables every second and perform
		 * actions if certain conditions are met */
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
			if (m_giveaway.checkConditions(time(nullptr), reason)) {
				std::string output = "[GIVEAWAY: ";
				output += reason == 1 ? "followers" : "timed";
				output += "] ";
				output += m_giveaway.getItem();
				output += reason == 1 ? " (next code in "
					+ std::to_string(m_giveaway.followers())
					+ " followers)" : "";
				sendMsg(output);
			}
			m_eventManager.setUsed("checkgiveaway");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
