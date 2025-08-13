SOURCE_INC_FILE()

// Implementation for the Client drawing system, responsible for outputting draw calls for a given frame.

#include "Client.h"

int DEBUG_GetUITreeDepth_Recursive(const ClientUIPartitionNode& Node)
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

void DEBUG_DrawUINode_Recursive(ViewportID Viewport, ClientFrameState& Frame, const ClientUIPartitionNode& Node, float ColorPerDepth, int DepthLevel)
{
	// Output a rectangular draw call with the given color per depth multiplied by depth level, then recursively call on children.
	
	uint16_t colorIntensity = (uint8_t)(ColorPerDepth * DepthLevel);
	
	RectangleDrawCallData* nodeRect = (RectangleDrawCallData*)Frame.FramePlatformAPI.NewDrawCall(Viewport, DrawCallType::RECTANGLE);
	nodeRect->dimensions = Node.Dimensions;
	nodeRect->origin = Node.RelativePosition;
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

void OutputDrawCalls(ClientSessionState& Client, ClientFrameState& Frame)
{
	if (Frame.FramePlatformAPI.NewDrawCall == nullptr)
	{
		// Drawing not supported.
		return;
	}

	if (Client.bDrawUIDebug)
	{
		// Draw debug UI view.
		DEBUG_DrawUIPartitionInteraction(Client.MainViewport.ID, Frame);
	}
}