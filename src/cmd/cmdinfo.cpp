#include <ctime>
#include <json/json.h>
#include <iomanip>
#include <sstream>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("cmdinfo");
/* description of the command */
CMDDESCR("show information about a custom command");
/* command usage synopsis */
CMDUSAGE("$cmdinfo [-acCmu] CMD");

static bool mod;
static bool atime, crtime, creator, mtime, uses;

static std::string info(Json::Value *cmd);

/* cmdinfo: show information about a custom command */
std::string CommandHandler::cmdinf(struct cmdinfo *c)
{
	std::string cmd;

	int opt;
	OptionParser op(c->fullCmd, "acCmu");
	static struct OptionParser::option long_opts[] = {
		{ "atime", NO_ARG, 'a' },
		{ "ctime", NO_ARG, 'c' },
		{ "creator", NO_ARG, 'C' },
		{ "help", NO_ARG, 'h' },
		{ "mtime", NO_ARG, 'm' },
		{ "uses", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	atime = crtime = creator = mtime = uses = mod = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			mod = atime = true;
			break;
		case 'c':
			mod = crtime = true;
			break;
		case 'C':
			mod = creator = true;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'm':
			mod = mtime = true;
			break;
		case 'u':
			mod = uses = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}
	if (!mod)
		atime = crtime = creator = mtime = uses = true;

	if (op.optind() == c->fullCmd.length()
			|| (cmd = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	switch (source(cmd)) {
	case DEFAULT:
		return "[CMDINFO] " + cmd + " is a default command";
	case CUSTOM:
		return "[CMDINFO] " + info(m_customCmds->getCom(cmd));
	default:
		return CMDNAME + ": command not found: " + cmd;
	}

}

/* info: return requested information about cmd */
static std::string info(Json::Value *cmd)
{
	std::ostringstream out;
	time_t at, ct, mt;
	std::tm atm, ctm, mtm;

	if (cmd->isMember("atime"))
		at = (time_t)(*cmd)["atime"].asInt64();
	else
		at = 0;
	ct = (time_t)(*cmd)["ctime"].asInt64();
	mt = (time_t)(*cmd)["mtime"].asInt64();

#ifdef __linux__
	atm = *localtime(&at);
	ctm = *localtime(&ct);
	mtm = *localtime(&mt);
#endif
#ifdef _WIN32
	localtime_s(&atm, &at);
	localtime_s(&ctm, &ct);
	localtime_s(&mtm, &mt);
#endif

	out << (*cmd)["cmd"].asString() + ": ";
	if (crtime || creator) {
		out << " Created";
		if (creator)
			out << " by " + (*cmd)["creator"].asString();
		if (crtime) {
			out << " at " << std::put_time(&ctm, "%R %Z %d/%m/%Y");
			out << " (" << utils::conv_time(time(nullptr) - ct)
				<< " ago)";
		}
		out << ".";
	}
	if (mtime) {
		if ((!mod && ct != mt) || mod) {
			out << " Last modified at "
				<< std::put_time(&mtm, "%R %Z %d/%m/%Y");
			out << " (" << utils::conv_time(time(nullptr) - mt)
				<< " ago).";
		}
	}
	if (atime) {
		if (at) {
			out << " Last used at "
				<< std::put_time(&atm, "%R %Z %d/%m/%Y");
			out << " (" << utils::conv_time(time(nullptr) - at)
				<< " ago).";
		} else if (mod) {
			out << " Never used.";
		}
	}
	if (uses)
		out << " Used " << (*cmd)["uses"].asInt() << " time"
			<< ((*cmd)["uses"].asInt() == 1 ? "" : "s") << ".";

	return out.str();
}
