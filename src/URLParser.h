#pragma once

#include <string>

class URLParser {
	public:
		struct URL {
			std::string full;
			std::string domain;
			std::string subdomain;
			bool twitter;
			std::string tweetID;
		};
		URLParser();
		bool parse(const std::string &url);
		URL *getLast();
		bool wasModified();

	private:
		URL last;
		bool m_modified;
};
