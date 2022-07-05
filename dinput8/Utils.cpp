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
