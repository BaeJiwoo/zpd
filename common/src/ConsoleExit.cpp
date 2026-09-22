#include "ConsoleExit.hpp"
#include <Windows.h>
#include <iostream>

void WaitForConsoleExit(const char* applicationName, int exitCode)
{
    std::cout << '\n' << applicationName << " stopped (exit code " << exitCode << ").\n";

    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(input, &mode))
        return;

    std::cout << "Press Enter to exit..." << std::flush;
    INPUT_RECORD record{};
    DWORD read = 0;
    while (ReadConsoleInputW(input, &record, 1, &read) && read != 0) {
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown &&
            record.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
            std::cout << '\n';
            break;
        }
    }
}
