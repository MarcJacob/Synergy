SOURCE_INC_FILE()

// Implementation for the Client drawing system, responsible for outputting draw calls for a given frame.

#include "Client.h"

void OutputDrawCalls(ClientSessionState& State, ClientFrameState& FrameData)
{
	if (FrameData.FramePlatformAPI.NewDrawCall == nullptr)
	{
		// Drawing not supported.
		return;
	}
}