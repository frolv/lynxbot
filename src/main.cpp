#include <cpr/cpr.h>
#include <fstream>
#include <regex>
#include <string>
#include <utils.h>
#include "TwitchBot.h"
#ifdef __linux__
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/wait.h>
#endif

/* LynxBot authorization URL */
static const char *AUTH_URL = "https://api.twitch.tv/kraken/oauth2/"
"authorize?response_type=token&client_id=kkjhmekkzbepq0pgn34g671y5nexap8&"
"redirect_uri=https://frolv.github.io/lynxbot/twitchauthconfirm.html&"
"scope=channel_editor+channel_subscriptions";

/* botData stores settings for initializing a TwitchBot instance */
struct botData {
	std::string name;
	std::string channel;
	std::string pass;
	std::string access_token;
};

void launchBot(botData *bd);
bool readSettings(botData *bd, std::string &error);
void twitchAuth(botData *bd);
bool authtest(const std::string &token, std::string &user);
void writeAuth(const std::string &token);
#ifdef __linux__
static int open_linux();
#endif
#ifdef _WIN32
static int open_win();
#endif

/* LynxBot: a Twitch.tv IRC bot for Oldschool Runescape */
int main(int argc, char **argv)
{
	botData bd;
	std::string error;

	if (argc > 2) {
		std::cerr << "usage: " << argv[0] << " [CHANNEL]" << std::endl;
		return 1;
	}

	if (!readSettings(&bd, error)) {
		std::cerr << error << std::endl;
		std::cin.get();
		return 1;
	}

	/* authenticate with twitch */
	if (bd.access_token.empty()) {
		twitchAuth(&bd);
		writeAuth(bd.access_token);
	}

	if (argc == 2) {
		bd.channel = argv[1];
		if (bd.channel[0] != '#')
			bd.channel = '#' + bd.channel;
	}

	launchBot(&bd);
	return 0;
}

/* launchBot: start a TwitchBot instance */
void launchBot(botData *bd)
{
	TwitchBot bot(bd->name, bd->channel, bd->pass);

	if (bot.isConnected())
		bot.serverLoop();
}

/* readSettings: read LynxBot settings file */
bool readSettings(botData *bd, std::string &error)
{
	/* open settings */
	std::string path = utils::configdir() + utils::config("settings");
	std::ifstream reader(path);
	if (!reader.is_open()) {
		error = "could not locate " + path;
		return false;
	}

	std::string line;
	uint8_t lineNum = 0;

	while (std::getline(reader, line)) {
		++lineNum;
		/* remove whitespace */
		line.erase(std::remove_if(line.begin(), line.end(), isspace),
				line.end());

		/* lines starting with * are comments */
		if (utils::startsWith(line, "*"))
			continue;

		std::regex lineRegex("(name|channel|password|twitchtok):(.+?)");
		std::smatch match;
		const std::string s = line;
		if (std::regex_match(s.begin(), s.end(), match, lineRegex)) {
			if (match[1].str() == "name") {
				bd->name = match[2].str();
			} else if (match[1].str() == "channel") {
				if (!utils::startsWith(match[2].str(), "#"))
					error = "Channel must start with #";
				bd->channel = match[2].str();
			} else if (match[1].str() == "password") {
				if (!utils::startsWith(match[2].str(), "oauth:"))
					error = "Password must be a valid "
						 "oauth token, starting with "
						 "\"oauth:\".";
				bd->pass = match[2].str();
			} else {
				bd->access_token = match[2].str();
			}
		} else {
			error = "Syntax error on line "
				+ std::to_string(lineNum) + " of " + path;
		}
		if (!error.empty())
			return false;
	}

	reader.close();
	return true;
}

/* twitchAuth: interactively authorize LynxBot with a Twitch account */
void twitchAuth(botData *bd)
{
	char c = '\0';
	std::cout << "In order for the $status command to work, LynxBot must be"
		" authorized to update your Twitch channel settings.\nWould you"
		" like to authorize LynxBot now? (y/n) ";
	while (c != 'y' && c != 'n')
		std::cin >> c;
	if (c == 'n') {
		bd->access_token = "NULL";
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
		std::cin.get();
		exit(1);
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
		bd->access_token = token;
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

/* writeAuth: write access token to settings file */
void writeAuth(const std::string &token)
{
	if (token.empty())
		return;

	std::string path = utils::configdir() + utils::config("settings");
	std::ofstream writer(path, std::ios::app);
	writer << "twitchtok: " << token << std::endl;
	writer.close();
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
