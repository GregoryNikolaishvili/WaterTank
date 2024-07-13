// https://wiki.dfrobot.com/Throw-in_Type_Liquid_Level_Transmitter_SKU_KIT0139
// Output Signal : 4-20mA

// https://wiki.dfrobot.com/Gravity__Analog_Current_to_Voltage_Converter_for_4~20mA_Application__SKU_SEN0262
// Detection Range : 0~25mA DC
// This current-to-voltage module linearly converts 0-25mA current signals into 0-3V voltage signals. Industrial sensors or devices usually have the current signal output of 4-20 mA. 
// With this current-to-voltage module, your main control board can easily read the current signals output from industrial sensors or devices. 
// Normally, current signals lower than 4mA can be used for fault diagnosis, and current signals higher than 20mA can be used for overrun detection. 
// Therefore, this module is designed with a wide range of 0-25mA detection range, which is compatible with fault detection, overrun detection applications. 

static const long REFERENCE_VOLTAGE_MV = 1100; // Internal 1.1V = 1100 mV 

MovingAverageFilter voltageAvg(10, VOLTAGE_ERR_VALUE);

extern MinMaxVoltageSettingStructure tankVoltageSettings;

static void initPressureSensor()
{
	analogReference(INTERNAL1V1);
	pinMode(PIN_PRESSURE_SENSOR_BIG, INPUT);
}

static void processPressureSensor()
{
	int milliVolts = readVoltageMV();
	
	int oldValue = voltageAvg.getCurrentValue();
	int newValue = voltageAvg.process(milliVolts);

	newValue /= 10;
	newValue *= 10; // Round

	pressureSensor.setValue(newValue);
}

static int readVoltageMV()
{
	int sum = 0;
	for (byte i = 0; i < 16; i++)
	{
		sum += analogRead(PIN_PRESSURE_SENSOR_BIG);
		mqtt.loop();
	}

	return (sum * REFERENCE_VOLTAGE_MV) / 16 / 1024;
}

static int getWaterTankPercent()
{
	int voltage = (voltageAvg.getCurrentValue() / 10) * 10; // Round

	// Divide all voltages by 10 and compare with empty and full values
	voltage = voltage / 10;
	int empty = tankVoltageSettings.empty / 10;
	int full = tankVoltageSettings.full / 10;

	if ((voltage < empty - 100) || (voltage > full + 100)) // mV
		return PERCENT_ERR_VALUE;

	if (voltage <= empty)
		return 0;
	if (voltage >= full)
		return 100;
	return (voltage - empty) * (long)100 / (full - empty);
}
