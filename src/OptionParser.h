#pragma once

#include <string>

#ifndef EOF
# define EOF (-1)
#endif

#define NO_ARG  0
#define REQ_ARG 1

#define ERR_SIZE 256

class OptionParser {

	public:
		struct option {
			const char *name;
			int type;
			int val;
		};

		OptionParser(const std::string &cmd, const char *optstr);
		int getopt();
		int getopt_long(struct option *long_opts);
		size_t optind() const;
		char *optarg();
		int optopt() const;
		char *opterr();

	private:
		const char *m_cmdstr;
		const char *m_optstr;
		char m_cmd[ERR_SIZE];
		char m_opterr[ERR_SIZE];
		size_t m_cmdlen;
		size_t m_optind;
		char m_optarg[ERR_SIZE];
		uint8_t m_state;
		int m_optopt;
		int parseopt(int c);
		int type(int c) const;
		void puterr(int type);

};
