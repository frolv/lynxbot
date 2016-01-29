#pragma once

class Moderator {

	public:
		Moderator();
		~Moderator();
		bool isValidMsg(const std::string &msg, const std::string &nick, std::string &reason);
		void whitelist(const std::string &site);
		void permit(const std::string &nick);
		std::string getFormattedWhitelist() const;
	private:
		std::vector<std::string> m_whitelist;
		std::vector<std::string> m_permitted;
		bool checkLink(const std::string &msg) const;
		bool checkSpam(const std::string &msg) const;
		bool checkCaps(const std::string &msg) const;

};