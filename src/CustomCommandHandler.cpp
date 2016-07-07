#include <ctime>
#include <json/json.h>
#include <fstream>
#include <iostream>
#include <utils.h>
#include "CommandHandler.h"

CustomCommandHandler::CustomCommandHandler(commandMap *defaultCmds,
		TimerManager *tm, const std::string &wheelCmd)
	: m_cmp(defaultCmds), m_tmp(tm), m_wheelCmd(wheelCmd)
{
	time_t t;
	bool added;

	if (!(m_active = utils::readJSON("customcmds.json", m_commands))) {
		std::cerr << "Could not read customcmds.json.";
		return;
	}

	if (!m_commands.isMember("commands")
			|| !m_commands["commands"].isArray()) {
		m_active = false;
		std::cerr << "customcmds.json is improperly configured";
		return;
	}

	t = time(nullptr);
	added = false;
	for (Json::Value &val : m_commands["commands"]) {
		/* add new values to old commands */
		if (!val.isMember("ctime")) {
			val["ctime"] = (Json::Int64)t;
			added = true;
		}
		if (!val.isMember("mtime")) {
			val["mtime"] = (Json::Int64)t;
			added = true;
		}
		if (!val.isMember("creator")) {
			val["creator"] = "unknown";
			added = true;
		}
		if (!val.isMember("uses")) {
			val["uses"] = 0;
			added = true;
		}
		if (!val.isMember("active")) {
			val["active"] = true;
			added = true;
		}
		if (added)
			write();
		if (!(val.isMember("cmd") && val.isMember("response")
					&& val.isMember("cooldown"))) {
			m_active = false;
			std::cerr << "customcmds.json is improperly configured";
			return;
		}
		if (!validName(val["cmd"].asString(), true)) {
			m_active = false;
			std::cerr << val["cmd"].asString()
				<< " is an invalid command name - change "
				"or remove it";
			return;
		}
		if (val["cooldown"].asInt() < 0) {
			m_active = false;
			std::cerr << "command \"" << val["cmd"].asString()
				<< "\" has a negative cooldown - change "
				"or remove it";
			return;
		}

		m_tmp->add(val["cmd"].asString(), val["cooldown"].asInt());
	}
}

CustomCommandHandler::~CustomCommandHandler() {}

bool CustomCommandHandler::isActive()
{
	return m_active;
}

bool CustomCommandHandler::addCom(const std::string &cmd,
		const std::string &response, const std::string &nick,
		time_t cooldown)
{
	time_t t;

	if (!validName(cmd))
		return false;
	Json::Value command;
	command["cmd"] = cmd;
	command["response"] = response;
	command["cooldown"] = (Json::Int64)cooldown;
	command["ctime"] = (Json::Int64)(t = time(nullptr));
	command["mtime"] = (Json::Int64)t;
	command["creator"] = nick;
	command["uses"] = 0;

	m_commands["commands"].append(command);
	m_tmp->add(cmd, cooldown);
	write();
	return true;
}

bool CustomCommandHandler::delCom(const std::string &cmd)
{
	Json::ArrayIndex ind = 0;
	Json::Value def, rem;
	while (ind < m_commands["commands"].size()) {
		Json::Value val = m_commands["commands"].get(ind, def);
		if (val["cmd"] == cmd) break;
		++ind;
	}

	if (ind == m_commands["commands"].size())
		return false;

	m_commands["commands"].removeIndex(ind, &rem);
	m_tmp->remove(cmd);
	write();
	return true;
}

bool CustomCommandHandler::editCom(const std::string &cmd,
		const std::string &newResp, time_t newcd)
{
	auto *com = getCom(cmd);
	if (com->empty())
		return false;
	if (!newResp.empty())
		(*com)["response"] = newResp;
	if (newcd != -1)
		(*com)["cooldown"] = (Json::Int64)newcd;
	(*com)["mtime"] = (Json::Int64)time(nullptr);
	write();
	return true;
}

bool CustomCommandHandler::activate(const std::string &cmd)
{
	Json::Value *com;

	if ((com = getCom(cmd))->empty())
		return false;
	(*com)["active"] = true;
	write();
	return true;
}

bool CustomCommandHandler::deactivate(const std::string &cmd)
{
	Json::Value *com;

	if ((com = getCom(cmd))->empty())
		return false;
	(*com)["active"] = false;
	write();
	return true;
}

Json::Value *CustomCommandHandler::getCom(const std::string &cmd)
{
	for (auto &val : m_commands["commands"]) {
		if (val["cmd"] == cmd)
			return &val;
	}

	/* returns an empty value if the command is not found */
	return &m_emptyVal;
}

const Json::Value *CustomCommandHandler::commands()
{
	return &m_commands;
}

void CustomCommandHandler::write()
{
	utils::writeJSON("customcmds.json", m_commands);
}

bool CustomCommandHandler::validName(const std::string &cmd, bool loading)
{
	/* if CCH is loading commands from file (in constructor) */
	/* it doesn't need to check against its stored commands */
	return m_cmp->find(cmd) == m_cmp->end() && cmd != m_wheelCmd
		&& cmd.length() < 20 && (loading ? true : getCom(cmd)->empty());
}
