// dinput8.cpp: Direct Input 8 Hook
#include "pch.h"
#include "dinput8.h"

DirectInput8Create_t OriginalFunction = nullptr;
HMODULE OriginalLibrary = nullptr;

DINPUT8_API HRESULT DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut,
	LPUNKNOWN punkOuter)
{
	if (OriginalFunction)
		return OriginalFunction(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	return S_FALSE;
}
