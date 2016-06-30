#include <cstdio>
#include <cstring>
#include "OptionParser.h"

#define INV_OPT (-1)

#define UNREC_TOK 127
#define INVAL_OPT 126
#define NOPRV_ARG 125
#define UNREC_LNG 124
#define INVAL_ARG 123
#define NOARG_LNG 122

OptionParser::OptionParser(const std::string &cmd, const char *optstr)
	: m_cmdstr(cmd.c_str()), m_optstr(optstr), m_cmdlen(cmd.length()),
	m_optind(0), m_state(0), m_optopt('\0')
{
	/* skip over and grab the command */
	while (m_cmdstr[m_optind]
			&& !isspace((m_cmd[m_optind] = m_cmdstr[m_optind])))
		++m_optind;
	m_cmd[m_optind] = '\0';
	while (isspace(m_cmdstr[m_optind]))
		++m_optind;
}

/* getopt: parse command options */
int OptionParser::getopt()
{
	int c;

	while (m_optind < m_cmdlen) {
		c = m_cmdstr[m_optind];
		switch (m_state) {
		case 0:
			while (isspace((c = m_cmdstr[m_optind])))
				++m_optind;
			if (c == '-') {
				m_state = 1;
				/* '-' is last char in string - treat as arg */
				if (++m_optind == m_cmdlen) {
					--m_optind;
					return EOF;
				}
			} else {
				return EOF;
			}
			break;
		case 1:
			/* isolated '-' - treat as arg */
			if (isspace(c)) {
				--m_optind;
				return EOF;
			}
			/* -- indicates end of options */
			if (c == '-') {
				if (!m_cmdstr[++m_optind]) {
					return EOF;
				} else if (!isspace(m_cmdstr[m_optind])) {
					puterr(UNREC_TOK);
					return '?';
				} else {
					while (isspace(m_cmdstr[++m_optind]))
						;
					return EOF;
				}
			}
			m_state = 2;
			return parseopt(c);
		case 2:
			/* end of current option cluster - find next */
			if (isspace(c)) {
				++m_optind;
				m_state = 0;
				continue;
			}
			return parseopt(c);
		default:
			/* unreachable */
			return EOF;
		}
	}
	return EOF;
}

/* getopt_long: parse command options, with support for long options
 * that start with -- */
int OptionParser::getopt_long(struct option *long_opts)
{
	int c;

	while (m_optind < m_cmdlen) {
		c = m_cmdstr[m_optind];
		switch (m_state) {
		case 0:
			while (isspace((c = m_cmdstr[m_optind])))
				++m_optind;
			if (c == '-') {
				m_state = 1;
				/* '-' is last char in string - treat as arg */
				if (++m_optind == m_cmdlen) {
					--m_optind;
					return EOF;
				}
			} else {
				return EOF;
			}
			break;
		case 1:
			/* isolated '-' - treat as arg */
			if (isspace(c)) {
				--m_optind;
				return EOF;
			}
			if (c == '-') {
				if (!m_cmdstr[m_optind + 1]) {
					++m_optind;
					return EOF;
				} else if (!isspace(m_cmdstr[m_optind + 1])) {
					/* long option */
					readlong();
					m_state = 0;
					return parselong(long_opts);
				} else {
					/* end of options */
					while (isspace(m_cmdstr[++m_optind]))
						;
					return EOF;
				}
			}
			m_state = 2;
			return parseopt(c);
		case 2:
			/* end of current option cluster - find next */
			if (isspace(c)) {
				++m_optind;
				m_state = 0;
				continue;
			}
			return parseopt(c);
		default:
			/* unreachable */
			return EOF;
		}
	}
	return EOF;
}

size_t OptionParser::optind() const
{
	return m_optind;
}

char *OptionParser::optarg()
{
	return m_optarg;
}

int OptionParser::optopt() const
{
	return m_optopt;
}

char *OptionParser::opterr()
{
	return m_opterr;
}

