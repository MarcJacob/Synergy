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

	// Latest recorded cursor location relative to cursor viewport. TODO Associate to specific action events for greater precision.
	Vector2s CursorLocation;

	// Latest recorded viewport hovered by cursor. TODO: Same as CursorLocation.
	ViewportID CursorViewport;
};

// TEST CODE Entity data structure for spawning and displaying dynamically spawned entities.
struct TestEntity
{
	Vector2f Location;
	ColorRGBA Color;
	uint8_t Size;
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

	// TEST CODE Entity buffer.
	struct
	{
		static constexpr uint16_t BUFFER_SIZE = 256;

		void SpawnEntity(Vector2f Location, ColorRGBA Color, uint8_t Size)
		{
			if (EntityCount >= BUFFER_SIZE)
			{
				std::cout << "Cannot spawn entity: Buffer max size reached.\n";
				return;
			}

			TestEntity& newEntity = Buffer[EntityCount];
			newEntity.Location = Location;
			newEntity.Color = Color;
			newEntity.Size = Size;

			EntityCount++;
		}

		TestEntity Buffer[BUFFER_SIZE];
		size_t EntityCount;

	} Entities;
};

#define CastClientState(MemPtr) (*(ClientState*)(MemPtr))

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

	// TEST CODE Zero out entities memory.
	State.Entities = {};
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
	RectangleDrawCallData* rect = (RectangleDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	rect->x = 100 + (uint16_t)(100 * sinf(FrameData.FrameTime * FrameData.FrameNumber / 2.f));
	rect->y = 100;
	rect->width = 10;
	rect->height = 10;
	rect->color.full = 0xFFFF0000;

	// TEST CODE Add a drawcall linking the red rectangle to the top left corner of the viewport with a yellow line.
	LineDrawCallData* line = (LineDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::LINE));
	line->x = rect->x;
	line->y = rect->y;
	line->destX = 0;
	line->destY = 0;
	line->width = 10;
	line->color.full = 0xFFFF00FF;

	// TEST CODE Add drawcall for the player as a white rectangle.
	RectangleDrawCallData* player = (RectangleDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	player->x = (uint16_t)(State.PlayerCoordinates.x);
	player->y = (uint16_t)(State.PlayerCoordinates.y);
	player->width = 10;
	player->height = 10;
	player->color.full = 0xFFFFFFFF;

	// TEST CODE draw test entities.
	for (size_t entityIndex = 0; entityIndex < State.Entities.EntityCount; entityIndex++)
	{
		TestEntity& entity = State.Entities.Buffer[entityIndex];
		
		RectangleDrawCallData* entityRect = (RectangleDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
		entityRect->x = entity.Location.x;
		entityRect->y = entity.Location.y;
		entityRect->width = entity.Size;
		entityRect->height = entity.Size;
		entityRect->color.full = entity.Color.full;
	}
}

void ProcessInputs(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	// "Advance" the state of non-RELEASED inputs.
	for (uint8_t actionKeyIndex = 0; actionKeyIndex < (uint8_t)(ActionKey::ACTION_KEY_COUNT); actionKeyIndex++)
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

		uint8_t keyIndex = (uint8_t)(event.key);

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

		if (inputEventIndex == FrameData.InputEvents.EventCount - 1)
		{
			// Last event gives us the cursor position and viewport ofor this frame.
			// TODO Change input system so each event's data is fully recorded.

			State.Input.CursorLocation = event.cursorLocation;
			State.Input.CursorViewport = event.viewport;
		}
	}
}

DLL_EXPORT void RunClientFrame(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	ProcessInputs(Context, FrameData);

	OutputDrawCalls(Context, FrameData);

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
		ColorRGBA color = { 255, 0, 0, 255 }; // Red. TODO Let's define some readable color values.
		uint8_t size = 10;
		State.Entities.SpawnEntity(location, color, size);
	}
}

DLL_EXPORT void ShutdownClient(ClientContext& Context)
{
	std::cout << "Shutting down client.\n";
}