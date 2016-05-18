#include "OptionParser.h"

OptionParser::OptionParser(const std::string &optstr, const std::string &options)
	: m_optstr(optstr), m_options(options), m_optind(optstr.find(' ')),
	m_state(0), m_optopt('\0')
{}

OptionParser::~OptionParser() {}

int16_t OptionParser::getopt()
{
	if (m_optind == std::string::npos) {
		m_optind = m_optstr.length();
		return EOF;
	}
	while (m_optind < m_optstr.length()) {
		int16_t c = m_optstr[m_optind];
		std::string optarg;
		switch (m_state) {
		case 0:
			/* looking for next '-' */
			if (c == '-')
				m_state = 1;
			else if (c != ' ') {
				/* no '-' found - end of options reached */
				return EOF;
			}
			/* c is another space - keep looking */
			m_optind++;
			break;
		case 1:
			/* first char after a '-' */
			if (c == ' ') {
				/* isolated '-' in string - treat as arg */
				m_optind--;
				return EOF;
			}
			m_optind++;
			if (valid(c)) {
				m_optopt = c;
				if (arg(c)) {
					/* c requires an argument */
					if (m_optind == m_optstr.length())
						return '?';
					m_state = 3;
				} else {
					/* c doesn't require an argument */
					m_state = 2;
					return c;
				}
			} else {
				/* invalid option */
				m_optopt = c;
				m_state = 2;
				return '?';
			}
			break;
		case 2:
			/* multiple characters after a '-' */
			m_optind++;
			if (c == ' ') {
				/* end of current option - look for next '-' */
				m_state = 0;
			} else if (valid(c)) {
				m_optopt = c;
				if (arg(c)) {
					/* c requires an argument */
					m_state = 3;
				} else {
					/* c doesn't require an argument */
					return c;
				}
			} else {
				/* invalid option */
				m_optopt = c;
				return '?';
			}
			break;
		case 3:
			/* option requires an argument */
			while (m_optstr[m_optind] == ' ')
				m_optind++;
			if (m_optind == m_optstr.length())
				return '?';
			if (m_optstr[m_optind] == '-') {
				/* start of next option - no arg provided */
				m_optind++;
				m_state = 1;
				return '?';
			}
			/* grab the argument up to the next space */
			while (m_optind < m_optstr.length()
					&& m_optstr[m_optind] != ' ')
				optarg += m_optstr[m_optind++];
			m_optarg = optarg;
			m_state = 0;
			return m_optopt;
		default:
			/* uncreachable (hopefully) */
			break;
		}
	}
	return EOF;
}

std::string::size_type OptionParser::optind()
{
	return m_optind;
}

std::string OptionParser::optarg()
{
	return m_optarg;
}

int16_t OptionParser::optopt()
{
	return m_optopt;
}

bool OptionParser::valid(int16_t c)
{
	return m_options.find(static_cast<char>(c)) != std::string::npos;
}

bool OptionParser::arg(int16_t c)
{
	std::string::size_type pos = m_options.find(static_cast<char>(c));
	return pos != std::string::npos && pos < m_options.length() - 1
		&& m_options[pos + 1] == ':';
}
