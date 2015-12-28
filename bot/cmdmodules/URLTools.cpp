#include "..\stdafx.h"

std::string HTTPReq(const std::string &hostname, const std::string &address) {
	
	// initialize winsock and open a connection to the host server
	WSADATA wsa;
	if (int32_t error = WSAStartup(MAKEWORD(2, 2), &wsa)) {
		return "Error in WSAStartup. Code: " + error;
	}

	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (int32_t error = getaddrinfo(hostname.c_str(), "80", &hints, &servinfo)) {
		return "Getaddrinfo failed. Error: " + error;
	}

	SOCKET sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sock == INVALID_SOCKET) {
		return "Socket creation failed with error " + WSAGetLastError();
	}

	if (connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return "Socket connection failed with error " + WSAGetLastError();
	}

	freeaddrinfo(servinfo);
	std::clog << "Connection to " << hostname << " successful." << std::endl;

	// send the HTTP request
	std::string httpReq = "GET " + address + " HTTP/1.1\r\nHost: " + hostname + "\r\nConnection: close\r\n\r\n";
	send(sock, httpReq.c_str(), httpReq.length(), 0);

	// recieve the response
	char buf[1000];
	int bytes = recv(sock, buf, 1000, 0);
	if (bytes <= 0) {
		return "Did not recieve any data from the server.";
	}
	buf[bytes] = '\0';

	// clean up
	closesocket(sock);
	WSACleanup();

	return std::string(buf);

}

std::string extractCMLData(const std::string &httpResp, const std::string &rsn) {

	std::regex dataRegex("(\\d+," + rsn + ",[\\d\\.]+,[\\d\\.]+)", std::regex_constants::ECMAScript | std::regex_constants::icase);
	std::smatch match;
	// find the required data
	if (std::regex_search(httpResp.begin(), httpResp.end(), match, dataRegex)) {
	
		std::string data = match[1];
		std::vector<std::string> elems;
		// split data into tokens
		split(data, ',', elems);
		std::string ehp = elems[2];

		if (ehp.find(".") != std::string::npos) {
			// truncate to one decimal place
			ehp = ehp.substr(0, ehp.find(".") + 2);
		}
		return "Name: " + elems[1] + ", Rank: " + elems[0] + ", EHP: " + ehp + " (+" + elems[3] + " this week).";

	}
	else {
		return "Player either does not exist or has not been tracked on CML.";
	}

}