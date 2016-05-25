#include <cpr/cpr.h>
#include <tw/reader.h>
#include <tw/oauth.h>

tw::Reader::Reader(Authenticator *auth)
	: m_auth(auth)
{
}

/* read: read a single tweet specified by tweetID */
bool tw::Reader::read(const std::string &tweetID)
{
	const std::string path = API + "?id=" + tweetID;
	std::string auth_str = authenticate(tweetID);
	cpr::Response resp = cpr::Get(cpr::Url(path),
			cpr::Header{{ "Authorization", auth_str }});
	Json::Reader reader;
	if (!reader.parse(resp.text, m_response))
		return false;
	if (m_response.isMember("errors"))
		return false;
	return true;
}

/* result: build a formatted result string from the currently stored tweet */
std::string tw::Reader::result()
{
	std::string output, time, retweets, likes;

	time = m_response["created_at"].asString();
	time = time.substr(0, time.find('+') - 1);
	retweets = m_response["retweet_count"].asString();
	likes = m_response["favorite_count"].asString();

	output += "@" + m_response["user"]["name"].asString() + ": \"";
	output += m_response["text"].asString() + "\" ";
	output += "on " + time + " (";
	output += retweets + " RT" + (retweets == "1" ? "" : "s") + ", ";
	output += likes + " like" + (likes == "1" ? "" : "s") + ")";
	return output;
}

/* authenticate: create the oauth authentication header for a http request */
std::string tw::Reader::authenticate(const std::string &tweetID)
{
	std::string auth_str;
	struct tw::Authenticator::oauth_data data;

	/* generate an oauth singature and receive data */
	m_auth->siggen("GET", API, {{ "id", tweetID }}, {});
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
