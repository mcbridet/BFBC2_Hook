#pragma once

class PatchSSL
{
public:
    PatchSSL();
    PatchSSL(const PatchSSL&) {}

    static PatchSSL& getInstance() {
        static PatchSSL* _instance = nullptr;

        if (_instance == nullptr)
            _instance = new PatchSSL();

        return *_instance;
    }

    bool patchSSLVerification();

private:
    bool enablePatch;

    DWORD dwCodeSize;
    DWORD dwCodeOffset;
    DWORD dwEntryPoint;
};
