#ifndef CMDHANDLER_H
#define CMDHANDLER_H

#include <random>
#include <string>
#include <tw/authenticator.h>
#include <unordered_map>
#include "config.h"
#include "EventManager.h"
#include "GEReader.h"
#include "Giveaway.h"
#include "lynxbot.h"
#include "Moderator.h"
#include "permissions.h"
#include "RSNList.h"
#include "SelectionWheel.h"
#include "TimerManager.h"
#include "URLParser.h"

#define DEFAULT 1
#define CUSTOM  2

#define MAX_LEN 128

class Moderator;
class CustomHandler;
class GEReader;
class TimerManager;
class EventManager;
class SelectionWheel;
class URLParser;
class Giveaway;
class RSNList;
class ConfigReader;

class CmdHandler {
	public:
		typedef int(CmdHandler::*cmdfun)(char *, struct command *);
		typedef std::unordered_map<std::string, cmdfun> cmdmap;

		CmdHandler(const char *name, const char *channel,
				const char *token, Moderator *mod,
				URLParser *urlp, EventManager *evt,
				Giveaway *giv, ConfigReader *cfgr,
				tw::Authenticator *auth);
		~CmdHandler();

		void process_cmd(char *out, char *nick, char *cmdstr, perm_t p);
		int process_resp(char *out, char *msg, char *nick);
		bool counting() const;
		void count(const char *nick, const char *message);

	private:
		const char *bot_name;
		const char *bot_channel;
		const char *twitch_token;
		Moderator *moderator;
		URLParser *urlparser;
		cmdmap default_cmds;
		GEReader gereader;
		TimerManager cooldowns;
		SelectionWheel swheel;
		CustomHandler *custom_cmds;
		EventManager *evtman;
		Giveaway *giveaway;
		RSNList stored_rsns;
		ConfigReader *cfg;
		tw::Authenticator *tw_auth;
		Json::Value responses;
		Json::Value fashion;
		bool responding;
		bool count_active;
		std::vector<std::string> counted_users;
		std::unordered_map<std::string, uint16_t> message_counts;
		std::unordered_map<std::string, std::string> help;
		std::random_device rd;
		std::mt19937 gen;
		char poll[MAX_LEN];
		int return_status;
		std::vector<std::string> eightball_responses = {
			"It is certain",
			"It is decidedly so",
			"Without a doubt",
			"Yes, definitely",
			"You may rely on it",
			"As I see it, yes",
			"Most likely",
			"Outlook good",
			"Yes",
			"Signs point to yes",
			"Reply hazy try again",
			"Ask again later",
			"Better not tell you now",
			"Cannot predict now",
			"Concentrate and ask again",
			"Don't count on it",
			"My reply is no",
			"My sources say no",
			"Outlook not so good",
			"Very doubtful"
		};

		void process_default(char *out, struct command *c);
		void process_custom(char *out, struct command *c);

		/* default bot commands */
		int about(char *out, struct command *c);
		int active(char *out, struct command *c);
		int age(char *out, struct command *c);
		int calc(char *out, struct command *c);
		int cgrep(char *out, struct command *c);
		int cmdinfo(char *out, struct command *c);
		int cml(char *out, struct command *c);
		int duck(char *out, struct command *c);
		int ehp(char *out, struct command *c);
		int eightball(char *out, struct command *c);
		int fashiongen(char *out, struct command *c);
		int ge(char *out, struct command *c);
		int level(char *out, struct command *c);
		int man(char *out, struct command *c);
		int manual(char *out, struct command *c);
		int rsn(char *out, struct command *c);
		int submit(char *out, struct command *c);
		int twitter(char *out, struct command *c);
		int uptime(char *out, struct command *c);
		int wheel(char *out, struct command *c);
		int xp(char *out, struct command *c);

		/* moderator only commands */
		int addcom(char *out, struct command *c);
		int addrec(char *out, struct command *c);
		int count(char *out, struct command *c);
		int delcom(char *out, struct command *c);
		int delrec(char *out, struct command *c);
		int editcom(char *out, struct command *c);
		int editrec(char *out, struct command *c);
		int mod(char *out, struct command *c);
		int permit(char *out, struct command *c);
		int setgiv(char *out, struct command *c);
		int setrec(char *out, struct command *c);
		int showrec(char *out, struct command *c);
		int status(char *out, struct command *c);
		int strawpoll(char *out, struct command *c);
		int whitelist(char *out, struct command *c);

		/* helpers */
		uint8_t source(const char *cmd);
		int getrsn(char *out, size_t len, const char *text,
				const char *nick, int username = 0);
		void add_commands();
		void populate_help();
};

class CustomHandler {
	public:
		CustomHandler(CmdHandler::cmdmap *cmap, TimerManager *tm,
				const char *wheelcmd, const char *name,
				const char *channel);
		~CustomHandler();
		bool active();
		bool addcom(const char *cmd, char *response, const char *nick,
				time_t cooldown);
		bool delcom(const char *cmd);
		bool editcom(const char *cmd, const char *response,
				time_t newcd);
		bool activate(const char *cmd);
		bool deactivate(const char *cmd);
		bool rename(const char *cmd, const char *newcmd);
		Json::Value *getcom(const char *cmd);
		const Json::Value *commands();
		size_t size();
		bool valid_name(const char *cmd, bool loading = false);
		void write();
		char *error();
		char *format(const Json::Value *cmd, const char *nick);

	private:
		bool enabled;
		CmdHandler::cmdmap *default_cmds;
		TimerManager *cooldowns;
		const char *wheel_cmd;
		const char *bot_name;
		const char *bot_channel;
		Json::Value custom_cmds;
		char fmtresp[MAX_MSG];
		char err[MAX_LEN];
		bool cmdcheck();
};

#endif /* CMDHANDLER_H */
