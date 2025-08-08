// Contains symbols that are shared and implemented over multiple files in the Client code, for its major functionality.
// Can only be included in the main Client Translation Unit.

#if (TRANSLATION_UNIT != SYNERGY_CLIENT_MAIN)
static_assert(0, "Client.h can only be included inside the SYNERGY_CLIENT_MAIN translation unit ! Found it with " __BASE_FILE__);
#endif

#ifndef CLIENT_INCLUDED
#define CLIENT_INCLUDED

#include "SynergyClientAPI.h"

/*
	State of the Client as a whole. Persistent memory pointer provided by the platform is cast to this.
*/
struct ClientSessionState
{
	ViewportID MainViewportID;

	ClientInputState Input;
};

// MAJOR PROCEDURES

/*
	Processes the various input buffers sent by the platform layer to update the Client Input State.
*/
void ProcessInputs(ClientSessionState& State, ClientFrameRequestData& FrameData);

/*
	Generates the UI Partition tree for a frame and performs all logical links between UI and data.
*/
void PerformUILogicPass(ClientSessionState& State, ClientFrameRequestData& FrameData);

/*
	Generates all draw calls to render the end state of a frame. Includes UI and various dynamic elements.
*/
void OutputDrawCalls(ClientSessionState& State, ClientFrameRequestData& FrameData);

#endif // CLIENT_INCLUDED