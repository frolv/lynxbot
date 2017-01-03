/*
 * LynxBot: a Twitch.tv IRC bot for Old School Runescape
 * Copyright (C) 2016-2017 Alexei Frolov
 */

#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <string.h>
#include <utils.h>
#include "config.h"
#include "lynxbot.h"
#include "option.h"
#include "TwitchBot.h"

#ifdef __linux__
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

/* LynxBot authorization URL */
static const char *AUTH_URL = "https://api.twitch.tv/kraken/oauth2/"
"authorize?response_type=token&client_id=kkjhmekkzbepq0pgn34g671y5nexap8&"
"redirect_uri=https://frolv.github.io/lynxbot/twitchauthconfirm.html&"
"scope=channel_editor+channel_subscriptions+channel_check_subscription";

/* github api for latest release */
static const char *RELEASE_API =
"https://api.github.com/repos/frolv/lynxbot/releases/latest";

struct botset {
	std::string name;		/* twitch username of bot */
	std::string channel;		/* channel to join */
	std::string pass;		/* oauth token for account */
	std::string access_token;	/* access token for user's twitch */
};

void checkupdates();
void launchBot(struct botset *b, ConfigReader *cfgr);
void twitchAuth(struct botset *b);
bool authtest(const std::string &token, std::string &user);

#ifdef __linux__
static int open_linux(const char *url);
#endif

#ifdef _WIN32
static int open_win(const char *url);
#endif

/* LynxBot: a Twitch.tv IRC bot for Old School Runescape */
int main(int argc, char **argv)
{
	struct botset b;
	std::string path;
	int update;

	int c;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "no-check-update", NO_ARG, 'n' },
		{ "version", NO_ARG, 'v' },
		{ 0, 0, 0 }
	};

	opt_init();
	update = 1;
	while ((c = l_getopt_long(argc, argv, "hnv", long_opts)) != EOF) {
		switch (c) {
		case 'h':
			printf("usage: lynxbot [CHANNEL]\n%s - A Twitch.tv IRC "
					"bot for Old School Runescape\n"
					"Documentation can be found at %s\n",
					BOT_NAME, BOT_WEBSITE);
			return 0;
		case 'n':
			update = 0;
			break;
		case 'v':
			printf("%s %s\nCopyright (C) 2016-2017 Alexei Frolov\n"
					"This program is distributed as free "
					"software\nunder the terms of the MIT "
					"License.\n", BOT_NAME, BOT_VERSION);
			return 0;
		case '?':
			fprintf(stderr, "%s\n", l_opterr());
			fprintf(stderr, "usage: %s [CHANNEL]\n", argv[0]);
			return 1;
		default:
			fprintf(stderr, "usage: %s [CHANNEL]\n", argv[0]);
			return 1;
		}
	}

	if (argc - l_optind > 1) {
		fprintf(stderr, "usage: %s [CHANNEL]\n", argv[0]);
		return 1;
	}

	/* check if a new version is available */
	if (update)
		checkupdates();

	/* read the config file */
	path = utils::configdir() + utils::config("config");
	ConfigReader cfgr(path.c_str());
	if (!cfgr.read()) {
		WAIT_INPUT();
		return 1;
	}

	b.name = cfgr.get("name");
	b.channel = cfgr.get("channel");
	b.pass = cfgr.get("password");
	b.access_token = cfgr.get("twitchtok");

	/* authenticate with twitch */
	if (b.access_token == "UNSET") {
		twitchAuth(&b);
		cfgr.set("twitchtok", b.access_token);
		cfgr.write();
	}

	/* overwrite channel with arg */
	if (l_optind != argc)
		b.channel = argv[l_optind];
	if (b.channel[0] != '#')
		b.channel = '#' + b.channel;

	launchBot(&b, &cfgr);
	return 0;
}

/* launchBot: start a TwitchBot instance */
void launchBot(struct botset *b, ConfigReader *cfgr)
{
	TwitchBot bot(b->name.c_str(), b->channel.c_str(), b->pass.c_str(),
			b->access_token.c_str(), cfgr);

	if (bot.connect())
		bot.server_loop();
}

