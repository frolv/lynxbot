#pragma once

/* Sends a HTTP GET request to the specifed adress on the specifed host server. */
std::string HTTPReq(const std::string &hostname, const std::string &address);