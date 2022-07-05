#include "pch.hpp"
#include "PatchSSL.hpp"

#include "Config.hpp"

PatchSSL::PatchSSL()
{
	Config* config = &Config::getInstance();
	enablePatch = config->hook->patchSSL;

	HANDLE hModule = GetModuleHandle(NULL);

	dwCodeSize = Utils::GetSizeOfCode(hModule);
	dwCodeOffset = Utils::OffsetToCode(hModule);
	dwEntryPoint = (DWORD)hModule + dwCodeOffset;
}

bool PatchSSL::patchSSLVerification()
{
	BOOST_LOG_NAMED_SCOPE("SSLPatch")

	BOOST_LOG_TRIVIAL(info) << "Patching SSL Certificate Verification...";

	DWORD sSSLPatchAddr = Utils::FindPattern(dwEntryPoint, dwCodeSize, (BYTE*)"\x5E\xB8\x03\x10\x00\x00\x5D\xC3", "xxxxxxxx");

	if (sSSLPatchAddr != NULL)
	{
		BOOST_LOG_TRIVIAL(debug) << "Found SSL Certificate Verification code at " << std::hex << std::uppercase << sSSLPatchAddr;

		*(BYTE*)(sSSLPatchAddr + 2) = 0x15;
		*(BYTE*)(sSSLPatchAddr + 3) = 0x00;

		BOOST_LOG_TRIVIAL(info) << "Successfully patched SSL Certificate Verification code!";
		return TRUE;
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to find SSL Certificate Verification code!";
		BOOST_LOG_TRIVIAL(error) << "Game executable is not compatible with this version of hook. (It either is outdated, compressed or protected in some other way)";
		BOOST_LOG_TRIVIAL(error) << "Cannot continue now, if you want to launch the game but without SSL Certificate Verification please disable SSL Patch in config file.";

		return FALSE;
	}
}
