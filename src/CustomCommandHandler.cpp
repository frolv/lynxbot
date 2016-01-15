#include "stdafx.h"

CustomCommandHandler::CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm) :m_cmp(defaultCmds), m_tmp(tm) {

	m_active = utils::readJSON("customcmds.json", m_commands);
	if (!m_active) {
		std::cerr << "Could not read customcmds.json.";
		return;
	}

	if (!m_commands.isMember("commands") || !m_commands["commands"].isArray()) {
		m_active = false;
		std::cerr << "customcmds.json is improperly configured.";
		return;
	}

	for (Json::Value &val : m_commands["commands"]) {

		if (!(val.isMember("cmd") && val.isMember("response") && val.isMember("cooldown"))) {
			m_active = false;
			std::cerr << "customcmds.json is improperly configured.";
			return;
		}

		if (!validName(val["cmd"].asString())) {
			m_active = false;
			std::cerr << val["cmd"].asString() << " is an invalid command name - change or remove it.";
			return;
		}

		m_tmp->add(val["cmd"].asString(), val["cooldown"].asInt());

	}

}

CustomCommandHandler::~CustomCommandHandler() {}

bool CustomCommandHandler::isActive() {
	return m_active;
}

void CustomCommandHandler::addCom(const std::string &cmd, const std::string &response, std::time_t cooldown) {

	Json::Value command;
	command["cmd"] = cmd;
	command["response"] = response;
	command["cooldown"] = cooldown;

	m_commands["commands"].append(command);

}

bool CustomCommandHandler::validName(const std::string &cmd) {
	return !(*m_cmp)[cmd] && cmd.length() < 20;
}