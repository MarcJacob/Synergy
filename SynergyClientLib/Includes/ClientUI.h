// Contains symbols and inline functions for the Client's UI system.

#include "SynergyCore.h"

struct UINodePresentationDef;

// A node in the UI partition tree. End product of the Partition Pass.
struct UIPartitionNode
{
	// Parent node.
	UIPartitionNode* Parent = nullptr;

	// Child nodes.
	UIPartitionNode* Children = nullptr;
	// Number of child nodes.
	size_t ChildCount = 0;

	// Position on screen relative to parent. Corresponds to top left corner of rectangle bounds.
	Vector2s RelativePosition = {};

	// Position on screen relative to Viewport's top left corner. Corresponds to top left corner of rectangle bounds.
	// Only available after Partition & Interaction passes.
	Vector2s AbsolutePosition = {};

	// Dimensions of UI element rectangle bounds in screen units.
	Vector2s Dimensions = {};

	// Whether this element is being interacted with. Can only be true during the interaction pass.
	bool bIsInteracted = false;

	// Pointer to a Presentation Definition for this node. If assigned will be used to output draw calls
	// to represent the node's state.
	UINodePresentationDef* PresentationDef = nullptr;
};

// Wrapper for the Root node of a UI Partition Tree. Effectively contains the overall state of the UI.
struct UIPartitionTree
{
	// Root Node of the tree. Its position is relative to the top left corner of whatever viewport it is part of.
	UIPartitionNode* RootNode;

	// Allocator for the working memory assigned to the UI Tree. Child elements should allocate memory from it instead of the frame memory.
	MemoryAllocator Memory;
};

/*
	Goes down the tree and determines which element has been hit at the interaction position.
*/
UIPartitionNode* FindNodeAtPosition(UIPartitionTree Tree, Vector2s ViewportPosition);

/*
	Updates the absolute position of the passed node's children.
	Gets called recursively on children. 
	The passed node's absolute position is taken as a "base" and is left unchanged.
*/
void ProcessChildNodesAbsolutePosition_Recursive(UIPartitionNode& Node)
{
	// Set absolute position of children from our absolute position and their relative position.
	for(size_t childNodeIndex = 0; childNodeIndex < Node.ChildCount; childNodeIndex++)
	{
		Node.Children[childNodeIndex].AbsolutePosition = Node.AbsolutePosition + Node.Children[childNodeIndex].RelativePosition;
		ProcessChildNodesAbsolutePosition_Recursive(Node.Children[childNodeIndex]);
	}
}