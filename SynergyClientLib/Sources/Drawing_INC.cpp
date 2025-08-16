SOURCE_INC_FILE()

// Implementation for the Client drawing system, responsible for outputting draw calls for a given frame.

#include "Client.h"

int DEBUG_GetUITreeDepth_Recursive(const UIPartitionNode& Node)
{
	int maxChildDepth = 0;
	for (int childIndex = 0; childIndex < Node.ChildCount; childIndex++)
	{
		int depth = DEBUG_GetUITreeDepth_Recursive(Node.Children[childIndex]);
		if (depth > maxChildDepth)
		{
			maxChildDepth = depth;
		}
	}

	return 1 + maxChildDepth;
}

void DEBUG_DrawUINode_Recursive(ViewportID Viewport, ClientFrameState& Frame, const UIPartitionNode& Node, float ColorPerDepth, int DepthLevel)
{
	// Output a rectangular draw call with the given color per depth multiplied by depth level, then recursively call on children.
	
	uint16_t colorIntensity = (uint8_t)(ColorPerDepth * DepthLevel);
	
	RectangleDrawCallData* nodeRect = (RectangleDrawCallData*)Frame.FramePlatformAPI.NewDrawCall(Viewport, DrawCallType::RECTANGLE);
	nodeRect->dimensions = Node.Dimensions;
	nodeRect->origin = Node.AbsolutePosition;
	nodeRect->color = Node.bIsInteracted ? GetColorWithIntensity(COLOR_Green, colorIntensity) : GetColorWithIntensity(COLOR_White, colorIntensity);
	for (int childIndex = 0; childIndex < Node.ChildCount; childIndex++)
	{
		DEBUG_DrawUINode_Recursive(Viewport, Frame, Node.Children[childIndex], ColorPerDepth, DepthLevel + 1);
	}
}

/*
	Draws a debug view of the UI's partition tree along with the "path" to the current pointed element if any. 
*/
void DEBUG_DrawUIPartitionInteraction(ViewportID Viewport, ClientFrameState& Frame)
{
	if (Frame.MainViewportUITree.RootNode == nullptr)
	{
		// UI Partition Tree doesn't exist.
		return;
	}

	// Go down the tree, drawing elements as rectangles getting whiter and whiter with each level.
	// If the element is interacted with, give it a green hue.

	int treeDepth = DEBUG_GetUITreeDepth_Recursive(*Frame.MainViewportUITree.RootNode);

	float colorPerDepthLevel = 255.f / treeDepth;

	DEBUG_DrawUINode_Recursive(Viewport, Frame, *Frame.MainViewportUITree.RootNode, colorPerDepthLevel, 1);
}

/*
	Triggers the Node to call on its assigned Presentation Definition to output drawcalls using the passed NewDrawCall function.
	Called recursively on the node's children, depth-first, meaning intersecting nodes will have visual precedence depending on who is higher
	in the tree and drawn later.
*/
void DrawUINode_Recursive(UIPartitionNode& Node, FrameNewDrawCallFunc& NewDrawCall, ViewportID TargetViewport)
{
	if (Node.PresentationDef != nullptr)
	{
		Node.PresentationDef->DrawNode(Node, NewDrawCall, TargetViewport);
	}

	for (int childIndex = 0; childIndex < Node.ChildCount; childIndex++)
	{
		DrawUINode_Recursive(Node.Children[childIndex], NewDrawCall, TargetViewport);
	}
}

/*
	Goes through the entire UI Partition Tree and draws it onto the passed viewport.
*/
void DrawUI(ClientFrameState& Frame, UIPartitionTree& Tree, ViewportID TargetViewport)
{
	DrawUINode_Recursive(*Tree.RootNode, *Frame.FramePlatformAPI.NewDrawCall, TargetViewport);
}

void OutputDrawCalls(ClientSessionState& Client, ClientFrameState& Frame)
{
	if (Frame.FramePlatformAPI.NewDrawCall == nullptr)
	{
		// Drawing not supported.
		return;
	}

	DrawUI(Frame, Frame.MainViewportUITree, Client.MainViewport.ID);

	if (Client.bDrawUIDebug)
	{
		// Draw debug UI view.
		DEBUG_DrawUIPartitionInteraction(Client.MainViewport.ID, Frame);
	}
}

// UI NODE DRAWING FUNCTIONS IMPLEMENTATION

// Simple Rectangle.
void UINodePresentationDrawFunc_Rectangle(	const UIPartitionNode& Node,
	 										FrameNewDrawCallFunc& NewDrawCall,
											ViewportID TargetViewport,
											const UINodePresentationDef* PresentationDef)
{
	if (PresentationDef == nullptr)
	{
		// ASSERT This UI Node Draw Function requires the appropriate Presentation Def structure passed to it.
		return;
	}

	const UINodePresentationDef_Rectangle* rectanglePresentationDef = (const UINodePresentationDef_Rectangle*)PresentationDef;

	RectangleDrawCallData* rectDrawCall = (RectangleDrawCallData*)NewDrawCall(TargetViewport, DrawCallType::RECTANGLE);
	rectDrawCall->dimensions = Node.Dimensions;
	rectDrawCall->origin = Node.AbsolutePosition;

	if (!rectanglePresentationDef->bHighlightWhenInteracted || !Node.bIsInteracted)
	{
		rectDrawCall->color = rectanglePresentationDef->RectangleColor;
	}
	else
	{
		rectDrawCall->color = GetColorWithIntensity(rectanglePresentationDef->RectangleColor, 1.2f);
	}
}