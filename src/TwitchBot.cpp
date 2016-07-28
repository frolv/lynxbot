#include <cpr/cpr.h>
#include <cstdio>
#include <fstream>
#include <regex>
#include <tw/reader.h>
#include <utils.h>
#include "lynxbot.h"
#include "TwitchBot.h"

#define MAX_LEN 1024

static const char *TWITCH_SERV = "irc.twitch.tv";
static const char *TWITCH_PORT = "80";

static std::string urltitle(const std::string &resp);

TwitchBot::TwitchBot(const std::string &nick, const std::string &channel,
		const std::string &password, const std::string &token,
		ConfigReader *cfgr)
	: m_connected(false), m_password(password.c_str()), m_nick(nick),
	m_channel(channel), m_token(token), m_client(TWITCH_SERV, TWITCH_PORT),
	m_cmdHandler(nick, channel.substr(1), token, &m_mod, &m_parser,
			&m_event, &m_giveaway, cfgr, &m_auth), m_cfgr(cfgr),
	m_event(cfgr), m_giveaway(channel.substr(1), time(nullptr), cfgr),
	m_mod(&m_parser, cfgr)
{
	std::string err;

	/* create giveaway checking event */
	m_event.add("checkgiveaway", 10, time(nullptr));

	/* read the subscriber messages */
	parseSubMsg(m_subMsg, "submessage");
	parseSubMsg(m_resubMsg, "resubmessage");

	if (!utils::parseBool(m_urltitles, m_cfgr->get("url_titles"),
				err)) {
		std::cerr << m_cfgr->path() << ": url_titles: "
			<< err << " (defaulting to true)" << std::endl;
		m_urltitles = true;
		std::cin.get();
	}
}

TwitchBot::~TwitchBot()
{
	m_client.cdisconnect();
}

bool TwitchBot::connected() const
{
	return m_connected;
}

/* connect: connect to IRC */
bool TwitchBot::connect()
{
	char buf[MAX_LEN];

	if (!(m_connected = m_client.cconnect()))
		return false;

	/* send required IRC data: PASS, NICK, USER */
	_sprintf(buf, MAX_LEN, "PASS %s", m_password);
	send_raw(buf);
	_sprintf(buf, MAX_LEN, "NICK %s", m_nick.c_str());
	send_raw(buf);
	_sprintf(buf, MAX_LEN, "USER %s", m_nick.c_str());
	send_raw(buf);

	/* enable tags in PRIVMSGs */
	_sprintf(buf, MAX_LEN, "CAP REQ :twitch.tv/tags");
	send_raw(buf);

	/* join channel */
	_sprintf(buf, MAX_LEN, "JOIN %s", m_channel.c_str());
	send_raw(buf);

	m_tick = std::thread(&TwitchBot::tick, this);
	return true;
}

/* disconnect: disconnect from Twitch server */
void TwitchBot::disconnect()
{
	m_client.cdisconnect();
	m_connected = false;
	m_tick.join();
}

/* server_loop: continously receive and process data */
void TwitchBot::server_loop()
{
	char buf[MAX_LEN];

	/* continously receive data from server */
	while (true) {
		if (m_client.cread(buf, MAX_LEN) <= 0) {
			fprintf(stderr, "No data received. Exiting.\n");
			disconnect();
			break;
		}
		printf("[RECV] %s\n", buf);
		process_data(buf);
		if (!m_connected)
			break;
	}
}

/* send_raw: format data and send to client */
bool TwitchBot::send_raw(char *data)
{
	size_t end;
	int bytes;

	if ((end = strlen(data)) > MAX_LEN - 3) {
		fprintf(stderr, "Message too long: %s\n", data);
		return false;
	}

	if (data[end - 1] != '\n' && data[end - 2] != '\r')
		strcpy(data + end, "\r\n");

	/* send formatted data */
	bytes = m_client.cwrite(data);
	printf("%s %s\n", bytes > 0 ? "[SENT]" : "Failed to send:", data);

	/* return true iff data was sent succesfully */
	return bytes > 0;
}

/* send_msg: send a PRIVMSG to the connected channel */
bool TwitchBot::send_msg(const std::string &msg)
{
	char buf[MAX_LEN];

	_sprintf(buf, MAX_LEN, "PRIVMSG %s :%s", m_channel.c_str(), msg.c_str());
	return send_raw(buf);
}

/* sendPong: send an IRC PONG */
bool TwitchBot::pong(char *ping)
{
	/* first six chars are "PING :", server name starts after */
	strcpy(++ping, "PONG");
	ping[4] = ' ';
	return send_raw(ping);
}

/* processData: send data to designated function */
void TwitchBot::process_data(char *data)
{
	if (strstr(data, "PRIVMSG")) {
		process_privmsg(data);
	} else if (strstr(data, "PING")) {
		pong(data);
	} else if (strstr(data, "Error log") || strstr(data, "Login unsuccess")) {
		disconnect();
		fprintf(stderr, "\nCould not login to Twitch IRC.\nMake sure "
				"%s is configured correctly\n",
				utils::config("config").c_str());
		WAIT_INPUT();
	}
}

