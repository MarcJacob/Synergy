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

void ProcessInputs(ClientSessionState& State, ClientFrameData& FrameData);
void OutputDrawCalls(ClientSessionState& State, ClientFrameData& FrameData);

#endif // CLIENT_INCLUDED