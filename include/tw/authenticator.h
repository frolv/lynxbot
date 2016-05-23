#pragma once

#include <string>
#include <vector>

namespace tw {

class Authenticator {
	public:
		typedef std::vector<std::pair<std::string, std::string>> param_vec;
		Authenticator();
		~Authenticator();
		void interactiveSetup();
		void siggen(const std::string &method, const std::string &URL,
				const param_vec &head_params,
				const param_vec &body_params);
	private:
		std::string m_consumerkey = "junk";
		std::string m_consumersecret = "_junk";
		std::string m_token = "trash";
		std::string m_tokensecret = "_trash";
		std::string m_signature;
		void readKeys();
};

}
