#include <ctime>
#include <fstream>
#include <iostream>
#include <utils.h>
#include "EventManager.h"
#include "lynxbot.h"

EventManager::EventManager(ConfigReader *cfgr)
	: cfg(cfgr), messages_active(false)
{
	read_messages();
	messages_active = message_list.size() > 0;
}

EventManager::~EventManager() {}

/* active: return true if recurring messages are active */
bool EventManager::active()
{
	return messages_active;
}

/* activate: enable recurring messages */
void EventManager::activate()
{
	messages_active = true;
}

/* deactivate: disable recurring messages */
void EventManager::deactivate()
{
	messages_active = false;
}

/* addmsg: add a recurring message */
bool EventManager::addmsg(const char *msg, time_t cd, bool write)
{
	if (message_list.size() >= 5 || cd % 300 != 0 || cd > 3600)
		return false;
	message_list.push_back({ std::string(msg), cd });
	reload_messages();
	if (write)
		write_messages();
	return true;
}

/* delmsg: delete a recurring message */
bool EventManager::delmsg(size_t id)
{
	if (id < 1 || id > message_list.size())
		return false;
	auto it = message_list.begin() + (id - 1);
	message_list.erase(it);
	reload_messages();
	write_messages();
	return true;
}

/* editmsg: modify recurring message id */
bool EventManager::editmsg(size_t id, const char *msg, time_t cd)
{
	if (id < 1 || id > message_list.size())
		return false;
	auto &pair = message_list[id - 1];
	if (msg)
		pair.first = std::string(msg);
	if (cd != -1) {
		pair.second = cd;
		reload_messages();
	}
	write_messages();
	return true;
}

/* msglist: return a formatted string of all messages and intervals */
std::string EventManager::msglist() const
{
	std::string output = "(";
	output += messages_active ? "active" : "inactive";
	output += ") ";
	for (size_t i = 0; i < message_list.size(); ++i) {
		/* only display first 35 characters of each message */
		output += std::to_string(i + 1) + ": "
			+ message(i, 35)
			+ (i == message_list.size() - 1 ? "" : ", ");
	}
	if (message_list.empty())
		output += "No recurring messages exist.";
	return output;
}

/* message: return a single message and its interval */
std::string EventManager::message(size_t id, int lim) const
{
	std::string output;

	const std::string &msg = message_list[id].first;
	if (lim == -1)
		output += msg;
	else
		output += msg.length() < (size_t)lim
			? msg : (msg.substr(0, lim - 3) + "...");
	output += " [" + std::to_string(message_list[id].second / 60) + "m]";

	return output;
}

/* messages: return a pointer to messages vector */
std::vector<std::pair<std::string, time_t>> *EventManager::messages()
{
	return &message_list;
}

/* read_messages: read recurring messages from file */
void EventManager::read_messages()
{
	size_t i, max;
	bool error, valid;
	uint32_t cd;
	std::string err;
	std::vector<std::unordered_map<std::string, std::string>> &recurring =
		cfg->olist()["recurring"];

	max = recurring.size();
	if (max > 5) {
		max = 5;
		printf("%s: only reading first five messages\n",
				cfg->path());
	}

	error = false;
	for (i = 0; i < max; ++i) {
		valid = true;
		auto map = recurring[i];

		if (map.find("message") == map.end()) {
			err = "no message provided";
			valid = false;
		}
		if (map.find("period") == map.end()) {
			err = "no period provided";
			valid = false;
		}

		if (valid) {
			if (!utils::parse_int(cd, map["period"], err)) {
				valid = false;
			} else if (!addmsg(map["message"].c_str(),
						cd * 60, false)) {
				err = "cooldown must be multiple of 5 minutes "
					"and no longer than 60 minutes";
				valid = false;
			}
		}

		if (!valid) {
			error = true;
			fprintf(stderr, "%s: recurring message %lu: %s\n"
					"skipping message\n", cfg->path(),
					i + 1, err.c_str());
			continue;
		}
	}
	if (error)
		WAIT_INPUT();
}

/* write_messages: write recurring messages to file */
void EventManager::write_messages()
{
	std::vector<std::unordered_map<std::string, std::string>> &recurring =
		cfg->olist()["recurring"];
	recurring.clear();
	for (auto &p : message_list) {
		std::unordered_map<std::string, std::string> map;
		map["message"] = p.first;
		map["period"] = std::to_string(p.second / 60);
		recurring.emplace_back(map);
	}
	cfg->write();
}

/* reload_messages: load all messages into timer */
void EventManager::reload_messages()
{
	for (std::vector<std::string>::size_type i = 0;
			i < message_list.size(); ++i) {
		/* update offsets for all existing messages and new message */
		std::string name = "msg" + std::to_string(i);
		remove(name);
		uint16_t offset =
			static_cast<uint16_t>(i * (300 / message_list.size()));
		add(name, message_list[i].second, time(nullptr) + offset);
	}
}