/* process_privmsg: parse a chat message and perform relevant actions */
bool TwitchBot::process_privmsg(char *privmsg)
{
	char *nick, *msg;
	perm_t p;

	if (strstr(privmsg, ":twitchnotify") == privmsg) {
		/* parse_submsg(privmsg); */
		return true;
	}

	if (!parse_privmsg(privmsg, &nick, &msg, &p))
		return false;

	/* check if message contains a URL */
	m_parser.parse(std::string(msg));

	/* perform message moderation */
	if (P_ISREG(p) && m_mod.active() && moderate(nick, std::string(msg)))
		return true;

	if (msg[0] == '$' && msg[1]) {
		std::string out = m_cmdHandler.processCommand(nick, msg + 1, p);
		if (!out.empty())
			send_msg(out);
		return true;
	}

	return true;

	/* 	/1* count *1/ */
	/* 	if (m_cmdHandler.isCounting() && utils::startsWith(msg, "+") */
	/* 			&& msg.length() > 1) { */
	/* 		m_cmdHandler.count(nick, msg.substr(1)); */
	/* 		return true; */
	/* 	} */

	/* 	/1* link information *1/ */
	/* 	if (m_parser.wasModified()) { */
	/* 		URLParser::URL *url = m_parser.getLast(); */
	/* 		/1* print info about twitter statuses *1/ */
	/* 		if (url->twitter && !url->tweetID.empty()) { */
	/* 			tw::Reader twr(&m_auth); */
	/* 			if (twr.read_tweet(url->tweetID)) { */
	/* 				send_msg(twr.result()); */
	/* 				return true; */
	/* 			} */
	/* 			std::cerr << "could not read tweet" << std::endl; */
	/* 			return false; */
	/* 		} */
	/* 		/1* get the title of the url otherwise *1/ */
	/* 		if (m_urltitles) { */
	/* 			resp = cpr::Get(cpr::Url(url->full), */
	/* 					cpr::Header{{ "Connection", "close" }}); */
	/* 			std::string title; */
	/* 			std::string s = url->subdomain + url->domain; */
	/* 			if (!(title = urltitle(resp.text)).empty()) { */
	/* 				send_msg("[URL] " + title */
	/* 						+ " (at " + s + ")"); */
	/* 				return true; */
	/* 			} */
	/* 			return false; */
	/* 		} */
	/* 	} */

	/* 	/1* check for responses *1/ */
	/* 	std::string output = m_cmdHandler.processResponse(msg); */
	/* 	if (!output.empty()) */
	/* 		send_msg("@" + nick + ", " + output); */

	/* 	return true; */

	/* } else if (std::regex_search(PRIVMSG.begin(), PRIVMSG.end(), */
	/* 			match, subRegex)) { */
	/* 	/1* send sub/resub messages *1/ */
	/* 	std::string nick, fmt, len; */

	/* 	nick = match[2].str(); */
	/* 	if (match[3].str().empty()) { */
	/* 		fmt = m_subMsg; */
	/* 		len = "1"; */
	/* 	} else { */
	/* 		fmt = m_resubMsg; */
	/* 		len = match[3].str(); */
	/* 	} */
	/* 	if (!fmt.empty()) */
	/* 		send_msg(formatSubMsg(fmt, nick, len)); */
	/* 	return true; */

	/* } else { */
	/* 	std::cerr << "Could not extract data" << std::endl; */
	/* 	return false; */
	/* } */
	/* return false; */
}

/* parse_privmsg: parse twitch message to extract nick, msg and permissions */
bool TwitchBot::parse_privmsg(char *privmsg, char **nick, char **msg, perm_t *p)
{
	char *tok, *s;

	if (!(*nick = strstr(privmsg, " :")))
		return false;
	*nick += 2;
	if (!(s = strchr(*nick, '!')))
		return false;
	*s = '\0';

	/* set user privileges */
	P_RESET(*p);
	if (strcmp(*nick, m_channel.c_str() + 1) == 0
			|| strcmp(*nick, "brainsoldier") == 0)
		P_STOWN(*p);
	tok = strstr(privmsg, "mod=");
	if (tok[4] == '1')
		P_STMOD(*p);
	tok = strstr(tok, "subscriber=");
	if (tok[11] == '1')
		P_STSUB(*p);

	/* extract channel and check if equivalent to bot's */
	tok = strchr(s + 1, '#');
	s = strchr(tok, ' ');
	*s = '\0';
	if (strcmp(tok, m_channel.c_str()) != 0)
		return false;

	/* read actual message into msg */
	*msg = s + 2;
	if ((s = strchr(*msg, '\r')))
		*s = '\0';

	return true;
}

/* moderate: check if message is valid; penalize nick if not */
bool TwitchBot::moderate(const std::string &nick, const std::string &msg)
{
	std::string reason;
	if (!m_mod.isValidMsg(msg, nick, reason)) {
		uint8_t offenses = m_mod.getOffenses(nick);
		static const std::string warnings[3] = { "first", "second",
			"FINAL" };
		std::string warning;
		if (offenses < 4) {
			/* timeout for 2^(offenses - 1) minutes */
			uint16_t t = 60 * (uint16_t)pow(2, offenses - 1);
			send_msg("/timeout " + nick + " " + std::to_string(t));
			warning = warnings[offenses - 1] + " warning";
		} else {
			send_msg("/ban " + nick);
			warning = "Permanently banned";
		}
		send_msg(nick + " - " + reason + " (" + warning + ")");
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
					send_msg(((*m_event.messages())[i]).first);
				m_event.setUsed("msg" + std::to_string(i));
				break;
			}
		}
		if (m_giveaway.active() && m_event.ready("checkgiveaway")) {
			if (m_giveaway.checkConditions(time(nullptr)))
				send_msg(m_giveaway.giveaway());
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
			err = "invalid format character -- '";
			err += c;
			err += "'";
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
			ins = m_channel.substr(1);
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
	size_t i;
	std::string title;

	if ((i = resp.find("<title>")) != std::string::npos) {
		title = resp.substr(i + 7);
		title = title.substr(0, title.find('<'));
		return utils::decode(title);
	}
	return "";
}
