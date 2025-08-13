// Defines symbols used by the Client to output draw calls to the platform layer which it must support.

#ifndef SYNERGYCLIENT_DRAWBUFFER_INCLUDED
#define SYNERGYCLIENT_DRAWBUFFER_INCLUDED

#include <iostream>

#include "SynergyCore.h"

/*
	Basic "inversed" RGBA color structure used by the Client to understand and express colors. The platform may need to translate it to its own format,
	perhaps even ignoring some colors if necessary.
	T = Transparency. At 0, the color is perfectly opaque. At 255, it is perfectly transparent. This makes it less cumbersome to write common
	color values that are usually opaque anyway.
*/
union ColorRGBT
{
	struct
	{
		uint8_t r, g, b, t;
	};

	uint32_t full;
};

/*
	Type of a draw call. Tells the render layer what kind of drawing to do, and what function to call to retrieve the correct
	data structure from the draw call memory.
*/
enum class DrawCallType
{
	EMPTY, // Empty draw call that wasn't initialized from 0ed memory.
	LINE, // Draw pixels in a straight line from origin to specific destination coordinates with a given width.
	RECTANGLE, // Draw pixels with a corner origin and a specific width and height.
	ELLIPSE, // Draw pixels in an ellipse with a specific origin and radius.
	BITMAP, // Same as rectangle, but stretch / shrink a bitmap of pixels to fit inside the rectangle.
	INVALID, // Keep this value at the bottom of the enum. Indicates a draw call that was read from faulty memory.
};

// Base class for all draw call data classes. Contains spatial and visual transform information relevant to all types.
struct DrawCall
{
	DrawCallType type;

	// Origin coordinates of the draw call to be interpreted differently depending on type.
	Vector2s origin;

	// Rotation in degrees of the drawn shape.
	uint16_t angleDeg;

	ColorRGBT color;
};

/*
	Data for a Line type draw call. Origin coordinates should be interpreted as the start point of the line.
	Angle should be interpreted as Origin-to-Destination axis along cosinus, and left normal of that axis along sinus. 
*/
struct LineDrawCallData : public DrawCall
{
	// Destination point of the line.
	Vector2s destination;

	// Width of the line along its main axis in pixels.
	uint16_t width;
};

/*
	Data for a Rectangle type draw call. Origin coordinates should be interpreted as top left corner position.
	Angle should be interpreted as Rectangle Width along cosinus, Height along sinus at Angle = 0.
*/
struct RectangleDrawCallData : public DrawCall
{
	// Dimensions of rectangle.
	Vector2s dimensions;
};

/*
	Data for a Ellipse or Circle type draw call. 
	Origin coordinates should be interpreted as the center of the ellipse where the medians intersect.
	Angle should be interpreted as radius X along cosinus, radius Y along sinus at Angle = 0.
	If Y == 0, draw a circle with X as its radius. Use the circleRadius member only for a more obvious approach to requesting a circle draw.
*/
struct EllipseDrawCallData : public DrawCall
{
	union
	{
		// Radii towards the "sides" and "top-to-bottom" axis of the ellipse when Angle = 0.
		Vector2s ellipticRadii;
		// Radius of the circle.
		uint16_t circleRadius;
	};
};

/*
	Data for Bitmap type draw call. Behaves the same as a Rectangle draw call but attempts to stretch / shrink the pixels with the given resolution
	to fit the rectangle.
*/
struct BitmapDrawCallData : public RectangleDrawCallData
{
	Vector2s bitmapResolution;
};

/*
	Returns the expected actual size of a draw call data structure.
	Returns 0 if the call is invalid for any reason.
*/
inline size_t GetDrawCallSize(DrawCallType CallType)
{
	switch (CallType)
	{
	case(DrawCallType::LINE):
		return sizeof(LineDrawCallData);
	case(DrawCallType::RECTANGLE):
		return sizeof(RectangleDrawCallData);
	case(DrawCallType::ELLIPSE):
		return sizeof(EllipseDrawCallData);
	case(DrawCallType::BITMAP):
		return sizeof(BitmapDrawCallData);
	default:
		return 0;
	}
}

#endif