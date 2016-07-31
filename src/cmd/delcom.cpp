#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("delcom");
/* description of the command */
_CMDDESCR("delete custom commands");
/* command usage synopsis */
_CMDUSAGE("$delcom CMD...");

/* number of deleted and invalid commands */
static size_t nd, ni;

static void deletecom(CustomCommandHandler *ccmd, const char *cmd,
		const char **del, const char **inv);
static void formatoutput(char *out, const char **del, const char **inv);

/* delcom: delete custom commands */
std::string CommandHandler::delcom(char *out, struct command *c)
{
	const char **del, **inv;

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
	nd = ni = 0;
	while ((opt = getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (!m_customCmds->isActive()) {
		_sprintf(out, MAX_MSG, "%s: custom commands are "
				"currently disabled", c->argv[0]);
		return "";
	}

	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	del = (const char **)malloc(m_customCmds->size() * sizeof(*del));
	inv = (const char **)malloc(m_customCmds->size() * sizeof(*inv));

	for (; optind < c->argc; ++optind)
		deletecom(m_customCmds, c->argv[optind], del, inv);
	formatoutput(out, del, inv);
	free(del);
	free(inv);
	return "";
}

/* deletecom: delete a single command */
static void deletecom(CustomCommandHandler *ccmd, const char *cmd,
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
		_sprintf(out, MAX_MSG, "deleted: ");
		out = strchr(out, '\0');
		for (i = 0; i < nd; ++i) {
			_sprintf(out, MAX_MSG, "$%s%s", del[i],
					i == nd - 1 ? "" : " ");
			out = strchr(out, '\0');
		}
	}
	if (ni) {
		_sprintf(out, MAX_MSG, "%snot found: ", nd ? " | " : "");
		out = strchr(out, '\0');
		for (i = 0; i < ni; ++i) {
			_sprintf(out, MAX_MSG, "$%s%s", inv[i],
					i == ni - 1 ? "" : " ");
			out = strchr(out, '\0');
		}
	}
}
