const int AQUA_FULL_DISTANCE = 110; // mm
const int AQUA_EMPTY_DISTANCE = 550; // mm

const int SUMP_FULL_DISTANCE = 110; // mm
const int SUMP_EMPTY_DISTANCE = 350; // mm

const int HOSP_FULL_DISTANCE = 110; // mm
const int HOSP_EMPTY_DISTANCE = 350; // mm

byte currentSonarId = 0;

NewPing sonars[TANK_COUNT] = {
	NewPing(PIN_ULTRASONIC_1_TX, PIN_ULTRASONIC_1_RX, 250), // NewPing setup of pins and maximum distance.
	NewPing(PIN_ULTRASONIC_2_TX, PIN_ULTRASONIC_2_RX, 250),
	NewPing(PIN_ULTRASONIC_3_TX, PIN_ULTRASONIC_3_RX, 250)
};

extern SettingStructure settings[TANK_COUNT];

void initUltrasonicSensors()
{
	state_clear_error_bit(ERR_ULTRASONIC_1 | ERR_ULTRASONIC_2 | ERR_ULTRASONIC_3);
}

void startMeasuringWaterLevel(byte sonarId)
{
	//Serial.print("Start measuring sensor #");
	//Serial.println(sonarId + 1);
	
	sonars[currentSonarId].timer_stop();
	currentSonarId = sonarId;
	sonars[sonarId].ping_timer(echoCheck);
}

void echoCheck() {
	//Serial.print("Checking sensor #");
	//Serial.println(currentSonarId + 1);

	if (sonars[currentSonarId].check_timer())
	{
		int  distance = sonars[currentSonarId].ping_result / US_ROUNDTRIP_CM;
		if (distance == 32767)
			distance = -1;

		Serial.print("Result of sensor #");
		Serial.print(currentSonarId + 1);
		Serial.print(": ");
		Serial.println(distance);

	if (distance <= 0)
	{
			setUltrasoundSensorState(currentSonarId, MAX_DISTANCE);
			//state_set_error_bit(1 >> (currentSonarId + 1));
	}
	else
	{
			setUltrasoundSensorState(currentSonarId, distance);
			//state_clear_error_bit(1 >> (currentSonarId + 1));
	}
	}
	else
	{
		//Serial.print("Waiting for sensor #");
		//Serial.println(currentSonarId + 1);
	}
}

void processUltrasonicSensors()
{
	for (byte id = 0; id < TANK_COUNT; id++)
	{
		int  distance = sonars[id].ping_cm();
		
	if (distance <= 0)
	{
			setUltrasoundSensorState(id, MAX_DISTANCE);
			state_clear_error_bit(1 >> (id + 1));
	}
	else
	{
			setUltrasoundSensorState(id, distance);
			state_clear_error_bit(1 >> (id + 1));
		}
	}
}

void setUltrasoundSensorState(byte id, int value)
{
	if (ultrasound_sensor_distances[id] != value)
	{
		ultrasound_sensor_distances[id] = value;

		setWaterLevelPercent(id, value);

		PublishSensorState(id);

		Serial.print(F("Ultrasonic #"));
		Serial.print(id + 1);
		Serial.print(F(" = "));
		Serial.print(value);
		Serial.print(F(" cm, "));
		Serial.print(ultrasound_sensor_percents[id]);
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

		PublishSensorState(id);
	}
}