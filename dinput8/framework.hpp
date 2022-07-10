#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0502				// Target WinXP SP2
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Disable stupid MS "warnings" a.k.a errors

#define CLIENT_FESL_PORT 18390
#define CLIENT_THEATER_PORT 18395
#define SERVER_FESL_PORT 19021
#define SERVER_THEATER_PORT 19026

#define PACKET_MAX_LENGTH 8192
#define HEADER_LENGTH 4

// Windows Header Files
#include <Windows.h>

// Standard (Visual) C++
#include <string>

// Common hook headers
#include "ExitCode.hpp"
#include "Utils.hpp"
