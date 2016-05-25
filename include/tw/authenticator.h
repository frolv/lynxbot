#pragma once

#include <string>
#include <vector>

namespace tw {

class Authenticator {
	public:
		typedef std::vector<std::pair<std::string, std::string>> param_vec;
		struct oauth_data {
			std::string c_key;
			std::string nonce;
			std::string sig;
			std::string sig_method;
			std::string timestamp;
			std::string token;
			std::string version;
		};
		Authenticator();
		void siggen(const std::string &method, const std::string &URL,
				const param_vec &head_params,
				const param_vec &body_params);
		struct oauth_data authData();
	private:
		std::string m_configpath;
		std::string m_consumerkey;
		std::string m_consumersecret;
		std::string m_token;
		std::string m_tokensecret;
		struct oauth_data m_data;
		int readKeys();
		void interactiveSetup();
};

}
