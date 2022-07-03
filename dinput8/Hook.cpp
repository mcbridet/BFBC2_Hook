#include "pch.h"
#include "Hook.h"
#include "Utils.h"

DWORD WINAPI HookInit(LPVOID /*lpParameter*/)
{
	Hook::getInstance();
	return TRUE;
}

Hook::Hook()
{
	config = &Config::getInstance();
	
	InitLogging();
	ConsoleIntro();

	BOOST_LOG_FUNCTION()
	BOOST_LOG_NAMED_SCOPE("Init")

	BOOST_LOG_TRIVIAL(info) << "Initializing...";
	// TODO
	BOOST_LOG_TRIVIAL(info) << "Initialized successfully!";
}

void Hook::InitLogging()
{
	using namespace std;
	namespace logging = boost::log;
	namespace expr = logging::expressions;

	bool enableConsole = config->hook->showConsole;
	bool enableLogFile = config->hook->createLog;
	bool consoleLogLevel = config->hook->consoleLogLevel;
	bool fileLogLevel = config->hook->fileLogLevel;

	logging::add_common_attributes();
	logging::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());

	if (enableConsole)
	{
		// Create a console for Debug output
		AllocConsole();

		// Redirect standard error, output to console
		// std::cout, std::clog, std::cerr, std::cin
		FILE* fDummy;

		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);

		cout.clear();
		clog.clear();
		cerr.clear();
		cin.clear();

		// std::wcout, std::wclog, std::wcerr, std::wcin
		HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
		SetStdHandle(STD_ERROR_HANDLE, hConOut);
		SetStdHandle(STD_INPUT_HANDLE, hConIn);

		wcout.clear();
		wclog.clear();
		wcerr.clear();
		wcin.clear();

		auto consoleSink = logging::add_console_log(std::cout);
		consoleSink->set_filter(logging::trivial::severity >= consoleLogLevel);
		consoleSink->set_formatter(expr::format("[%1% %2%] %3%: %4%")
			% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
			% expr::format_named_scope("Scope", logging::keywords::format = "%C")
			% logging::trivial::severity
			% expr::smessage);
	}

	if (enableLogFile)
		logging::add_file_log(config->hook->logPath)->set_filter(logging::trivial::severity >= fileLogLevel);
}


void Hook::ConsoleIntro()
{
	Utils::CenterPrint("Battlefield: Bad Company 2 Hook by Marek Grzyb (@GrzybDev)", "=", true);
	Utils::CenterPrint("Homepage: https://grzyb.dev/project/BFBC2_Hook", " ", false);
	Utils::CenterPrint("Source code: https://github.com/GrzybDev/BFBC2_Hook", " ", false);
	Utils::CenterPrint("Noticed a bug? Fill a bug report here: https://github.com/GrzybDev/BFBC2_Hook/issues", " ", false);
	Utils::CenterPrint("Licensed under GNU Lesser General Public License v3, Contributions of any kind welcome!", " ", true);
	Utils::CenterPrint("", "=", true);
}
