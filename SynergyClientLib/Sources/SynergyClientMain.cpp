#define TRANSLATION_UNIT SYNERGY_CLIENT_MAIN

// Implementation of exported symbols directly used by the Platform & Render layer. Effectively the "Entry points" of the Client library.

#include "Client.h"

#include <iostream>

// Source includes
#include "Graph_INC.cpp"
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
	ClientSessionState& Client = CastClientState(Context.PersistentMemoryBuffer.Memory);

	// Build Client State Object

	Client = {};

	std::cout << "Allocating Main Viewport.\n";
	Client.MainViewport.ID = Context.Platform.AllocateViewport("Synergy Client", { 800, 600 });
	Client.MainViewport.Dimensions = { 800, 600 };

	Client.Input = {};

	Client.PersistentMemoryAllocator = MakeStackAllocator(Context.PersistentMemoryBuffer.Memory + sizeof(ClientSessionState)
		, Context.PersistentMemoryBuffer.Size - sizeof(ClientSessionState));

	// Until we get a proper UI graphics system going, set Debug UI as enabled by default.
	Client.bDrawUIDebug = true;

	// Allocate Graph from persistent memory.
	Client.Graph = Client.PersistentMemoryAllocator.Allocate<ClientGraph>();

	// Build a simple graph to test.
	ClientGraphEditTransaction initTransaction;

	
	// Root Node
	ClientGraphEditTransaction::Node* root = initTransaction.CreateNode({ Client.Graph->RootNodeID, SNODE_INVALID_ID, "Root" }, nullptr);

	// Two direct children
	ClientGraphEditTransaction::Node* child1 = initTransaction.CreateNode({ SNODE_INVALID_ID, SNODE_INVALID_ID, "Child 1" }, root);
	ClientGraphEditTransaction::Node* child2 = initTransaction.CreateNode({ SNODE_INVALID_ID, SNODE_INVALID_ID, "Child 2" }, root);

	// One grandchild, child of Child 2
	ClientGraphEditTransaction::Node* child2_1 = initTransaction.CreateNode({ SNODE_INVALID_ID, SNODE_INVALID_ID, "Childe 2 - 1" }, child2);

	// Monodirectional connection from grandchild to child 1
	initTransaction.AddOrEditConnection(child2_1, child1, { SNODE_INVALID_ID, SNODE_INVALID_ID, SNodeConnectionAccessLevel::PUBLIC });

	if (!Client.Graph->ApplyEditTransaction(initTransaction))
	{
		std::cerr << "Error when applying Init Transaction to Client Graph !\n";
	}
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