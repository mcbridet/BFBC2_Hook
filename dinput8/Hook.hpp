﻿#pragma once
#include "Config.hpp"

DWORD WINAPI HookInit(LPVOID lpParameter);

class Hook
{
public:
	Hook();
    Hook(const Hook&) {}

    static Hook& getInstance() {
        static Hook* _instance = nullptr;

        if (_instance == nullptr)
            _instance = new Hook();

        return *_instance;
    }

private:
    Config* config;

	static void ConsoleIntro();
    void InitLogging();
};