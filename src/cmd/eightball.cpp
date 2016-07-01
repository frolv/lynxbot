#include "../CommandHandler.h"

/* 8ball: respond to questions */
std::string CommandHandler::eightball(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 6
			|| c->fullCmd[c->fullCmd.length() - 1] != '?')
		return "[8 BALL] Ask me a question.";
	std::uniform_int_distribution<> dis(0, m_eightball.size());
	return "[8 BALL] @" + c->nick + ", " + m_eightball[dis(m_gen)] + ".";
}
