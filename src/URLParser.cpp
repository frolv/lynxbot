#include <ctype.h>
#include <regex>
#include "URLParser.h"

URLParser::URLParser() :m_modified(false)
{
}

/* parse: search message for URL and extract data into m_last */
bool URLParser::parse(const std::string &url)
{
	std::regex urlRegex("(?:https?://)?(?:www\\.)?([a-zA-Z0-9]+\\.)*"
		"([a-zA-Z0-9\\-]+)((?:\\.[a-zA-Z]{2,3}){1,4})(/.+)?\\b");
	std::smatch match;
	if ((m_modified = std::regex_search(url.begin(), url.end(),
					match, urlRegex))) {
		/* get the domain name */
		std::string website = match[2].str() + match[3].str();
		m_last.full = match[0].str();
		m_last.domain = website;
		m_last.subdomain = match[1].str();
		m_last.tweetID = "";
		if ((m_last.twitter = website == "twitter.com")) {
			/* check if the URL is a twitter status */
			std::string::size_type ind;
			if (match.size() > 3 &&
					(ind = match[4].str().find("status/"))
					!= std::string::npos) {
				std::string s = match[4].str().substr(ind + 7);
				for (char c : s) {
					if (!isdigit(c)) break;
					m_last.tweetID += c;
				}
			}
		}
	}
	return m_modified;
}

URLParser::URL *URLParser::getLast()
{
	return &m_last;
}

bool URLParser::wasModified()
{
	return m_modified;
}
