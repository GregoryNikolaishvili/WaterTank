// https://wiki.dfrobot.com/Throw-in_Type_Liquid_Level_Transmitter_SKU_KIT0139
// Output Signal : 4-20mA

// https://wiki.dfrobot.com/Gravity__Analog_Current_to_Voltage_Converter_for_4~20mA_Application__SKU_SEN0262
// Detection Range : 0~25mA DC
// This current-to-voltage module linearly converts 0-25mA current signals into 0-3V voltage signals. Industrial sensors or devices usually have the current signal output of 4-20 mA. 
// With this current-to-voltage module, your main control board can easily read the current signals output from industrial sensors or devices. 
// Normally, current signals lower than 4mA can be used for fault diagnosis, and current signals higher than 20mA can be used for overrun detection. 
// Therefore, this module is designed with a wide range of 0-25mA detection range, which is compatible with fault detection, overrun detection applications. 


#define REFERENCE_VOLTAGE 1.1 // Internal

MovingAverageFilter depthsAvg[TANK_COUNT] =
{
	MovingAverageFilter(10, DEPTH_ERR_VALUE),
	MovingAverageFilter(10, DEPTH_ERR_VALUE)
};

int waterLevelDepths[TANK_COUNT]
{
	DEPTH_ERR_VALUE,
	DEPTH_ERR_VALUE
};

int waterLevelPercents[TANK_COUNT]
{
	0,
	0
};

void initPressureSensors()
{
	analogReference(INTERNAL1V1);
	pinMode(PIN_PRESSURE_SENSOR_SMALL, INPUT);
	pinMode(PIN_PRESSURE_SENSOR_BIG, INPUT);
}

void processPressureSensors()
{
	float voltageMV;
	int depthCM;
	// < 4 ma == Error
	// 0 ma == 0V
	// 4 ma == 480mV == 0cm
	// 20 ma == 2400mV == 500cm
	// Sense Resistor: 120ohm, 2400mv / 20ma = 120

	voltageMV = analogRead(PIN_PRESSURE_SENSOR_SMALL) * REFERENCE_VOLTAGE / 1024.0;
	depthCM = (voltageMV - 0.48) * 500.0 / (2.4 - 0.48);
	setPressureSensorState(0, depthCM);

  Serial.print(F("Voltage #1"));
  Serial.print(F(" = "));
  Serial.println(voltageMV);
  
	voltageMV = analogRead(PIN_PRESSURE_SENSOR_BIG) * REFERENCE_VOLTAGE / 1024.0;
	depthCM = (voltageMV - 0.48) * 500.0 / (2.4 - 0.48);
	setPressureSensorState(1, depthCM);

  Serial.print(F("Voltage #2"));
  Serial.print(F(" = "));
  Serial.println(voltageMV);
}

int getDeptValueOrError(byte id)
{
  int value = waterLevelDepths[id];

  if (value < -10 || value > 500) // error
    return DEPTH_ERR_VALUE;
  
  if (value < 0) // Zero?
    return 0;
  
  return value;
}

void setPressureSensorState(byte id, int value)
{
  Serial.print(F("Water pressure #"));
  Serial.print(id + 1);
  Serial.print(F(" Value = "));
  Serial.print(value);
  
	waterLevelDepths[id] = value;

	value = getDeptValueOrError(id);

	int oldValue = depthsAvg[id].getCurrentValue();
	int newValue = depthsAvg[id].process(value);

	if (oldValue != newValue || value == DEPTH_ERR_VALUE)
	{
		setWaterLevelPercent(id, newValue);

		PublishTankState(id);
	}

  Serial.print(F(", Depth = "));
  Serial.print(newValue);
  Serial.print(F(" cm, "));
  Serial.print(waterLevelPercents[id]);
  Serial.println(F("%"));
}


int getPressureSensorState(byte id)
{
	return waterLevelDepths[id];
}

int getWaterTankPercent(byte id)
{
	return waterLevelPercents[id];
}


void setWaterLevelPercent(int id, int depth)
{
	int perc = 100;
	if (depth == DEPTH_ERR_VALUE)
		perc = 255;
	else
		if (depth <= maxDepthSettings[id])
			perc = 100 - 100 * (maxDepthSettings[id] - depth) / maxDepthSettings[id];
	waterLevelPercents[id] = perc;
}

void recalcWaterLevelPercents()
{
	for (byte id = 0; id < TANK_COUNT; id++)
	{
		setWaterLevelPercent(id, waterLevelDepths[id]));

		PublishTankState(id);
	}
}
