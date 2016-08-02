#include <algorithm>
#include <iostream>
#include <utils.h>
#include "GEReader.h"

static void json_qsort(Json::Value &a, int lo, int hi);
static void swap(Json::Value &a, int i, int j);
static int json_bsearch(const Json::Value &a, const std::string &s,
		int lo, int hi);

GEReader::GEReader()
{
	if (!(m_active = utils::readJSON("itemids.json", m_itemIDs))) {
		std::cerr << "Failed to read RS Item IDs" << std::endl;
		std::cerr << "$ge command will be disabled for this "
			"session" << std::endl;
		return;
	}
	readNicks();
	sort();
}

GEReader::~GEReader() {};

/* active: return true if the GEReader is active */
bool GEReader::active() const
{
	return m_active;
}

/* getItem: return the json value of the item with name name */
Json::Value GEReader::getItem(const char *name) const
{
	int ind;
	const Json::Value &arr = m_itemIDs["items"];
	std::string n(name);

	std::transform(n.begin(), n.end(), n.begin(), tolower);
	n[0] = toupper(n[0]);

	if (m_nicks.find(n) != m_nicks.end())
		n = m_nicks.at(n);

	if ((ind = json_bsearch(arr, n, 0, arr.size() - 1)) != -1)
		return arr[ind];

	/* return empty value if item not found */
	return Json::Value();
}

/* readNicks: read all item nicknames into m_nicks */
void GEReader::readNicks()
{
	for (Json::Value &val : m_itemIDs["items"]) {
		if (val.isMember("nick")) {
			for (Json::Value &nick : val["nick"])
				m_nicks[nick.asString()]
					= val["name"].asString();
		}
	}
}

/* sort: sort the items array by name */
void GEReader::sort()
{
	Json::Value &arr = m_itemIDs["items"];
	json_qsort(arr, 0, arr.size() - 1);
}

/* json_qsort: quicksort a json array */
static void json_qsort(Json::Value &a, int lo, int hi)
{
	int i, last;

	if (lo >= hi)
		return;

	swap(a, lo, (lo + hi) / 2);
	last = lo;
	for (i = lo + 1; i <= hi; ++i) {
		if (a[i]["name"].asString() < a[lo]["name"].asString())
			swap(a, ++last, i);
	}
	swap(a, lo, last);
	json_qsort(a, lo, last - 1);
	json_qsort(a, last + 1, hi);
}

/* swap: swap two items in a json array */
static void swap(Json::Value &a, int i, int j)
{
	Json::Value tmp = a[i];
	a[i] = a[j];
	a[j] = tmp;
}

/* json_bsearch: return the index of the item with name s in a */
static int json_bsearch(const Json::Value &a, const std::string &s,
		int lo, int hi)
{
	int mid;

	if (lo > hi)
		return -1;

	mid = (lo + hi) / 2;
	if (a[mid]["name"].asString() == s)
		return mid;
	if (a[mid]["name"].asString() > s)
		return json_bsearch(a, s, lo, mid - 1);
	else
		return json_bsearch(a, s, mid + 1, hi);
}
