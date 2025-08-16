SOURCE_INC_FILE()

// Implementation for the Client UI system, responsible for the Logic pass of the UI and generating the UI Partition Tree for a frame.
// NOTE: Some stuff related to the Drawing pass of UI is also defined here but to explore that specific system one should start from the Drawing code.

#include "Client.h"

UIPartitionNode *FindNodeAtPosition(UIPartitionTree Tree, Vector2s ViewportPosition)
{
	// Go down the tree level by level, checking the Viewport Position against the bounding rectangle of each element.
	// Stop when the hit element has no children or none that are hit.

	UIPartitionNode* hitNode = Tree.RootNode;
	while(hitNode->ChildCount > 0)
	{
		// Test every child sequentially until one is hit. At that point, set it as the new hit node and loop.
		bool bChildHit = false;
		for(int childIndex = 0; childIndex < hitNode->ChildCount; childIndex++)
		{
			UIPartitionNode& child = hitNode->Children[childIndex];
			Vector2s rectStart = child.AbsolutePosition;
			Vector2s rectEnd = child.AbsolutePosition + child.Dimensions;

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
	UIPartitionTree& Tree = Frame.MainViewportUITree;
	
	// Helper function for allocating new nodes from this tree and automatically setting up the parent - child relationships.
	auto AllocChildren = [&Tree, bIsPartitionPass](UIPartitionNode& Parent, size_t Count = 1)
	{
		if (!bIsPartitionPass) return; // Do nothing if not in partition pass.

		// ASSERT FATAL IF PARENT ALREADY HAS CHILDREN (Can't grow the children buffer dynamically. If you need a dynamic number of children, determine their count in advance.)
		Parent.Children = Tree.Memory.Allocate<UIPartitionNode>(Count);
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
			Tree.RootNode = Tree.Memory.Allocate<UIPartitionNode>(); 	// Lacking a parent, the helper function can't be used. 
																			// This probably has some sort of deeper meaning I didn't intend.
			Tree.RootNode->RelativePosition = {};
			Tree.RootNode->Dimensions = Client.MainViewport.Dimensions;
		}
		UIPartitionNode& root = *Tree.RootNode;

		AllocChildren(root, 3);
		UIPartitionNode& topPanel = root.Children[0];
		UIPartitionNode& leftPanel = root.Children[1];
		UIPartitionNode& graphViewPanel = root.Children[2];

		// TOP PANEL
		if (bIsPartitionPass)
		{
			topPanel.RelativePosition = { }; // Top left corner
			topPanel.Dimensions = { Client.MainViewport.Dimensions.x, // Entire width
									(int16_t)(Client.MainViewport.Dimensions.y * 0.2f) // Fifth of height
			};

			topPanel.PresentationDef = &Client.UINodePresentations.GenericPanel;
		}

		// LEFT PANEL
		if (bIsPartitionPass)
		{
			leftPanel.RelativePosition = { 0, topPanel.Dimensions.y }; // Top left below top panel
			leftPanel.Dimensions = { (int16_t)(Client.MainViewport.Dimensions.x * 0.2f), // Fifth of width
									(int16_t)(Client.MainViewport.Dimensions.y - topPanel.Dimensions.y) // Entire height minus top panel.
			};

			leftPanel.PresentationDef = &Client.UINodePresentations.GenericPanel;
		}

		// GRAPH VIEW PANEL
		
		if (bIsPartitionPass)
		{
			graphViewPanel.RelativePosition = { leftPanel.Dimensions.x, topPanel.Dimensions.y }; // Top left below top panel to the right of left panel.
			graphViewPanel.Dimensions = { (int16_t)(Client.MainViewport.Dimensions.x - leftPanel.Dimensions.x), // Entire width minus left panel
										(int16_t)(Client.MainViewport.Dimensions.y - topPanel.Dimensions.y) 
			}; // Entire height minus top panel

			graphViewPanel.PresentationDef = &Client.UINodePresentations.GraphViewPanel;

			size_t nodeCount = 0;
			// For each Graph Node Representation available, draw it.
			for (GraphNodeRepresentationData& nodePresentation : Client.NodeRepresentations)
			{
				if (nodePresentation.nodeID == SNODE_INVALID_ID) break; // End of buffer reached.

				nodeCount++;
			}

			// Allocate one child node per graph node.
			AllocChildren(graphViewPanel, nodeCount);
		}

		
		size_t childIndex = 0;

		// GRAPH NODES
		for (GraphNodeRepresentationData& nodePresentation : Client.NodeRepresentations)
		{
			if (nodePresentation.nodeID == SNODE_INVALID_ID) break; // End of buffer reached.
			UIPartitionNode& graphNodeUINode = graphViewPanel.Children[childIndex++];

			if (bIsPartitionPass)
			{
				graphNodeUINode.Dimensions = { 50, 50 };
				graphNodeUINode.RelativePosition = nodePresentation.viewSpaceLocation - graphNodeUINode.Dimensions / 2;
			}

			graphNodeUINode.PresentationDef = Client.SelectedGraphNodeID == nodePresentation.nodeID ?
													&Client.UINodePresentations.GraphNode_Selected : &Client.UINodePresentations.GraphNode;

			if (graphNodeUINode.bIsInteracted && Client.Input.ActionKeyStateIs(ActionKey::MOUSE_LEFT, ActionInputState::UP))
			{
				Client.SelectedGraphNodeID = nodePresentation.nodeID;
			}
		}
		
	}
}