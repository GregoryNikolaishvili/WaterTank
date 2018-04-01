#include "WaterTank.h"

#include <Time.h>					// http://www.pjrc.com/teensy/td_libs_Time.html
#include <DS1307RTC.h>				// http://www.pjrc.com/teensy/td_libs_DS1307RTC.html

#include <NewPing.h>				// https://code.google.com/p/arduino-new-ping/
#include <Bounce.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>			// https://github.com/knolleary/pubsubclient

unsigned long halfSecondTicks = 0;
unsigned long secondTicks = 0;

unsigned int waterLevelControllerState = 0;

Bounce bouncerWL1(PIN_FLOAT_SWITCH_1, false, 10 * 1000UL, 10 * 1000UL); // 10 sec, 10 sec
Bounce bouncerWL2(PIN_FLOAT_SWITCH_2, false, 10 * 1000UL, 10 * 1000UL); // 10 sec, 10 sec
Bounce bouncerWL3(PIN_FLOAT_SWITCH_3, false, 10 * 1000UL, 10 * 1000UL); // 10 sec, 10 sec

bool solenoid_states[TANK_COUNT];
bool float_switch_states[TANK_COUNT];

int ultrasound_sensor_distances[TANK_COUNT];
int ultrasound_sensor_percents[TANK_COUNT];

SettingStructure settings[TANK_COUNT];


void setup()
{
	Serial.begin(115200);
	Serial.println("Initializing..");

	pinMode(PIN_BLINKING_LED, OUTPUT);
	digitalWrite(PIN_BLINKING_LED, LOW); // Turn on led at start

	for (byte id = 0; id < TANK_COUNT; id++)
	{
		float_switch_states[id] = true;
		ultrasound_sensor_distances[id] = MAX_DISTANCE;

		solenoid_states[id] = true; // say that it's open and close it
		setSolenoid(id, false);
	}

	pinMode(PIN_SOLENOID_IN1, OUTPUT);
	pinMode(PIN_SOLENOID_IN2, OUTPUT);

	pinMode(PIN_FLOAT_SWITCH_1, INPUT_PULLUP);
	pinMode(PIN_FLOAT_SWITCH_2, INPUT_PULLUP);
	pinMode(PIN_FLOAT_SWITCH_3, INPUT_PULLUP);

	readSettings();

	initUltrasonicSensors();

	InitEthernet();

	delay(2000);

	InitMqtt();

	Serial.println(F("Start"));

	processWaterLevels(); // duplicate. same is in loop
}

void loop()
{
	static unsigned long previousMillis = 0; // will store last time LED was updated
	unsigned long _current_millis = millis();

	uint32_t dt = previousMillis > _current_millis ? 1 + previousMillis + ~_current_millis : _current_millis - previousMillis;

	//if ((_current_millis < previousMillis) || (_current_millis - previousMillis >= 500))
	if (dt >= 500)
	{
		// save the last time we blinked the LED
		previousMillis = _current_millis;
		oncePerHalfSecond();
	}

	ProcessMqtt();
	//Alarm.delay(0);
}

void oncePerHalfSecond(void)
{
	halfSecondTicks++;

	// Blinking
	static uint8_t blinkingLedState = LOW;

	blinkingLedState = !blinkingLedState;
	digitalWrite(PIN_BLINKING_LED, blinkingLedState);

	//if ((halfSecondTicks - 2) % PROCESS_INTERVAL_TEMPERATURE_SENSOR_HALF_SEC == 0) // 1 second before processing temperatures
	//	solarSensor.readRTD_step1();
	//if ((halfSecondTicks - 1) % PROCESS_INTERVAL_TEMPERATURE_SENSOR_HALF_SEC == 0) // 0.5 second before processing temperatures
	//	solarSensor.readRTD_step2();
	//if (halfSecondTicks % PROCESS_INTERVAL_TEMPERATURE_SENSOR_HALF_SEC == 0)
	//{
	//	solarSensor.readRTD_step3();
	//	ProcessBoilerSensors();
	//}


	if ((halfSecondTicks % 2) == 0)
		oncePerSecond();
}

void oncePerSecond()
{
	if ((secondTicks % 5) == 0)
		oncePer5Second();

	secondTicks++;

	if ((secondTicks % 60) == 0)
		oncePer1Minute();

}

void oncePer5Second()
{
	processWaterLevels();

	ReconnectMqtt();
}


void oncePer1Minute()
{
	if (secondTicks > 0) // do not publish on startup
		PublishAllStates(true, false);
}


boolean state_set_error_bit(int mask)
{
	if (!state_is_error_bit_set(mask))
	{
		waterLevelControllerState |= mask;
		PublishControllerState();
		return true;
	}

	return false;
}


boolean state_clear_error_bit(int mask)
{
	if (state_is_error_bit_set(mask))
	{
		waterLevelControllerState &= ~mask;
		PublishControllerState();
		return true;
	}

	return false;
}

// called every second
void processWaterLevels()
{
	bouncerWL1.update();
	bouncerWL2.update();
	bouncerWL3.update();

	setFloatSwitchState(FLOAT_SWITCH_1, bouncerWL1.read());
	setFloatSwitchState(FLOAT_SWITCH_2, bouncerWL2.read());
	setFloatSwitchState(FLOAT_SWITCH_3, bouncerWL3.read());

	processUltrasonicSensors();

	for (byte id = 0; id < TANK_COUNT; id++)
		processTankWL(id);
}

void processTankWL(byte id)
{
	static unsigned long prevSumpFullSeconds[TANK_COUNT] = { 0, 0, 0 };

	boolean b1 = ultrasound_sensor_distances[id] <= settings[id].MaxDistance;  // 07FFF = Error and should be considered as full & empty at the same time
	boolean b2 = float_switch_states[id];

	if (b1 && b2) // if both are on, turn off solenoid immediatley
		setSolenoid(id, false);
	else
		if (b1 || b2) // if at least one is on 
		{
			if ((prevSumpFullSeconds[id] > 0) && (secondTicks - prevSumpFullSeconds[id]) >= SOLENOID_OFF_DELAY_SEC) // if 10 min passed since last time when at least one was on, turn off solenoid
				setSolenoid(id, false);
		}

	if (b1 || b2) // if at least one is on 
	{
		if (prevSumpFullSeconds[id] == 0)
			prevSumpFullSeconds[id] = secondTicks;
	}
	else
	{
		prevSumpFullSeconds[id] = 0;
		setSolenoid(id, true);
	}
}

void setFloatSwitchState(byte id, bool value)
{
	if (float_switch_states[id] != value)
	{
		float_switch_states[id] = value;

		PublishSensorState(id, false);

		Serial.print(F("Float switch #"));
		Serial.print(id + 1);
		Serial.print(F(" = "));
		Serial.println(value);
	}
}

bool isTankFull(byte id)
{
	return float_switch_states[id] || (ultrasound_sensor_distances[id] == MAX_DISTANCE) || (ultrasound_sensor_distances[id] <= settings[id].MaxDistance);
}