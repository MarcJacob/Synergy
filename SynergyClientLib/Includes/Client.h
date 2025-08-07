// Contains symbols that are shared and implemented over multiple files in the Client code, for its major functionality.
// Can only be included in the main Client Translation Unit.

#if (TRANSLATION_UNIT != SYNERGY_CLIENT_MAIN)
static_assert(0, "Client.h can only be included inside the SYNERGY_CLIENT_MAIN translation unit ! Found it with " __BASE_FILE__);
#endif

#ifndef CLIENT_INCLUDED
#define CLIENT_INCLUDED

#include "SynergyClientAPI.h"

// TEST CODE Entity data structure for spawning and displaying dynamically spawned entities.
struct TestEntity
{
	Vector2f Location;
	ColorRGBA Color;
	uint8_t Size;
};

/*
	State of the Client as a whole. Persistent memory pointer provided by the platform is cast to this.
*/
struct ClientSessionState
{
	ViewportID MainViewportID;

	ClientInputState Input;

	// TEST CODE move a rectangle around on the main viewport using input.
	Vector2f PlayerCoordinates;
	float PlayerSpeed;

	// TEST CODE Entity buffer.
	struct
	{
		static constexpr uint16_t BUFFER_SIZE = 256;

		void SpawnEntity(Vector2f Location, ColorRGBA Color, uint8_t Size)
		{
			if (EntityCount >= BUFFER_SIZE)
			{
				std::cout << "Cannot spawn entity: Buffer max size reached.\n";
				return;
			}

			TestEntity& newEntity = Buffer[EntityCount];
			newEntity.Location = Location;
			newEntity.Color = Color;
			newEntity.Size = Size;

			EntityCount++;
		}

		TestEntity Buffer[BUFFER_SIZE];
		size_t EntityCount;

	} Entities;
};

// MAJOR PROCEDURES

void ProcessInputs(ClientSessionState& State, ClientFrameData& FrameData);
void OutputDrawCalls(ClientSessionState& State, ClientFrameData& FrameData);

#endif // CLIENT_INCLUDED