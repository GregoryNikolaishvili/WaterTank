#define REQUIRESALARMS false // FOR DS18B20 library
#define MQTT_MAX_PACKET_SIZE 256 // FOR PubSubClient library
#define MQTT_SOCKET_TIMEOUT 5 // FOR PubSubClient library

#include "WaterTank.h"

#include <TimeLib.h>				// https://github.com/PaulStoffregen/Time
#include <DS1307RTC.h>				// https://github.com/PaulStoffregen/DS1307RTC

#include <NewPing.h>				// https://code.google.com/p/arduino-new-ping/
#include <Bounce.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>			// https://github.com/knolleary/pubsubclient

#include <avr/wdt.h>

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
	wdt_disable();

	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Initializing.. ver. 0.0.2"));

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
	pinMode(PIN_SOLENOID_IN3, OUTPUT);
	pinMode(PIN_SOLENOID_IN4, OUTPUT);
	pinMode(PIN_SOLENOID_IN5, OUTPUT);
	pinMode(PIN_SOLENOID_IN6, OUTPUT);

	pinMode(PIN_FLOAT_SWITCH_1, INPUT_PULLUP);
	pinMode(PIN_FLOAT_SWITCH_2, INPUT_PULLUP);
	pinMode(PIN_FLOAT_SWITCH_3, INPUT_PULLUP);

	readSettings();

	initUltrasonicSensors();

	InitEthernet();

	delay(2000);

	InitMqtt();

	//startMeasuringWaterLevel(0);
//	delay(500);
	//startMeasuringWaterLevel(1);
//	delay(500);
	//startMeasuringWaterLevel(2);
//	delay(500);

	processWaterLevels(); // duplicate. same is in loop

	Serial.println(F("Start"));

	wdt_enable(WDTO_8S);
}

void loop()
{
	static unsigned long previousMillis = 0; // will store last time LED was updated
	unsigned long _current_millis = millis();

	uint32_t dt = previousMillis > _current_millis ? 1 + previousMillis + ~_current_millis : _current_millis - previousMillis;

	if (dt >= 500)
	{
		wdt_reset();

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

	//if ((halfSecondTicks + 3) % 10 == 0) // 1.5 second before processing water levels
	//	startMeasuringWaterLevel(0);
	//else
	//	if ((halfSecondTicks + 2) % 10 == 0) // 1 second before processing water levels
	//		startMeasuringWaterLevel(1);
	//	else
	//		if ((halfSecondTicks + 1) % 10 == 0) // 0.5 second before processing temperatures
	//			startMeasuringWaterLevel(2);

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
		PublishAllStates(true);
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

// called every 5 second
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

	//TODO
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

		PublishSensorState(id);

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
