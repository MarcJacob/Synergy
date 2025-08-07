SOURCE_INC_FILE()

// Implementation for the Client drawing system, responsible for outputting draw calls for a given frame.

#include "Client.h"

void OutputDrawCalls(ClientSessionState& State, ClientFrameData& FrameData)
{
	if (FrameData.NewDrawCall == nullptr)
	{
		// Drawing not supported.
		return;
	}

	// TEST CODE Add a drawcall for a red rectangle
	RectangleDrawCallData* rect = (RectangleDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	rect->origin = { 100 + (int16_t)(100 * sinf(FrameData.FrameTime * FrameData.FrameNumber / 2.f)), 100 };
	rect->dimensions = { 10, 10 };
	rect->color.full = 0xFFFF0000;

	// TEST CODE Add a drawcall linking the red rectangle to the top left corner of the viewport with a yellow line.
	LineDrawCallData* line = (LineDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::LINE));
	line->origin = rect->origin;
	line->destination = { 0, 0 };
	line->width = 10;
	line->color.full = 0xFFFF00FF;

	// TEST CODE Add drawcall for the player as a white rectangle.
	RectangleDrawCallData* playerRect = (RectangleDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	playerRect->origin = State.PlayerCoordinates;
	playerRect->dimensions = { 10, 10 };
	playerRect->color.full = 0xFFFFFFFF;

	// TEST CODE draw test entities.
	for (size_t entityIndex = 0; entityIndex < State.Entities.EntityCount; entityIndex++)
	{
		TestEntity& entity = State.Entities.Buffer[entityIndex];

		RectangleDrawCallData* entityRect = (RectangleDrawCallData*)(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
		entityRect->origin = entity.Location;
		entityRect->dimensions = { entity.Size, entity.Size };
		entityRect->color.full = entity.Color.full;
	}
}