#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0502				// Target WinXP SP2
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Disable stupid MS "warnings" a.k.a errors

#define CLIENT_PLASMA_PORT 18390
#define CLIENT_THEATER_PORT 18395
#define SERVER_PLASMA_PORT 18321
#define SERVER_THEATER_PORT 18326
#define HTTP_PORT 8123

#define PACKET_MAX_LENGTH 8192

#define HEADER_VALUE_LENGTH 4
#define CATEGORY_OFFSET 0
#define TYPE_OFFSET   CATEGORY_OFFSET + HEADER_VALUE_LENGTH
#define LENGTH_OFFSET TYPE_OFFSET + HEADER_VALUE_LENGTH
#define HEADER_LENGTH LENGTH_OFFSET + HEADER_VALUE_LENGTH

// Windows Header Files
#include <Windows.h>

// Standard (Visual) C++
#include <string>

// Common hook headers
#include "ExitCode.hpp"
#include "Utils.hpp"
#include "ProxyCert.hpp"
#include "ExecutableType.hpp"
#include "ProxyStopException.hpp"
#include "ProxyType.hpp"
#include "Packet.hpp"
