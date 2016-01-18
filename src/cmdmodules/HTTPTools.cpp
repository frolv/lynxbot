#include "..\stdafx.h"

std::string HTTPGet(const std::string &hostname, const std::string &address) {
	
	SOCKET sock;
	WSADATA wsa;

	if (!utils::socketConnect(sock, wsa, "80", hostname.c_str())) {
		return "";
	}

	std::clog << "Connection to " << hostname << " successful." << std::endl;

	// send the HTTP request
	std::string httpReq = "GET " + address + " HTTP/1.1\r\nHost: " + hostname + "\r\nConnection: close\r\n\r\n";
	send(sock, httpReq.c_str(), httpReq.length(), 0);
	
	// recieve the response
	char buf[1000];
	uint16_t bytes = recv(sock, buf, 1000, 0);

	// hacky workaround for now - will eventually come back to this
	if (hostname == "services.runescape.com") {
		bytes = recv(sock, buf, 1000, 0);
	}

	if (bytes == 0) {
		return "Did not recieve any data from the server.";
	}
	// make sure string is null terminated
	buf[bytes] = '\0';

	// clean up
	closesocket(sock);
	WSACleanup();

	return std::string(buf);

}

std::string HTTPPost(const std::string &hostname, const std::string &address, const std::string &type, const std::string &data) {

	SOCKET sock;
	WSADATA wsa;

	if (!utils::socketConnect(sock, wsa, "80", hostname.c_str())) {
		return "";
	}

	std::clog << "Connection to " << hostname << " successful." << std::endl;

	// send the HTTP request
	std::string httpReq = "POST " + address + " HTTP/1.1\r\nHost: " + hostname + "\r\nContent-Type: "
		+ type + "\r\nContent-Length: " + std::to_string(data.length()) + "\r\n\r\n" + data;
	send(sock, httpReq.c_str(), httpReq.length(), 0);

	// recieve the response
	char buf[1000];
	uint16_t bytes = recv(sock, buf, 1000, 0);

	if (bytes == 0) {
		return "Did not recieve any data from the server.";
	}
	// make sure string is null terminated
	buf[bytes] = '\0';

	// clean up
	closesocket(sock);
	WSACleanup();

	return std::string(buf);

}