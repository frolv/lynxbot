#include <algorithm>
#include <ctime>
#include <sstream>
#include <openssl/hmac.h>
#include <tw/authenticator.h>
#include <tw/base64.h>
#include <tw/oauth.h>

#include <iostream>

tw::Authenticator::Authenticator()
{
}

/* siggen: generate an oauth signature for a twitter api request */
void tw::Authenticator::siggen(const std::string &method, const std::string &URL,
		const param_vec &head_params, const param_vec &body_params)
{
	/* generate ouath data */
	m_data.c_key = m_consumerkey;
	m_data.nonce = noncegen();
	m_data.sig_method = "HMAC-SHA1";
	m_data.timestamp = std::to_string(time(nullptr));
	m_data.token = m_token;
	m_data.version = "1.0";

	std::vector<std::string> enc_params;
	for (auto p : head_params)
		enc_params.push_back(pencode(p.first) + "=" + pencode(p.second));
	for (auto p : body_params)
		enc_params.push_back(pencode(p.first) + "=" + pencode(p.second));
	enc_params.push_back("oauth_consumer_key=" + m_data.c_key);
	/* enc_params.push_back("oauth_nonce=kYjzVBB8Y0ZFabxSWbWovY3uYSQ2pTgmZeNu2VS4cg"); */
	enc_params.push_back("oauth_nonce=" + m_data.nonce);
	enc_params.push_back("oauth_signature_method=" + m_data.sig_method);
	/* enc_params.push_back("oauth_timestamp=1318622958"); */
	enc_params.push_back("oauth_timestamp=" + m_data.timestamp);
	enc_params.push_back("oauth_token=" + m_data.token);
	enc_params.push_back("oauth_version=" + m_data.version);
	
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

	std::cout << signing_key << std::endl << std::endl;
	std::cout << base_str << std::endl << std::endl;

	unsigned char enc[1024];
	unsigned int enc_len;
	HMAC(EVP_sha1(), signing_key.c_str(), signing_key.length(),
			(unsigned char *)base_str.c_str(), base_str.length(),
			enc, &enc_len);
	/* std::cout << std::endl; */
	/* for (unsigned i = 0; i < enc_len; ++i) */
	/* 	std::cout << std::hex << (unsigned int)enc[i]; */
	/* std::cout << std::endl; */

	m_data.sig = base64_enc((char *)enc, enc_len);
	/* std::cout << m_signature << std::endl; */
}

struct tw::Authenticator::oauth_data tw::Authenticator::authData()
{
	return m_data;
}
