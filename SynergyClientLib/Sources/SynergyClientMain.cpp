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
	ClientSessionState& ClientState = CastClientState(Context.PersistentMemoryBuffer.Memory);

	// Build Client State Object

	ClientState = {};

	std::cout << "Allocating Main Viewport.\n";
	ClientState.MainViewport.ID = Context.Platform.AllocateViewport("Synergy Client", { 800, 600 });
	ClientState.MainViewport.Dimensions = { 800, 600 };

	ClientState.Input = {};

	ClientState.PersistentMemoryAllocator = MakeStackAllocator(Context.PersistentMemoryBuffer.Memory + sizeof(ClientSessionState)
		, Context.PersistentMemoryBuffer.Size - sizeof(ClientSessionState));

	// Until we get a proper UI graphics system going, set Debug UI as enabled by default.
	ClientState.bDrawUIDebug = true;
}

DLL_EXPORT void RunClientFrame(ClientSessionData& Context, ClientFrameRequestData& FrameData)
{
	ClientSessionState& clientState = CastClientState(Context.PersistentMemoryBuffer.Memory);

	// Build Frame State object
	ClientFrameState frameState = {};

	frameState.ActionInputs = &FrameData.ActionInputEvents;
	frameState.FrameTime = FrameData.FrameTime;

	frameState.FrameMemoryAllocator = MakeStackAllocator(FrameData.FrameMemoryBuffer.Memory, FrameData.FrameMemoryBuffer.Size);

	frameState.FramePlatformAPI.NewDrawCall = FrameData.NewDrawCall;

	frameState.CursorLocation = FrameData.CursorLocation;
	frameState.CursorViewport = FrameData.CursorViewport;

	ProcessInputs(clientState, frameState);
	
	// DEBUG INPUTS

	if (clientState.Input.ActionKeyStateIs(ActionKey::KEY_FUNC1, ActionInputState::UP))
	{
		clientState.bDrawUIDebug = !clientState.bDrawUIDebug;
	}

	// UI
	
	// Construct UI Tree and assign it a memory allocator
	{
		ClientUIPartitionTree& tree = frameState.MainViewportUITree;

		tree = {};

		// Allocate 4kB of memory for the purposes of building the UI tree, from the frame memory.
		tree.Memory = MakeStackAllocator((ByteBuffer)frameState.FrameMemoryAllocator.Allocate(4096), 4096);
	}

	// Perform Partition Pass
	BuildFrameUIPartitionTree(clientState, frameState, true); // -> Main Viewport UI Tree ready for collision checks
	
	// Perform interaction collision checks and determine which nodes, if any, are being interacted with and update their flag consequently.

	// #TEST CODE Just use the mouse cursor. A more complex algorithm can come later, taking into account which viewport the cursor is on and
	// a keyboard-based focus system.

	if (clientState.Input.CursorViewport != VIEWPORT_ERROR_ID)
	{
		ClientUIPartitionNode* InteractedNode = FindNodeAtPosition(frameState.MainViewportUITree, clientState.Input.CursorLocation);
		InteractedNode->bIsInteracted = true;
	}
	
	// Perform Interaction Pass
	BuildFrameUIPartitionTree(clientState, frameState, false); // -> Main Viewport UI Tree ready for drawing. Client state mutated.

	// #NOTE (MJ) For now UI does not have a dedicated draw pass, we rely on the debug draw which is performed in OutputDrawCalls.
	// I have yet to design the system for assigning a "draw type" to UI elements in a flexible way.

	// Output draw calls for this frame.
	OutputDrawCalls(clientState, frameState);
}

DLL_EXPORT void ShutdownClient(ClientSessionData& Context)
{
	std::cout << "Shutting down client.\n";
}