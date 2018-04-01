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
		digitalWrite(PIN_SOLENOID_IN1, LOW);
		digitalWrite(PIN_SOLENOID_IN2, LOW);
		delay(10);
		if (value)
			digitalWrite(PIN_SOLENOID_IN1, HIGH);
		else
			digitalWrite(PIN_SOLENOID_IN2, HIGH);

		delay(100);

		digitalWrite(PIN_SOLENOID_IN1, LOW);
		digitalWrite(PIN_SOLENOID_IN2, LOW);

		if (!value)
			prevSolenoidOffSeconds[id] = secondTicks;

		solenoid_states[id] = value;
		
		PublishSensorState(id, false);

		Serial.print(F("Solenoid #"));
		Serial.print(id);
		Serial.print(F(": "));
		Serial.println(value);
	}
}


