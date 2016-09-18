#pragma once

#include <string>
#include <json/json.h>
#include <tw/authenticator.h>

namespace tw {

class Authenticator;

class Reader {
	public:
		Reader(Authenticator *auth);
		bool read_tweet(const std::string &tweetID);
		bool read_user(const std::string &user);
		bool read_recent();
		std::string result();
	private:
		const std::string TWEET_API =
			"https://api.twitter.com/1.1/statuses/show.json";
		const std::string USER_API =
			"https://api.twitter.com/1.1/users/show.json";
		Authenticator *tw_auth;
		Json::Value response;
		int type;
		std::string authenticate(const std::string &api,
				Authenticator::param_vec hparams);
};

}
