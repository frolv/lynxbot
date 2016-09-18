#include <iostream>
#include <utils.h>
#include "lynxbot.h"
#include "SelectionWheel.h"

SelectionWheel::SelectionWheel()
{
	static const std::string reqs[6] = { "active", "name", "cmd",
		"desc", "usage", "cooldown"};

	srand(static_cast<uint32_t>(time(nullptr)));
	if (!utils::read_json("wheel.json", wheeldata)) {
		fprintf(stderr, "Could not read wheel.json. Wheel will "
				"be disabled for this session\n");
		enabled = false;
		WAIT_INPUT();
	} else {
		for (auto &s : reqs) {
			if (!wheeldata.isMember("wheel" + s)) {
				fprintf(stderr, "wheel%s variable is missing "
						"from wheel.json. Wheel will "
						"be disabled for this session\n",
						s.c_str());
				enabled = false;
				WAIT_INPUT();
				return;
			}
		}
		enabled = wheeldata["wheelactive"].asBool();
		cooldown = wheeldata["wheelcooldown"].asInt() * 60;
	}
}

SelectionWheel::~SelectionWheel() {}

bool SelectionWheel::active()
{
	return enabled;
}

/* valid: return true if category is valid */
bool SelectionWheel::valid(const char *category) const
{
	return wheeldata["categories"].isMember(category);
}

const char *SelectionWheel::name() const
{
	return wheeldata["wheelname"].asCString();
}

const char *SelectionWheel::cmd() const
{
	return wheeldata["wheelcmd"].asCString();
}

const char *SelectionWheel::desc() const
{
	return wheeldata["wheeldesc"].asCString();
}

const char *SelectionWheel::usage() const
{
	return wheeldata["wheelusage"].asCString();
}

/* choose: select an item from category for user nick */
const char *SelectionWheel::choose(const char *nick, const char *category)
{
	int ind;
	const char *selection;

	Json::Value &arr = wheeldata["categories"][category];
	if (!arr.isArray()) {
		fprintf(stderr, "wheel.json is improperly configured. "
				"Wheel will be disabled.\n");
		enabled = false;
		return "";
	}

	/* choose value at random from given category */
	ind = rand() % arr.size();
	selection = arr[ind].asCString();
	add(nick, selection);

	return selection;
}

/* ready: check if nick can make another selection */
bool SelectionWheel::ready(const char *nick) const
{
	return stored.find(nick) == stored.end()
		|| time(nullptr) - lastUsed(nick) >= cooldown;
}

/* add: add a selection to stored map */
void SelectionWheel::add(const std::string &nick, const std::string &selection)
{
	if (stored.find(nick) != stored.end()) {
		/* update if aleady exists */
		auto &r = stored.find(nick)->second;
		r.first = selection;
		r.second = time(nullptr);
	} else {
		/* create otherwise */
		wheel_map::value_type val =
			{ nick, std::make_pair(selection, time(nullptr)) };
		stored.insert(val);
	}
}

/* selection: get nick's current selection */
const char *SelectionWheel::selection(const char *nick) const
{
	if (stored.find(nick) == stored.end())
		return "";
	return stored.find(nick)->second.first.c_str();
}

time_t SelectionWheel::lastUsed(const std::string &nick) const
{
	return stored.find(nick)->second.second;
}
