#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("delcom");
/* description of the command */
CMDDESCR("delete custom commands");
/* command usage synopsis */
CMDUSAGE("$delcom CMD...");

/* number of deleted and invalid commands */
static size_t nd, ni;

static void deletecom(CustomHandler *ccmd, const char *cmd,
		const char **del, const char **inv);
static void formatoutput(char *out, const char **del, const char **inv);

/* delcom: delete custom commands */
int CmdHandler::delcom(char *out, struct command *c)
{
	const char **del, **inv;

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
	nd = ni = 0;
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

	if (!custom_cmds->active()) {
		snprintf(out, MAX_MSG, "%s: custom commands are "
				"currently disabled", c->argv[0]);
		return EXIT_FAILURE;
	}

	if (l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	del = (const char **)malloc(custom_cmds->size() * sizeof(*del));
	inv = (const char **)malloc(custom_cmds->size() * sizeof(*inv));

	for (; l_optind < c->argc; ++l_optind)
		deletecom(custom_cmds, c->argv[l_optind], del, inv);
	formatoutput(out, del, inv);
	free(del);
	free(inv);
	return ni ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* deletecom: delete a single command */
static void deletecom(CustomHandler *ccmd, const char *cmd,
		const char **del, const char **inv)
{
	if (ccmd->delcom(cmd))
		del[nd++] = cmd;
	else
		inv[ni++] = cmd;
}

/* formatoutput: print a formatted output message into out */
static void formatoutput(char *out, const char **del, const char **inv)
{
	size_t i;

	if (nd) {
		snprintf(out, MAX_MSG, "deleted: ");
		out = strchr(out, '\0');
		for (i = 0; i < nd; ++i) {
			snprintf(out, MAX_MSG, "$%s%s", del[i],
					i == nd - 1 ? "" : " ");
			out = strchr(out, '\0');
		}
	}
	if (ni) {
		snprintf(out, MAX_MSG, "%snot found: ", nd ? " | " : "");
		out = strchr(out, '\0');
		for (i = 0; i < ni; ++i) {
			snprintf(out, MAX_MSG, "$%s%s", inv[i],
					i == ni - 1 ? "" : " ");
			out = strchr(out, '\0');
		}
	}
}
