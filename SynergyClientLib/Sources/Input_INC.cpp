SOURCE_INC_FILE()

// Implementation for the Client input system, responsible for updating the Input State of the Client given a frame's input data / events.

#include "Client.h"

void ProcessInputs(ClientSessionState& State, ClientFrameData& FrameData)
{
	// "Advance" the state of non-RELEASED inputs.
	for (uint8_t actionKeyIndex = 0; actionKeyIndex < (uint8_t)(ActionKey::ACTION_KEY_COUNT); actionKeyIndex++)
	{
		switch (State.Input.ActionInputStates[actionKeyIndex])
		{
		case(ActionInputState::UP):
			State.Input.ActionInputStates[actionKeyIndex] = ActionInputState::RELEASED;
			break;
		case(ActionInputState::DOWN):
			State.Input.ActionInputStates[actionKeyIndex] = ActionInputState::HELD;
			break;
		default:
			break;
		}
	}

	// Read in Action inputs.
	for (size_t inputEventIndex = 0; inputEventIndex < FrameData.InputEvents.EventCount; inputEventIndex++)
	{
		ActionInputEvent& event = FrameData.InputEvents.Buffer[inputEventIndex];

		uint8_t keyIndex = (uint8_t)(event.key);

		if (!event.bRelease)
		{
			switch (State.Input.ActionInputStates[keyIndex])
			{
			case(ActionInputState::RELEASED):
			case(ActionInputState::UP):
				State.Input.ActionInputStates[keyIndex] = ActionInputState::DOWN;
			default:
				break;
			}
		}
		else
		{
			switch (State.Input.ActionInputStates[keyIndex])
			{
			case(ActionInputState::HELD):
			case(ActionInputState::DOWN):
				State.Input.ActionInputStates[keyIndex] = ActionInputState::UP;
			default:
				break;
			}
		}

		if (inputEventIndex == FrameData.InputEvents.EventCount - 1)
		{
			// Last event gives us the cursor position and viewport ofor this frame.

			State.Input.CursorLocation = event.cursorLocation;
			State.Input.CursorViewport = event.viewport;
		}
	}
}