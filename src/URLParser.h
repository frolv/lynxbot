#pragma once

#include <string>

class URLParser {
	public:
		struct url {
			std::string full;
			std::string domain;
			std::string subdomain;
			bool twitter;
			std::string tweet_id;
		};
		URLParser();
		bool parse(const std::string &url);
		struct url *last();
		bool modified();

	private:
		struct url last_url;
		bool url_modified;
};
