#include <ArduinoHA.h>

#include "WaterTank.h"
#include "HASwitchX.h"

#include <Bounce.h>
//#include <SPI.h>
#include <Ethernet.h>				// https://github.com/arduino-libraries/Ethernet

#include <MovingAverageFilter.h>
#include <avr/wdt.h>

// LED and timer variables
unsigned long halfSecondTicks = 0;
unsigned long secondTicks = 0;

EthernetClient client;
HADevice device;
HAMqtt mqtt(client, device, RELAY_COUNT);

// Bounce instances
Bounce bouncerValveOpenSwitch(PIN_BALL_VALVE_BIG_OPEN, false, 500UL, 500UL); // 0.5 sec, 0.5 sec
Bounce bouncerValveCloseSwitch(PIN_BALL_VALVE_BIG_CLOSED, false, 500UL, 500UL); // 0.5 sec, 0.5 sec

// HASwitchX instances
HASwitchX gardenPumpBig("pump_garden_big", "Big", PIN_RELAY_GARDEN_PUMP_BIG);
HASwitchX gardenPumpSmall("pump_garden_small", "Small", PIN_RELAY_GARDEN_PUMP_SMALL);

HASwitchX waterPump1("pump_1", "Pump 1", PIN_RELAY_CLEAN_WATER_PUMP);
HASwitchX waterPump2("pump_2", "Pump 2", PIN_RELAY_TECH_WATER_PUMP);
HASwitchX waterPump3("pump_3", "Pump 3", PIN_RELAY_RESERVE_4);

// Home Assistant sensors
HABinarySensor valveOpenSwitch("valve_open_switch");
HABinarySensor valveCloseSwitch("valve_close_switch");
HASensor valveState("valve_state");

HASensorNumber pressureSensor("pressure_sensor", HASensorNumber::PrecisionP0);
HASensorNumber waterTankSensor("water_tank", HASensorNumber::PrecisionP0);

static int ball_valve_state;

void setup()
{
	wdt_disable();

	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Initializing.. ver. 4.0.0"));

	pinMode(PIN_BLINKING_LED, OUTPUT);
	digitalWrite(PIN_BLINKING_LED, LOW); // Turn on led at start

	// Init relays
	initRelayPin(PIN_RELAY_GARDEN_PUMP_BIG);
	initRelayPin(PIN_RELAY_GARDEN_PUMP_SMALL);

	initRelayPin(PIN_RELAY_CLEAN_WATER_PUMP);
	initRelayPin(PIN_RELAY_TECH_WATER_PUMP);
	initRelayPin(PIN_RELAY_RESERVE_4);

	initializeBallValve();

	readSettings();

	initPressureSensor();

	InitEthernet();

	delay(500);

	device.enableSharedAvailability();
	device.enableLastWill();
	device.setName("Water tank controller");
	device.setSoftwareVersion("4.0.0");
	device.setManufacturer("Gregory Nikolaishvili");

	valveOpenSwitch.setName("Valve Open Switch");
	valveOpenSwitch.setIcon("mdi:valve-open");
	valveCloseSwitch.setName("Valve Close Switch");
	valveCloseSwitch.setIcon("mdi:valve-closed");

	valveState.setName("Valve State");
	valveState.setDeviceClass("enum");
	valveState.setIcon("mdi:valve");

	pressureSensor.setName("Voltage");
	pressureSensor.setDeviceClass("voltage");
	pressureSensor.setUnitOfMeasurement("mV");
	
	waterTankSensor.setName("Water Tank Level");
	waterTankSensor.setUnitOfMeasurement("%");

	processPressureSensor();

	processWaterLevels(); // duplicate. same is in loop

	InitMqtt();

	wdt_enable(WDTO_8S);

	Serial.println(F("Start"));
}

void loop()
{
	Ethernet.maintain();

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

	mqtt.loop();
}

static void oncePerHalfSecond(void)
{
	halfSecondTicks++;

	// Blinking
	static uint8_t blinkingLedState = LOW;

	blinkingLedState = ~blinkingLedState;
	digitalWrite(PIN_BLINKING_LED, blinkingLedState);

	processBallValveSwitches();

	if (halfSecondTicks % PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC == 0)
	{
		processWaterLevels();
	}

	if ((halfSecondTicks % 2) == 0)
		oncePerSecond();
}

static void oncePerSecond()
{
	if ((secondTicks % 5) == 0)
		oncePer5Second();

	secondTicks++;

	processBallValve();

	if ((secondTicks % 60) == 0)
		oncePer1Minute();
}

static void oncePer5Second()
{
}

static void oncePer1Minute()
{
}

// called every 30 second
static void processWaterLevels()
{
	processPressureSensor();

	int percent = getWaterTankPercent();
	waterTankSensor.setValue(percent);

	if (percent >= 100) // if full, turn off solenoid immediately
	{
		setBallValve(false); // Close
		return;
	}

	if (percent <= 80) // 20% delta
	{
		setBallValve(true); // Open
	}
}

static void initRelayPin(byte pin)
{
	pinMode(pin, OUTPUT);
	digitalWrite(pin, HIGH);
}