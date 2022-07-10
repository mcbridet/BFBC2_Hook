#include "pch.hpp"
#include "Config.hpp"

Config::Config()
{
	readConfig();
}

void Config::readConfig() const
{
	// Create an empty property tree object
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json("grzybdev.conf", pt);
	}
	catch (boost::property_tree::json_parser_error)
	{
		// Do nothing in case of file read error
		// Will use defaults
	}

	parseConfig(pt);
}

void Config::parseConfig(boost::property_tree::ptree pt) const
{
	// Section - Debug
	hook->showConsole = pt.get("debug.showConsole", true);
	hook->createLog = pt.get("debug.createLog", false);

	boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%Y-%m-%d_%H-%M-%S");

	std::ostringstream is;
	is.imbue(std::locale(is.getloc(), facet));
	is << timeLocal;

	hook->logPath = pt.get("debug.logPath", (boost::format("bfbc2_%1%.log") % is.str()).str());
	hook->consoleLogLevel = pt.get("debug.logLevelConsole", boost::log::trivial::info);
	hook->fileLogLevel = pt.get("debug.logLevelFile", boost::log::trivial::debug);

	// Section - Patches
	hook->verifyGameVersion = pt.get("patches.verifyGameVersion", true);
	hook->patchDNS = pt.get("patches.patchDNS", true);
	hook->patchSSL = pt.get("patches.patchSSL", true);

	// Section - Proxy
	hook->proxyEnable = pt.get("proxy.enable", true);
	hook->connectRetail = pt.get("proxy.connectToRetail", false);
}
