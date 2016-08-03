#include <algorithm>
#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("count");
/* description of the command */
CMDDESCR("manage message counts");
/* command usage synopsis */
CMDUSAGE("$count start|stop|display");

typedef std::unordered_map<std::string, uint16_t> countmap;

static void getresults(char *out, countmap *count);

/* count: manage message counts */
std::string CommandHandler::count(char *out, struct command *c)
{
	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return "";
	}

	if (strcmp(c->argv[optind], "start") == 0) {
		/* begin a new count */
		if (m_counting) {
			_sprintf(out, MAX_MSG, "%s: count is already running",
					c->argv[0]);
			return "";
		}
		m_usersCounted.clear();
		m_messageCounts.clear();
		m_counting = true;
		_sprintf(out, MAX_MSG, "Message counting has begun. Prepend "
				"your message with a '+' to have it counted.");
	} else if (strcmp(c->argv[optind], "stop") == 0) {
		/* end the current count */
		if (!m_counting) {
			_sprintf(out, MAX_MSG, "%s: no active count",
					c->argv[0]);
			return "";
		}
		m_counting = false;
		_sprintf(out, MAX_MSG, "Count ended. Use \"$count display\" "
				"to view results.");
	} else if (strcmp(c->argv[optind], "display") == 0) {
		/* display results from last count */
		if (m_counting)
			_sprintf(out, MAX_MSG, "%s: end count before "
					"viewing results", c->argv[0]);
		else if (m_messageCounts.empty())
			_sprintf(out, MAX_MSG, "%s: nothing to display",
					c->argv[0]);
		else
			getresults(out, &m_messageCounts);
	} else {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
	}
	return "";
}

/* getresults: return the top 10 items in count */
static void getresults(char *out, countmap *count)
{
	typedef std::pair<std::string, uint16_t> mcount;
	std::vector<mcount> pairs;
	size_t i, max;

	/* add each result to vector to be sorted */
	for (auto itr = count->begin(); itr != count->end(); ++itr)
		pairs.push_back(*itr);
	std::sort(pairs.begin(), pairs.end(), [=](mcount &a, mcount &b) {
				return a.second > b.second;
			});

	_sprintf(out, MAX_MSG, "[RESULTS] ");
	out = strchr(out, '\0');
	/* get top 10 results */
	max = pairs.size() > 10 ? 10 : pairs.size();
	for (i = 0; i < max; ++i) {
		mcount &pair = pairs[i];
		/* print rank and number of votes */
		_sprintf(out, MAX_MSG - 40 * i, "%lu. %s (%d)%s", i + 1,
				pair.first.c_str(), pair.second,
				i == max - 1 ? "" : ", ");
		out = strchr(out, '\0');
	}
}