/* parseopt: process a single option */
int OptionParser::parseopt(int c)
{
	int i;

	switch (type((m_optopt = c))) {
	case NO_ARG:
		m_optarg[0] = '\0';
		++m_optind;
		return c;
	case REQ_ARG:
		/* skip any whitespace between option and arg */
		while (isspace(m_cmdstr[++m_optind]))
			;
		if (m_cmdstr[m_optind] == '-' || m_optind == m_cmdlen) {
			puterr(NOPRV_ARG);
			return '?';
		}
		/* arg is the sequence of chars until next space */
		for (i = m_optind; m_cmdstr[m_optind]
				&& !isspace(m_cmdstr[m_optind]); ++m_optind)
			m_optarg[m_optind - i] = m_cmdstr[m_optind];
		m_optarg[m_optind - i] = '\0';
		return c;
	default:
		puterr(INVAL_OPT);
		return '?';
	}
}

/* type: determine whether c is a valid option and if it requires an argument */
int OptionParser::type(int c) const
{
	const char *s;

	if (!(s = strchr(m_optstr, c)))
		return INV_OPT;
	return *++s == ':' ? REQ_ARG : NO_ARG;
}

/* puterr: write an error message to m_opterr */
void OptionParser::puterr(int type)
{
	switch (type) {
	case UNREC_TOK:
		sprintf(m_opterr, "%s: char %lu: unexpected token -- '%c'",
				m_cmd, m_optind + 1, m_cmdstr[m_optind]);
		break;
	case INVAL_OPT:
		sprintf(m_opterr, "%s: invalid option -- '%c'", m_cmd, m_optopt);
		break;
	case NOPRV_ARG:
		sprintf(m_opterr, "%s: option requires an argument -- '%c'",
				m_cmd, m_optopt);
		break;
	case UNREC_LNG:
		sprintf(m_opterr, "%s: unrecognized option '--%s'", m_cmd,
				m_longopt);
		break;
	case INVAL_ARG:
		sprintf(m_opterr, "%s: option '--%s' doesn't allow an argument",
				m_cmd, m_longopt);
		break;
	case NOARG_LNG:
		sprintf(m_opterr, "%s: option '--%s' requires an argument",
				m_cmd, m_longopt);
		break;
	default:
		sprintf(m_opterr, "%s: could not parse options", m_cmd);
		break;
	}
}

/* read_long: read a long option into m_longopt */
void OptionParser::readlong()
{
	int i;

	for (i = ++m_optind; m_cmdstr[m_optind]
			&& !isspace(m_cmdstr[m_optind])
			&& m_cmdstr[m_optind] != '='; ++m_optind)
		m_longopt[m_optind - i] = m_cmdstr[m_optind];
	m_longopt[m_optind - i] = '\0';
}

/* parselong: parse a long option */
int OptionParser::parselong(struct option* lo)
{
	int i;

	for (; lo->name && lo->type && lo->val; ++lo) {
		if (strcmp(lo->name, m_longopt) == 0) {
			if (lo->type == NO_ARG) {
				m_optarg[0] = '\0';
				if (m_cmdstr[m_optind] == '=') {
					puterr(INVAL_ARG);
					return m_optopt = '?';
				}
				return m_optopt = lo->val;
			}
			/* find the start of argument */
			if (m_cmdstr[m_optind] == '=') {
				++m_optind;
			} else {
				while (isspace(m_cmdstr[m_optind]))
					++m_optind;
				if (m_optind == m_cmdlen
						|| m_cmdstr[m_optind] == '-') {
					puterr(NOARG_LNG);
					return m_optopt = '?';
				}
			}
			/* read the argument */
			for (i = m_optind; m_cmdstr[m_optind] &&
					!isspace(m_cmdstr[m_optind]); ++m_optind)
				m_optarg[m_optind - i] = m_cmdstr[m_optind];
			m_optarg[m_optind - i] = '\0';
			return m_optopt = lo->val;
		}
	}
	puterr(UNREC_LNG);
	return m_optopt = '?';
}
