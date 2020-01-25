#define REQUIRESALARMS false // FOR DS18B20 library
#define MQTT_MAX_PACKET_SIZE 300 // FOR PubSubClient library
#define MQTT_SOCKET_TIMEOUT 5 // FOR PubSubClient library

#include "WaterTank.h"

#include <TimeLib.h>				// https://github.com/PaulStoffregen/Time
#include <DS1307RTC.h>				// https://github.com/PaulStoffregen/DS1307RTC

#include <NewPing.h>				// https://code.google.com/p/arduino-new-ping/
#include <Bounce.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>			// https://github.com/knolleary/pubsubclient

#include <MovingAverageFilter.h>
#include <avr/wdt.h>

unsigned long halfSecondTicks = 0;
unsigned long secondTicks = 0;

unsigned int waterLevelControllerState = 0;

Bounce floatSwitch1(PIN_FLOAT_SWITCH_1, false, 60 * 1000UL, 60 * 1000UL); // 60 sec, 60 sec
Bounce floatSwitch2(PIN_FLOAT_SWITCH_2, false, 60 * 1000UL, 60 * 1000UL); // 60 sec, 60 sec
Bounce floatSwitch3(PIN_FLOAT_SWITCH_3, false, 60 * 1000UL, 60 * 1000UL); // 60 sec, 60 sec

