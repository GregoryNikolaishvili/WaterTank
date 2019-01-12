byte solenoidPins[2 * TANK_COUNT] = {
	PIN_SOLENOID_IN1,
	PIN_SOLENOID_IN2,
	PIN_SOLENOID_IN3,
	PIN_SOLENOID_IN4,
	PIN_SOLENOID_IN5,
	PIN_SOLENOID_IN6
};

void setSolenoid(byte id, bool value)
{
	static unsigned long prevSolenoidOffSeconds[TANK_COUNT] = { 0, 0, 0 };

	if (value)
	{
		if (secondTicks < (prevSolenoidOffSeconds[id] + SOLENOID_ON_DELAY_SEC)) // 5 min
			return;

		// if water is on floor 
		//	return false;

		// if tank is full (at least one sensors)
		if (isTankFull(id))
			return;
	}


	if (value != solenoid_states[id])
	{
		byte pinIn1 = solenoidPins[id] * 2;
		byte pinIn2 = solenoidPins[id] * 2 + 1;

		digitalWrite(pinIn1, LOW);
		digitalWrite(pinIn2, LOW);
		delay(10);
		if (value)
			digitalWrite(pinIn1, HIGH);
		else
			digitalWrite(pinIn2, HIGH);

		delay(100);

		digitalWrite(pinIn1, LOW);
		digitalWrite(pinIn2, LOW);

		if (!value)
			prevSolenoidOffSeconds[id] = secondTicks;

		solenoid_states[id] = value;

		PublishSensorState(id);

		Serial.print(F("Solenoid #"));
		Serial.print(id);
		Serial.print(F(": "));
		Serial.println(value);
	}
}


