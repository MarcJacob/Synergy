#include "SynergyClient.h"

#include "Public/SynergyClientDrawing.h"

#include <iostream> // TODO instead of using iostream the user of the library should provide
// contextual data so it can use any logging solution it wants.

// EXPORTED SYMBOLS DEFINITION

#define DLL_EXPORT extern "C" __declspec(dllexport)

/* 
	State of the Client as a whole. Persistent memory pointer provided by the platform is cast to this.
*/
struct ClientState
{
	ViewportID MainViewportID;
};

#define CastClientState(MemPtr) (*reinterpret_cast<ClientState*>(MemPtr))

DLL_EXPORT void Hello()
{
	std::cout << "Hello World from Synergy Client Lib !" << "\n";
	std::cout << "VERSION 0.1 - IN DEV\n";
}

DLL_EXPORT void StartClient(ClientContext& Context)
{
	std::cout << "Starting client.\nAllocating Main Viewport.\n";
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	State.MainViewportID = Context.Platform.AllocateViewport("Synergy Client", { 800, 600 });
}

void OutputDrawCalls(ClientContext& Context, ClientFrameData& FrameData)
{
	ClientState& State = CastClientState(Context.PersistentMemory.Memory);

	if (FrameData.NewDrawCall == nullptr)
	{
		// Drawing not supported.
		return;
	}

	// Add a drawcall for a red rectangle
	RectangleDrawCallData* rect = reinterpret_cast<RectangleDrawCallData*>(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::RECTANGLE));
	rect->x = 100 + static_cast<uint16_t>(100 * sinf(FrameData.FrameTime * FrameData.FrameNumber / 2.f));
	rect->y = 100;
	rect->width = 10;
	rect->height = 10;
	rect->color.full = 0xFFFF0000;

	// Add a drawcall linking the red rectangle to the top left corner of the viewport with a yellow line.
	LineDrawCallData* line = reinterpret_cast<LineDrawCallData*>(FrameData.NewDrawCall(State.MainViewportID, DrawCallType::LINE));
	line->x = rect->x;
	line->y = rect->y;
	line->destX = 0;
	line->destY = 0;
	line->width = 10;
	line->color.full = 0xFFFFFF00;
}

DLL_EXPORT void RunClientFrame(ClientContext& Context, ClientFrameData& FrameData)
{
	OutputDrawCalls(Context, FrameData);
}

DLL_EXPORT void ShutdownClient(ClientContext& Context)
{
	std::cout << "Shutting down client.\n";
}