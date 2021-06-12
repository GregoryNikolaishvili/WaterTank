static int ball_valve_state[TANK_COUNT];
static byte ball_valve_switch_state[TANK_COUNT];

static byte hBridgePins[2 * HBRIDGE_COUNT] = {
	PIN_HBRIDGE_BIG_IN1,
	PIN_HBRIDGE_BIG_IN2,
	PIN_HBRIDGE_SMALL_IN1,
	PIN_HBRIDGE_SMALL_IN2
};

void InitializeFloatSwitches()
{
	float_switch_states[0] = !floatSwitch1.read();
	float_switch_states[1] = !floatSwitch2.read();
	//float_switch_states[2] = !floatSwitch3.read();
}

void InitializeBallValves()
{
	for (byte id = 0; id < TANK_COUNT; id++)
	{
		ball_valve_switch_state[id] = '\0';
	}

  bouncerBigOpen.read();
  bouncerBigClose.read();
  bouncerSmallOpen.read();
  bouncerSmallClose.read();

  delay(600);

  bouncerBigOpen.update();
  bouncerBigClose.update();
  bouncerSmallOpen.update();
  bouncerSmallClose.update();

	setBallValveSwitchState(0, bouncerBigOpen.read(), bouncerBigClose.read());
	setBallValveSwitchState(1, bouncerSmallOpen.read(), bouncerSmallClose.read());

	for (byte i = 0; i < 2 * HBRIDGE_COUNT; i++)
	{
		pinMode(hBridgePins[i], OUTPUT);
		digitalWrite(hBridgePins[i], LOW);
	}

	//////////////
	//SetHBridge(0, -1); // Start closing
	//delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	//SetHBridge(0, 0); // remove power from ball valves

	//SetHBridge(0, 1); // Start closing
	//delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	//SetHBridge(0, 0); // remove power from ball valves

	//SetHBridge(0, -1); // Start closing
	//delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	//SetHBridge(0, 0); // remove power from ball valves

	//SetHBridge(0, 1); // Start closing
	//delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	//SetHBridge(0, 0); // remove power from ball valves
	/////////


	for (byte id = 0; id < TANK_COUNT; id++)
	{
		Serial.print(F("Closing valve #"));
		Serial.println(id + 1);
		if (ball_valve_switch_state[id] != 'C')
		{
			SetHBridge(id, -1); // Start closing
			delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
			SetHBridge(id, 0); // remove power from ball valves
		}
		ball_valve_state[id] = BALL_VALVE_FULLY_CLOSED;
		Serial.println(F("Closed"));
	}
}


int getBallValveState(byte id)
{
	return ball_valve_state[id];
}

void setBallValve(byte id, bool value)
{
	static unsigned long prevOffSeconds[TANK_COUNT] = { 0, 0 };

	if (value > 0)
	{
		if (ball_valve_state[id] <= 0 && secondTicks < (prevOffSeconds[id] + BALL_VALVE_ON_DELAY_SEC)) // 5 min
		{
			Serial.print(F("Delaying ball valve #"));
			Serial.print(id);
			Serial.print(F(" ON. Seconds left:"));
			Serial.println((prevOffSeconds[id] + BALL_VALVE_ON_DELAY_SEC) - secondTicks);
			return;
		}

		// if water is on floor 
		//	return false;

		// if tank is full (at least one sensors)
		if (isTankFull(id))
		{
			Serial.println("Tank is full");
			return;
		}
	}

	// If open is requested and valve is closed (or closing) OR 
	// if close is requested and valve is open (or opening)
	if ((value && ball_valve_state[id] < 0) || (!value && ball_valve_state[id] > 0))
	{
		if (value)
		{
			openBallValve(id);
		}
		else
		{
			prevOffSeconds[id] = secondTicks;
			closeBallValve(id);
		}
	}
}

void closeBallValve(byte id)
{
	Serial.println("Closing ball valve");
	ball_valve_state[id] = -BALL_VALVE_OPEN_CLOSE_SECONDS;
	SetHBridge(id, -1); // Negative, start closing
	PublishTankState(id);
}

void openBallValve(byte id)
{
	Serial.println("Opening ball valve");
	ball_valve_state[id] = BALL_VALVE_OPEN_CLOSE_SECONDS;
	SetHBridge(id, 1); // Positive, start opening
	PublishTankState(id);
}

void setBallValveSwitchState(byte id, bool openValue, bool closedValue)
{
	bool isOpen = !openValue;
	bool isClosed = !closedValue;

	char state;
	if (isOpen)
	{
		state = isClosed ? 'B' : 'O';
	}
	else
	{
		state = isClosed ? 'C' : '?';
	}

	if (ball_valve_switch_state[id] != state)
	{
		ball_valve_switch_state[id] = state;

		PublishTankState(id);

		Serial.print(F("Ball valve switch #"));
		Serial.print(id + 1);
		Serial.print(F(" = "));
		Serial.println(state);
	}
}

void processBallValve()
{
	for (byte id = 0; id < TANK_COUNT; id++)
	{
		int state = ball_valve_state[id];
		if (state == BALL_VALVE_FULLY_CLOSED || state == BALL_VALVE_FULLY_OPEN)
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
				ball_valve_state[id] = BALL_VALVE_FULLY_OPEN; // Open
			else
				ball_valve_state[id] = BALL_VALVE_FULLY_CLOSED; // Closed
		}
		else
			ball_valve_state[id] = state;
		PublishTankState(id);
	}
}

static void SetHBridge(byte id, char value)
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
	//
	//	Serial.print(F("HBridge #"));
	//	Serial.print(id);
	//	Serial.print(F(": "));
	//	Serial.println((int)value);
}
