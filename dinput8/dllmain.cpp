// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "dinput8.h"

void LoadSystemLibrary(const char* libraryName)
{
    char libraryPath[MAX_PATH];

	GetSystemDirectoryA(libraryPath, MAX_PATH);
    strcat_s(libraryPath, "\\");
    strcat_s(libraryPath, libraryName);

    OriginalLibrary = LoadLibraryA(libraryPath);

    if (OriginalLibrary > reinterpret_cast<HMODULE>(31))
        OriginalFunction = reinterpret_cast<DirectInput8Create_t>(GetProcAddress(OriginalLibrary, "DirectInput8Create"));
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
	    // Load system library so game can use APIs as if it loaded it normally
        LoadSystemLibrary("dinput8.dll");
    }

    return TRUE;
}

