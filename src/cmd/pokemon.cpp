#include <cpr/cpr.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

#define NONE	(-1)
#define POKEMON	0
#define ITEM	1
#define TYPE	2

/* full name of the command */
CMDNAME("pokemon");
/* description of the command */
CMDDESCR("look up pokemon information");
/* command usage synopsis */
CMDUSAGE("pokemon [-t] ARG");

static const std::string API = "http://pokeapi.co/api/v2";

static int gen;		/* the generation to look up */

static std::string typeinfo(const std::string &type);

/* pokemon: look up pokemon information */
std::string CommandHandler::pokemon(struct cmdinfo *c)
{
	int action;
	std::string arg;

	int opt;
	OptionParser op(c->fullCmd, "t");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "type", NO_ARG, 't' },
		{ 0, 0, 0 }
	};

	action = NONE;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 't':
			action = TYPE;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()
			|| (arg = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	switch (action) {
	case TYPE:
		return typeinfo(arg);
	case NONE:
		return CMDNAME + ": must provide an action";
	}
	return "";
}

/* typeinfo: get information about a type */
static std::string typeinfo(const std::string &type)
{
	/* offensive modifiers */
	static const std::string OSE = "double_damage_to";
	static const std::string ONE = "half_damage_to";
	static const std::string ODE = "no_damage_to";

	/* defensive modifiers */
	static const std::string DSE = "double_damage_from";
	static const std::string DNE = "half_damage_from";
	static const std::string DDE = "no_damage_from";

	cpr::Response resp;
	size_t i;
	std::string out;
	Json::Value val;
	Json::Reader reader;

	resp = cpr::Get(cpr::Url(API + "/type/" + type),
			cpr::Header {{ "Connection", "close" }});
	if (!reader.parse(resp.text, val))
		return CMDNAME + ": could not parse response";

	if (val.isMember("detail"))
		return CMDNAME + ": not a type: " + type;

	Json::Value &dmg = val["damage_relations"];

	out += "[TYPE] " + type + ": (OFFENSE)";
	if (!dmg[OSE].empty()) {
		out += " Super effective against";
		for (i = 0; i < dmg[OSE].size(); ++i) {
			out += " " + dmg[OSE][(int)i]["name"].asString();
			if (i != dmg[OSE].size() - 1)
				out += ",";
		}
		out += ".";
	}

	out += " Not very effective against";
	for (i = 0; i < dmg[ONE].size(); ++i) {
		out += " " + dmg[ONE][(int)i]["name"].asString();
		if (i != dmg[ONE].size() - 1)
			out += ",";
	}

	if (!dmg[ODE].empty()) {
		out += ". Doesn't affect";
		for (i = 0; i < dmg[ODE].size(); ++i) {
			out += " " + dmg[ODE][(int)i]["name"].asString();
			if (i != dmg[ODE].size() - 1)
				out += ",";
		}
	}

	out += ". | (DEFENSE)";
	out += " Weak against";
	for (i = 0; i < dmg[DSE].size(); ++i) {
		out += " " + dmg[DSE][(int)i]["name"].asString();
		if (i != dmg[DSE].size() - 1)
			out += ",";
	}

	out += ". Resistant to";
	for (i = 0; i < dmg[DNE].size(); ++i) {
		out += " " + dmg[DNE][(int)i]["name"].asString();
		if (i != dmg[DNE].size() - 1)
			out += ",";
	}

	if (!dmg[DDE].empty()) {
		out += ". Not affected by";
		for (i = 0; i < dmg[DDE].size(); ++i) {
			out += " " + dmg[DDE][(int)i]["name"].asString();
			if (i != dmg[DDE].size() - 1)
				out += ",";
		}
	}

	out += ".";
	return out;
}
