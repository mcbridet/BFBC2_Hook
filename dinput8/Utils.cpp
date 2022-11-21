#include "pch.hpp"
#include "Utils.hpp"

void Utils::CenterPrint(std::string text, const char* fillChar, bool newLine)
{
	using namespace std;

	if (!text.empty())
		text = " " + text + " ";

	CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_screen_buffer_info);

	const int columns = console_screen_buffer_info.srWindow.Right - console_screen_buffer_info.srWindow.Left + 1;
	const int padding_size = (columns / 2) - (text.size() / 2);

	PrintChar(fillChar, padding_size);
	cout << text;
	PrintChar(fillChar, padding_size);

	if (newLine)
		cout << endl;
}

void Utils::PrintChar(const char* charToPrint, const unsigned int count)
{
	using namespace std;

	for (unsigned int i = 0; i < count; i++)
		cout << charToPrint;
}

DWORD Utils::FindPattern(DWORD dwStart, DWORD dwLen, BYTE* pszPatt, const char pszMask[])
{
	unsigned int i = NULL;
	int iLen = strlen(pszMask) - 1;

	for (DWORD dwRet = dwStart; dwRet < dwStart + dwLen; dwRet++)
	{
		if (*(BYTE*)dwRet == pszPatt[i] || pszMask[i] == '?')
		{
			if (pszMask[i + 1] == '\0')
				return (dwRet - iLen);

			i++;
		}
		else
			i = NULL;
	}

	return NULL;
}

DWORD Utils::GetSizeOfCode(HANDLE hHandle)
{
	auto hModule = static_cast<HMODULE>(hHandle);

	if (!hModule)
		return NULL;

	auto pDosHeader = PIMAGE_DOS_HEADER(hModule);

	if (!pDosHeader)
		return NULL;

	auto pNTHeader = PIMAGE_NT_HEADERS((LONG)hModule + pDosHeader->e_lfanew);

	if (!pNTHeader)
		return NULL;

	PIMAGE_OPTIONAL_HEADER pOptionalHeader = &pNTHeader->OptionalHeader;

	if (!pOptionalHeader)
		return NULL;

	return pOptionalHeader->SizeOfCode;
}

DWORD Utils::OffsetToCode(HANDLE hHandle)
{
	auto hModule = static_cast<HMODULE>(hHandle);

	if (!hModule)
		return NULL;

	auto pDosHeader = PIMAGE_DOS_HEADER(hModule);

	if (!pDosHeader)
		return NULL;

	auto pNTHeader = PIMAGE_NT_HEADERS((LONG)hModule + pDosHeader->e_lfanew);

	if (!pNTHeader)
		return NULL;

	PIMAGE_OPTIONAL_HEADER pOptionalHeader = &pNTHeader->OptionalHeader;

	if (!pOptionalHeader)
		return NULL;

	return pOptionalHeader->BaseOfCode;
}

UINT Utils::DecodeInt(unsigned char* data, int bytes)
{
	int num, i;

	for (num = i = 0; i < bytes; i++)
		// num |= (data[i] << (i << 3)); // little endian
		num |= (data[i] << ((bytes - 1 - i) << 3)); // big endian

	return num;
}

std::string Utils::GetPacketData(std::string data)
{
	std::string final_str;

	const boost::char_separator<char> token_sep("\n");

	using tokenizer = boost::tokenizer<boost::char_separator<char>>;
	tokenizer tokens(data, token_sep);

	BOOST_FOREACH(std::string const& token, tokens)
	{
		final_str += token;
		final_str += ", ";
	}

	return final_str.substr(0, final_str.size() - 2);
}
