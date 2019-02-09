static byte hBridgePins[2 * BALL_VALVE_COUNT] = {
	PIN_HBRIDGE1_IN1,
	PIN_HBRIDGE1_IN2,
	PIN_HBRIDGE2_IN1,
	PIN_HBRIDGE2_IN2,
	PIN_HBRIDGE3_IN1,
	PIN_HBRIDGE3_IN2,
	PIN_HBRIDGE4_IN1,
	PIN_HBRIDGE4_IN2
};

int ball_valve_state[BALL_VALVE_COUNT] = { 0, 0, 0, 0 };

void setBallValve(byte id, bool value)
{
	static unsigned long prevOffSeconds[BALL_VALVE_COUNT] = { 0, 0, 0, 0 };

	if (value > 0)
	{
		if ((prevOffSeconds[id] > 0) && (secondTicks < (prevOffSeconds[id] + BALL_VALVE_ON_DELAY_SEC))) // 5 min
			return;

		// if water is on floor 
		//	return false;

		// if tank is full (at least one sensors)
		if (isTankFull(id))
			return;
	}


	if ((value && ball_valve_state[id] < 0) || (!value && ball_valve_state[id] > 0))
	{
		if (value)
		{
			ball_valve_state[id] = BALL_VALVE_OPEN_CLOSE_SECONDS;
			SetHBridge(id, 1); // Positive
		}
		else
		{
			prevOffSeconds[id] = secondTicks;
			ball_valve_state[id] = -BALL_VALVE_OPEN_CLOSE_SECONDS;
			SetHBridge(id, -1); // Negative
		}
		PublishSensorState(id);
	}
}


void processBallValve()
{
	for (byte id = 0; id < BALL_VALVE_COUNT; id++)
	{
		byte state = ball_valve_state[id];
		if (state == 0 || state == 0xFF || state == -0xFF)
			continue;

		bool opening;
		if (state > 0)
		{
			state--;
			opening = true;
		}
		else
		{
			state++;
			opening = false;
		}

		if (state == 0)
		{
			SetHBridge(id, 0); // Neutral
			if (opening)
				ball_valve_state[id] = 0xFF; // Open
			else
				ball_valve_state[id] = -0xFF; // Closed
		}
		else
			ball_valve_state[id] = state;
	}
}

void SetHBridge(byte id, char value)
{
	byte pinIn1 = hBridgePins[id * 2];
	byte pinIn2 = hBridgePins[id * 2 + 1];

	digitalWrite(pinIn1, LOW);
	digitalWrite(pinIn2, LOW);

	if (value > 0)
		digitalWrite(pinIn1, HIGH);
	else
		if (value < 0)
			digitalWrite(pinIn2, HIGH);

	Serial.print(F("Ball valve #"));
	Serial.print(id);
	Serial.print(F(": "));
	Serial.println((int)value);
}

