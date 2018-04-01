const int AQUA_FULL_DISTANCE = 110; // mm
const int AQUA_EMPTY_DISTANCE = 550; // mm

const int SUMP_FULL_DISTANCE = 110; // mm
const int SUMP_EMPTY_DISTANCE = 350; // mm

const int HOSP_FULL_DISTANCE = 110; // mm
const int HOSP_EMPTY_DISTANCE = 350; // mm


NewPing us_sensor1(PIN_ULTRASONIC_1_TX, PIN_ULTRASONIC_1_RX, 300); // NewPing setup of pins and maximum distance.
NewPing us_sensor2(PIN_ULTRASONIC_2_TX, PIN_ULTRASONIC_2_RX, 300);
NewPing us_sensor3(PIN_ULTRASONIC_3_TX, PIN_ULTRASONIC_3_RX, 300);

extern SettingStructure settings[TANK_COUNT];

void initUltrasonicSensors()
{
	state_clear_error_bit(ERR_ULTRASONIC_1 | ERR_ULTRASONIC_2 | ERR_ULTRASONIC_3);
}

void processUltrasonicSensors()
{
	int  distance = us_sensor1.ping_cm();
	if (distance <= 0)
	{
		setUltrasoundSensorState(ULTRASOUND_SENSOR_1, MAX_DISTANCE);
		state_set_error_bit(ERR_ULTRASONIC_1);
	}
	else
	{
		setUltrasoundSensorState(ULTRASOUND_SENSOR_1, distance);
		state_clear_error_bit(ERR_ULTRASONIC_1);
	}

	distance = us_sensor2.ping_cm();
	if (distance <= 0)
	{
		setUltrasoundSensorState(ULTRASOUND_SENSOR_2, MAX_DISTANCE);
		state_set_error_bit(ERR_ULTRASONIC_2);
	}
	else
	{
		setUltrasoundSensorState(ULTRASOUND_SENSOR_2, distance);
		state_clear_error_bit(ERR_ULTRASONIC_2);
	}

	us_sensor3.ping_cm();
	if (distance <= 0)
	{
		setUltrasoundSensorState(ULTRASOUND_SENSOR_3, MAX_DISTANCE);
		state_set_error_bit(ERR_ULTRASONIC_3);
	}
	else
	{
		setUltrasoundSensorState(ULTRASOUND_SENSOR_3, distance);
		state_clear_error_bit(ERR_ULTRASONIC_3);
	}
}

void setUltrasoundSensorState(byte id, int value)
{
	if (ultrasound_sensor_distances[id] != value)
	{
		ultrasound_sensor_distances[id] = value;

		setWaterLevelPercent(id, value);

		PublishSensorState(id, false);

		Serial.print(F("Ultrasonic #"));
		Serial.print(id + 1);
		Serial.print(F(" = "));
		Serial.print(value);
		Serial.print(F(" cm, "));
		Serial.print(ultrasound_sensor_distances[id]);
		Serial.println(F("%"));
	}
}


void setWaterLevelPercent(int id, int distance)
{
	int perc = 0;
	if (distance == MAX_DISTANCE)
		perc = 255;
	else
	if (distance < settings[id].MaxDistance)
		perc = 100 - 100 * (distance - settings[id].MinDistance) / (settings[id].MaxDistance - settings[id].MinDistance);
	ultrasound_sensor_percents[id] = perc;
}


void recalcWaterLevelPercents()
{
	for (byte id = 0; id < TANK_COUNT; id++)
	{
		setWaterLevelPercent(id, ultrasound_sensor_distances[id]);

		PublishSensorState(id, false);
	}
}