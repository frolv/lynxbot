#include <algorithm>
#include <ctime>
#include <tw/authenticator.h>
#include <tw/oauth.h>

#include <iostream>

tw::Authenticator::Authenticator()
{
}

tw::Authenticator::~Authenticator()
{
}

/* siggen: generate an oauth signature for a twitter api request */
void tw::Authenticator::siggen(const std::string &method, const std::string &URL,
		const param_vec &head_params, const param_vec &body_params)
{
	std::vector<std::string> enc_params;
	for (auto p : head_params)
		enc_params.push_back(pencode(p.first) + "=" + pencode(p.second));
	for (auto p : body_params)
		enc_params.push_back(pencode(p.first) + "=" + pencode(p.second));
	enc_params.push_back("oauth_consumer_key=" + m_consumerkey);
	enc_params.push_back("oauth_nonce=" + noncegen());
	enc_params.push_back("oauth_signature_method=HMAC-SHA1");
	enc_params.push_back("oauth_timestamp=" + std::to_string(time(nullptr)));
	enc_params.push_back("oauth_token=" + m_token);
	enc_params.push_back("oauth_version=1.0");
	
	std::sort(enc_params.begin(), enc_params.end());

	std::string param_str;
	for (unsigned i = 0; i < enc_params.size(); ++i) {
		param_str += enc_params[i];
		if (i != enc_params.size() - 1)
			param_str += '&';
	}

	std::string base_str;
	base_str += method + "&";
	base_str += pencode(URL) + "&";
	base_str += pencode(param_str);

	std::string signing_key;
	signing_key += pencode(m_consumersecret) + "&";
	signing_key += pencode(m_tokensecret);
}
