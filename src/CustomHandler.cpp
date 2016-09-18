#include <ctime>
#include <json/json.h>
#include <string.h>
#include <utils.h>
#include "CmdHandler.h"
#include "strfmt.h"

static bool valid_resp(const char *resp, char *err);

CustomHandler::CustomHandler(CmdHandler::cmdmap *cmap, TimerManager *tm,
		const char *wheelcmd, const char *name, const char *channel)
	: default_cmds(cmap), cooldowns(tm), wheel_cmd(wheelcmd),
	bot_name(name), bot_channel(channel)
{
	if (!(enabled = utils::read_json("customcmds.json", custom_cmds))) {
		fprintf(stderr, "Could not read customcmds.json.\n");
		return;
	}

	if (!custom_cmds.isMember("commands")
			|| !custom_cmds["commands"].isArray()) {
		enabled = false;
		fprintf(stderr, "customcmds.json is improperly configured\n");
		return;
	}

	if (!cmdcheck())
		fprintf(stderr, "Custom commands disabled.\n");
}

CustomHandler::~CustomHandler() {}

bool CustomHandler::active()
{
	return enabled;
}

/* addcom: add a new custom command cmd with response and cooldown */
bool CustomHandler::addcom(const char *cmd, char *response,
		const char *nick, time_t cooldown)
{
	time_t t;
	char resp[MAX_MSG];
	Json::Value command;

	if (!valid_name(cmd)) {
		snprintf(err, MAX_LEN, "invalid command name: $%s", cmd);
		return false;
	}
	if (!valid_resp(response, err))
		return false;

	resp[0] = '\0';
	if (*response == '/')
		strcat(resp, " ");
	strcat(resp, response);

	command["active"] = true;
	command["cmd"] = cmd;
	command["response"] = resp;
	command["cooldown"] = (Json::Int64)cooldown;
	command["ctime"] = (Json::Int64)(t = time(nullptr));
	command["mtime"] = (Json::Int64)t;
	command["creator"] = nick;
	command["uses"] = 0;

	custom_cmds["commands"].append(command);
	cooldowns->add(cmd, cooldown);
	write();
	return true;
}

/* delCom: delete command cmd if it exists */
bool CustomHandler::delcom(const char *cmd)
{
	Json::ArrayIndex ind;
	Json::Value val, def, rem;

	for (ind = 0; ind < custom_cmds["commands"].size(); ++ind) {
		val = custom_cmds["commands"].get(ind, def);
		if (strcmp(val["cmd"].asCString(), cmd) == 0)
			break;
	}

	if (ind == custom_cmds["commands"].size())
		return false;

	custom_cmds["commands"].removeIndex(ind, &rem);
	cooldowns->remove(cmd);
	write();
	return true;
}

/* editCom: modify the command cmd with newResp and newcd */
bool CustomHandler::editcom(const char *cmd, const char *response, time_t newcd)
{
	Json::Value *com;
	char resp[MAX_MSG];

	if (!(com = getcom(cmd))) {
		snprintf(err, MAX_LEN, "not a command: $%s", cmd);
		return false;
	}
	if (response) {
		resp[0] = '\0';
		if (*response == '/')
			strcat(resp, " ");
		strcat(resp, response);

		if (!valid_resp(resp, err))
			return false;

		(*com)["response"] = resp;
	}
	if (newcd != -1) {
		(*com)["cooldown"] = (Json::Int64)newcd;
		cooldowns->remove(cmd);
		cooldowns->add(cmd, newcd);
	}
	(*com)["mtime"] = (Json::Int64)time(nullptr);
	write();
	return true;
}

/* activate: activate the command cmd */
bool CustomHandler::activate(const char *cmd)
{
	Json::Value *com;

	if (!(com = getcom(cmd))) {
		snprintf(err, MAX_LEN, "not a command: $%s", cmd);
		return false;
	}
	if (!valid_resp((*com)["response"].asCString(), err))
		return false;

	(*com)["active"] = true;
	write();
	return true;
}

/* deactivate: deactivate the command cmd */
bool CustomHandler::deactivate(const char *cmd)
{
	Json::Value *com;

	if (!(com = getcom(cmd))) {
		snprintf(err, MAX_LEN, "not a command: $%s", cmd);
		return false;
	}

	(*com)["active"] = false;
	write();
	return true;
}

/* rename: rename custom command cmd to newcmd */
bool CustomHandler::rename(const char *cmd, const char *newcmd)
{
	Json::Value *com;

	if (!(com = getcom(cmd))) {
		snprintf(err, MAX_LEN, "not a command: $%s", cmd);
		return false;
	}
	if (!valid_name(newcmd)) {
		snprintf(err, MAX_LEN, "invalid command name: $%s", newcmd);
		return false;
	}

	(*com)["cmd"] = newcmd;
	(*com)["mtime"] = (Json::Int64)time(nullptr);
	cooldowns->remove(cmd);
	cooldowns->add(newcmd, (*com)["cooldown"].asInt64());
	write();
	return true;
}

