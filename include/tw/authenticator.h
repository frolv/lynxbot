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
		void interactiveSetup();
		void siggen(const std::string &method, const std::string &URL,
				const param_vec &head_params,
				const param_vec &body_params);
		struct oauth_data authData();
	private:
		std::string m_consumerkey = "FvZiduZNLVbklinilyU8IxwfL";
		std::string m_consumersecret = "wEK41HoIKW56fBCsZ69fzT3c1I8L8ddknMy7yGEj04SMFP2Ubc";
		std::string m_token = "4224622768-ijLYyFRxJrv9E67QZlc2i9xgnj4e7gKN3KR1k97";
		std::string m_tokensecret = "6RzIA9xlLsyeqnNlC2hCHZ3QZVMRY3bzJRz8up6bSOrsR";
		struct oauth_data m_data;
		/* std::string m_consumerkey = "xvz1evFS4wEEPTGEFPHBog"; */
		/* std::string m_consumersecret = "kAcSOqF21Fu85e7zjz7ZN2U4ZRhfV3WpwPAoE3Z7kBw"; */
		/* std::string m_token = "370773112-GmHxMAgYyLbNEtIKZeRNFsMKPR9EyMZeS9weJAEb"; */
		/* std::string m_tokensecret = "LswwdoUaIvS8ltyTth4J50vUPVVHtR2YPi5kE"; */
		void readKeys();
};

}
