#include "stdafx.h"

CustomCommandHandler::CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm, const std::string &wheelCmd)
	:m_cmp(defaultCmds), m_tmp(tm), m_wheelCmd(wheelCmd) {

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
		if (!validName(val["cmd"].asString(), true)) {
			m_active = false;
			std::cerr << val["cmd"].asString() << " is an invalid command name - change or remove it.";
			return;
		}
		if (val["cooldown"].asInt() < 0) {
			m_active = false;
			std::cerr << "Command \"" << val["cmd"].asString() << "\" has a negative cooldown - change or remove it.";
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

	// todo: change to bool
	Json::Value command;
	command["cmd"] = cmd;
	command["response"] = response;
	command["cooldown"] = cooldown;

	m_commands["commands"].append(command);
	m_tmp->add(cmd, cooldown);
	writeToFile();

}

bool CustomCommandHandler::delCom(const std::string &cmd) {

	Json::ArrayIndex ind = 0;
	Json::Value def, rem;
	while (ind < m_commands["commands"].size()) {
		auto &val = m_commands["commands"].get(ind, def);
		if (val["cmd"] == cmd) break;
		++ind;
	}

	if (ind == m_commands["commands"].size()) {
		return false;
	}

	m_commands["commands"].removeIndex(ind, &rem);
	writeToFile();
	return true;

}

Json::Value CustomCommandHandler::getCom(const std::string &cmd) {

	for (auto &val : m_commands["commands"]) {
		if (val["cmd"] == cmd) {
			return val;
		}
	}

	return Json::Value();

}

void CustomCommandHandler::writeToFile() {

	std::ofstream ccfile;
	ccfile.open(utils::getApplicationDirectory() + "\\customcmds.json");
	Json::StyledWriter sw;
	ccfile << sw.write(m_commands);
	ccfile.close();

}

bool CustomCommandHandler::validName(const std::string &cmd, bool loading) {
	// if CCH is loading commands from file (in constructor), it doesn't need to check against its stored commands
	return !(*m_cmp)[cmd] && cmd != m_wheelCmd && cmd.length() < 20 && (loading ? true : getCom(cmd).empty());
}