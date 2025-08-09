// Contains symbols and inline functions for the Client's UI system.

#include "SynergyCore.h"

// A node in the UI partition tree. End product of the Partition Pass.
struct ClientUIPartitionNode
{
	// Parent node.
	ClientUIPartitionNode* Parent = nullptr;

	// Child nodes.
	ClientUIPartitionNode* Children = nullptr;
	// Number of child nodes.
	size_t ChildCount = 0;

	// Position on screen relative to parent. Corresponds to top left corner of rectangle bounds.
	Vector2s RelativePosition = {};

	// Dimensions of UI element rectangle bounds in screen units.
	Vector2s Dimensions = {};

	// Whether this element is being interacted with. Can only be true during the interaction pass.
	bool bIsInteracted = false;
};

// Wrapper for the Root node of a UI Partition Tree. Effectively contains the overall state of the UI.
struct ClientUIPartitionTree
{
	// Root Node of the tree. Its position is relative to the top left corner of whatever viewport it is part of.
	ClientUIPartitionNode* RootNode;

	// Allocator for the working memory assigned to the UI Tree. Child elements should allocate memory from it instead of the frame memory.
	MemoryAllocator Memory;
};

/*
	Goes down the tree and determines which element has been hit at the interaction position.
*/
ClientUIPartitionNode* FindNodeAtPosition(ClientUIPartitionTree Tree, Vector2s ViewportPosition);