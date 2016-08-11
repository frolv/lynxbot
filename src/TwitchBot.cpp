#include <cpr/cpr.h>
#include <cstdio>
#include <fstream>
#include <regex>
#include <tw/reader.h>
#include <utils.h>
#include "lynxbot.h"
#include "timers.h"
#include "TwitchBot.h"

static const char *TWITCH_SERV = "irc.twitch.tv";
static const char *TWITCH_PORT = "80";

static std::string urltitle(const std::string &resp);

TwitchBot::TwitchBot(const char *name, const char *channel,
		const char *password, const char *token,
		ConfigReader *cfgr)
	: m_connected(false), m_password(password), m_nick(name),
	m_channel(channel), m_token(token),
	m_cmdhnd(name, channel + 1, token, &m_mod, &m_parser, &m_event,
			&m_giveaway, cfgr, &m_auth),
	m_cfgr(cfgr), m_event(cfgr),
	m_giveaway(channel + 1, time(nullptr), cfgr),
	m_mod(name, &m_parser, cfgr)
{
	std::string err;
	bool error;

	init_timers(m_channel + 1);
	/* create uptime checking event */
	m_event.add("checkuptime", 60, time(nullptr));

	/* create giveaway checking event */
	m_event.add("checkgiveaway", 10, time(nullptr));

	/* read the subscriber messages */
	parseSubMsg(m_subMsg, "submessage");
	parseSubMsg(m_resubMsg, "resubmessage");

	error = false;
	if (!utils::parseBool(m_urltitles, m_cfgr->get("url_titles"),
				err)) {
		fprintf(stderr, "%s: url_titles: %s (defaulting to true)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_urltitles = true;
		error = true;
	}
	if (!utils::parseBool(m_familiarity, m_cfgr->get("familiarity_mode"),
				err)) {
		fprintf(stderr, "%s: familiarity_mode: %s "
				"(defaulting to false)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_familiarity = false;
		error = true;
	}
	if (error)
		WAIT_INPUT();
}

TwitchBot::~TwitchBot()
{
	disconnect();
}

bool TwitchBot::connected() const
{
	return m_connected;
}

/* connect: connect to IRC */
bool TwitchBot::connect()
{
	char buf[MAX_MSG];

	if (!(m_connected = !cconnect(&m_client, TWITCH_SERV, TWITCH_PORT)))
		return false;

	/* send required IRC data: PASS, NICK, USER */
	_sprintf(buf, MAX_MSG, "PASS %s", m_password);
	send_raw(buf);
	_sprintf(buf, MAX_MSG, "NICK %s", m_nick);
	send_raw(buf);
	_sprintf(buf, MAX_MSG, "USER %s", m_nick);
	send_raw(buf);

	/* enable tags in PRIVMSGs */
	_sprintf(buf, MAX_MSG, "CAP REQ :twitch.tv/tags");
	send_raw(buf);

	/* _sprintf(buf, MAX_MSG, "CAP REQ :twitch.tv/membership"); */
	/* send_raw(buf); */

	if (strlen(m_channel) > 32) {
		fprintf(stderr, "error: channel name too long\n");
		disconnect();
		return false;
	}

	/* join channel */
	_sprintf(buf, MAX_MSG, "JOIN %s", m_channel);
	send_raw(buf);

	m_tick = std::thread(&TwitchBot::tick, this);
	return true;
}

/* disconnect: disconnect from Twitch server */
void TwitchBot::disconnect()
{
	cdisconnect(&m_client);
	m_connected = false;
	m_tick.join();
}

/* server_loop: continously receive and process data */
void TwitchBot::server_loop()
{
	char buf[MAX_MSG];

	/* continously receive data from server */
	while (1) {
		if (cread(&m_client, buf, MAX_MSG) <= 0) {
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

	if ((end = strlen(data)) > MAX_MSG - 3) {
		fprintf(stderr, "Message too long: %s\n", data);
		return false;
	}

	if (data[end - 1] != '\n' && data[end - 2] != '\r')
		strcpy(data + end, "\r\n");

	/* send formatted data */
	bytes = cwrite(&m_client, data);
	printf("%s %s\n", bytes > 0 ? "[SENT]" : "Failed to send:", data);

	/* return true iff data was sent succesfully */
	return bytes > 0;
}

/* send_msg: send a PRIVMSG to the connected channel */
bool TwitchBot::send_msg(const char *msg)
{
	char buf[MAX_MSG + 64];

	_sprintf(buf, MAX_MSG + 64, "PRIVMSG %s :%s", m_channel, msg);
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
	char out[MAX_MSG];

	if (strstr(privmsg, ":twitchnotify") == privmsg) {
		if (process_submsg(out, privmsg))
			send_msg(out);
		return true;
	}

	if (!parse_privmsg(privmsg, &nick, &msg, &p)) {
		fprintf(stderr, "error: failed to extract data from message\n");
		return false;
	}

	/* check if message contains a URL */
	m_parser.parse(msg);

	/* perform message moderation */
	if (P_ISREG(p) && m_mod.active() && moderate(nick, msg))
		return true;

	if ((msg[0] == '$' || (m_familiarity && msg[0] == '!')) && msg[1]) {
		m_cmdhnd.process_cmd(out, nick, msg + 1, p);
		send_msg(out);
		return true;
	}

	/* count */
	if (m_cmdhnd.counting() && msg[0] == '+' && msg[1]) {
		m_cmdhnd.count(nick, msg + 1);
		return true;
	}

	/* get URL information */
	if (m_parser.wasModified()) {
		if (!process_url(out))
			return false;
		if (*out) {
			send_msg(out);
			return true;
		}
	}

	/* check for responses */
	if (m_cmdhnd.process_resp(out, msg, nick))
		send_msg(out);

	return true;
}

/* process_url: extract url information and store in out */
bool TwitchBot::process_url(char *out)
{
	URLParser::URL *url;
	cpr::Response resp;
	std::string title;
	char buf[MAX_MSG];

	url = m_parser.getLast();
	if (url->twitter && !url->tweetID.empty()) {
		/* print info about twitter statuses */
		tw::Reader twr(&m_auth);
		if (twr.read_tweet(url->tweetID)) {
			_sprintf(out, MAX_MSG, "%s", twr.result().c_str());
			return true;
		}
		fprintf(stderr, "could not read tweet\n");
		return false;
	}
	/* get title of url if url_titles is active */
	if (m_urltitles) {
		resp = cpr::Get(cpr::Url(url->full),
				cpr::Header{{ "Connection", "close" }});
		strcat(buf, url->subdomain.c_str());
		strcat(buf, url->domain.c_str());
		if (!(title = urltitle(resp.text)).empty()) {
			_sprintf(out, MAX_MSG, "[URL] %s (at %s)",
					title.c_str(), buf);
			return true;
		}
		return false;
	}
	*out = '\0';
	return true;
}

/* parse_privmsg: parse twitch message to extract nick, msg and permissions */
bool TwitchBot::parse_privmsg(char *privmsg, char **nick, char **msg, perm_t *p)
{
	char *tok, *s;

	if (*privmsg != '@' || !(tok = strstr(privmsg, " :")))
		return false;
	*nick = tok + 2;
	if (!(s = strchr(*nick, '!')))
		return false;
	*s = '\0';

	/* set user privileges */
	P_RESET(*p);
	if (strcmp(*nick, m_channel + 1) == 0
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
	if (strcmp(tok, m_channel) != 0)
		return false;

	/* read actual message into msg */
	*msg = s + 2;
	if ((s = strchr(*msg, '\r')))
		*s = '\0';

	return true;
}

/* process_submsg: extract data from subscription message and write to out */
bool TwitchBot::process_submsg(char *out, char *submsg)
{
	char *nick, *s, *t;
	const char *msg;

	if (!strstr(submsg, "subscribed")) {
		*out = '\0';
		return false;
	}

	s = strchr(submsg, '#');
	t = strchr(s, ' ');
	*t = '\0';
	if (strcmp(s, m_channel) != 0)
		return false;

	nick = t + 2;
	t = strchr(nick, ' ');
	*t = '\0';

	if ((s = strstr(t + 1, "for"))) {
		s += 4;
		t = strchr(s, ' ');
		*t = '\0';
		msg = m_resubMsg.c_str();
	} else {
		s = submsg;
		*s = '1';
		s[1] = '\0';
		msg = m_subMsg.c_str();
	}
	_sprintf(out, MAX_MSG, "%s", formatSubMsg(msg, nick, s).c_str());
	return true;
}

/* moderate: check if message is valid; penalize nick if not */
bool TwitchBot::moderate(const std::string &nick, const std::string &msg)
{
	static const std::string warnings[3] = { "first", "second", "FINAL" };
	std::string reason, warning;
	char out[MAX_MSG];
	int offenses, t;

	if (!m_mod.validmsg(msg, nick.c_str(), reason)) {
		offenses = m_mod.offenses(nick);
		if (offenses < 4) {
			/* timeout for 2^(offenses - 1) minutes */
			t = 60 * (int)pow(2, offenses - 1);
			_sprintf(out, MAX_MSG, "/timeout %s %d", nick.c_str(), t);
			send_msg(out);
			warning = warnings[offenses - 1] + " warning";
		} else {
			_sprintf(out, MAX_MSG, "/ban %s", nick.c_str());
			send_msg(out);
			warning = "Permanently banned";
		}
		_sprintf(out, MAX_MSG, "%s - %s (%s)", nick.c_str(),
				reason.c_str(), warning.c_str());
		send_msg(out);
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
				if (m_event.active())
					send_msg(((*m_event.messages())[i])
							.first.c_str());
				m_event.setUsed("msg" + std::to_string(i));
				break;
			}
		}
		if (m_giveaway.active() && m_event.ready("checkgiveaway")) {
			if (m_giveaway.checkConditions(time(nullptr)))
				send_msg(m_giveaway.giveaway().c_str());
			m_event.setUsed("checkgiveaway");
		}
		if (m_event.ready("checkuptime")) {
			check_channel(m_channel + 1);
			m_event.setUsed("checkuptime");
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
		fprintf(stderr, "%s: %s: %s\n", m_cfgr->path().c_str(),
				which.c_str(), err.c_str());
		WAIT_INPUT();
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
			ins = m_channel + 1;
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
