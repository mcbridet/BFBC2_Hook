#include "pch.hpp"
#include "PatchSSL.hpp"

#include "Config.hpp"

PatchSSL::PatchSSL()
{
	Config* config = &Config::getInstance();
	enablePatch = config->hook->patchSSL;
	retryCount = config->hook->sslPatchRetryCount;

	HANDLE hModule = GetModuleHandle(nullptr);

	dwCodeSize = Utils::GetSizeOfCode(hModule);
	dwCodeOffset = Utils::OffsetToCode(hModule);
	dwEntryPoint = (DWORD)hModule + dwCodeOffset;
}

bool PatchSSL::patchSSLVerification()
{
	BOOST_LOG_NAMED_SCOPE("SSLPatch")

	for (int i = 0; i < retryCount; i++)
	{
		BOOST_LOG_TRIVIAL(info) << boost::format("Patching SSL Certificate Verification... (Attempt %i/%i)") % (i + 1) % retryCount;

		DWORD sSSLPatchAddr = Utils::FindPattern(dwEntryPoint, dwCodeSize, (BYTE*)"\x5E\xB8\x03\x10\x00\x00\x5D\xC3",
			"xxxxxxxx");

		if (sSSLPatchAddr != NULL)
		{
			BOOST_LOG_TRIVIAL(debug) << boost::format("Found SSL Certificate Verification code at %1$#x") % sSSLPatchAddr;

			*(BYTE*)(sSSLPatchAddr + 2) = 0x15;
			*(BYTE*)(sSSLPatchAddr + 3) = 0x00;

			BOOST_LOG_TRIVIAL(info) << "Successfully patched SSL Certificate Verification code!";
			return TRUE;
		}

		BOOST_LOG_TRIVIAL(warning) << "Failed to find SSL Certificate Verification code, backing off for 1 second...";
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	BOOST_LOG_TRIVIAL(error) << "Failed to find SSL Certificate Verification code!";
	BOOST_LOG_TRIVIAL(error) <<
 "Game executable is not compatible with this version of hook. (It either is outdated, compressed or protected in some other way)";
	BOOST_LOG_TRIVIAL(error) <<
 "Cannot continue now, if you want to launch the game but without SSL Certificate Verification please disable SSL Patch in config file.";

	return FALSE;
}
