#include <algorithm>
#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
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
int CmdHandler::count(char *out, struct command *c)
{
	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (strcmp(c->argv[l_optind], "start") == 0) {
		/* begin a new count */
		if (count_active) {
			snprintf(out, MAX_MSG, "%s: count is already running",
					c->argv[0]);
			return EXIT_FAILURE;
		}
		counted_users.clear();
		message_counts.clear();
		count_active = true;
		snprintf(out, MAX_MSG, "Message counting has begun. Prepend "
				"your message with a '+' to have it counted.");
	} else if (strcmp(c->argv[l_optind], "stop") == 0) {
		/* end the current count */
		if (!count_active) {
			snprintf(out, MAX_MSG, "%s: no active count",
					c->argv[0]);
			return EXIT_FAILURE;
		}
		count_active = false;
		snprintf(out, MAX_MSG, "Count ended. Use \"$count display\" "
				"to view results.");
	} else if (strcmp(c->argv[l_optind], "display") == 0) {
		/* display results from last count */
		if (count_active)
			snprintf(out, MAX_MSG, "%s: end count before "
					"viewing results", c->argv[0]);
		else if (message_counts.empty())
			snprintf(out, MAX_MSG, "%s: nothing to display",
					c->argv[0]);
		else
			getresults(out, &message_counts);
	} else {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
	}
	return EXIT_SUCCESS;
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

	snprintf(out, MAX_MSG, "[RESULTS] ");
	out = strchr(out, '\0');
	/* get top 10 results */
	max = pairs.size() > 10 ? 10 : pairs.size();
	for (i = 0; i < max; ++i) {
		mcount &pair = pairs[i];
		/* print rank and number of votes */
		snprintf(out, MAX_MSG - 40 * i, "%lu. %s (%d)%s", i + 1,
				pair.first.c_str(), pair.second,
				i == max - 1 ? "" : ", ");
		out = strchr(out, '\0');
	}
}
