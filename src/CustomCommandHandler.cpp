#include <ctime>
#include <json/json.h>
#include <fstream>
#include <iostream>
#include <utils.h>
#include "CommandHandler.h"

static bool valid_resp(const std::string &resp, std::string &err);

CustomCommandHandler::CustomCommandHandler(commandMap *defaultCmds,
		TimerManager *tm, const std::string &wheelCmd,
		const std::string &name, const std::string &channel)
	: m_cmp(defaultCmds), m_tmp(tm), m_wheelCmd(wheelCmd),
	m_name(name), m_channel(channel)
{
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

	if (!cmdcheck())
		std::cerr << "\nCustom commands disabled." << std::endl;
}

CustomCommandHandler::~CustomCommandHandler() {}

bool CustomCommandHandler::isActive()
{
	return m_active;
}

/* addCom: add a new custom command cmd with response and cooldown */
bool CustomCommandHandler::addcom(const std::string &cmd,
		const std::string &response, const std::string &nick,
		time_t cooldown)
{
	time_t t;

	if (!validName(cmd)) {
		m_error = "invalid command name: $" + cmd;
		return false;
	}
	if (!valid_resp(response, m_error))
		return false;
	Json::Value command;
	command["active"] = true;
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

/* delCom: delete command cmd if it exists */
bool CustomCommandHandler::delcom(const std::string &cmd)
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

/* editCom: modify the command cmd with newResp and newcd */
bool CustomCommandHandler::editcom(const std::string &cmd,
		const std::string &newResp, time_t newcd)
{
	auto *com = getcom(cmd);
	if (com->empty()) {
		m_error = "not a command: $" + cmd;
		return false;
	}
	if (!valid_resp(newResp, m_error))
		return false;
	if (!newResp.empty())
		(*com)["response"] = newResp;
	if (newcd != -1) {
		(*com)["cooldown"] = (Json::Int64)newcd;
		m_tmp->remove(cmd);
		m_tmp->add(cmd, newcd);
	}
	(*com)["mtime"] = (Json::Int64)time(nullptr);
	write();
	return true;
}

/* activate: activate the command cmd */
bool CustomCommandHandler::activate(const std::string &cmd)
{
	Json::Value *com;

	if ((com = getcom(cmd))->empty()) {
		m_error = "not a command: $" + cmd;
		return false;
	}
	if (!valid_resp((*com)["response"].asString(), m_error))
		return false;
	(*com)["active"] = true;
	write();
	return true;
}

/* deactivate: deactivate the command cmd */
bool CustomCommandHandler::deactivate(const std::string &cmd)
{
	Json::Value *com;

	if ((com = getcom(cmd))->empty()) {
		m_error = "not a command: $" + cmd;
		return false;
	}
	(*com)["active"] = false;
	write();
	return true;
}

/* rename: rename custom command cmd to newcmd */
bool CustomCommandHandler::rename(const std::string &cmd,
		const std::string &newcmd)
{
	Json::Value *com;

	if ((com = getcom(cmd))->empty()) {
		m_error = "not a command: $" + cmd;
		return false;
	}
	if (!validName(newcmd)) {
		m_error = "invalid command name: $" + newcmd;
		return false;
	}
	(*com)["cmd"] = newcmd;
	(*com)["mtime"] = (Json::Int64)time(nullptr);
	m_tmp->remove(cmd);
	m_tmp->add(newcmd, (*com)["cooldown"].asInt64());
	write();
	return true;
}

/* getcom: return command value if it exists, empty value otherwise */
Json::Value *CustomCommandHandler::getcom(const std::string &cmd)
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

/* write: write all commands to file */
void CustomCommandHandler::write()
{
	utils::writeJSON("customcmds.json", m_commands);
}

/* validName: check if cmd is a valid command name */
bool CustomCommandHandler::validName(const std::string &cmd, bool loading)
{
	/* if CCH is loading commands from file (in constructor) */
	/* it doesn't need to check against its stored commands */
	return m_cmp->find(cmd) == m_cmp->end() && cmd != m_wheelCmd
		&& cmd.length() < 20 && (loading ? true : getcom(cmd)->empty());
}

std::string CustomCommandHandler::error() const
{
	return m_error;
}

/* format: format a response for cmd */
std::string CustomCommandHandler::format(const Json::Value *cmd,
		const std::string &nick) const
{
	size_t ind;
	std::string out, ins;
	char c;

	ind = 0;
	out = (*cmd)["response"].asString();
	while ((ind = out.find('%', ind)) != std::string::npos) {
		c = out[ind + 1];
		out.erase(ind, 2);
		switch (c) {
		case '%':
			ins = "%";
			break;
		case 'N':
			ins = "@" + nick + ",";
			break;
		case 'b':
			ins = m_name;
			break;
		case 'c':
			ins = m_channel;
			break;
		case 'n':
			ins = nick;
			break;
		case 'u':
			ins = utils::formatInteger((*cmd)["uses"].asString());
			break;
		default:
			break;
		}
		out.insert(ind, ins);
		ind += ins.length();
	}
	return out;
}

/* cmdcheck: check the validity of a command and add missing fields */
bool CustomCommandHandler::cmdcheck()
{
	time_t t;
	bool added;

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
		if (!(val.isMember("cmd") && val.isMember("response")
					&& val.isMember("cooldown"))) {
			m_active = false;
			std::cerr << "customcmds.json is improperly configured";
			return false;
		}
		if (!validName(val["cmd"].asString(), true)) {
			m_active = false;
			std::cerr << val["cmd"].asString()
				<< " is an invalid command name - change "
				"or remove it";
			return false;
		}
		if (val["cooldown"].asInt() < 0) {
			m_active = false;
			std::cerr << "command \"" << val["cmd"].asString()
				<< "\" has a negative cooldown - change "
				"or remove it";
			return false;
		}
		/* check validity of response */
		if (!valid_resp(val["response"].asString(), m_error)) {
			std::cerr << "Custom command " << val["cmd"].asString()
				<< ": " << m_error << std::endl;
			val["active"] = false;
			added = true;
		}
		if (added)
			write();
		m_tmp->add(val["cmd"].asString(), val["cooldown"].asInt64());
	}
	return true;
}

/* valid_resp: check if a response has valid format characters */
static bool valid_resp(const std::string &resp, std::string &err)
{
	static const std::string fmt_c = "%Nbcnu";
	size_t ind;
	int c;

	ind = -1;
	while ((ind = resp.find('%', ind + 1)) != std::string::npos) {
		if (ind == resp.length() - 1) {
			err = "unexpected end of line after '%' in response";
			return false;
		}
		c = resp[ind + 1];
		if (fmt_c.find(c) == std::string::npos) {
			err = "invalid format sequence '%";
			err += (char)c;
			err += "' in response";
			return false;
		}
		if (c == '%')
			++ind;
	}
	return true;
}
