#include "stdafx.h"
#include "Utils.h"

bool utils::startsWith(const std::string &str, const std::string &prefix)
{
	return str.find(prefix) == 0;
}

bool utils::endsWith(const std::string &str, const std::string &suffix)
{
	return str.find(suffix) == str.length() - suffix.length();
}

std::vector<std::string> &utils::split(const std::string &str, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (!item.empty())
			elems.push_back(item);
	}
	return elems;
}

std::string utils::formatInteger(std::string &integer)
{
	int pos = integer.length() - 3;
	if (pos < 1)
		return integer;
	while (pos > 0) {
		integer.insert(pos, ",");
		pos -= 3;
	}
	return integer;
}

std::string utils::appdir()
{
#ifdef _WIN32
	return appdir_win();
#endif
#ifdef __linux__
	return appdir_unix();
#endif
}

static std::string appdir_win()
{
	char pBuf[1000];
	int bytes;
	if ((bytes = GetModuleFileName(NULL, pBuf, sizeof(pBuf))) == 0)
		return "";

	// strip filename from path
	std::string path(pBuf);
	path = path.substr(0, path.find_last_of("\\"));

	return path;
}

static std::string appdir_unix()
{
	DIR *d;
	d = opendir(".");
}

bool utils::readJSON(const std::string &filename, Json::Value &val)
{
	Json::Reader reader;
	std::ifstream fileStream(appdir() + "/json/" + filename, std::ifstream::binary);

	if (!reader.parse(fileStream, val)) {
		std::cerr << reader.getFormattedErrorMessages() << std::endl;
		return false;
	}

	return true;
}

bool utils::socketConnect(SOCKET &s, WSADATA &wsa, const char *port, const char *server)
{
	if (int32_t error = WSAStartup(MAKEWORD(2, 2), &wsa)) {
		std::cerr << "WSAStartup failed. Error " << error << ": " << WSAGetLastError() << std::endl;
		return false;
	}

	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (int32_t error = getaddrinfo(server, port, &hints, &servinfo)) {
		std::cerr << "getaddrinfo failed. Error " << error << ": " << WSAGetLastError() << std::endl;
		return false;
	}

	s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo-> ai_protocol);
	if (s == INVALID_SOCKET) {
		std::cerr << "Socket initializiation failed. Error: " << WSAGetLastError() << std::endl;
		return false;
	}

	if (connect(s, servinfo->ai_addr, servinfo->ai_addrlen) == SOCKET_ERROR) {
		std::cerr << "Socket connection failed. Error: " << WSAGetLastError() << std::endl;
		closesocket(s);
		return false;
	}

	freeaddrinfo(servinfo);
	return true;
}
