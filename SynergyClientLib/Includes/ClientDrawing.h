// Contains symbols and inline functions for the Client's drawing system.

#include "SynergyClientAPI_Drawing.h"

#include "Graph/SynergyGraph.h"

// Define inlined operators for manipulating colors

/*
	Returns the color passed in with the applied normalized intensity (0 to 1). Does not affect the Transparency channel.
*/
inline ColorRGBT GetColorWithIntensity(const ColorRGBT& Color, float IntensityMul)
{
	return { (uint8_t)(Color.r * IntensityMul), (uint8_t)(Color.g * IntensityMul), (uint8_t)(Color.b * IntensityMul), Color.t };
}

/*
	Returns the color passed in with the applied intensity (full blackness with 0, current at 255 and so on). Does not affect the Transparency channel.
*/
inline ColorRGBT GetColorWithIntensity(const ColorRGBT& Color, uint16_t IntensityIndex)
{
	return GetColorWithIntensity(Color, (float)IntensityIndex / 255);
}

/*
	Returns a mix of the two passed colors.
	The Transparency channel gets "mixed" as well, which might change later (relative transparency should probably increase the influence of one color over the
	other in the mix).
*/
inline ColorRGBT MixColors(const ColorRGBT& A, const ColorRGBT& B)
{
	ColorRGBT mix;
	mix.full = A.full | B.full; // This isn't perfect of course but will suffice for now as we tend to mix very simple colors together.
	return mix;
}

// Define some static common color values

constexpr ColorRGBT COLOR_Black =		{ 255, 0, 0 };
constexpr ColorRGBT COLOR_White =		{ 255, 255, 255 };

constexpr ColorRGBT COLOR_Red =			{ 255, 0, 0 };
constexpr ColorRGBT COLOR_Yellow =		{ 255, 255, 0 };
constexpr ColorRGBT COLOR_Green =		{ 0, 255, 0 };
constexpr ColorRGBT COLOR_Cyan =		{ 0, 255, 255 };
constexpr ColorRGBT COLOR_Blue =		{ 0, 0, 255 };

// Graph drawing

/*
	Data structure for the persistent representation of a node on the user interface.
	Effectively the "Front end state" of the node.
*/
struct GraphNodeRepresentationData
{
	// ID of the node this represents.
	SNodeGUID nodeID = SNODE_INVALID_ID;

	// Virtual location of the node in the graph view.
	Vector2f viewSpaceLocation = {};
};

// UI drawing

// Type definition for a function that is able to output draw calls from a UI Node, a New Draw Call function, and if needed a Presentation Def data structure.
typedef void (UINodeDrawingFunction)(const struct UIPartitionNode& Node,
									FrameNewDrawCallFunc& NewDrawCall,
									ViewportID TargetViewport,
									const struct UINodePresentationDef* PresentationDef);

/*
	Defines the graphical representation of a node.
	The simplest definition possible only contains a function pointer able to output draw calls from a UI Node and the
	necessary frame system call for allocating the calls.

	More complex presentations can be built by extending this data structure with parameters and giving it a drawing function
	that fetches the definition from the Node and casts it to the appropriate type.

	For an example of such an extension, see the Rectangle presentation def below.
*/
struct UINodePresentationDef
{
	// Calls the definition's underlying Drawing Function. If it is not assigned, no draw calls will be emitted for the node.
	inline void DrawNode(const UIPartitionNode& Node, FrameNewDrawCallFunc& NewDrawCall, ViewportID TargetViewport)
	{
		if (DrawingFunctionPtr != nullptr) DrawingFunctionPtr(Node, NewDrawCall, TargetViewport, this);
		// Assert if the Drawing Function Ptr is not assigned ? Or should invisible nodes just not be assigned a Presentation in the first place ?
	}

	// Pointer to full Drawing function for this definition to output Draw Calls.
	UINodeDrawingFunction* DrawingFunctionPtr = nullptr;
};

// UI Node Drawing Function for a simple color-filled rectangle area. Requires a UINodePresentationDef_Rectangle presentation def structure
// to be passed.
UINodeDrawingFunction UINodePresentationDrawFunc_Rectangle;

// Simple Colored Rectangle presentation data structure, filling the Node's space with a specific color.
struct UINodePresentationDef_Rectangle : public UINodePresentationDef
{
	UINodePresentationDef_Rectangle(ColorRGBT Color, bool bCanHighlight): RectangleColor(Color), bHighlightWhenInteracted(bCanHighlight)
	{
		DrawingFunctionPtr = UINodePresentationDrawFunc_Rectangle;
	}

	ColorRGBT RectangleColor;
	bool bHighlightWhenInteracted;
};

