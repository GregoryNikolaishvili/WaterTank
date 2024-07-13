static byte hBridgePins[2 * HBRIDGE_COUNT] = {
	PIN_HBRIDGE_BIG_IN1,
	PIN_HBRIDGE_BIG_IN2,
	PIN_HBRIDGE_SMALL_IN1,
	PIN_HBRIDGE_SMALL_IN2
};

static void initializeBallValve()
{
	bouncerValveOpenSwitch.read();
	bouncerValveCloseSwitch.read();

	delay(600);

	bouncerValveOpenSwitch.update();
	bouncerValveCloseSwitch.update();

	valveOpenSwitch.setState(bouncerValveOpenSwitch.read());
	valveCloseSwitch.setState(bouncerValveCloseSwitch.read());

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

	Serial.println(F("Closing valve"));
	if (!valveCloseSwitch.getCurrentState())
	{
		SetHBridge(0, -1); // Start closing
		delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
		SetHBridge(0, 0); // remove power from ball valves
	}

	ball_valve_state = BALL_VALVE_FULLY_CLOSED;
	valveState.setValue("Closed");
	Serial.println(F("Closed"));
}

static void setBallValve(bool value)
{
	static unsigned long prevOffSeconds = 0;

	if (value > 0)
	{
		if (ball_valve_state <= 0 && secondTicks < (prevOffSeconds + BALL_VALVE_ON_DELAY_SEC)) // 5 min
		{
			Serial.print(F("Delaying ball valve ON. Seconds left:"));
			Serial.println((prevOffSeconds + BALL_VALVE_ON_DELAY_SEC) - secondTicks);
			return;
		}

		// if tank is full (at least one sensors)
		if (getWaterTankPercent() >= 100)
		{
			Serial.println("Tank is full");
			return;
		}
	}

	// If open is requested and valve is closed (or closing) OR 
	// if close is requested and valve is open (or opening)
	if ((value && ball_valve_state < 0) || (!value && ball_valve_state > 0))
	{
		if (value)
		{
			openBallValve();
		}
		else
		{
			prevOffSeconds = secondTicks;
			closeBallValve();
		}
	}
}

static void closeBallValve()
{
	Serial.println("Closing ball valve");
	ball_valve_state = -BALL_VALVE_OPEN_CLOSE_SECONDS;
	SetHBridge(0, -1); // Negative, start closing
	valveState.setValue("Closing");
}

static void openBallValve()
{
	Serial.println("Opening ball valve");
	ball_valve_state = BALL_VALVE_OPEN_CLOSE_SECONDS;
	SetHBridge(0, 1); // Positive, start opening
	valveState.setValue("Opening");
}

static void processBallValve()
{
	if (ball_valve_state == BALL_VALVE_FULLY_CLOSED || ball_valve_state == BALL_VALVE_FULLY_OPEN)
		return;

	bool opening;
	if (ball_valve_state > 0)
	{
		ball_valve_state--;
		opening = true;
	}
	else
	{
		ball_valve_state++;
		opening = false;
	}

	if (ball_valve_state == 0)
	{
		SetHBridge(0, 0); // Neutral
		if (opening)
		{
			ball_valve_state = BALL_VALVE_FULLY_OPEN; // Open
			valveState.setValue("Open");
		}
		else
		{
			ball_valve_state = BALL_VALVE_FULLY_CLOSED; // Closed
			valveState.setValue("Closed");
		}
	}
}

static void processBallValveSwitches()
{
	if (bouncerValveOpenSwitch.update())
		valveOpenSwitch.setState(bouncerValveOpenSwitch.read());
	if (bouncerValveCloseSwitch.update())
		valveCloseSwitch.setState(bouncerValveCloseSwitch.read());
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
