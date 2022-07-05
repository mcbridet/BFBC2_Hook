#pragma once

class Utils
{
public:
    static void CenterPrint(std::string text, const char* fillChar, bool newLine);
    static void PrintChar(const char* charToPrint, unsigned int count);

    static DWORD FindPattern(DWORD dwStart, DWORD dwLen, BYTE* pszPatt, const char pszMask[]);
    static DWORD OffsetToCode(HANDLE hHandle);
    static DWORD GetSizeOfCode(HANDLE hHandle);
};
