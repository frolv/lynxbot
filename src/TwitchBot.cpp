#include <cpr/cpr.h>
#include <cstdio>
#include <fstream>
#include <regex>
#include <tw/reader.h>
#include <utils.h>
#include "lynxbot.h"
#include "timers.h"
#include "TwitchBot.h"

#define BUFFER_SIZE (MAX_MSG * 4)

static const char *TWITCH_SERV = "irc.twitch.tv";
static const char *TWITCH_PORT = "80";

static std::string urltitle(const std::string &resp);

TwitchBot::TwitchBot(const char *name, const char *channel,
		const char *password, const char *token,
		ConfigReader *cfgr)
	: bot_connected(false), bot_password(password), bot_name(name),
	bot_channel(channel), bot_token(token),
	cmdhnd(name, channel + 1, token, &mod, &parser, &evtman,
			&giveaway, cfgr, &auth),
	cfg(cfgr), evtman(cfgr),
	giveaway(channel + 1, time(nullptr), cfgr),
	mod(name, channel, &parser, cfgr, &client, &active_users)
{
	std::string err;
	bool error;

	/* create uptime checking event */
	if (strcmp(token, "NULL") != 0)
		evtman.add("checkuptime", 60, time(nullptr));

	/* create giveaway checking event */
	evtman.add("checkgiveaway", 10, time(nullptr));

	/* read the subscriber messages */
	parse_submsg(submsg, "submessage");
	parse_submsg(resubmsg, "resubmessage");

	error = false;
	if (!utils::parseBool(url_titles, cfg->get("url_titles"), err)) {
		fprintf(stderr, "%s: url_titles: %s (defaulting to true)\n",
				cfg->path(), err.c_str());
		url_titles = true;
		error = true;
	}
	if (!utils::parseBool(familiarity_mode, cfg->get("familiarity_mode"),
				err)) {
		fprintf(stderr, "%s: familiarity_mode: %s "
				"(defaulting to false)\n",
				cfg->path(), err.c_str());
		familiarity_mode = false;
		error = true;
	}
	if (!utils::parseBool(auto_disable, cfg->get("auto_disable"), err)) {
		fprintf(stderr, "%s: auto_disable: %s (defaulting to true)\n",
				cfg->path(), err.c_str());
		auto_disable = true;
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
	return bot_connected;
}

/* connect: connect to IRC */
bool TwitchBot::connect()
{
	char buf[MAX_MSG];

	if (!(bot_connected = !cconnect(&client, TWITCH_SERV, TWITCH_PORT)))
		return false;

	/* send required IRC data: PASS, NICK, USER */
	snprintf(buf, MAX_MSG, "PASS %s", bot_password);
	send_raw(&client, buf);
	snprintf(buf, MAX_MSG, "NICK %s", bot_name);
	send_raw(&client, buf);
	snprintf(buf, MAX_MSG, "USER %s", bot_name);
	send_raw(&client, buf);

	/* enable tags in PRIVMSGs */
	snprintf(buf, MAX_MSG, "CAP REQ :twitch.tv/tags");
	send_raw(&client, buf);

	/* receive join and part information */
	snprintf(buf, MAX_MSG, "CAP REQ :twitch.tv/membership");
	send_raw(&client, buf);

	/* allow access to additional twitch commands */
	snprintf(buf, MAX_MSG, "CAP REQ :twitch.tv/commands");
	send_raw(&client, buf);

	if (strlen(bot_channel) > 32) {
		fprintf(stderr, "error: channel name too long\n");
		disconnect();
		return false;
	}

	/* join channel */
	snprintf(buf, MAX_MSG, "JOIN %s", bot_channel);
	send_raw(&client, buf);

	tick_thread = std::thread(&TwitchBot::tick, this);
	init_timers(bot_channel + 1, bot_token);

	return true;
}

/* disconnect: disconnect from Twitch server */
void TwitchBot::disconnect()
{
	if (bot_connected) {
		cdisconnect(&client);
		bot_connected = false;
	}
	if (tick_thread.joinable())
		tick_thread.join();
}

/* server_loop: continously receive and process data */
void TwitchBot::server_loop()
{
	char buf[BUFFER_SIZE];
	char *pos, *s;
	int bytes, shift;
	size_t len;

	pos = buf;
	len = 0;
	/* continously receive data from server */
	while (1) {
		shift = 0;
		if ((bytes = cread(&client, pos, BUFFER_SIZE - len)) < 0) {
			perror("read");
			fprintf(stderr, "LynxBot exiting.\n");
			disconnect();
			break;
		} else if (bytes == 0) {
			continue;
		}
		pos += bytes;
		len += bytes;
		if (len < BUFFER_SIZE - 1) {
			/* keep reading until full message has been received */
			if (*(pos - 1) != '\n' && *(pos - 2) != '\r')
				continue;
		} else {
			/* end string at last newline, shift rest to front */
			s = pos;
			while (*--s != '\n' && len)
				--len;
			*s = '\0';
			shift = 1;
		}

		printf("[RECV] %s\n", buf);
		process_data(buf);
		if (shift) {
			strcpy(buf, s + 1);
			len = BUFFER_SIZE - len - 1;
			pos = buf + len;
		} else {
			pos = buf;
			len = 0;
		}

		if (!bot_connected)
			break;
	}
}

/* processData: send data to designated function */
void TwitchBot::process_data(char *data)
{
	if (strstr(data, "PRIVMSG")) {
		process_privmsg(data);
	} else if (strstr(data, "PING")) {
		pong(&client, data);
	} else if (strstr(data, "353")) {
		extract_names_list(data);
	} else if (strstr(data, "JOIN") || strstr(data, "PART")) {
		process_user(data);
	} else if (strstr(data, "USERNOTICE")) {
		process_resub(data);
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
			send_msg(&client, bot_channel, out);
		return true;
	}

	if (!parse_privmsg(privmsg, &nick, &msg, &p)) {
		fprintf(stderr, "error: failed to extract data from message\n");
		return false;
	}

	/* JOIN and PART messages on Twitch are not sent immediately */
	/* add everyone who sends a message to names to keep it up to date */
	active_users[nick] = 1;

	/* check if message contains a URL */
	parser.parse(msg);

	/* perform message moderation */
	if (P_ISREG(p) && mod.active() && moderate(nick, msg))
		return true;

	if ((msg[0] == '$' || (familiarity_mode && msg[0] == '!')) && msg[1]) {
		cmdhnd.process_cmd(out, nick, msg + 1, p);
		send_msg(&client, bot_channel, out);
		return true;
	}

	/* count */
	if (cmdhnd.counting() && msg[0] == '+' && msg[1]) {
		cmdhnd.count(nick, msg + 1);
		return true;
	}

	/* get URL information */
	if (parser.wasModified()) {
		if (!process_url(out))
			return false;
		if (*out) {
			send_msg(&client, bot_channel, out);
			return true;
		}
	}

	/* check for responses */
	if (cmdhnd.process_resp(out, msg, nick))
		send_msg(&client, bot_channel, out);

	return true;
}

/* process_url: extract url information and store in out */
bool TwitchBot::process_url(char *out)
{
	URLParser::URL *url;
	cpr::Response resp;
	std::string title;
	char buf[MAX_MSG];

	url = parser.getLast();
	if (url->twitter && !url->tweetID.empty()) {
		/* print info about twitter statuses */
		tw::Reader twr(&auth);
		if (twr.read_tweet(url->tweetID)) {
			snprintf(out, MAX_MSG, "%s", twr.result().c_str());
			return true;
		}
		fprintf(stderr, "could not read tweet\n");
		return false;
	}
	/* get title of url if url_titles is active */
	if (url_titles) {
		resp = cpr::Get(cpr::Url(url->full),
				cpr::Header{{ "Connection", "close" }});
		strcat(buf, url->subdomain.c_str());
		strcat(buf, url->domain.c_str());
		if (!(title = urltitle(resp.text)).empty()) {
			snprintf(out, MAX_MSG, "[URL] %s (at %s)",
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
	if (strcmp(*nick, bot_channel + 1) == 0
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
	if (strcmp(tok, bot_channel) != 0)
		return false;

	/* read actual message into msg */
	*msg = s + 2;
	if ((s = strchr(*msg, '\r')))
		*s = '\0';

	return true;
}

/* process_submsg: extract data from subscription message and write to out */
bool TwitchBot::process_submsg(char *out, char *msgbuf)
{
	char *nick, *s, *t;
	const char *msg;

	if (!strstr(msgbuf, "subscribed!")) {
		*out = '\0';
		return false;
	}

	s = strchr(msgbuf, '#');
	t = strchr(s, ' ');
	*t = '\0';
	if (strcmp(s, bot_channel) != 0)
		return false;

	nick = t + 2;
	t = strchr(nick, ' ');
	*t = '\0';
	msg = submsg.c_str();

	snprintf(out, MAX_MSG, "%s", format_submsg(msg, nick, "1").c_str());
	return true;
}

/* process_resub: extract data from resub message send response */
bool TwitchBot::process_resub(char *msgbuf)
{
	char *nick, *months, *s;
	const char *msg;
	char out[MAX_MSG];

	if (!strstr(msgbuf, "msg-id=resub"))
		return false;

	nick = strstr(msgbuf, "display-name") + 13;
	s = strchr(nick, ';');
	*s = '\0';

	months = strstr(s + 1, "msg-param-months") + 17;
	s = strchr(months, ';');
	*s = '\0';
	msg = resubmsg.c_str();

	snprintf(out, MAX_MSG, "%s", format_submsg(msg, nick, months).c_str());
	send_msg(&client, bot_channel, out);
	return true;
}

/* extract_names_list: extract names from data into active_users */
void TwitchBot::extract_names_list(char *data)
{
	char *s;

	for (s = data; s; data = s + 1) {
		if ((s = strchr(data, '\n')))
			*s = '\0';
		/* 353 signifies names list */
		if (strstr(data, "353")) {
			read_names(data);
			continue;
		}
		/* 366 signifies end of names list */
		if (strstr(data, "366"))
			break;
	}
}

/* read_names: read channel names list and add to active_users */
void TwitchBot::read_names(char *names)
{
	char *s;

	s = names + 1;
	names = strchr(s, ':') + 1;
	for (; s; names = s + 1) {
		if ((s = strchr(names, ' ')))
			*s = '\0';
		active_users[names] = 1;
	}
}

/* process_user: read joins and parts into active_users */
void TwitchBot::process_user(char *data)
{
	char *s, *t;
	int type;

	for (s = data; s; data = s + 1) {
		if ((s = strchr(data, '\n')))
			*s = '\0';

		if (strstr(data, "JOIN"))
			type = 1;
		else if (strstr(data, "PART"))
			type = 0;
		else
			continue;

		/* confirm join channel is bot's channel */
		if (!(t = strchr(data, '#')) ||
				strncmp(t, bot_channel, strlen(bot_channel)) != 0)
			continue;

		if (!(t = strchr(data, '!')))
			continue;
		*t = '\0';

		active_users[++data] = type;
	}
}

/* moderate: check if message is valid; penalize nick if not */
bool TwitchBot::moderate(const std::string &nick, const std::string &msg)
{
	static const std::string warnings[3] = { "first", "second", "FINAL" };
	std::string reason, warning;
	char out[MAX_MSG];
	int offenses, t;

	if (!mod.validmsg(msg, nick.c_str(), reason)) {
		offenses = mod.offenses(nick);
		if (offenses < 4) {
			/* timeout for 2^(offenses - 1) minutes */
			t = 60 * (int)pow(2, offenses - 1);
			snprintf(out, MAX_MSG, "/timeout %s %d", nick.c_str(), t);
			send_msg(&client, bot_channel, out);
			warning = warnings[offenses - 1] + " warning";
		} else {
			snprintf(out, MAX_MSG, "/ban %s", nick.c_str());
			send_msg(&client, bot_channel, out);
			warning = "Permanently banned";
		}
		snprintf(out, MAX_MSG, "%s - %s (%s)", nick.c_str(),
				reason.c_str(), warning.c_str());
		send_msg(&client, bot_channel, out);
		return true;
	}

	return false;
}

/* tick: repeatedly check variables and perform actions if conditions met */
void TwitchBot::tick()
{
	/* check every second */
	while (bot_connected) {
		for (std::vector<std::string>::size_type i = 0;
				i < evtman.messages()->size(); ++i) {
			if (evtman.ready("msg" + std::to_string(i))) {
				if (evtman.active())
					send_msg(&client, bot_channel,
							((*evtman.messages())[i])
							.first.c_str());
				evtman.setUsed("msg" + std::to_string(i));
				break;
			}
		}
		if (giveaway.active() && evtman.ready("checkgiveaway")) {
			if (giveaway.check(time(nullptr)))
				send_msg(&client, bot_channel,
						giveaway.giveaway().c_str());
			evtman.setUsed("checkgiveaway");
		}
		if (evtman.ready("checkuptime")) {
			check_channel(bot_channel + 1, bot_token);
			evtman.setUsed("checkuptime");
			if (auto_disable) {
				if (channel_uptime())
					evtman.activate();
				else
					evtman.deactivate();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

/* parse_submsg: read which from cfg into tgt and verify validity */
void TwitchBot::parse_submsg(std::string &tgt, const std::string &which)
{
	static const std::string fmt_c = "%Ncmn";
	size_t ind;
	std::string fmt, err;
	char c;

	ind = -1;
	fmt = cfg->get(which);
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
		fprintf(stderr, "%s: %s: %s\n", cfg->path(),
				which.c_str(), err.c_str());
		WAIT_INPUT();
		fmt = "";
	}
	tgt = fmt;
}

/* format_submsg: replace placeholders in format string with data */
std::string TwitchBot::format_submsg(const std::string &format,
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
			ins = bot_channel + 1;
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
