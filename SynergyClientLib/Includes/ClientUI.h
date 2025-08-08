// Contains symbols and inline functions for the Client's UI system.

// A node in the UI partition tree. End product of the Partition Pass.
struct ClientUIPartitionNode
{
	// Child nodes.
	ClientUIPartitionNode* Children;
	size_t ChildCount;

	// Position on screen relative to parent. Corresponds to top left corner of rectangle bounds.
	Vector2s RelativePosition;

	// Dimensions of UI element rectangle bounds in screen units.
	Vector2s Dimensions;
};