/* twitchAuth: interactively authorize LynxBot with a Twitch account */
void twitchAuth(struct botset *b)
{
	int c, status;
	std::string token, user;

	printf("In order for the certain bot commands to work, %s must be "
			"authorized to update your Twitch channel settings.\n"
			"Would you like to authorize %s now? (y/n) ",
			BOT_NAME, BOT_NAME);
	while ((c = getchar()) != EOF && c != 'n' && c != 'y')
		;
	if (c == EOF) {
		putchar('\n');
		exit(0);
	} else if (c == 'n') {
		b->access_token = "NULL";
		return;
	}

#ifdef __linux__
	status = open_linux(AUTH_URL);
#endif
#ifdef _WIN32
	status = open_win(AUTH_URL);
#endif
	if (status != 0) {
		fprintf(stderr, "Could not open web browser\nPlease navigate "
				"to the following URL manually:\n%s\n,",
				AUTH_URL);
		WAIT_INPUT();
	}

	printf("The authorization URL has been opened in your browser. Sign in "
			"with your Twitch account and click \"Authorize\" to "
			"proceed.\n");

	printf("After you have clicked \"Authorize\" you will be redirected to "
			"a webpage with an access token.\nEnter the access "
			"token here:\n");
	while (token.empty())
		std::getline(std::cin, token);

	if (authtest(token, user)) {
		b->access_token = token;
		printf("Welcome, %s!\n %s has successfully been authorized "
				"with your Twitch account.",
				user.c_str(), BOT_NAME);
	} else {
		printf("Invalid token. Authorization failed.\n");
	}
	WAIT_INPUT();
}

/* authtest: test access token validity */
bool authtest(const std::string &token, std::string &user)
{
	Json::Value json;
	cpr::Response resp = cpr::Get(cpr::Url("https://api.twitch.tv/kraken"),
			cpr::Header{{ "Authorization", "OAuth " + token }});

	Json::Reader reader;
	if (!reader.parse(resp.text, json))
		return false;

	if (json["token"]["valid"].asBool()) {
		user = json["token"]["user_name"].asString();
		return true;
	}
	return false;
}

/* check for new lynxbot version and prompt user to install */
void checkupdates()
{
	static const char *accept = "application/vnd.github.v3+json";
	cpr::Response resp;
	Json::Value js;
	Json::Reader reader;
	int c;

	if (strchr(BOT_VERSION, '-'))
		return;

	resp = cpr::Get(cpr::Url(RELEASE_API),
			cpr::Header{{ "Accept", accept }});
	if (!reader.parse(resp.text, js))
		return;

	if (strcmp(js["tag_name"].asCString(), BOT_VERSION) != 0) {
		printf("A new version of %s (%s) is available.\nWould you like "
				"to open the download page? (y/n)\n", BOT_NAME,
				js["tag_name"].asCString());
		while ((c = getchar()) != EOF && c != 'y' && c != 'n')
			;
		if (c == EOF)
			exit(0);
		if (c == 'n')
			return;
#ifdef __linux__
		open_linux(js["html_url"].asCString());
#endif

#ifdef _WIN32
		open_win(js["html_url"].asCString());
#endif
		exit(0);
	}
}

#ifdef __linux__
/* open_linux: launch browser on linux systems */
static int open_linux(const char *url)
{
	switch (fork()) {
		case -1:
			perror("fork");
			exit(1);
		case 0:
			execl("/usr/bin/xdg-open", "xdg-open",
					url, (char *)NULL);
			perror("/usr/bin/xdg-open");
			exit(1);
		default:
			int status;
			wait(&status);
			return status >> 8;
	}
}
#endif

#ifdef _WIN32
/* open_win: launch browser on windows systems */
static int open_win(const char *url)
{
	HINSTANCE ret;

	ret = ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
	return *(int *)&ret <= 32;
}
#endif
