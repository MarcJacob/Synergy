// Contains public-facing declarations for Client API structures.

#ifndef SYNERGY_CLIENT_INCLUDED
#define SYNERGY_CLIENT_INCLUDED

#include <stdint.h>

union ColorRGBA
{
	struct
	{
		uint8_t r, g, b, a;
	};

	uint32_t full;
};

/*
	Type of a draw call. Tells the render layer what kind of drawing to do, and what function to call to retrieve the correct
	data structure from the draw call memory.
*/
enum DrawCallType
{
	INVALID, // Invalid draw call that wasn't correctly initialized from 0ed memory, or a signal that end of buffer was reached.
	RECTANGLE, // Draw pixels with a corner origin and a specific width and height.
	ELLIPSE, // Draw pixels in an ellipse with a specific origin and radius.
	BITMAP,
};

// Base class for all draw call data classes. Contains spatial and visual transform information relevant to all types.
struct DrawCall
{
	DrawCallType

	// Origin coordinates of the draw call to be interpreted differently depending on type.
	uint16_t x, y;

	// Rotation in degrees of the drawn shape.
	uint16_t angleDeg;

	ColorRGBA color;
};

/*
	Data for a Rectangle type draw call. Origin coordinates should be interpreted as top left corner position.
	Angle should be interpreted as Rectangle Width along cosinus, Height along sinus at Angle = 0.
*/
struct RectangleDrawCallData : public DrawCall
{
	// Dimensions of rectangle.
	uint16_t width, height;
};

/*
	Contains all draw calls emitted by the client over a single frame.
*/
struct ClientFrameDrawCallBuffer
{
	/* 
		Allocates room in the buffer to build a drawcall of the given type.
		Returns non-null pointer to the place in memory to build the drawcall in if successful.
	*/
	void* BuildDrawCall(DrawCallType Type);
};

// Generic interface for a memory manager usable by the Client for Persistent and Frame memory.
struct ClientMemoryManager
{
	uint8_t* MemoryPtr = nullptr;
	void* (*Allocate)(size_t Size) = nullptr;
	void (*Free)(void* Ptr) = nullptr;
};

// Collection of platform functions that can be called from Client code.
struct PlatformAPI
{
};

// Persistent context data for a single execution of a client. Effectively acts as the Client's static memory.
struct ClientContext
{
	enum State
	{
		INITIALIZED,
		RUNNING,
		ENDED
	};

	State State;

	// Memory manager for persistent memory allocations whose lifetime is managed by the Client.
	ClientMemoryManager PersistentMemory;
};

// Data associated with a single frame of the Client's execution, during which it should integrate the passage of time, react to inputs
// and output draw calls and audio samples.
struct ClientFrameData
{
	size_t FrameNumber;
	float FrameTime;

	// General-purpose Memory for this specific frame. Anything allocated here should be wiped automatically at the end of the frame by the platform.
	ClientMemoryManager FrameMemory;

	// 
};

// Contains function pointers associated with symbol names for easier symbol loading on the platform and to provide a centralized calling
// site for Platform to Client calls.
struct SynergyClientAPI
{
	// Outputs a Hello message with version info on standard output.
	void (*Hello)() = nullptr;

	// Starts a new client session with the given context. The context should be in INITIALIZED state.
	void (*StartClient)(ClientContext& Context) = nullptr;

	// Runs a single frame on the client session associated with the provided context. The context should be in RUNNING state.
	// Frame data needs to be filled in completely.
	void (*RunClientFrame)(ClientContext& Context, ClientFrameData& FrameData) = nullptr;

	// Shuts down the client cleanly.
	void (*ShutdownClient)(ClientContext& Context) = nullptr;

	// Checks that all essential functions have been successfully loaded.
	bool APISuccessfullyLoaded()
	{
		return Hello != nullptr
			&& StartClient != nullptr
			&& RunClientFrame != nullptr
			&& ShutdownClient != nullptr;
	}
};

#endif