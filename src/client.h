#pragma once

#include <iostream>
#include <string>

#ifdef _WIN32
# include <Winsock2.h>
#endif

class Client {
	public:
		Client(const char *serv, const char *port);
		~Client();
		bool cconnect();
		void cdisconnect();
		int32_t cwrite(const char *msg);
		int32_t cread(char *buf, size_t sz);
	private:
		const char *m_serv, *m_port;
		bool m_connected;
#ifdef __linux__
		int m_fd;
		bool connect_unix();
#endif
#ifdef _WIN32
		SOCKET m_socket;
		WSADATA m_wsa;
		bool connect_win();
#endif
};
