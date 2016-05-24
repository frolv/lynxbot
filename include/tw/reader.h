#pragma once

#include <string>
#include <tw/authenticator.h>

namespace tw {

class Authenticator;

class Reader {
	public:
		Reader(Authenticator *auth);
		void read(const std::string &tweetID);
		std::string result();
	private:
		const std::string API =
			"https://api.twitter.com/1.1/statuses/show.json"; 
		Authenticator *m_auth;
		std::string m_response;
		std::string authenticate(const std::string &tweetID);
};

}
