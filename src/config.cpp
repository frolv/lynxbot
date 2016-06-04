#include <cstdio>
#include <ctype.h>
#include <cstring>
#include "config.h"

#define MAX_SIZE 2048

static void skipline(FILE *f, int *c);

static struct setting settings[] = {
	{ "name", STRING },
	{ "channel", STRING },
	{ "password", STRING },
	{ "twitchtok", STRING },
	{ "enable_moderation", STRING },
	{ "ban_urls", STRING },
	{ "max_message_len", STRING },
	{ "max_pattern", STRING },
	{ "max_char", STRING },
	{ "cap_len", STRING },
	{ "cap_ratio", STRING },
	{ "whitelist", LIST },
	{ "giveaway_active", STRING },
	{ "follower_giveaway", STRING },
	{ "follower_limit", STRING },
	{ "timed_giveaway", STRING },
	{ "time_interval", STRING },
	{ "recurring", OLIST },
	{ "submessage", STRING },
	{ "extra8ballresponses", LIST }
};

ConfigReader::ConfigReader(const char *path)
	: m_path(path)
{
}

bool ConfigReader::read()
{
	FILE *f;
	int c, state, i, line;
	size_t nsettings;
	char setting[MAX_SIZE], *s;

	if (!(f = fopen(m_path, "r"))) {
		perror(m_path);
		return false;
	}

	nsettings = sizeof(settings) / sizeof(settings[0]);
	state = 0;
	line = 1;
	s = setting;
	while ((c = getc(f)) != EOF) {
		switch (state) {
		case 0:
			if (c == '\n') {
				++line;
				continue;
			}
			if (!isblank(c)) {
				if (c == '#') {
					skipline(f, &c);
					++line;
				} else {
					*s++ = c;
					state = 1;
				}
			}
			break;
		case 1:
			if (c != '\n' && c != '=' && c != ' ' && c != '\t')
				*s++ = c;
			if (c == '=') {
				*s = '\0';
				for (i = 0; i < nsettings; ++i) {
					if (strcmp(settings[i].key.c_str(), setting) == 0) {
						if (!readSetting(f, i, &line))
							return false;
						s = setting;
						state = 0;
						break;
					}
				}
				if (state == 0)
					continue;
				fprintf(stderr, "%s: line %d: not a valid "
						"setting -- %s\n", m_path,
						line, setting);
				return false;
			}
			if (c == '\n') {
				fprintf(stderr, "%s: line %d: unexpected "
						"end of line\n", m_path, line);
				return false;
			}
			break;
		default:
			break;
		}
	}
	return true;
}

bool ConfigReader::write()
{
}

std::string ConfigReader::getSetting(const std::string &setting)
{
}

bool ConfigReader::readSetting(FILE *f, int i, int *line)
{
	switch (settings[i].val_type) {
	case STRING:
		return readString(f, i, line);
	case LIST:
		return readList(f, i, line);
	default:
		return readOList(f, i, line);
	}
}

bool ConfigReader::readString(FILE *f, int i, int *line)
{
	int c;
	char value[MAX_SIZE], *s;

	s = value;
	while (isblank((c = getc(f))))
		;
	if (c == '\n' || c == EOF) {
		fprintf(stderr, "%s: line %d: no setting provided\n",
				m_path, *line);
		return false;
	}
	*s++ = c;
	while ((c = getc(f)) != '\n' && c != EOF)
		*s++ = c;
	*s = '\0';
	if (c == '\n')
		++*line;
	printf("%s\n%s\n", settings[i].key.c_str(), value);
	return true;
}

bool ConfigReader::readList(FILE *f, int i, int *line)
{
	int c;
	char value[MAX_SIZE], *s;

	s = value;
	while ((c = getc(f)) != '{' && c != '\n' && c != EOF)
		;
	if (c != '{') {
		fprintf(stderr, "%s: line %d: no setting provided\n",
				m_path, *line);
		return false;
	}
	while ((c = getc(f)) != '}' && c != EOF) {
		if (c != '\t' && c != '\n') {
			*s++ = c;
		} else {
			if (c == '\n')
				++*line;
		}
	}
	if (c == EOF) {
		fprintf(stderr, "%s: unexpected end of file\n", m_path);
		return false;
	}
	*s = 0;
	printf("%s\n%s\n", settings[i].key.c_str(), value);
	return true;
}

bool ConfigReader::readOList(FILE *f, int i, int *line)
{
}

static void skipline(FILE *f, int *c)
{
	while ((*c = getc(f)) != '\n' && *c != EOF)
		;
}
