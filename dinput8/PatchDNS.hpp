#pragma once

hostent* __stdcall DNSResolve(const char* hostname);

class PatchDNS
{
public:
    PatchDNS();
    PatchDNS(const PatchDNS&) {}

    static PatchDNS& getInstance() {
        static PatchDNS* _instance = nullptr;

        if (_instance == nullptr)
            _instance = new PatchDNS();

        return *_instance;
    }

    bool patchDNSResolution();
};
