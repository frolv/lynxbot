#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../lynxbot.h"
#include "../option.h"
#include "../stringparse.h"
#include "../strfmt.h"

#include <inttypes.h>

/* full name of the command */
CMDNAME("ge");
/* description of the command */
CMDDESCR("look up item prices");
/* command usage synopsis */
CMDUSAGE("$ge [-n AMT] ITEM");

static const char *EXCHANGE_API =
	"https://api.rsbuddy.com/grandExchange?a=guidePrice&i=";

static int64_t extract_price(const char *resp);

/* ge: look up item prices */
int CmdHandler::ge(char *out, struct command *c)
{
	int64_t amt, price;
	cpr::Response resp;
	const Json::Value *item;
	char buf[MAX_MSG];
	char num[RSN_BUF];
	char *s;

	int opt, hex;
	static struct l_option long_opts[] = {
		{ "amount", REQ_ARG, 'n' },
		{ "help", NO_ARG, 'h' },
		{ "hex", NO_ARG, 'z' },
		{ 0, 0, 0 }
	};

	if (!gereader.active()) {
		snprintf(out, MAX_MSG, "%s: GE reader is inactive", c->argv[0]);
		return EXIT_FAILURE;
	}

	opt_init();
	amt = 1;
	hex = 0;
	while ((opt = l_getopt_long(c->argc, c->argv, "n:", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'n':
			if (!parsenum_mult(l_optarg, &amt)) {
				snprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (amt < 0) {
				snprintf(out, MAX_MSG, "%s: amount cannot be "
						"negative", c->argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'z':
			hex = 1;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	argvcat(buf, c->argc, c->argv, l_optind, 1);
	while ((s = strchr(buf, '_')))
		*s = ' ';

	/* find item in database */
	if (!(item = gereader.find_item(buf))) {
		snprintf(out, MAX_MSG, "%s: item not found: %s",
				c->argv[0], buf);
		return EXIT_FAILURE;
	}

	/* read ge api and extract data */
	snprintf(buf, MAX_MSG, "%s%d", EXCHANGE_API, (*item)["id"].asInt());
	resp = cpr::Get(cpr::Url(buf), cpr::Header{{ "Connection", "close" }});
	strcpy(buf, resp.text.c_str());

	if ((price = extract_price(buf)) == -1) {
		snprintf(out, MAX_MSG, "%s: could not extract price",
				c->argv[0]);
		return EXIT_FAILURE;
	}

	strcpy(out, "[GE] ");
	out = strchr(out, '\0');
	if (amt != 1) {
		if (hex) {
			snprintf(out, RSN_BUF, "0x%" PRIX64, amt);
		} else {
			snprintf(buf, RSN_BUF, "%" PRIu64, amt);
			fmtnum(out, RSN_BUF, buf);
		}
		strcat(out, "x ");
		out = strchr(out, '\0');
	}
	if (hex) {
		snprintf(num, RSN_BUF, "0x%" PRIX64, (uint64_t)(amt * price));
	} else {
		snprintf(buf, RSN_BUF, "%" PRIu64, amt * price);
		fmtnum(num, RSN_BUF, buf);
	}
	snprintf(out, MAX_MSG, "%s: %s gp", (*item)["name"].asCString(), num);

	return EXIT_SUCCESS;
}

/* extract_price: return the price of the json data in resp */
static int64_t extract_price(const char *resp)
{
	Json::Reader reader;
	Json::Value item;

	if (reader.parse(resp, item))
		return item["overall"].asInt64();
	else
		return -1;
}
