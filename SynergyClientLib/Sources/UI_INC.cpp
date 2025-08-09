SOURCE_INC_FILE()

// Implementation for the Client UI system, responsible for the Logic pass of the UI and generating the UI Partition Tree for a frame.
// NOTE: Some stuff related to the Drawing pass of UI is also defined here but to explore that specific system one should start from the Drawing code.

#include "Client.h"

ClientUIPartitionNode *FindNodeAtPosition(ClientUIPartitionTree Tree, Vector2s ViewportPosition)
{
	// Go down the tree level by level, checking the Viewport Position against the bounding rectangle of each element.
	// Stop when the hit element has no children or none that are hit.

	ClientUIPartitionNode* hitNode = Tree.RootNode;
	while(hitNode->ChildCount > 0)
	{
		// Test every child sequentially until one is hit. At that point, set it as the new hit node and loop.
		bool bChildHit = false;
		for(int childIndex = 0; childIndex < hitNode->ChildCount; childIndex++)
		{
			ClientUIPartitionNode& child = hitNode->Children[childIndex];
			Vector2s rectStart = child.RelativePosition;
			Vector2s rectEnd = child.RelativePosition + child.Dimensions;

			if (ViewportPosition.x >= rectStart.x && ViewportPosition.x <= rectEnd.x
			&& ViewportPosition.y >= rectStart.y && ViewportPosition.y <= rectEnd.y)
			{
				hitNode = &child;
				bChildHit = true;
				break;
			}
		}
		
		if (!bChildHit)
		{
			break;
		}
	}

    return hitNode;
}

/*
	Double-use function performing, in this order, the Partition pass followed by the Interaction pass.
	Partition pass is in charge of initially building the UI tree. By the end of the pass, every UI element must have a position and dimensions
	assigned relative to the entire viewport.

	Interaction pass happens after processing which node is currently being in focus / interaction (usually meaning the mouse cursor is on top of it),
	giving a chance for the UI to mutate itself and the overall Client state before the frame gets drawn.

	It might have been possible to make it single-pass, but I wasn't satisfied with the idea of using the previous frame's UI collision test results.
	In a way this is more "immediate" to me than what the traditional single-pass approach to immediate mode UI is.
*/
void BuildFrameUIPartitionTree(ClientSessionState& Client, ClientFrameState& Frame, const bool bIsPartitionPass)
{
	ClientUIPartitionTree& Tree = Frame.MainViewportUITree;
	
	// Helper function for allocating new nodes from this tree and automatically setting up the parent - child relationships.
	auto AllocChildren = [&Tree, bIsPartitionPass](ClientUIPartitionNode& Parent, size_t Count = 1)
	{
		if (!bIsPartitionPass) return; // Do nothing if not in partition pass.

		// ASSERT FATAL IF PARENT ALREADY HAS CHILDREN (Can't grow the children buffer dynamically. If you need a dynamic number of children, determine their count in advance.)
		Parent.Children = Tree.Memory.Allocate<ClientUIPartitionNode>(Count);
		Parent.ChildCount = Count;
		for (size_t childIndex = 0; childIndex < Count; childIndex++)
		{
			Parent.Children[childIndex].Parent = &Parent;
		}
	};

	// Separate boolean for readability.
	// #NOTE: You can also directly use the bIsInteracted member of a node since it can only be true during the interaction pass.
	const bool bIsInteractionPass = !bIsPartitionPass;

	// ROOT NODE
	{
		if (bIsPartitionPass)
		{
			// Create root node as a single viewport-spanning node.
			Tree.RootNode = Tree.Memory.Allocate<ClientUIPartitionNode>(); 	// Lacking a parent, the helper function can't be used. 
																			// This probably has some sort of deeper meaning I didn't intend.
			Tree.RootNode->RelativePosition = {};
			Tree.RootNode->Dimensions = Client.MainViewport.Dimensions;
		}
		ClientUIPartitionNode& root = *Tree.RootNode;

		// Create one child button in the middle of the screen.
		AllocChildren(root, 1);

		ClientUIPartitionNode& centerButton = root.Children[0];
		// CENTER BUTTON
		{
			// Button can either be enlarged or not, which is part of the persistent client state.
			Vector2s normalSize = { 200, 200 };
			Vector2s enlargedSize = { 400, 400 };
			
			centerButton.Dimensions = Client.bButtonEnlarged ? enlargedSize : normalSize;
			
			// If interacted with, output a message on screen.
			if (centerButton.bIsInteracted && !Client.bButtonEnlarged)
			{
				if (Client.Input.ActionKeyStateIs(ActionKey::MOUSE_LEFT, ActionInputState::UP))
				{
					std::cout << "The button was interacted with !!! It shall GROW.\n";
					Client.bButtonEnlarged = true;
				}
			}
			else if (bIsInteractionPass && !centerButton.bIsInteracted)
			{
				Client.bButtonEnlarged = false;
			}

			// Place it so its center lies at the center of its parent.
			centerButton.RelativePosition = centerButton.Parent->Dimensions / 2 - centerButton.Dimensions / 2;
		}
	}
}