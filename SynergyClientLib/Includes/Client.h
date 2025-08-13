// Contains symbols that are shared and implemented over multiple files in the Client code, for its major functionality.
// Can only be included in the main Client Translation Unit.

#if (TRANSLATION_UNIT != SYNERGY_CLIENT_MAIN)
static_assert(0, "Client.h can only be included inside the SYNERGY_CLIENT_MAIN translation unit ! Found it with " __BASE_FILE__);
#endif

#ifndef CLIENT_INCLUDED
#define CLIENT_INCLUDED

#include "SynergyClientAPI.h"
#include "SynergyCore.h"

#include "ClientDrawing.h"
#include "ClientUI.h"

/*
	State of the Client as a whole. Persistent memory pointer provided by the platform is cast to this.
*/
struct ClientSessionState
{
	struct
	{
		ViewportID ID;
		Vector2s Dimensions;
	} MainViewport;

	ClientInputState Input;

	MemoryAllocator PersistentMemoryAllocator;

	// Backend state data
	bool bButtonEnlarged = false; // Whether the button at the center of the screen was enlarged.

	// DEBUG DATA
	bool bDrawUIDebug = false;
};

/*
	State of a Frame being run on the client. Frame memory pointer provided by the platform is cast to this,
	and is further constructed by the rest of the Frame request data and mutated by client subsystems over the frame.
*/
struct ClientFrameState
{
	float FrameTime;

	// Action input events from before the frame started.
	const ActionInputEventBuffer* ActionInputs;

	MemoryAllocator FrameMemoryAllocator;

	struct
	{
		DrawCall* (*NewDrawCall)(ViewportID TargetViewportID, DrawCallType Type);
	} FramePlatformAPI;

	// Location of the cursor when the frame started.
    Vector2s CursorLocation;

    // Viewport the cursor was in when the frame started.
    ViewportID CursorViewport;

	// Intermediate data generated and used by the frame, for the frame.
	
	// UI Partition Tree for drawing the Main Viewport's UI.
	ClientUIPartitionTree MainViewportUITree;
};

// MAJOR PROCEDURES

/*
	Processes the various input buffers sent by the platform layer to update the Client Input State.
*/
void ProcessInputs(ClientSessionState& State, ClientFrameState& FrameData);

/*
	Builds the UI Partition Tree for the Main Viewport, defining every UI element and their logic.
*/
void BuildFrameUIPartitionTree(ClientSessionState& Client, ClientFrameState& Frame, const bool bIsPartitionPass);

/*
	Generates all draw calls to render the end state of a frame. Includes UI and various dynamic elements.
*/
void OutputDrawCalls(ClientSessionState& State, ClientFrameState& FrameData);

#endif // CLIENT_INCLUDED