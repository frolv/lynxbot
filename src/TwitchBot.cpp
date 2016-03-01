#include <http/http.h>
#include "stdafx.h"
#include "TwitchBot.h"

#define MAX_BUFFER_SIZE 2048

TwitchBot::TwitchBot(const std::string nick, const std::string channel, const std::string password)
	: m_connected(false), m_nick(nick), m_channelName(channel), m_socket(NULL), m_mod(&m_parser),
	m_cmdHandler(nick, &m_mod, &m_parser), m_giveaway(channel.substr(1), time(nullptr))
{

	const std::string serv = "irc.twitch.tv", port = "6667";

	m_connected = utils::socketConnect(m_socket, m_wsa, port.c_str(), serv.c_str());
	if (m_connected) {

		// send required IRC data: PASS, NICK, USER
		sendData("PASS " + password);
		sendData("NICK " + nick);
		sendData("USER " + nick);

		// enables tags in PRIVMSGs
		sendData("CAP REQ :twitch.tv/tags");

		// join channel
		sendData("JOIN " + channel);

		m_tick = std::thread(&TwitchBot::tick, this);

	}

}

TwitchBot::~TwitchBot()
{
	closesocket(m_socket);
	WSACleanup();
}

bool TwitchBot::isConnected() const
{
	return m_connected;
}

void TwitchBot::disconnect()
{
	m_connected = false;
	closesocket(m_socket);
	WSACleanup();
}

void TwitchBot::serverLoop()
{

	int32_t bytes;
	char buf[MAX_BUFFER_SIZE];

	while (true) {

		// receive data from server
		bytes = recv(m_socket, buf, MAX_BUFFER_SIZE - 1, 0);
		buf[bytes] = '\0';

		// quit program if no data is received
		if (bytes <= 0) {
			std::cerr << "No data received. Exiting." << std::endl;
			disconnect();
			break;
		}

		std::cout << "[RECV] " << buf << std::endl;

		processData(std::string(buf));

	}

}

bool TwitchBot::sendData(const std::string &data) const
{

	// format string by adding CRLF
	std::string formatted = data + (utils::endsWith(data, "\r\n") ? "" : "\r\n");

	// send formatted data
	int32_t bytes = send(m_socket, formatted.c_str(), formatted.length(), NULL);
	std::cout << (bytes > 0 ? "[SENT] " : "Failed to send: ") << formatted << std::endl;

	// return true iff data was sent succesfully
	return bytes > 0;

}

bool TwitchBot::sendMsg(const std::string &msg) const
{
	return sendData("PRIVMSG " + m_channelName + " :" + msg);
}

bool TwitchBot::sendPong(const std::string &ping) const
{
	// first six chars are "PING :", server name starts after
	return sendData("PONG " + ping.substr(6));
}

void TwitchBot::processData(const std::string &data)
{

	if (data.find("Error logging in") != std::string::npos) {
		disconnect();
		std::cerr << "\nCould not log in to Twitch IRC. Make sure your settings.txt file is configured correctly." << std::endl;
		std::cin.get();
	}
	else if (utils::startsWith(data, "PING")) {
		sendPong(data);
	}
	else if (data.find("PRIVMSG") != std::string::npos) {
		processPRIVMSG(data);
	}

}

bool TwitchBot::processPRIVMSG(const std::string &PRIVMSG)
{

	// regex to extract all necessary data from message
	std::regex privmsgRegex("subscriber=(\\d).*user-type=(.*) :(\\w+)!\\3@\\3.* PRIVMSG (#\\w+) :(.+)");
	std::regex subRegex(":twitchnotify.* PRIVMSG (#\\w+) :(.+) just subscribed!");
	std::smatch match;

	if (std::regex_search(PRIVMSG.begin(), PRIVMSG.end(), match, privmsgRegex)) {

		const bool subscriber = match[1].str() == "1";
		const std::string type = match[2].str();
		const std::string nick = match[3].str();
		const std::string channel = match[4].str();
		const std::string msg = match[5].str();

		// confirm message is from current channel
		if (channel != m_channelName) {
			return false;
		}
		
		// channel owner or mod
		const bool privileges = nick == channel.substr(1) || !type.empty() || nick == "brainsoldier";

		// check if the message contains a URL
		m_parser.parse(msg);

		// check if message is valid
		if (!privileges && !subscriber && moderate(nick, msg)) {
			return true;
		}

		// all chat commands start with $
		if (utils::startsWith(msg, "$") && msg.length() > 1) {
			std::string output = m_cmdHandler.processCommand(nick, msg.substr(1), privileges);
			if (!output.empty()) {
				sendMsg(output);
			}
			return true;
		}

		// count
		if (m_cmdHandler.isCounting() && utils::startsWith(msg, "+") && msg.length() > 1) {
			m_cmdHandler.count(nick, msg.substr(1));
			return true;
		}

		// link information
		if (m_parser.wasModified()) {
			URLParser::URL *url = m_parser.getLast();
			if (url->twitter && !url->tweetID.empty()) {
				// stuff
			}
			return true;
		}

		std::string output = m_cmdHandler.processResponse(msg);
		if (!output.empty()) {
			sendMsg("@" + nick + ", " + output);
		}
		
		return true;

	}
	/*
	else if (m_giveaway.checkSubs() && std::regex_search(PRIVMSG.begin(), PRIVMSG.end(), match, subRegex)) {
		/* subscriber giveaway * /
		const std::string channel = match[1].str();
		const std::string nick = match[2].str();
		sendMsg("/w " + nick + " Thanks for subscribing to " + channel + "! Enjoy your prize: " + m_giveaway.getItem());
		return true;
	}
	*/
	else {
		std::cerr << "Could not extract data." << std::endl;
		return false;
	}

}

bool TwitchBot::moderate(const std::string &nick, const std::string &msg)
{

	std::string reason;
	if (!m_mod.isValidMsg(msg, nick, reason)) {
		uint8_t offenses = m_mod.getOffenses(nick);
		static const std::string warnings[5] = { "first", "second", "third", "fourth", "FINAL" };
		std::string warning;
		if (offenses < 6) {
			// timeout for 60 * 2^(offenses - 1) seconds
			sendMsg("/timeout " + nick + " " + std::to_string(60 * (uint16_t)pow(2, offenses - 1)));
			warning = warnings[offenses - 1] + " warning";
		}
		else {
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

	std::time_t curr, last = time(nullptr), last2 = last - 1150, last3 = last;
	while (m_connected) {
		// check a set of variables every second and perform actions if certain conditions are met
		if ((curr = time(nullptr)) - last >= 300) {
			sendMsg("Remember to follow https://twitter.com/RandaliciousRS and https://twitter.com/brainsoldier . God bless.");
			last = curr;
		}
		else if (curr - last2 > 1200) {
			sendMsg("Watch the latest Hexis podcast: https://www.youtube.com/watch?v=Kl9I6RnY9GM Submit topics for the next podcast here: http://bit.ly/1PXYEj1");
			last2 = curr;
		}
		if (curr - last3 > 10) {
			if (m_giveaway.active() && m_giveaway.checkConditions(curr)) {
				sendMsg("[GIVEAWAY] " + m_giveaway.getItem());
			}
			last3 = curr;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

}