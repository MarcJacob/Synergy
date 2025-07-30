#include "SynergyClient.h"
#include "SynergyCore.h"

#include "Public/SynergyClientDrawBuffer.h"

#include <iostream> // TODO instead of using iostream the user of the library should provide
// contextual data so it can use any logging solution it wants.

// EXPORTED SYMBOLS DEFINITION

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
	std::cout << "Running client frame " << FrameData.FrameNumber << "\n\tFrame Time = " << FrameData.FrameTime << "\n";

	if (!FrameData.DrawCallBuffer->BeginWrite())
	{
		return;
	}

	// Add a drawcall for a red rectangle
	RectangleDrawCallData* rect = reinterpret_cast<RectangleDrawCallData*>(FrameData.DrawCallBuffer->NewDrawCall(DrawCallType::RECTANGLE));
	rect->x = 50 + FrameData.FrameNumber * FrameData.FrameTime;
	rect->y = 50;
	rect->width = 100;
	rect->height = 100;
	rect->color.full = 0xFFFF0000;
}

DLL_EXPORT void ShutdownClient(ClientContext& Context)
{
	std::cout << "Shutting down client.\n";
}