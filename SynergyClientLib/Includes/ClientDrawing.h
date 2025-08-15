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

// Graph drawing functions

/*
	Data structure for the persistent representation of a node on the user interface.
	Effectively the "Front end state" of the node.
*/
struct NodeRepresentationData
{
	// ID of the node this represents.
	SNodeGUID nodeID = SNODE_INVALID_ID;

	// Virtual location of the node in the graph view.
	Vector2f viewSpaceLocation = {};
};