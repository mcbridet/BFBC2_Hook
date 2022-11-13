#include "pch.hpp"
#include "Hook.hpp"

#include "PatchDNS.hpp"
#include "PatchSSL.hpp"
#include "Proxy.hpp"

DWORD WINAPI HookInit(LPVOID /*lpParameter*/)
{
	Hook::getInstance();
	return TRUE;
}

Hook::Hook()
{
	config = &Config::getInstance();

	PatchDNS* dnsPatch = &PatchDNS::getInstance();
	PatchSSL* sslPatch = &PatchSSL::getInstance();
	
	InitLogging();

	BOOST_LOG_FUNCTION()
	BOOST_LOG_TRIVIAL(info) << "Initializing...";

	VerifyGameVersion();

	if (config->hook->patchDNS)
	{
		if (!dnsPatch->patchDNSResolution())
		{
			MessageBoxA(NULL, "Failed to initialize hook!\nThe game will now close\n\nMore details in hook log.", "Initialization Failure", MB_OK | MB_ICONWARNING);
			ExitProcess(FAILED_TO_PATCH_DNS_RESOLUTION);
		}
	}

	if (config->hook->patchSSL)
	{
		if (!sslPatch->patchSSLVerification())
		{
			MessageBoxA(NULL, "Failed to initialize hook!\nThe game will now close\n\nMore details in hook log.", "Initialization Failure", MB_OK | MB_ICONWARNING);
			ExitProcess(FAILED_TO_PATCH_SSL_CERTIFICATE_VERIFICATION);
		}
	}

	BOOST_LOG_TRIVIAL(info) << "Initialized successfully!";

	if (config->hook->proxyEnable)
		CreateThread(nullptr, 0, ProxyInit, nullptr, 0, nullptr);
}

void Hook::InitLogging()
{
	using namespace std;
	namespace logging = boost::log;
	namespace expr = logging::expressions;

	bool enableConsole = config->hook->showConsole;
	bool enableLogFile = config->hook->createLog;
	int consoleLogLevel = config->hook->consoleLogLevel;
	int fileLogLevel = config->hook->fileLogLevel;

	logging::add_common_attributes();
	logging::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());

	auto logFormat = expr::format("[%1% %2%] %3%: %4%")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
		% expr::format_named_scope("Scope", logging::keywords::format = "%C")
		% logging::trivial::severity
		% expr::smessage;

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
		consoleSink->set_formatter(logFormat);

		ConsoleIntro();
	}

	if (enableLogFile)
	{
		auto fileSink = logging::add_file_log(config->hook->logPath);
		fileSink->set_filter(logging::trivial::severity >= fileLogLevel);
		fileSink->set_formatter(logFormat);
	}
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

void Hook::VerifyGameVersion()
{
	BOOST_LOG_NAMED_SCOPE("GameVersion")

	// "ROMEPC795745" - Client R11
	DWORD mClientVersionAddr = Utils::FindPattern(0x1400000, 0x600000, (BYTE*)"\x22\x52\x4F\x4D\x45\x50\x43\x37\x39\x35\x37\x34\x35\x22", "xxxxxxxxxxxxxx");

	// "ROMEPC851434" - Server R34
	DWORD mServerVersionAddr = Utils::FindPattern(0x1600000, 0x600000, (BYTE*)"\x22\x52\x4F\x4D\x45\x50\x43\x38\x35\x31\x34\x33\x34\x22", "xxxxxxxxxxxxxx");

	if (config->hook->verifyGameVersion && (mClientVersionAddr == NULL && mServerVersionAddr == NULL))
	{
		MessageBoxA(NULL, "Failed to initialize hook!\r\nUnknown client/server detected!\r\nPlease verify the integrity of your files and try again.", "Initialization Failure", MB_OK | MB_ICONWARNING);
		ExitProcess(INVALID_GAME_VERSION);
	}

	std::string exeType;

	if (mClientVersionAddr != NULL)
	{
		exeType = "Client";
		this->exeType = CLIENT;
	}
	else if (mServerVersionAddr != NULL)
	{
		exeType = "Server";
		this->exeType = SERVER;
	}
	else
	{
		exeType = "!!!!!! UNKNOWN !!!!!!";
		this->exeType = UNKNOWN;
	}

	BOOST_LOG_TRIVIAL(info) << "Detected executable type: " << exeType;
}
