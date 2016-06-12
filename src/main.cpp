#include <cpr/cpr.h>
#include <cstdio>
#include <fstream>
#include <regex>
#include <string>
#include <utils.h>
#include "config.h"
#include "TwitchBot.h"
#include "version.h"
#ifdef __linux__
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/* LynxBot authorization URL */
static const char *AUTH_URL = "https://api.twitch.tv/kraken/oauth2/"
"authorize?response_type=token&client_id=kkjhmekkzbepq0pgn34g671y5nexap8&"
"redirect_uri=https://frolv.github.io/lynxbot/twitchauthconfirm.html&"
"scope=channel_editor+channel_subscriptions";

const char *BOT_NAME = "LynxBot";
const char *BOT_VERSION = "1.3.0";
const char *BOT_WEBSITE = "https://frolv.github.io/lynxbot";

struct botset {
	std::string name;
	std::string channel;
	std::string pass;
	std::string access_token;
};

void launchBot(struct botset *b, ConfigReader *cfgr);
void twitchAuth(struct botset *b);
bool authtest(const std::string &token, std::string &user);
#ifdef __linux__
static int open_linux();
#endif
#ifdef _WIN32
static int open_win();
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

	while ((c = getopt_long(argc, argv, "", long_opts, NULL)) != EOF) {
		switch (c) {
		case 'h':
			printf("usage: lynxbot [CHANNEL]\n%s: A Twitch.tv IRC "
					"bot for Old School Runescape\n"
					"Documentation can be found at %s\n",
					BOT_NAME, BOT_WEBSITE);
			return 0;
		case 'v':
			printf("%s v%s\nCopyright (C) 2016 Alexei Frolov\n"
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
	char c = '\0';
	std::cout << "In order for the $status command to work, LynxBot must be"
		" authorized to update your Twitch channel settings.\nWould you"
		" like to authorize LynxBot now? (y/n) ";
	while (c != 'y' && c != 'n')
		std::cin >> c;
	if (c == 'n') {
		b->access_token = "NULL";
		return;
	}

	int status;
#ifdef __linux__
	status = open_linux();
#endif
#ifdef _WIN32
	status = open_win();
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

	std::string token, user;
	std::cout << "After you have clicked \"Authorize\" you will be "
		"redirected to a webpage with an access token." << std::endl;
	std::cout << "Enter the access token here:" << std::endl;
	while (token.empty())
		std::getline(std::cin, token);

	if (authtest(token, user)) {
		b->access_token = token;
		std::cout << "Welcome, " << user << "!\nLynxBot has "
			"successfully been authorized with "
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

#ifdef __linux__
/* open_linux: launch browser on linux systems */
static int open_linux()
{
	switch (fork()) {
		case -1:
			perror("fork");
			exit(1);
		case 0:
			execl("/usr/bin/xdg-open", "xdg-open", AUTH_URL, (char *)NULL);
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
static int open_win()
{
	int i = (int)ShellExecute(NULL, "open", AUTH_URL, NULL, NULL, SW_SHOWNORMAL);
	return i <= 32;
}
#endif
