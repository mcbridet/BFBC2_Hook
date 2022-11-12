// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.hpp"

// Standard (Visual) C++
#include <Unknwn.h>
#include <tchar.h>
#include <iostream>

// Boost
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

// Detours
#include <detours.h>
#pragma comment(lib, "detours.lib")

// WinSock2
#include "WinSock2.h"
#pragma comment(lib, "ws2_32.lib")

// OpenSSL
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")

// Cpprestsdk
#include <cpprest/ws_client.h>
#include <cpprest/http_client.h>
#pragma comment(lib, "cpprest143_2_10.lib")

#endif //PCH_H
