#include "stdafx.h"
#include "URLParser.h"

URLParser::URLParser() :m_modified(false) {}

bool URLParser::parse(const std::string &url) {

	std::regex urlRegex("(?:https?://)?(?:[a-zA-Z0-9]{1,4}\\.)*([a-zA-Z0-9\\-]+)((?:\\.[a-zA-Z]{2,4}){1,4})(/.+)?\\b");
	std::smatch match;
	if (std::regex_search(url.begin(), url.end(), match, urlRegex)) {
		// get the domain name
		std::string website = match[1].str() + match[2].str();
		last.domain = website;
		last.tweetID = "";
		if (last.twitter = website == "twitter.com") {
			// check if the URL is a twitter status
			std::string::size_type ind;
			if (match.size() > 3 && (ind = match[3].str().find("status/")) != std::string::npos) {
				// the ID of the tweet is everything after the "status/"
				last.tweetID = match[3].str().substr(ind + 7);
			}
		}
		m_modified = true;
	}
	else {
		m_modified = false;
	}
	return m_modified;
}

URLParser::URL *URLParser::getLast() {
	return &last;
}

bool URLParser::wasModified() {
	return m_modified;
}