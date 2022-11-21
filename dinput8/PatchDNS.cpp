#include "pch.hpp"
#include "PatchDNS.hpp"

extern "C" {
hostent* (WINAPI __stdcall* OriginalGethostbyname)(const char* hostname) = gethostbyname;
}

hostent* __stdcall DNSResolve(const char* hostname)
{
	BOOST_LOG_FUNCTION()
	BOOST_LOG_TRIVIAL(debug) << hostname << " -> localhost";
	return OriginalGethostbyname("localhost");
}

PatchDNS::PatchDNS()
{
}

bool PatchDNS::patchDNSResolution()
{
	BOOST_LOG_NAMED_SCOPE("DNSPatch")
	BOOST_LOG_TRIVIAL(info) << "Patching DNS Resolution...";

	const auto winsock2 = GetModuleHandleA("ws2_32.dll");
	OriginalGethostbyname = reinterpret_cast<hostent * (__stdcall*)(const char*)>(GetProcAddress(
		winsock2, "gethostbyname"));

	// Attach detour for gethostbyname
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&reinterpret_cast<PVOID&>(OriginalGethostbyname), DNSResolve);

	LONG commitStatus = DetourTransactionCommit();

	if (commitStatus != NO_ERROR)
	{
		/* Cleanup COM and Winsock */
		WSACleanup();

		BOOST_LOG_TRIVIAL(error) << "Failed to patch DNS Resolution!";
		BOOST_LOG_TRIVIAL(error) << "Detour library returned " << commitStatus <<
 " error code while transacting commit.";
		BOOST_LOG_TRIVIAL(error) << "Could not change gethostbyname function from ws2_32.dll to custom hook function.";
		return FALSE;
	}

	BOOST_LOG_TRIVIAL(info) << "Successfully patched DNS Resolution...";
	return TRUE;
}