Bounce bouncerBV1Open(PIN_BALL_VALVE1_OPEN, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerBV1Close(PIN_BALL_VALVE1_CLOSED, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerBV2Open(PIN_BALL_VALVE2_OPEN, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerBV2Close(PIN_BALL_VALVE2_CLOSED, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerBV3Open(PIN_BALL_VALVE3_OPEN, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerBV3Close(PIN_BALL_VALVE3_CLOSED, false, 500UL, 500UL); // 0.5 sec, 0.5 sec

bool float_switch_states[TANK_COUNT];

int ultrasound_sensor_distances[TANK_COUNT];
int ultrasound_sensor_percents[TANK_COUNT];

static byte relayPins[RELAY_COUNT] = {
  PIN_RELAY_CLEAN_WATER_PUMP,
  PIN_RELAY_TECH_WATER_PUMP,
  PIN_RELAY_GARDEN_PUMP,
  PIN_RELAY_RESERVE_3,
  PIN_RELAY_RESERVE_4
};


SettingStructure settings[TANK_COUNT];


void setup()
{
	wdt_disable();

	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Initializing.. ver. 1.0.6"));

	pinMode(PIN_BLINKING_LED, OUTPUT);
	digitalWrite(PIN_BLINKING_LED, LOW); // Turn on led at start

	// Init relays
	for (byte i = 0; i < RELAY_COUNT; i++)
	{
		digitalWrite(relayPins[i], HIGH);
		pinMode(relayPins[i], OUTPUT);
	}

	//if (dtNBR_ALARMS != 30)
	//	Serial.println("Alarm count mismatch");

	for (byte id = 0; id < TANK_COUNT; id++)
	{
		ultrasound_sensor_distances[id] = MAX_DISTANCE;
	}

	//pinMode(PIN_FLOAT_SWITCH_1, INPUT_PULLUP);
	//pinMode(PIN_FLOAT_SWITCH_2, INPUT_PULLUP);
	//pinMode(PIN_FLOAT_SWITCH_3, INPUT_PULLUP);

	//pinMode(PIN_BALL_VALVE1_OPEN, INPUT_PULLUP);
	//pinMode(PIN_BALL_VALVE1_CLOSED, INPUT_PULLUP);
	//pinMode(PIN_BALL_VALVE2_OPEN, INPUT_PULLUP);
	//pinMode(PIN_BALL_VALVE2_CLOSED, INPUT_PULLUP);
	//pinMode(PIN_BALL_VALVE3_OPEN, INPUT_PULLUP);
	//pinMode(PIN_BALL_VALVE3_CLOSED, INPUT_PULLUP);

	InitializeFloatSwitches();
	InitializeBallValves();

	readSettings();

	initUltrasonicSensors();

	InitEthernet();

	startMeasuringWaterLevel(0);
	delay(500);
	startMeasuringWaterLevel(1);
	delay(500);
	startMeasuringWaterLevel(2);
	delay(500);

	processWaterLevels(); // duplicate. same is in loop

	InitMqtt();

	wdt_enable(WDTO_8S);

	Serial.println(F("Start"));
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

	processBallValveSwitches();

	if ((halfSecondTicks + 3) % PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC == 0) // 1.5 second before processing water levels
		startMeasuringWaterLevel(0);
	else
		if ((halfSecondTicks + 2) % PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC == 0) // 1 second before processing water levels
			startMeasuringWaterLevel(1);
		else
			if ((halfSecondTicks + 1) % PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC == 0) // 0.5 second before processing water levels
				startMeasuringWaterLevel(2);

	if (halfSecondTicks % PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC == 0)
	{
		processWaterLevels();
	}


	if ((halfSecondTicks % 2) == 0)
		oncePerSecond();
}

void oncePerSecond()
{
	if ((secondTicks % 5) == 0)
		oncePer5Second();

	secondTicks++;

	processBallValve();

	if ((secondTicks % 60) == 0)
		oncePer1Minute();
}

void oncePer5Second()
{
	ReconnectMqtt();
}


void oncePer1Minute()
{
	if (secondTicks > 0) // do not publish on startup
		PublishAllStates(false);
}

bool InvertRelayState(byte id, bool state)
{
	if (id == 1 || id == 2)
		return !state;

	return state;
}

void relaySet(byte id, bool state)
{
	if (state)
		relayOn(id);
	else
		relayOff(id);
}

void relayOn(byte id)
{
	if (id < RELAY_COUNT)
	{
		digitalWrite(relayPins[id], InvertRelayState(id, LOW));
		PublishRelayState(id, true);
	}
}

void relayOff(byte id)
{
	if (id < RELAY_COUNT)
	{
		digitalWrite(relayPins[id], InvertRelayState(id, HIGH));
		PublishRelayState(id, false);
	}
}

bool isRelayOn(byte id)
{
	if (id < RELAY_COUNT)
		return InvertRelayState(id, !digitalRead(relayPins[id]));
	return false;
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

void processFloatSwitches()
{
	if (floatSwitch1.update())
		setFloatSwitchState(0, floatSwitch1.read());
	if (floatSwitch2.update())
		setFloatSwitchState(1, floatSwitch2.read());
	if (floatSwitch3.update())
		setFloatSwitchState(2, floatSwitch3.read());
}

void processBallValveSwitches()
{
	if (bouncerBV1Open.update() || bouncerBV1Close.update())
		setBallValveSwitchState(0, bouncerBV1Open.read(), bouncerBV1Close.read());
	if (bouncerBV2Open.update() || bouncerBV2Close.update())
		setBallValveSwitchState(1, bouncerBV2Open.read(), bouncerBV2Close.read());
	if (bouncerBV3Open.update() || bouncerBV3Close.update())
		setBallValveSwitchState(2, bouncerBV3Open.read(), bouncerBV3Close.read());
}

// called every 5 second
void processWaterLevels()
{
	processBallValveSwitches();
	processFloatSwitches();

	//processUltrasonicSensors();
	for (byte id = 0; id < TANK_COUNT; id++)
		processTankWL(id);
}

void processTankWL(byte id)
{
	int distance = GetIsrSonarDistance(id);

	setUltrasoundSensorState(id, distance);

	//Serial.print("Ultrasonic: ");
	//Serial.print(id);
	//Serial.print(" = ");
	//Serial.println(distance);

	static unsigned long prevFullSeconds[] = { 0, 0, 0, 0 };

	//boolean err = distance == MAX_DISTANCE;
	boolean b1 = distance <= settings[id].MinDistance;  // 07FFF = Error and should be considered as full & empty at the same time
	boolean b2 = float_switch_states[id];

	//Serial.print(F("Id = "));
	//Serial.print(id);
	//Serial.print(F(", Distance = "));
	//Serial.print(distance);
	//Serial.print(F(", Min = "));
	//Serial.print(settings[id].MinDistance);
	//Serial.print(F(", B1 = "));
	//Serial.print(b1);
	//Serial.print(F(", B2 = "));
	//Serial.println(b2);

	if (b1 && b2) // if both are on, turn off solenoid immediatley
	{
		setBallValve(id, false); // Close

		if (prevFullSeconds[id] == 0)
			prevFullSeconds[id] = secondTicks;
		return;
	}

	if (b1 || b2) // if at least one is on 
	{
		if (prevFullSeconds[id] > 0)
		{
			if ((secondTicks - prevFullSeconds[id]) >= BALL_VALVE_OFF_DELAY_SEC) // if 10 min passed since last time when at least one was on, turn off solenoid
			{
				prevFullSeconds[id] = secondTicks;
				setBallValve(id, false); // Close
				return;
			}

			Serial.print(F("Delaying ball valve #"));
			Serial.print(id + 1);
			Serial.print(F(" OFF. Seconds left:"));
			Serial.println((prevFullSeconds[id] + BALL_VALVE_OFF_DELAY_SEC) - secondTicks);
			return;
		}

		//todo
		if (prevFullSeconds[id] == 0)
			prevFullSeconds[id] = secondTicks;
		return;
	}

	if (distance >= (settings[id].MinDistance + 5)) // 5 cm delta
	{
		prevFullSeconds[id] = 0;
		setBallValve(id, true); // Open
	}
}

void setFloatSwitchState(byte id, bool value)
{
	value = !value;

	if (float_switch_states[id] != value)
	{
		float_switch_states[id] = value;

		PublishTankState(id);

		Serial.print(F("Float switch #"));
		Serial.print(id + 1);
		Serial.print(F(" = "));
		Serial.println(value);
	}
}

bool isTankFull(byte id)
{
	return float_switch_states[id] || (ultrasound_sensor_distances[id] == MAX_DISTANCE) ||
		(ultrasound_sensor_distances[id] <= settings[id].MinDistance);
}
