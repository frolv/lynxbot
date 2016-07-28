#include <cpr/cpr.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

#define NONE	(-1)
#define POKEMON	0
#define ITEM	1
#define TYPE	2
#define NATURE	3

/* full name of the command */
CMDNAME("pokemon");
/* description of the command */
CMDDESCR("look up pokemon information");
/* command usage synopsis */
CMDUSAGE("pokemon [-g GEN] <-N|-T> ARG");

static const std::string API = "http://pokeapi.co/api/v2";

/* the generation to look up */
static int gen;

static std::string natureinfo(const std::string &nature);
static std::string typeinfo(const std::string &type);

/* pokemon: look up pokemon information */
std::string CommandHandler::pokemon(char *out, struct command *c)
{
	int action;
	std::string arg;

	int opt;
	OptionParser op(c->fullCmd, "NTg:");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "gen", REQ_ARG, 'g' },
		{ "nature", NO_ARG, 'N' },
		{ "type", NO_ARG, 'T' },
		{ 0, 0, 0 }
	};

	action = NONE;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'g':
			try {
				if ((gen = std::stoi(op.optarg())) < 1
						|| gen > 6)
					return CMDNAME + ": generation must be "
						"between 1 and 6";
			} catch (std::invalid_argument) {
				return CMDNAME + ": invalid number -- '"
					+ std::string(op.optarg()) + "'";
			} catch (std::out_of_range) {
				return CMDNAME + ": number too large";
			}
			break;
		case 'N':
			if (action != NONE)
				return CMDNAME + ": only one operation can be "
					"used at a time";
			action = NATURE;
			break;
		case 'T':
			if (action != NONE)
				return CMDNAME + ": only one operation can be "
					"used at a time";
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
	case NATURE:
		return natureinfo(arg);
	case TYPE:
		return typeinfo(arg);
	case NONE:
		return CMDNAME + ": must provide an action";
	}
	return "";
}

/* natureinfo: get information about a nature */
static std::string natureinfo(const std::string &nature)
{
	cpr::Response resp;
	Json::Value val;
	Json::Reader reader;
	std::string out;

	resp = cpr::Get(cpr::Url(API + "/nature/" + nature),
			cpr::Header {{ "Connection", "close" }});
	if (!reader.parse(resp.text, val))
		return CMDNAME + ": could not parse response";

	if (val.isMember("detail"))
		return CMDNAME + ": not a nature: " + nature;

	out += "[NATURE] " + nature + ": ";
	out += "increased " + val["increased_stat"]["name"].asString() + ", ";
	out += "decreased " + val["decreased_stat"]["name"].asString() + ".";

	return out;
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
