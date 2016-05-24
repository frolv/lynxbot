#include <cpr/cpr.h>
#include <tw/reader.h>
#include <tw/oauth.h>

#include <iostream>

tw::Reader::Reader(Authenticator *auth)
	: m_auth(auth)
{
}

void tw::Reader::read(const std::string &tweetID)
{
	const std::string path = API + "?id=" + tweetID;
	std::string auth_str = authenticate(tweetID);
	cpr::Response resp = cpr::Get(cpr::Url(path),
			cpr::Header{{ "Authorization", auth_str }});
	m_response = resp.text;
}

std::string tw::Reader::result()
{
	return m_response;
}

std::string tw::Reader::authenticate(const std::string &tweetID)
{
	std::string auth_str;
	struct tw::Authenticator::oauth_data data;

	m_auth->siggen("GET", API, {{ "id", tweetID }}, {{}});
	data = m_auth->authData();

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
