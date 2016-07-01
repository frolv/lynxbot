#include <algorithm>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("count");
/* description of the command */
CMDDESCR("manage message counts");
/* command usage synopsis */
CMDUSAGE("$count start|stop|display");

static std::string getresults(std::unordered_map<std::string, uint16_t> *count);

/* count: manage message counts */
std::string CommandHandler::count(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::vector<std::string> argv;

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

	utils::split(c->fullCmd, ' ', argv);
	if (argv.size() != 2 || !(argv[1] == "start" || argv[1] == "stop"
				|| argv[1] == "display"))
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (argv[1] == "start") {
		/* begin a new count */
		if (m_counting)
			return CMDNAME + ": count is already running";
		m_usersCounted.clear();
		m_messageCounts.clear();
		m_counting = true;
		return "Message counting has begun. Prepend your message with "
			"a '+' to have it counted.";
	} else if (argv[1] == "stop") {
		/* end the current count */
		if (!m_counting)
			return CMDNAME + ": no active count";
		m_counting = false;
		return "Count ended. Use \"$count display\" to view results.";
	} else {
		/* display results from last count */
		if (m_counting)
			return CMDNAME + ": end count before viewing results";
		if (m_messageCounts.empty())
			return CMDNAME + ": nothing to display";
		else
			return getresults(&m_messageCounts);
	}
}

/* getresults: return the top 10 items in count */
static std::string getresults(std::unordered_map<std::string, uint16_t> *count)
{
	typedef std::pair<std::string, uint16_t> mcount;
	std::vector<mcount> pairs;
	std::string results;

	/* add each result to vector to be sorted */
	for (auto itr = count->begin(); itr != count->end(); ++itr)
		pairs.push_back(*itr);
	std::sort(pairs.begin(), pairs.end(), [=](mcount &a, mcount &b) {
				return a.second > b.second;
			});

	results = "[RESULTS] ";
	/* get top 10 results */
	size_t max = pairs.size() > 10 ? 10 : pairs.size();
	for (size_t i = 0; i < max; ++i) {
		mcount &pair = pairs[i];
		/* print rank and number of votes */
		results += std::to_string(i + 1) + ". "
			+ pair.first + " (" + std::to_string(pair.second) + ")"
			+ (i == max - 1 ? "" : ", ");
	}
	return results;
}
