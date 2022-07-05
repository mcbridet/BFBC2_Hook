// dinput8.h: Header for Direct Input 8 Hook
#pragma once
#include "pch.hpp"

#define DINPUT8_API __declspec(dllexport)

using DirectInput8Create_t = HRESULT(*)(
	HINSTANCE hinst,
	DWORD dwVersion,
	REFIID riidltf,
	LPVOID* ppvOut,
	LPUNKNOWN punkOuter
	);

extern DirectInput8Create_t OriginalFunction;
extern HMODULE OriginalLibrary;

extern "C" {
	DINPUT8_API HRESULT DirectInput8Create(
		HINSTANCE hinst,
		DWORD dwVersion,
		REFIID riidltf,
		LPVOID* ppvOut,
		LPUNKNOWN punkOuter
	);
}
