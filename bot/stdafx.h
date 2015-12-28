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
#include <map>
#include <fstream>

// project headers
#include "TwitchBot.h"
#include "CommandHandler.h"
#include "Utils.h"
#include "cmdmodules\ExpressionParser.h"
#include "cmdmodules\HTTPTools.h"
#include "cmdmodules\SkillMap.h"

// library for winsock2
#pragma comment(lib, "ws2_32.lib")