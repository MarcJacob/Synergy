#include "SynergyClient.h"
#include "SynergyClientDrawing.h"
#include "SynergyClientInput.h"

#include <iostream> // TODO instead of using iostream the user of the library should provide
// contextual data so it can use any logging solution it wants.

// EXPORTED SYMBOLS DEFINITION

#define DLL_EXPORT extern "C" __declspec(dllexport)

enum ActionInputState
{
	RELEASED,
	DOWN,
	HELD,
	UP
};

struct ClientInputState
{
	ActionInputState ActionInputStates[ActionKey::ACTION_KEY_COUNT];
};

/* 
	State of the Client as a whole. Persistent memory pointer provided by the platform is cast to this.
*/
struct ClientState
{
	ViewportID MainViewportID;

	ClientInputState Input;

	// TEST CODE move a rectangle around on the main viewport using input.
	Vector2f PlayerCoordinates;
	float PlayerSpeed;
};

#define CastClientState(MemPtr) (*reinterpret_cast<ClientState*>(MemPtr))

DLL_EXPORT void Hello()
{
	std::cout << "Hello World from Synergy Client Lib !" << "\n";
	std::cout << "VERSION 0.1.1 - IN DEV\n";
}

DLL_EXPORT void StartClient(ClientContext& Context)
{
	std::cout << "Starting client.\n";
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	std::cout << "Allocating Main Viewport.\n";
	State.MainViewportID = Context.Platform.AllocateViewport("Synergy Client", { 800, 600 });

	State.Input = {};

	// TEST CODE Init player coordinates and speed.
	State.PlayerCoordinates = {0, 0};
	State.PlayerSpeed = 20.f;
}

void OutputDrawCalls(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	if (FrameData.NewDrawCall == nullptr)
	{
		// Drawing not supported.
		return;
	}

	// TEST CODE Add a drawcall for a red rectangle
	RectangleDrawCallData* rect = reinterpret_cast<RectangleDrawCallData*>(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	rect->x = 100 + static_cast<uint16_t>(100 * sinf(FrameData.FrameTime * FrameData.FrameNumber / 2.f));
	rect->y = 100;
	rect->width = 10;
	rect->height = 10;
	rect->color.full = 0xFFFF0000;

	// TEST CODE Add a drawcall linking the red rectangle to the top left corner of the viewport with a yellow line.
	LineDrawCallData* line = reinterpret_cast<LineDrawCallData*>(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::LINE));
	line->x = rect->x;
	line->y = rect->y;
	line->destX = 0;
	line->destY = 0;
	line->width = 10;
	line->color.full = 0xFFFFFFFF;

	// TEST CODE Add drawcall for the player as a white rectangle.
	RectangleDrawCallData* player = reinterpret_cast<RectangleDrawCallData*>(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	player->x = static_cast<uint16_t>(State.PlayerCoordinates.x);
	player->y = static_cast<uint16_t>(State.PlayerCoordinates.y);
	player->width = 10;
	player->height = 10;
	player->color.full = 0xFFFFFFFF;
}

void ProcessInputs(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	// "Advance" the state of non-RELEASED inputs.
	for (uint8_t actionKeyIndex = 0; actionKeyIndex < static_cast<uint8_t>(ActionKey::ACTION_KEY_COUNT); actionKeyIndex++)
	{
		switch(State.Input.ActionInputStates[actionKeyIndex])
			{
				case(ActionInputState::UP):
					State.Input.ActionInputStates[actionKeyIndex] = ActionInputState::RELEASED;
					break;
				case(ActionInputState::DOWN):
					State.Input.ActionInputStates[actionKeyIndex] = ActionInputState::HELD;
					break;
				default:
					break;
			}
	}

	// Read in Action inputs.
	for (size_t inputEventIndex = 0; inputEventIndex < FrameData.InputEvents.EventCount; inputEventIndex++)
	{
		ActionInputEvent& event = FrameData.InputEvents.Buffer[inputEventIndex];

		uint8_t keyIndex = static_cast<uint8_t>(event.Key);

		if (!event.bRelease)
		{
			switch(State.Input.ActionInputStates[keyIndex])
			{
				case(ActionInputState::RELEASED):
				case(ActionInputState::UP):
					State.Input.ActionInputStates[keyIndex] = ActionInputState::DOWN;
				default:
					break;
			}
		}
		else
		{
			switch(State.Input.ActionInputStates[keyIndex])
			{
				case(ActionInputState::HELD):
				case(ActionInputState::DOWN):
					State.Input.ActionInputStates[keyIndex] = ActionInputState::UP;
				default:
					break;
			}
		}
	}
}

DLL_EXPORT void RunClientFrame(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	ProcessInputs(Context, FrameData);

	OutputDrawCalls(Context, FrameData);

	// TEST CODE Read in inputs and move the player.
	if (State.Input.ActionInputStates[static_cast<uint8_t>(ActionKey::ARROW_RIGHT)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.x += State.PlayerSpeed * FrameData.FrameTime;
	}
	if (State.Input.ActionInputStates[static_cast<uint8_t>(ActionKey::ARROW_LEFT)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.x -= State.PlayerSpeed * FrameData.FrameTime;
	}
	if (State.Input.ActionInputStates[static_cast<uint8_t>(ActionKey::ARROW_DOWN)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.y += State.PlayerSpeed * FrameData.FrameTime;
	}
	if (State.Input.ActionInputStates[static_cast<uint8_t>(ActionKey::ARROW_UP)] == ActionInputState::HELD)
	{
		State.PlayerCoordinates.y -= State.PlayerSpeed * FrameData.FrameTime;
	}
}

DLL_EXPORT void ShutdownClient(ClientContext& Context)
{
	std::cout << "Shutting down client.\n";
}