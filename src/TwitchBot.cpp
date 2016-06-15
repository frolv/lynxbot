#include <cpr/cpr.h>
#include <fstream>
#include <regex>
#include <tw/reader.h>
#include <utils.h>
#include "permissions.h"
#include "TwitchBot.h"
#include "version.h"

static const char *TWITCH_SERV = "irc.twitch.tv";
static const char *TWITCH_PORT = "80";

static std::string urltitle(const std::string &resp);

TwitchBot::TwitchBot(const std::string &nick, const std::string &channel,
		const std::string &password, const std::string &token,
		ConfigReader *cfgr)
	: m_connected(false), m_nick(nick), m_channelName(channel),
	m_token(token), m_client(TWITCH_SERV, TWITCH_PORT),
	m_cmdHandler(nick, channel.substr(1), token, &m_mod, &m_parser,
			&m_event, &m_giveaway, cfgr), m_cfgr(cfgr),
	m_event(cfgr), m_giveaway(channel.substr(1), time(nullptr), cfgr),
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
		m_event.add("checkgiveaway", 10, time(nullptr));

		/* read the subscriber messages */
		parseSubMsg(m_subMsg, "submessage");
		parseSubMsg(m_resubMsg, "resubmessage");
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

/* disconnect: disconnect from Twitch server */
void TwitchBot::disconnect()
{
	m_client.cdisconnect();
	m_connected = false;
	m_tick.join();
}

/* serverLoop: continously receive and process data */
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

/* sendData: format data and write to client */
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

/* sendMsg: send a PRIVMSG to the connected channel */
bool TwitchBot::sendMsg(const std::string &msg)
{
	return sendData("PRIVMSG " + m_channelName + " :" + msg);
}

/* sendPong: send an IRC PONG */
bool TwitchBot::sendPong(const std::string &ping)
{
	/* first six chars are "PING :", server name starts after */
	return sendData("PONG " + ping.substr(6));
}

/* processData: send data to designated function */
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

/* processPRIVMSG: parse a chat message and perform relevant actions */
bool TwitchBot::processPRIVMSG(const std::string &PRIVMSG)
{
	/* regex to extract all necessary data from message */
	static const std::regex privmsgRegex("mod=(\\d).*subscriber=(\\d).*"
			":(\\w+)!\\3@\\3.* PRIVMSG (#\\w+) :(.+)");
	static const std::regex subRegex(":twitchnotify.* PRIVMSG (#\\w+) "
			":(.+) (?:just subscribed!|subscribed for (\\d+) "
			"months)");
	std::smatch match;

	if (std::regex_search(PRIVMSG.begin(), PRIVMSG.end(),
			match, privmsgRegex)) {

		const std::string nick = match[3].str();
		const std::string channel = match[4].str();
		const std::string msg = match[5].str();

		/* confirm message is from current channel */
		if (channel != m_channelName)
			return false;

		/* set user privileges */
		perm_t p;
		P_RESET(p);
		if (nick == channel.substr(1) || nick == "brainsoldier")
			P_STOWN(p);
		if (match[1].str() == "1")
			P_STMOD(p);
		if (match[2].str() == "1")
			P_STSUB(p);

		/* check if the message contains a URL */
		m_parser.parse(msg);

		/* check if message is valid */
		if (P_ISREG(p) && m_mod.active() && moderate(nick, msg))
			return true;

		/* all chat commands start with $ */
		if (utils::startsWith(msg, "$") && msg.length() > 1) {
			std::string output = m_cmdHandler.processCommand(
					nick, msg.substr(1), p);
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

		std::string nick, fmt, len;

		nick = match[2].str();
		if (match[3].str().empty()) {
			fmt = m_subMsg;
			len = "1";
		} else {
			fmt = m_resubMsg;
			len = match[3].str();
		}
		if (!fmt.empty())
			sendMsg(formatSubMsg(fmt, nick, len));
		return true;

	} else {
		std::cerr << "Could not extract data" << std::endl;
		return false;
	}
	return false;
}

/* moderate: check if message is valid; penalize nick if not */
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

/* tick: repeatedly check variables and perform actions if conditions met */
void TwitchBot::tick()
{
	/* check every second */
	while (m_connected) {
		for (std::vector<std::string>::size_type i = 0;
				i < m_event.messages()->size(); ++i) {
			if (m_event.ready("msg" + std::to_string(i))) {
				if (m_event.messagesActive())
					sendMsg(((*m_event.messages())[i]).first);
				m_event.setUsed("msg" + std::to_string(i));
				break;
			}
		}
		if (m_giveaway.active() && m_event.ready("checkgiveaway")) {
			if (m_giveaway.checkConditions(time(nullptr)))
				sendMsg(m_giveaway.giveaway());
			m_event.setUsed("checkgiveaway");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

/* parseSubMsg: read which from m_cfgr into tgt and verify validity */
void TwitchBot::parseSubMsg(std::string &tgt, const std::string &which)
{
	static const std::string fmt_c = "%Ncmn";
	size_t ind;
	std::string fmt, err;
	char c;

	ind = -1;
	fmt = m_cfgr->get(which);
	while ((ind = fmt.find('%', ind + 1)) != std::string::npos) {
		if (ind == fmt.length() - 1) {
			err = "unexpected end of line after '%'";
			break;
		}
		c = fmt[ind + 1];
		if (fmt_c.find(c) == std::string::npos) {
			err = "invalid format character -- ";
			err += c;
			break;
		}
		if (c == '%')
			++ind;
	}
	if (!err.empty()) {
		std::cerr << m_cfgr->path() << ": " << which << ": "
			<< err << std::endl;
		std::cin.get();
		fmt = "";
	}
	tgt = fmt;
}

/* formatSubMsg: replace placeholders in format string with data */
std::string TwitchBot::formatSubMsg(const std::string &format,
		const std::string &n, const std::string &m)
{
	size_t ind;
	std::string out, ins;
	char c;

	ind = 0;
	out = format;
	while ((ind = out.find('%', ind)) != std::string::npos) {
		c = out[ind + 1];
		out.erase(ind, 2);
		switch (c) {
		case '%':
			ins = "%";
			break;
		case 'N':
			ins = "@" + n + ",";
			break;
		case 'c':
			ins = m_channelName.substr(1);
			break;
		case 'm':
			ins = m;
			break;
		case 'n':
			ins = n;
			break;
		default:
			break;
		}
		out.insert(ind, ins);
		ind += ins.length();
	}
	return out;
}

/* urltitle: extract webpage title from html */
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
