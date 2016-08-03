#include <cpr/cpr.h>
#include <tw/reader.h>
#include <tw/oauth.h>
#include <utils.h>

#define ERR   0
#define TWEET 1
#define USER  2

static std::string format_tweet(const Json::Value &resp);
static std::string format_user(const Json::Value &resp);

tw::Reader::Reader(Authenticator *auth)
	: m_auth(auth)
{
}

/* read_tweet: read a single tweet specified by tweetID */
bool tw::Reader::read_tweet(const std::string &tweetID)
{
	const std::string path = TWEET_API + "?id=" + tweetID;
	std::string auth_str = authenticate(TWEET_API, {{ "id", tweetID }});
	cpr::Response resp = cpr::Get(cpr::Url(path),
			cpr::Header{{ "Authorization", auth_str }});
	Json::Reader reader;
	if (!reader.parse(resp.text, m_response)) {
		m_type = ERR;
		return false;
	}
	if (m_response.isMember("errors")) {
		m_type = ERR;
		return false;
	}
	m_type = TWEET;
	return true;
}

/* read_user: read the profile of user */
bool tw::Reader::read_user(const std::string &user)
{
	const std::string path = USER_API + "?screen_name=" + user;
	std::string auth_str = authenticate(USER_API,{{ "screen_name", user }});
	cpr::Response resp = cpr::Get(cpr::Url(path),
			cpr::Header{{ "Authorization", auth_str }});
	Json::Reader reader;
	if (!reader.parse(resp.text, m_response)) {
		m_type = ERR;
		return false;
	}
	if (m_response.isMember("errors")) {
		m_type = ERR;
		return false;
	}
	m_type = USER;
	return true;
}

/* read_recent: read the most recent tweet of last read user */
bool tw::Reader::read_recent()
{
	if (m_type != USER)
		return false;
	return read_tweet(m_response["status"]["id_str"].asString());
}

/* result: build a formatted result string from the currently stored tweet */
std::string tw::Reader::result()
{
	switch (m_type) {
	case TWEET:
		return format_tweet(m_response);
	case USER:
		return format_user(m_response);
	default:
		return "";
	}
}

/* authenticate: create the oauth authentication header for a http request */
std::string tw::Reader::authenticate(const std::string &api,
		Authenticator::param_vec hparams)
{
	std::string auth_str;
	struct tw::Authenticator::oauth_data data;

	/* generate an oauth singature and receive data */
	m_auth->siggen("GET", api, hparams, {});
	data = m_auth->authData();

	/* add all components to string */
	auth_str += "OAuth ";
	auth_str += "oauth_consumer_key=\"" + pencode(data.c_key) + "\", ";
	auth_str += "oauth_nonce=\"" + pencode(data.nonce) + "\", ";
	auth_str += "oauth_signature=\"" + pencode(data.sig) + "\", ";
	auth_str += "oauth_signature_method=\"" + pencode(data.sig_method) + "\", ";
	auth_str += "oauth_timestamp=\"" + pencode(data.timestamp) + "\", ";
	auth_str += "oauth_token=\"" + pencode(data.token) + "\", ";
	auth_str += "oauth_version=\"" + pencode(data.version) + "\"";

	return auth_str;
}

/* format_tweet: return a formatted string of information about a tweet */
static std::string format_tweet(const Json::Value &resp)
{
	std::string output, status, time, retweets, likes;

	time = resp["created_at"].asString();
	time = time.substr(0, time.find('+') - 1);
	retweets = resp["retweet_count"].asString();
	likes = resp["favorite_count"].asString();
	status = resp["text"].asString();

	output += "@" + resp["user"]["screen_name"].asString() + ": \"";
	output += utils::decode(status) + "\" ";
	output += "on " + time + " (";
	output += utils::formatInteger(retweets) + " RT"
		+ (retweets == "1" ? "" : "s") + ", ";
	output += utils::formatInteger(likes) + " like"
		+ (likes == "1" ? "" : "s") + ")";
	return output;
}

/* format_user: return a formatted string of information about a twitter user */
static std::string format_user(const Json::Value &resp)
{
	std::string output, name, desc, tweets, followers;

	name = resp["screen_name"].asString();
	desc = resp["description"].asString();
	tweets = resp["statuses_count"].asString();
	followers = resp["followers_count"].asString();

	output += "@" + name + ": " + desc + " ";
	output += "(" + utils::formatInteger(tweets) + " tweet"
		+ (tweets == "1" ? "" : "s") + ", ";
	output += utils::formatInteger(followers) + " follower"
		+ (followers == "1" ? "" : "s") + ")";
	return output;
}
