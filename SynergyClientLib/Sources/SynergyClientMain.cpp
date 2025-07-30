#include "SynergyClient.h"
#include "SynergyCore.h"

#include <iostream> // TODO instead of using iostream the user of the library should provide
// contextual data so it can use any logging solution it wants.

#define DLL_EXPORT extern "C" __declspec(dllexport)

DLL_EXPORT void Hello()
{
	std::cout << "Hello World from Synergy Client Lib !" << "\n";
	std::cout << "VERSION 0.1 - IN DEV\n";
}

DLL_EXPORT void StartClient(ClientContext& Context)
{
	std::cout << "Starting client.\n";
}

DLL_EXPORT void RunClientFrame(ClientContext& Context, ClientFrameData& FrameData)
{
	std::cout << "Running client frame." << FrameData.FrameNumber << "\n\tFrame Time = " << FrameData.FrameTime << "\n";
}

DLL_EXPORT void ShutdownClient(ClientContext& Context)
{
	std::cout << "Shutting down client.\n";
}