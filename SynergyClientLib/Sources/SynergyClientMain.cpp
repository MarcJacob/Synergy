#define TRANSLATION_UNIT SYNERGY_CLIENT_MAIN

// Implementation of exported symbols directly used by the Platform & Render layer. Effectively the "Entry points" of the Client library.

#include "Client.h"

#include <iostream>

// Source includes
#include "Input_INC.cpp"
#include "UI_INC.cpp"
#include "Drawing_INC.cpp"

// EXPORTED SYMBOLS DEFINITION

#define DLL_EXPORT extern "C" __declspec(dllexport)

#define CastClientState(MemPtr) (*(ClientSessionState*)(MemPtr))

DLL_EXPORT void Hello()
{
	std::cout << "Hello World from Synergy Client Lib !" << "\n";
	std::cout << "VERSION 0.1.1 - IN DEV\n";
}

DLL_EXPORT void StartClient(ClientSessionData& Context)
{
	std::cout << "Starting client.\n";
	ClientSessionState& State = CastClientState(Context.PersistentMemoryBuffer.Memory);

	std::cout << "Allocating Main Viewport.\n";
	State.MainViewportID = Context.Platform.AllocateViewport("Synergy Client", { 800, 600 });

	State.Input = {};
}

DLL_EXPORT void RunClientFrame(ClientSessionData& Context, ClientFrameRequestData& FrameData)
{
	ClientSessionState& State = CastClientState(Context.PersistentMemoryBuffer.Memory);

	ProcessInputs(State, FrameData);
	OutputDrawCalls(State, FrameData);
}

DLL_EXPORT void ShutdownClient(ClientSessionData& Context)
{
	std::cout << "Shutting down client.\n";
}