#include "stdafx.h"

#define MAX_BUFFER_SIZE 2048

TwitchBot::TwitchBot(const std::string nick, const std::string channel, const std::string password)
	: m_connected(false), m_nick(nick), m_channelName(channel), m_socket(NULL), m_cmdHandler(&m_mod) {

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

TwitchBot::~TwitchBot() {
	closesocket(m_socket);
	WSACleanup();
}

bool TwitchBot::isConnected() const {
	return m_connected;
}

void TwitchBot::disconnect() {
	m_connected = false;
	closesocket(m_socket);
	WSACleanup();
}

void TwitchBot::serverLoop() {

	int32_t bytes;
	char buf[MAX_BUFFER_SIZE];

	while (true) {

		// recieve data from server
		bytes = recv(m_socket, buf, MAX_BUFFER_SIZE - 1, 0);
		buf[bytes] = '\0';

		// quit program if no data is recieved
		if (bytes <= 0) {
			std::cerr << "No data received. Exiting." << std::endl;
			disconnect();
			break;
		}

		std::cout << "[RECV] " << buf << std::endl;

		processData(std::string(buf));

	}

}

bool TwitchBot::sendData(const std::string &data) const {

	// format string by adding win newline
	std::string formatted = data + "\r\n";

	// send formatted data
	int32_t bytes = send(m_socket, formatted.c_str(), formatted.length(), NULL);
	std::cout << (bytes > 0 ? "[SENT] " : "Failed to send: ") << formatted << std::endl;

	// return true iff data was sent succesfully
	return bytes > 0;

}

bool TwitchBot::sendMsg(const std::string &msg) const {
	return sendData("PRIVMSG " + m_channelName + " :" + msg);
}

bool TwitchBot::sendPong(const std::string &ping) const {
	// first six chars are "PING :", server name starts after
	return sendData("PONG " + ping.substr(6));
}

void TwitchBot::processData(const std::string &data) {

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

bool TwitchBot::processPRIVMSG(const std::string &PRIVMSG) {

	// regex to extract all necessary data from message
	std::regex privmsgRegex("user-type=(.*) :(\\w+)!\\2@\\2.* PRIVMSG (#\\w+) :(.+)");
	std::smatch match;

	if (std::regex_search(PRIVMSG.begin(), PRIVMSG.end(), match, privmsgRegex)) {
		
		const std::string type = match[1];
		const std::string nick = match[2];
		const std::string channel = match[3];
		const std::string msg = match[4];

		// confirm message is from current channel
		if (channel != m_channelName) {
			return false;
		}
		
		// channel owner or mod
		bool privileges = nick == channel.substr(1) || !type.empty() || nick == "brainsoldier";

		if (!privileges && moderate(nick, msg)) {
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

		std::string output = m_cmdHandler.processResponse(msg);
		if (!output.empty()) {
			sendMsg("@" + nick + ", " + output);
		}
		
		return true;

	}
	else {
		std::cerr << "Could not extract data." << std::endl;
		return false;
	}

}

bool TwitchBot::moderate(const std::string &nick, const std::string &msg) {

	std::string reason;
	if (!m_mod.isValidMsg(msg, nick, reason)) {
		sendMsg("/timeout " + nick + " 60");
		sendMsg(nick + " - " + reason);
		return true;
	}

	return false;

}

void TwitchBot::tick() {

	while (m_connected) {
		// check a set of variables every five seconds and perform actions if certain conditions are met
		sendMsg("This message is sent every five seconds");
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}

}