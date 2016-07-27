#include <ctime>
#include <iomanip>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("submit");
/* description of the command */
CMDDESCR("submit a message to the streamer");
/* command usage synopsis */
CMDUSAGE("$submit MSG");

/* submit: submit a message to the streamer */
std::string CommandHandler::submit(struct command *c)
{
	std::string output = "@" + c->nick + ", ";
	const std::string path = utils::configdir() + utils::config("submit");
	std::ofstream writer(path, std::ios::app);
	time_t t;
	std::tm tm;

	int opt;
	OptionParser op(c->fullCmd, "");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	t = time(nullptr);
#ifdef __linux__
	tm = *localtime(&t);
#endif
#ifdef _WIN32
	localtime_s(&tm, &t);
#endif
	writer << "[" << std::put_time(&tm, "%Y-%m-%d %R") << "] ";
	writer << c->nick << ": " << c->fullCmd.substr(op.optind()) << std::endl;

	return output + "your topic has been submitted. Thank you.";
}
