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
		struct oauth_data auth_data();
	private:
		std::string configpath;
		std::string consumerkey;
		std::string consumersecret;
		std::string token;
		std::string tokensecret;
		struct oauth_data odata;
		int read_keys();
		void interactive_setup();
};

}