/* getcom: return command value if it exists, empty value otherwise */
Json::Value *CustomHandler::getcom(const char *cmd)
{
	for (auto &val : custom_cmds["commands"]) {
		if (strcmp(val["cmd"].asCString(), cmd) == 0)
			return &val;
	}

	return NULL;
}

const Json::Value *CustomHandler::commands()
{
	return &custom_cmds;
}

/* size: return number of custom commands */
size_t CustomHandler::size()
{
	return custom_cmds["commands"].size();
}

/* write: write all commands to file */
void CustomHandler::write()
{
	utils::write_json("customcmds.json", custom_cmds);
}

/* valid_name: check if cmd is a valid command name */
bool CustomHandler::valid_name(const char *cmd, bool loading)
{
	/* when loading commands from file, don't check against stored cmds */
	return default_cmds->find(cmd) == default_cmds->end()
		&& strcmp(cmd, wheel_cmd) != 0
		&& strlen(cmd) < 32 && (loading || !getcom(cmd));
}

char *CustomHandler::error()
{
	return err;
}

/* format: format a response for cmd */
char *CustomHandler::format(const Json::Value *cmd, const char *nick)
{
	const char *s;
	char *t;
	char num[MAX_LEN];

	t = fmtresp;
	for (s = (*cmd)["response"].asCString(); *s; ++s) {
		if (*s == '%') {
			switch (s[1]) {
			case '%':
				strcpy(t, "%");
				break;
			case 'N':
				snprintf(t, MAX_MSG - (t - fmtresp),
						"@%s,", nick);
				break;
			case 'b':
				strcpy(t, bot_name);
				break;
			case 'c':
				strcpy(t, bot_channel);
				break;
			case 'n':
				strcpy(t, nick);
				break;
			case 'u':
				snprintf(num, MAX_LEN, "%d",
						(*cmd)["uses"].asInt());
				fmtnum(t, MAX_LEN, num);
				break;
			default:
				/* should never happen */
				break;
			}
			t = strchr(t, '\0');
			++s;
			continue;
		}
		*t++ = *s;
	}
	*t = '\0';

	return fmtresp;
}

/* cmdcheck: check the validity of a command and add missing fields */
bool CustomHandler::cmdcheck()
{
	time_t t;
	bool added;

	t = time(nullptr);
	added = false;
	for (Json::Value &val : custom_cmds["commands"]) {
		/* add new values to old commands */
		if (!val.isMember("ctime")) {
			val["ctime"] = (Json::Int64)t;
			added = true;
		}
		if (!val.isMember("mtime")) {
			val["mtime"] = (Json::Int64)t;
			added = true;
		}
		if (!val.isMember("creator")) {
			val["creator"] = "unknown";
			added = true;
		}
		if (!val.isMember("uses")) {
			val["uses"] = 0;
			added = true;
		}
		if (!val.isMember("active")) {
			val["active"] = true;
			added = true;
		}
		if (!(val.isMember("cmd") && val.isMember("response")
					&& val.isMember("cooldown"))) {
			enabled = false;
			fprintf(stderr, "command '%s' is missing required fields\n",
					val["cmd"].asCString());
			return false;
		}
		if (!valid_name(val["cmd"].asCString(), true)) {
			enabled = false;
			fprintf(stderr, "'%s' is an invalid command name -"
					" change or remove it\n",
					val["cmd"].asCString());
			return false;
		}
		if (val["cooldown"].asInt() < 0) {
			enabled = false;
			fprintf(stderr, "command '%s' has a negative cooldown -"
					" change or remove it\n",
					val["cmd"].asCString());
			return false;
		}
		/* check validity of response */
		if (!valid_resp(val["response"].asCString(), err)) {
			fprintf(stderr, "command '%s': %s\n",
					val["cmd"].asCString(), err);
			val["active"] = false;
			added = true;
		}
		if (added)
			write();
		cooldowns->add(val["cmd"].asString(), val["cooldown"].asInt64());
	}
	return true;
}

/* valid_resp: check if a response has valid format characters */
static bool valid_resp(const char *resp, char *err)
{
	static const char *fmt_c = "%Nbcnu";
	const char *s;
	int c;

	s = resp;
	while ((s = strchr(s, '%'))) {
		if (!(c = s[1])) {
			snprintf(err, MAX_LEN, "unexpected end of line after "
					"'%%' in response '%s'\n", resp);
			return false;
		}
		if (!strchr(fmt_c, c)) {
			snprintf(err, MAX_LEN, "invalid format sequence '%%%c'"
					" in reponse '%s'\n", c, resp);
			return false;
		}
		if (c == '%')
			++s;
		++s;
	}
	return true;
}
