#include <iostream>

#ifndef SYNERGYCLIENT_DRAWBUFFER_INCLUDED
#define SYNERGYCLIENT_DRAWBUFFER_INCLUDED

union ColorRGBA
{
	struct
	{
		uint8_t r, g, b, a;
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
	uint16_t x, y;

	// Rotation in degrees of the drawn shape.
	uint16_t angleDeg;

	ColorRGBA color;
};

/*
	Data for a Line type draw call. Origin coordinates should be interpreted as the start point of the line.
	Angle should be interpreted as Origin-to-Destination axis along cosinus, and left normal of that axis along sinus. 
*/
struct LineDrawCallData : public DrawCall
{
	// Destination point of the line.
	uint16_t destX, destY;

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
	uint16_t width, height;
};

/*
	Data for a Ellipse type draw call. Origin coordinates should be interpreted as the center of the ellipse where the medians intersect.
	Angle should be interpreted as radius X along cosinus, radius Y along sinus at Angle = 0.
*/
struct EllipseDrawCallData : public DrawCall
{
	float radiusX, radiusY;
};

/*
	Data for Bitmap type draw call. Behaves the same as a Rectangle draw call but attempts to stretch / shrink the pixels with the given resolution
	to fit the rectangle.
	TODO Figure out pixel memory management strategy.
*/
struct BitmapDrawCallData : public RectangleDrawCallData
{
	uint16_t resolutionX, resolutionY;
	// TODO Bitmap stuff in here. How do we handle variable sized pixel buffers ? Probably need some guarantee that the pixel data stays in
	// memory somewhere. AllocateBitmap platform call ? Use Client memory ?
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

/*
	Contains all draw calls emitted by the client over a single frame.
	TODO IMPORTANT This needs to be overhauled into a system that puts more power into the platform's hands when it comes to creating and managing draw calls.
	The platform should probably supply functions to create draw calls, and then manage them however it pleases. That way fewer symbols need to be exported or
	defined in header directly.
*/
struct ClientFrameDrawCallBuffer
{
	/*
		To be called before writing into the buffer. Zeroes out the buffer and puts the buffer object into a writeable state.
		Returns whether the buffer is writeable.
	*/
	bool BeginWrite()
	{
		// Reset Cursor position to 0 and zero out buffer memory. Output warning message if buffer is too small.
		// Output error if buffer is not valid.

		if (Buffer == nullptr)
		{
			std::cerr << "ERROR: Attempted to make draw call buffer writeable without an actual buffer being provided !\n";
			return false;
		}

		if (BufferSize < sizeof(DrawCall))
		{
			std::cerr << "WARNING: Made a draw call buffer of size " << BufferSize << " writeable, which is too small for any kind of draw call.\n";
		}

		CursorPosition = 0;
		memset(Buffer, 0, BufferSize);
		return true;
	}

	/*
		Provided the buffer isn't full, returns the memory address where a draw call of the passed type can be built.
		Make sure to call BeginWrite() before the first call to NewDrawCall().
		Returns nullptr if the buffer is too small to allocate the given draw call type.
	*/
	DrawCall* NewDrawCall(DrawCallType Type)
	{
		size_t requiredSize = GetDrawCallSize(Type);

		if (requiredSize > BufferSize - CursorPosition)
		{
			std::cerr << "ERROR: Out of memory in draw call buffer. Attempted to create draw call of type " << static_cast<uint16_t>(Type)
				<< " with only " << BufferSize - CursorPosition << " bytes available.\n";
			return nullptr;
		}

		// Advance cursor by the required number of bytes and return the address where we can build the draw call.
		DrawCall* address = reinterpret_cast<DrawCall*>(Buffer + CursorPosition);
		address->type = Type;

		CursorPosition += requiredSize;
		return address;
	}

	/*
		To be called before reading through the buffer. Puts the buffer object into a readable state.
		Returns whether the buffer is readable.
	*/
	bool BeginRead()
	{
		// Reset Cursor Position to 0. Run safety checks on the buffer and check that the first draw call's type is a valid value.
		if (Buffer == nullptr || BufferSize < sizeof(DrawCall))
		{
			// Here both the buffer itself being null or it being too small are considered fatal errors, 
			// as the buffer should have been discarded during the writing stage in either of those cases.
			std::cerr << "FATAL ERROR: Attempted to start reading a draw call buffer without an actual buffer being provided, or one that is too small !\n"
				<< "Please make sure the buffer is discarded during the writing stage.\n";
			return false;
		}

		if (reinterpret_cast<DrawCall*>(Buffer)[0].type >= DrawCallType::INVALID)
		{
			std::cerr << "ERROR: Attempted to start reading a draw call buffer from faulty memory.\n";
			return false;
		}

		CursorPosition = 0;
		return true;
	}

	/*
		Returns next draw call in the buffer.
		Make sure to call BeginRead() before the first call to GetNext().
		Advances the Cursor to the first byte of the next call, meaning it should be equal to BufferSize when reading the entire buffer is done.
		Returns nullptr for any error or reaching the end of the buffer.
	*/
	DrawCall* GetNext()
	{
		// Interpret Cursor Position as current reading position. Read first bytes as DrawCall structure and inspect type.
		// Make sure that there is enough room left in the buffer to hold the relevant extended data structure.
		// Then, return the DrawCall structure.

		DrawCall* nextCall = reinterpret_cast<DrawCall*>(Buffer + CursorPosition);
		if (CursorPosition == BufferSize || nextCall->type == DrawCallType::EMPTY)
		{
			// End of buffer reached.
			return nullptr;
		}

		
		size_t actualDrawCallSize = GetDrawCallSize(nextCall->type);

		if (BufferSize - CursorPosition < actualDrawCallSize)
		{
			std::cerr << "ERROR: Inconsistent draw call buffer size. Check that is was populated correctly. Read draw call of type " << static_cast<uint16_t>(nextCall->type)
				<< " with only " << BufferSize - CursorPosition << " bytes available.\n";
			return nullptr;
		}

		// Is it now guaranteed that the drawcall exists and has enough memory "ahead" of it to initialize its full data structure.
		// Advance the cursor and return a pointer to the call we just read.
		CursorPosition += actualDrawCallSize;
		return nextCall;
	}

	// Pre-allocated memory for holding draw call structures.
	uint8_t* Buffer = nullptr;

	// Buffer size in BYTES.
	size_t BufferSize = 0;

	// When filling the buffer in, is the write cursor. When reading the buffer, is the read cursor.
	size_t CursorPosition = 0;
};

#endif