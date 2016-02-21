#pragma once

// default libraries
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <sdkddkver.h>
#include <string>
#include <iostream>
#include <regex>
#include <vector>
#include <sstream>
#include <stack>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <time.h>
#include <random>
#include <thread>

// project libraries
#include <json\json.h>
#include <json\json-forwards.h>
#include <ExpressionParser.h>

// project headers
#include "version.h"
#include "Utils.h"
#include "CommandHandler.h"

// library for winsock2
#pragma comment(lib, "ws2_32.lib")
