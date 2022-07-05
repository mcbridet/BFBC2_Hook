// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.hpp"
#include "dinput8.hpp"
#include "Hook.hpp"

void LoadSystemLibrary(const char* libraryName)
{
	char libraryPath[MAX_PATH];

	GetSystemDirectoryA(libraryPath, MAX_PATH);
	strcat_s(libraryPath, "\\");
	strcat_s(libraryPath, libraryName);

	OriginalLibrary = LoadLibraryA(libraryPath);

	if (OriginalLibrary > reinterpret_cast<HMODULE>(31))
		OriginalFunction = reinterpret_cast<DirectInput8Create_t>(  // NOLINT(clang-diagnostic-cast-function-type)
			GetProcAddress(OriginalLibrary, "DirectInput8Create"));
}

void UnprotectGameMemory()
{
	HMODULE hModule = GetModuleHandle(NULL);

	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + header->e_lfanew);

	// Unprotect the entire PE image
	SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
	DWORD oldProtect;
	VirtualProtect(hModule, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		// Load system library so game can use APIs as if it loaded it normally
		LoadSystemLibrary("dinput8.dll");

		// Unprotect game memory so we can read and write freely
		UnprotectGameMemory();

		// Launch hook code in new thread
		CreateThread(nullptr, 0, HookInit, nullptr, 0, nullptr);
	}

	return TRUE;
}
