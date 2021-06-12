// https://wiki.dfrobot.com/Throw-in_Type_Liquid_Level_Transmitter_SKU_KIT0139
// Output Signal : 4-20mA

// https://wiki.dfrobot.com/Gravity__Analog_Current_to_Voltage_Converter_for_4~20mA_Application__SKU_SEN0262
// Detection Range : 0~25mA DC
// This current-to-voltage module linearly converts 0-25mA current signals into 0-3V voltage signals. Industrial sensors or devices usually have the current signal output of 4-20 mA. 
// With this current-to-voltage module, your main control board can easily read the current signals output from industrial sensors or devices. 
// Normally, current signals lower than 4mA can be used for fault diagnosis, and current signals higher than 20mA can be used for overrun detection. 
// Therefore, this module is designed with a wide range of 0-25mA detection range, which is compatible with fault detection, overrun detection applications. 


static const long REFERENCE_VOLTAGE = 1100; // Internal 1.1V = 1100 mV 

MovingAverageFilter voltageAvg[TANK_COUNT] =
{
	MovingAverageFilter(10, VOLTAGE_ERR_VALUE),
	MovingAverageFilter(10, VOLTAGE_ERR_VALUE)
};

const uint8_t PressureSensorPins[TANK_COUNT] = { PIN_PRESSURE_SENSOR_BIG , PIN_PRESSURE_SENSOR_SMALL };

extern MinMaxVoltageSettingStructure tankVoltageSettings[TANK_COUNT];

void initPressureSensors()
{
	analogReference(INTERNAL1V1);
	pinMode(PIN_PRESSURE_SENSOR_BIG, INPUT);
	pinMode(PIN_PRESSURE_SENSOR_SMALL, INPUT);
}

void processPressureSensors()
{
	for (byte id = 0; id < TANK_COUNT; id++)
	{
		int value = readVoltage(id);
		setPressureSensorState(id, value);
	}
}

int readVoltage(int id)
{
	uint8_t pin = PressureSensorPins[id];
	int sum = 0;

	for (byte i = 0; i < 16; i++)
	{
		sum += analogRead(pin);
		ProcessMqtt();
	}

	return ((sum / 16) * REFERENCE_VOLTAGE) / 1024;
}

void setPressureSensorState(byte id, int value)
{
	int oldValue = voltageAvg[id].getCurrentValue();
	int newValue = voltageAvg[id].process(value);

  newValue /= 10;
  newValue *= 10; // Round
  
	if (oldValue != newValue)
	{
		PublishTankState(id);
	}

//  Serial.print(F("Voltage #"));
//  Serial.print(id);
//  Serial.print(F(" = "));
//  Serial.print(value);
//  Serial.print(F(" mV, Avg = "));
//	Serial.print(newValue);
//	Serial.println(F(" mV"));
}


int getPressureSensorState(byte id)
{
	return (voltageAvg[id].getCurrentValue() / 10) * 10; // 1% accuracy is enough 
}

int getWaterTankPercent(byte id)
{
	int voltage = getPressureSensorState(id) / 10; 
	int empty = tankVoltageSettings[id].empty / 10; // 1% accuracy is enough
	int full = tankVoltageSettings[id].full / 10;

	if ((voltage < empty - 100) || (voltage > full + 100)) // mV
		return PERCENT_ERR_VALUE;

	if (voltage <= empty)
		return 0;
	if (voltage >= full)
		return 100;
	return (voltage - empty) * (long)100 / (full - empty);
}
