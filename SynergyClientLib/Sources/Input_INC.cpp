SOURCE_INC_FILE()

// Implementation for the Client input system, responsible for updating the Input State of the Client given a frame's input data / events.

#include "Client.h"

// Process the Action Inputs buffer and update the Client Input State.
void ProcessActionInputEvents(ClientInputState& InputState, const ActionInputEventBuffer& ActionInputEvents)
{
	// "Advance" the state of non-RELEASED inputs.
	for (uint8_t actionKeyIndex = 0; actionKeyIndex < (uint8_t)(ActionKey::ACTION_KEY_COUNT); actionKeyIndex++)
	{
		switch (InputState.ActionInputStates[actionKeyIndex])
		{
		case(ActionInputState::UP):
			InputState.ActionInputStates[actionKeyIndex] = ActionInputState::RELEASED;
			break;
		case(ActionInputState::DOWN):
			InputState.ActionInputStates[actionKeyIndex] = ActionInputState::HELD;
			break;
		default:
			break;
		}
	}

	// Read in Action inputs.
	for (size_t inputEventIndex = 0; inputEventIndex < ActionInputEvents.EventCount; inputEventIndex++)
	{
		ActionInputEvent& event = ActionInputEvents.Buffer[inputEventIndex];

		uint8_t keyIndex = (uint8_t)(event.key);

		if (!event.bRelease)
		{
			switch (InputState.ActionInputStates[keyIndex])
			{
			case(ActionInputState::RELEASED):
			case(ActionInputState::UP):
				InputState.ActionInputStates[keyIndex] = ActionInputState::DOWN;
			default:
				break;
			}
		}
		else
		{
			switch (InputState.ActionInputStates[keyIndex])
			{
			case(ActionInputState::HELD):
			case(ActionInputState::DOWN):
				InputState.ActionInputStates[keyIndex] = ActionInputState::UP;
			default:
				break;
			}
		}

		if (inputEventIndex == ActionInputEvents.EventCount - 1)
		{
			// Last event gives us the cursor position and viewport ofor this frame.

			InputState.CursorLocation = event.cursorLocation;
			InputState.CursorViewport = event.viewport;
		}
	}
}

void ProcessInputs(ClientSessionState& Client, ClientFrameState& Frame)
{
	ProcessActionInputEvents(Client.Input, *Frame.ActionInputs);
}