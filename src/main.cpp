/*
 * LynxBot: a Twitch.tv IRC bot for Old School Runescape
 * Copyright (C) 2016 Alexei Frolov
 */

#include <cpr/cpr.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>
#include <utils.h>
#include "config.h"
#include "TwitchBot.h"
#include "version.h"

#ifdef __linux__
# include <getopt.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

/* LynxBot authorization URL */
static const char *AUTH_URL = "https://api.twitch.tv/kraken/oauth2/"
"authorize?response_type=token&client_id=kkjhmekkzbepq0pgn34g671y5nexap8&"
"redirect_uri=https://frolv.github.io/lynxbot/twitchauthconfirm.html&"
"scope=channel_editor+channel_subscriptions";

/* github api for latest release */
static const char *RELEASE_API =
"https://api.github.com/repos/frolv/lynxbot/releases/latest";

/* bot information */
const char *BOT_NAME = "LynxBot";
const char *BOT_VERSION = "v1.3.1";
const char *BOT_WEBSITE = "https://frolv.github.io/lynxbot";

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

#ifdef __linux__
	int c;
	static struct option long_opts[] = {
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'v' },
		{ 0, 0, 0, 0 }
	};

	while ((c = getopt_long(argc, argv, "hv", long_opts, NULL)) != EOF) {
		switch (c) {
		case 'h':
			printf("usage: lynxbot [CHANNEL]\n%s - A Twitch.tv IRC "
					"bot for Old School Runescape\n"
					"Documentation can be found at %s\n",
					BOT_NAME, BOT_WEBSITE);
			return 0;
		case 'v':
			printf("%s %s\nCopyright (C) 2016 Alexei Frolov\n"
					"This program is distributed as free "
					"software\nunder the terms of the MIT "
					"License.\n", BOT_NAME, BOT_VERSION);
			return 0;
		default:
			fprintf(stderr, "usage: %s [CHANNEL]\n", argv[0]);
			return 1;
		}
	}
#endif

	if (argc > 2) {
		fprintf(stderr, "usage: %s [CHANNEL]\n", argv[0]);
		return 1;
	}

	/* check if a new version is available */
	checkupdates();

	/* read the config file */
	path = utils::configdir() + utils::config("config");
	ConfigReader cfgr(path);
	if (!cfgr.read()) {
		std::cin.get();
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
	if (argc == 2)
		b.channel = argv[1];
	if (b.channel[0] != '#')
		b.channel = '#' + b.channel;

	launchBot(&b, &cfgr);
	return 0;
}

/* launchBot: start a TwitchBot instance */
void launchBot(struct botset *b, ConfigReader *cfgr)
{
	TwitchBot bot(b->name, b->channel, b->pass, b->access_token, cfgr);

	if (bot.isConnected())
		bot.serverLoop();
}

/* twitchAuth: interactively authorize LynxBot with a Twitch account */
void twitchAuth(struct botset *b)
{
	char c;
	int status;
	std::string token, user;

	printf("In order for the $status command to work, %s must be authorized"
			" to update your Twitch channel settings.\nWould you "
			"like to authorize %s now? (y/n) ", BOT_NAME, BOT_NAME);
	while ((c = getchar()) != 'n' && c != 'y')
		;
	if (c == 'n') {
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
		std::cerr << "Could not open web browser" << std::endl;
		std::cout << "Please navigate to the following URL manually:"
			<< std::endl << AUTH_URL << std::endl;
		std::cin.get();
	}

	std::cout << "The authorization URL has been opened in your "
		"browser. Sign in with your Twitch account and click "
		"\"Authorize\" to proceed." << std::endl;

	std::cout << "After you have clicked \"Authorize\" you will be "
		"redirected to a webpage with an access token." << std::endl;
	std::cout << "Enter the access token here:" << std::endl;
	while (token.empty())
		std::getline(std::cin, token);

	if (authtest(token, user)) {
		b->access_token = token;
		std::cout << "Welcome, " << user << "!\n" << BOT_NAME
			<< " has successfully been authorized with "
			"your Twitch account." << std::endl;
	} else {
		std::cout << "Invalid token. Authorization failed."
			<< std::endl;
	}
	std::cin.get();
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
	Json::Value js;
	Json::Reader reader;
	char c;

	cpr::Response resp = cpr::Get(cpr::Url(RELEASE_API),
			cpr::Header{{ "Accept", accept }});
	if (!reader.parse(resp.text, js))
		return;

	if (strcmp(js["tag_name"].asCString(), BOT_VERSION) != 0) {
		printf("A new version of %s (%s) is available.\nWould you like "
				"to open the download page? (y/n)\n", BOT_NAME,
				js["tag_name"].asCString());
		while ((c = getchar()) != 'y' && c != 'n')
			;
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
	int i = (int)ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
	return i <= 32;
}
#endif
