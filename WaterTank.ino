#include "WaterTank.h"

#include <TimeLib.h>				// https://github.com/PaulStoffregen/Time
#include <DS1307RTC.h>				// https://github.com/PaulStoffregen/DS1307RTC

#include <NewPing.h>				// https://code.google.com/p/arduino-new-ping/
#include <Bounce.h>
#include <SPI.h>
#include <Ethernet.h>				// https://github.com/arduino-libraries/Ethernet
#include <PubSubClient.h>			// https://github.com/knolleary/pubsubclient

#include <MovingAverageFilter.h>
#include <avr/wdt.h>

unsigned long halfSecondTicks = 0;
unsigned long secondTicks = 0;

Bounce floatSwitch1(PIN_FLOAT_SWITCH_BIG, false, 60 * 1000UL, 60 * 1000UL); // 60 sec, 60 sec
Bounce floatSwitch2(PIN_FLOAT_SWITCH_SMALL, false, 60 * 1000UL, 60 * 1000UL); // 60 sec, 60 sec

Bounce bouncerBigOpen(PIN_BALL_VALVE_BIG_OPEN, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerBigClose(PIN_BALL_VALVE_BIG_CLOSED, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerSmallOpen(PIN_BALL_VALVE_SMALL_OPEN, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerSmallClose(PIN_BALL_VALVE_SMALL_CLOSED, false, 500UL, 500UL); // 0.5 sec, 0.5 sec

bool float_switch_states[TANK_COUNT];

static byte relayPins[RELAY_COUNT] = {
  PIN_RELAY_GARDEN_PUMP_BIG,
  PIN_RELAY_TECH_WATER_PUMP,
  PIN_RELAY_CLEAN_WATER_PUMP,
  PIN_RELAY_GARDEN_PUMP_SMALL,
  PIN_RELAY_RESERVE_4
};



void setup()
{
	wdt_disable();

	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Initializing.. ver. 3.0.3"));

	pinMode(PIN_BLINKING_LED, OUTPUT);
	digitalWrite(PIN_BLINKING_LED, LOW); // Turn on led at start

	// Init relays
	for (byte i = 0; i < RELAY_COUNT; i++)
	{
		digitalWrite(relayPins[i], HIGH);
		pinMode(relayPins[i], OUTPUT);
	}

	InitializeFloatSwitches();
	InitializeBallValves();

	readSettings();

	initPressureSensors();

	InitEthernet();

	delay(500);

	processPressureSensors();

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
}

void oncePerHalfSecond(void)
{
	halfSecondTicks++;

	// Blinking
	static uint8_t blinkingLedState = LOW;

	blinkingLedState = !blinkingLedState;
	digitalWrite(PIN_BLINKING_LED, blinkingLedState);

	processBallValveSwitches();

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
}


void oncePer1Minute()
{
	if (MqttIsConnected())
	{
		if (secondTicks > 0) // do not publish on startup
			PublishAllStates();
	}
	else
	{
		ReconnectMqtt();
	}
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

void processFloatSwitches()
{
	if (floatSwitch1.update())
		setFloatSwitchState(0, floatSwitch1.read());
	if (floatSwitch2.update())
		setFloatSwitchState(1, floatSwitch2.read());
	/*if (floatSwitch3.update())
	  setFloatSwitchState(2, floatSwitch3.read());*/
}

void processBallValveSwitches()
{
	if (bouncerBigOpen.update() || bouncerBigClose.update())
		setBallValveSwitchState(0, bouncerBigOpen.read(), bouncerBigClose.read());
	if (bouncerSmallOpen.update() || bouncerSmallClose.update())
		setBallValveSwitchState(1, bouncerSmallOpen.read(), bouncerSmallClose.read());
}

// called every 30 second
void processWaterLevels()
{
	// processBallValveSwitches(); Already done in loop
	processFloatSwitches();

	processPressureSensors();

	for (byte id = 0; id < TANK_COUNT; id++)
		processTankWL(id);
}

void processTankWL(byte id)
{
	static unsigned long prevFullSeconds[] = { 0, 0, 0, 0 };

	int percent = getWaterTankPercent(id); 

	boolean b1 = percent == PERCENT_ERR_VALUE;
	boolean b2 = float_switch_states[id];

//	Serial.print(F("Id = "));
//	Serial.print(id);
//	Serial.print(F(", Percent = "));
//	Serial.print(percent);
//	Serial.print(F(", B1 = "));
//	Serial.print(b1);
//	Serial.print(F(", B2 = "));
//	Serial.println(b2);

	if (b1 && b2) // if both are on, turn off solenoid immediately
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

			if (getBallValveState(id) >= 0) // not closing
			{
				Serial.print(F("Delaying ball valve #"));
				Serial.print(id);
				Serial.print(F(" OFF. Seconds left:"));
				Serial.println((prevFullSeconds[id] + BALL_VALVE_OFF_DELAY_SEC) - secondTicks);
			}
			return;
		}

		prevFullSeconds[id] = secondTicks;
		return;
	}

	if (percent <= 90) // 10% delta
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
	return float_switch_states[id] || (getWaterTankPercent(id) >= 100);
}
