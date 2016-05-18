#pragma once

#include <string>

#define EOF (-1)

class OptionParser {

	public:
		OptionParser(const std::string &optstr, const std::string &options);
		~OptionParser();
		int16_t getopt();
		std::string::size_type optind();
		std::string optarg();
		int16_t optopt();
		
	private:
		const std::string m_optstr;
		const std::string m_options;
		std::string::size_type m_optind;
		std::string m_optarg;
		uint8_t m_state;
		int16_t m_optopt;
		bool valid(int16_t c);
		bool arg(int16_t c);

};
