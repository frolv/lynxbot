#pragma once

/* Sends a HTTP GET request to the specifed adress on the specifed host server. */
std::string HTTPGet(const std::string &hostname, const std::string &address);
std::string HTTPPost(const std::string &hostname, const std::string &address, const std::string &type, const std::string &data);