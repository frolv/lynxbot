#pragma once

#include <string>
#include <json/json.h>
#include <tw/authenticator.h>

namespace tw {

class Authenticator;

class Reader {
	public:
		Reader(Authenticator *auth);
		bool read(const std::string &tweetID);
		std::string result();
	private:
		const std::string API =
			"https://api.twitter.com/1.1/statuses/show.json"; 
		Authenticator *m_auth;
		Json::Value m_response;
		std::string authenticate(const std::string &tweetID);
};

}
