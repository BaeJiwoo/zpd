#include "ClientApplication.hpp"
#include "ConsoleExit.hpp"

int main(int argc, char* argv[])
{
    const int exitCode = RunPacketClient(argc, argv);
    WaitForConsoleExit("Client", exitCode);
    return exitCode;
}
