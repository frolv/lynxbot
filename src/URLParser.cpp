#include <ctype.h>
#include <regex>
#include "URLParser.h"

URLParser::URLParser() :url_modified(false)
{
}

/* parse: search message for URL and extract data into last_url */
bool URLParser::parse(const std::string &url)
{
	std::regex urlRegex("(?:https?://)?(?:www\\.)?([a-zA-Z0-9]+\\.)*"
		"([a-zA-Z0-9\\-]+)((?:\\.[a-zA-Z]{2,3}){1,4})(/.+)?\\b");
	std::smatch match;
	if ((url_modified = std::regex_search(url.begin(), url.end(),
					match, urlRegex))) {
		/* get the domain name */
		std::string website = match[2].str() + match[3].str();
		last_url.full = match[0].str();
		last_url.domain = website;
		last_url.subdomain = match[1].str();
		last_url.tweet_id = "";
		if ((last_url.twitter = website == "twitter.com")) {
			/* check if the URL is a twitter status */
			std::string::size_type ind;
			if (match.size() > 3 &&
					(ind = match[4].str().find("status/"))
					!= std::string::npos) {
				std::string s = match[4].str().substr(ind + 7);
				for (char c : s) {
					if (!isdigit(c)) break;
					last_url.tweet_id += c;
				}
			}
		}
	}
	return url_modified;
}

struct URLParser::url *URLParser::last()
{
	return &last_url;
}

bool URLParser::modified()
{
	return url_modified;
}
