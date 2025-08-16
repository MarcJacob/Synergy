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

	// Zero out the provided persistent memory.
	memset(Context.PersistentMemoryBuffer.Memory, 0, Context.PersistentMemoryBuffer.Size);

	ClientSessionState& Client = CastClientState(Context.PersistentMemoryBuffer.Memory);

	// Build Client State Object

	std::cout << "Allocating Main Viewport.\n";
	Client.MainViewport.Dimensions = { 1920, 1080 };
	Client.MainViewport.ID = Context.Platform.AllocateViewport("Synergy Client", Client.MainViewport.Dimensions);

	Client.Input = {};

	Client.PersistentMemoryAllocator = MakeStackAllocator(Context.PersistentMemoryBuffer.Memory + sizeof(ClientSessionState)
		, Context.PersistentMemoryBuffer.Size - sizeof(ClientSessionState));

	Client.bDrawUIDebug = false;

	// Allocate Graph from persistent memory.
	Client.Graph = Client.PersistentMemoryAllocator.Allocate<ClientGraph>();

	// Initialize Graph Node Presentation data.
	for (SNodeGUID repIndex = 0; repIndex < sizeof(Client.NodeRepresentations) / sizeof(GraphNodeRepresentationData); repIndex++)
	{
		Client.NodeRepresentations[repIndex].nodeID = SNODE_INVALID_ID;
	}

	// TEST CODE Build Node Presentation Definition structures.
	Client.UINodePresentations.GenericPanel = UINodePresentationDef_Rectangle { GetColorWithIntensity(COLOR_White, 0.2f), false }; // Grey, non-highlightable.
	Client.UINodePresentations.GraphViewPanel = UINodePresentationDef_Rectangle { COLOR_White, false }; // White, non-highlightable.
	Client.UINodePresentations.GraphNode = UINodePresentationDef_Rectangle { GetColorWithIntensity(COLOR_Red, 0.5f), true }; // Dark red, highlightable.

	// TEST CODE Build a simple graph to test.
	ClientGraphEditTransaction initTransaction;
	initTransaction.TargetGraph = Client.Graph;

	// Root Node
	GraphEditNode* root = initTransaction.CreateNode({ Client.Graph->RootNodeID, SNODE_INVALID_ID, "Root" }, nullptr);

	// Two direct children
	GraphEditNode* child1 = initTransaction.CreateNode({ SNODE_INVALID_ID, SNODE_INVALID_ID, "Child 1" }, root);
	GraphEditNode* child2 = initTransaction.CreateNode({ SNODE_INVALID_ID, SNODE_INVALID_ID, "Child 2" }, root);

	// One grandchild, child of Child 2
	GraphEditNode* child2_1 = initTransaction.CreateNode({ SNODE_INVALID_ID, SNODE_INVALID_ID, "Child 2 - 1" }, child2);

	// Connection from grandchild to child 1.
	initTransaction.AddOrEditConnection(*child2_1, *child1, { SNODE_INVALID_ID, SNODE_INVALID_ID, SNodeConnectionAccessLevel::PUBLIC });

	if (!Client.Graph->ApplyEditTransaction(initTransaction))
	{
		std::cerr << "Error when applying Init Transaction to Client Graph !\n";
		return;
	}

	// TEST CODE Initialize node representation data

	// Let's collect the first 64 nodes, if they exist. Later we'll have ways of tracking which nodes actually exist or not.
	SNodeDef nodeDefsBuffer[64];
	size_t nodeCount = 0;

	for (SNodeGUID nodeID = 0; nodeID < 32; nodeID++)
	{
		SNodeDef def = Client.Graph->GetNodeDef(nodeID);

		if (def.id == SNODE_INVALID_ID)
		{	
			continue;
		}

		nodeDefsBuffer[nodeCount] = def;
		nodeCount++;
	}

	for (SNodeGUID nodeID = 0; nodeID < nodeCount; nodeID++)
	{
		// Create representation data.
		// Place the nodes at random over the view space.
		Client.NodeRepresentations[nodeID] = {
			nodeID,
			Vector2f { (float)(rand() % 400), float(rand() % 400) },
		};

		Client.NodeRepresentations[nodeID].nodeID = nodeID;
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
		UIPartitionTree& tree = frameState.MainViewportUITree;
		tree = {};

		// Allocate 4kB of memory for the purposes of building the UI tree, from the frame memory.
		tree.Memory = MakeStackAllocator((ByteBuffer)frameState.FrameMemoryAllocator.Allocate(4096), 4096);
	}

	// Perform Partition Pass
	BuildFrameUIPartitionTree(clientState, frameState, true); // -> Main Viewport UI Tree ready for collision checks
	
	// First Absolute Position pass before Interaction pass.
	ProcessChildNodesAbsolutePosition_Recursive(*frameState.MainViewportUITree.RootNode);

	// Perform interaction collision checks and determine which nodes, if any, are being interacted with and update their flag consequently.

	// #TEST CODE Just use the mouse cursor. A more complex algorithm can come later, taking into account which viewport the cursor is on and
	// a keyboard-based focus system.

	if (clientState.Input.CursorViewport != VIEWPORT_ERROR_ID)
	{
		UIPartitionNode* InteractedNode = FindNodeAtPosition(frameState.MainViewportUITree, clientState.Input.CursorLocation);
		InteractedNode->bIsInteracted = true;
	}
	
	// Perform Interaction Pass
	BuildFrameUIPartitionTree(clientState, frameState, false); // -> Main Viewport UI Tree ready for drawing. Client state mutated.

	// Second Absolute Position pass after Interaction pass and before Drawing.
	ProcessChildNodesAbsolutePosition_Recursive(*frameState.MainViewportUITree.RootNode);

	// Output draw calls for this frame.
	OutputDrawCalls(clientState, frameState);
}

DLL_EXPORT void ShutdownClient(ClientSessionData& Context)
{
	std::cout << "Shutting down client.\n";
}