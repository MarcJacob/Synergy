#define TRANSLATION_UNIT SYNERGY_CLIENT_MAIN

// Implementation of exported symbols directly used by the Platform & Render layer. Effectively the "Entry points" of the Client library.

#include "Client.h"

#include <iostream>

// Source includes
#include "Input_INC.cpp"
#include "Drawing_INC.cpp"

// EXPORTED SYMBOLS DEFINITION

#define DLL_EXPORT extern "C" __declspec(dllexport)

#define CastClientState(MemPtr) (*(ClientSessionState*)(MemPtr))

DLL_EXPORT void Hello()
{
	std::cout << "Hello World from Synergy Client Lib !" << "\n";
	std::cout << "VERSION 0.1.1 - IN DEV\n";
}

DLL_EXPORT void StartClient(ClientContext& Context)
{
	std::cout << "Starting client.\n";
	ClientSessionState& State = CastClientState(Context.PersistentMemory.Memory);

	std::cout << "Allocating Main Viewport.\n";
	State.MainViewportID = Context.Platform.AllocateViewport("Synergy Client", { 800, 600 });

	State.Input = {};

	// TEST CODE Init player coordinates and speed.
	State.PlayerCoordinates = {0, 0};
	State.PlayerSpeed = 20.f;

	// TEST CODE Zero out entities memory.
	State.Entities = {};
}

DLL_EXPORT void RunClientFrame(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientSessionState& State = CastClientState(Context.PersistentMemory.Memory);

	ProcessInputs(State, FrameData);

	OutputDrawCalls(State, FrameData);

	// TEST CODE Read in inputs and move the player.
	if (State.Input.ActionInputStates[(uint8_t)(ActionKey::ARROW_RIGHT)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.x += State.PlayerSpeed * FrameData.FrameTime;
	}
	if (State.Input.ActionInputStates[(uint8_t)(ActionKey::ARROW_LEFT)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.x -= State.PlayerSpeed * FrameData.FrameTime;
	}
	if (State.Input.ActionInputStates[(uint8_t)(ActionKey::ARROW_DOWN)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.y += State.PlayerSpeed * FrameData.FrameTime;
	}
	if (State.Input.ActionInputStates[(uint8_t)(ActionKey::ARROW_UP)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.y -= State.PlayerSpeed * FrameData.FrameTime;
	}

	if (State.Input.ActionInputStates[(uint8_t)(ActionKey::KEY_E)] == ActionInputState::UP)
	{
		// Spawn rectangle entity.
		Vector2f location = State.Input.CursorLocation;
		ColorRGBA color = { 0, 0, 255, 255 }; // Red.
		uint8_t size = 10;
		State.Entities.SpawnEntity(location, color, size);
	}
}

DLL_EXPORT void ShutdownClient(ClientContext& Context)
{
	std::cout << "Shutting down client.\n";
}