#pragma once

class URLParser {

	public:
		struct URL {
			std::string domain;
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