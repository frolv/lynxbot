#pragma once

/* Sends a HTTP GET request to the specifed adress on the specifed host server. */
std::string HTTPReq(const std::string &hostname, const std::string &address);
/* Extracts EHP data from an HTTP request response from the CML API. */
std::string extractCMLData(const std::string &httpResp, const std::string &rsn);