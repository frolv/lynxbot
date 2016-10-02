#include <ctype.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <utils.h>
#include "lynxbot.h"
#include "RSNList.h"

RSNList::RSNList()
{
	if (!read_rsns()) {
		fprintf(stderr, "Could not read rsns.json\n");
		WAIT_INPUT();
	}
}

/* add: add a new value with given nick and rsn */
int RSNList::add(const char *nick, const char *rsn)
{
	Json::Value *t, user, prev(Json::arrayValue);

	if (!valid(rsn))
		return 0;
	if ((t = find_rsn(nick))) {
		strcpy(error, "you already have a RSN set");
		return 0;
	}
	user["nick"] = nick;
	user["rsn"] = rsn;
	user["prev"] = prev;
	rsns["rsns"].append(user);
	utils::write_json("rsns.json", rsns);
	return 1;
}

/* change the rsn associated with nick */
int RSNList::edit(const char *nick, const char *rsn)
{
	Json::Value *user;

	if (!valid(rsn))
		return 0;
	if (!(user = find_rsn(nick))) {
		strcpy(error, "you don't have a RSN set");
		return 0;
	}
	(*user)["prev"].append((*user)["rsn"].asString());
	(*user)["rsn"] = rsn;
	utils::write_json("rsns.json", rsns);
	return 1;
}

/* delete saved data for nick */
int RSNList::del(const char *nick)
{
	Json::ArrayIndex ind;
	Json::Value def, rem, val;

	ind = 0;
	while (ind < rsns["rsns"].size()) {
		val = rsns["rsns"].get(ind, def);
		if (strcmp(val["nick"].asCString(), nick) == 0) break;
		++ind;
	}

	if (ind == rsns["rsns"].size())
		return 0;

	rsns["rsns"].removeIndex(ind, &rem);
	utils::write_json("rsns.json", rsns);
	return 1;
}

/* get the rsn associated with a twitch nick */
const char *RSNList::rsn(const char *nick)
{
	Json::Value *user;

	if (!(user = find_rsn(nick)))
		return NULL;
	return (*user)["rsn"].asCString();
}

/* get the twitch nick associated with a rsn */
const char *RSNList::nick(const char *rsn)
{
	Json::Value *user;

	if (!(user = find_nick(rsn)))
		return NULL;
	return (*user)["nick"].asCString();
}

const char *RSNList::err()
{
	return error;
}

/* check if the given rsn is valid */
int RSNList::valid(const char *rsn)
{
	Json::Value *t;
	const char *s;

	for (s = rsn; *s; ++s) {
		if (!isalnum(*s) && *s != '_' && *s != '-') {
			snprintf(error, 256, "'%s' contains invalid "
					"characters", rsn);
			return 0;
		}
	}
	if (strcmp(rsn, "lynx_titan") == 0) {
		strcpy(error, "nice try");
		return 0;
	}
	if (strlen(rsn) > 12) {
		strcpy(error, "invalid RSN - too long");
		return 0;
	}
	if ((t = find_nick(rsn))) {
		snprintf(error, 256, "the RSN '%s' is already taken by %s",
				rsn, (*t)["nick"].asCString());
		return 0;
	}
	return 1;
}

/* find the value with the given nick if it exists */
Json::Value *RSNList::find_rsn(const char *nick)
{
	for (auto &val : rsns["rsns"]) {
		if (strcmp(val["nick"].asCString(), nick) == 0)
			return &val;
	}
	return NULL;
}

/* find the value with the given rsn if it exists */
Json::Value *RSNList::find_nick(const char *rsn)
{
	for (auto &val : rsns["rsns"]) {
		if (strcmp(val["rsn"].asCString(), rsn) == 0)
			return &val;
	}
	return NULL;
}

/* read the rsns file into rsns */
int RSNList::read_rsns()
{
	if (!utils::read_json("rsns.json", rsns) || !rsns.isMember("rsns")
			|| !rsns["rsns"].isArray())
		return 0;
	for (auto &val : rsns["rsns"]) {
		if (!val.isMember("nick") && !val.isMember("curr")
			&& !val.isMember("prev")) {
			fprintf(stderr, "rsns.json is improperly configured\n");
			return 0;
		}
	}
	return 1;
}
