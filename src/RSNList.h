#pragma once

#include <json/json.h>
#include <string>

class RSNList {

	public:
		RSNList();
		int add(const char *nick, const char *rsn);
		int edit(const char *nick, const char *rsn);
		int del(const char *nick);
		const char *rsn(const char *nick);
		const char *nick(const char *rsn);
		const char *err();
	private:
		Json::Value rsns;
		char error[256];
		int valid(const char *rsn);
		Json::Value *find_rsn(const char *nick);
		Json::Value *find_nick(const char *rsn);
		int read_rsns();
};
