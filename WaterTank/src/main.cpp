#include <Arduino.h>
#include <ArduinoHA.h>

#include "main.h"
#include "ball_valve.h"
#include "network.h"

#include <HASwitchX.h>
#include <HAValveX.h>
#include <MovingAverageFilter.h>

#ifndef SIMULATION_MODE
#include <avr/wdt.h>
#endif

// LED and timer variables
unsigned long halfSecondTicks = 0;
unsigned long secondTicks = 0;

#ifndef SIMULATION_MODE
EthernetClient client;
#else
WiFiClient client;
#endif
HADevice device;
HAMqtt mqtt(client, device, RELAY_COUNT + 6);

// HASwitchX instances
HASwitchX gardenPumpBig("pump_garden_big", "Big", PIN_RELAY_GARDEN_PUMP_BIG, false);
HASwitchX gardenPumpSmall("pump_garden_small", "Small", PIN_RELAY_GARDEN_PUMP_SMALL, false);

HASwitchX waterPump1("pump_1", "Pump 1", PIN_RELAY_CLEAN_WATER_PUMP, true);
HASwitchX waterPump2("pump_2", "Pump 2", PIN_RELAY_TECH_WATER_PUMP, true);
HASwitchX waterPump3("pump_3", "Pump 3", PIN_RELAY_RESERVE_4, false);

// Home Assistant sensors
HAValveX waterValve("water_valve");

HABinarySensor valveOpenSwitch("valve_open_switch");
HABinarySensor valveCloseSwitch("valve_close_switch");

HASensorNumber pressureSensor("pressure_sensor", HASensorNumber::PrecisionP0);
HASensorNumber waterTankSensor("water_tank", HASensorNumber::PrecisionP0);

PressureSensorX *pressureSensorX;
BallValve *ballValve;

// called every 30 second
static void processWaterLevels()
{
	pressureSensorX->processPressureSensor(mqtt, false);

	int percent = pressureSensorX->getWaterTankPercent();
	waterTankSensor.setValue(percent);

	if (percent >= 100) // if full, turn off solenoid immediately
	{
		ballValve->setBallValve(false, secondTicks); // Close
		return;
	}

	if (percent <= 80) // 20% delta
	{
		ballValve->setBallValve(true, secondTicks); // Open
	}
}

static void oncePer1Minute()
{
}

static void oncePer5Second()
{
}

static void oncePerSecond()
{
	if ((secondTicks % 5) == 0)
		oncePer5Second();

	secondTicks++;

	ballValve->processBallValve();

	if ((secondTicks % 60) == 0)
		oncePer1Minute();
}

static void oncePerHalfSecond(void)
{
	halfSecondTicks++;

	// Blinking
	static uint8_t blinkingLedState = LOW;

	blinkingLedState = ~blinkingLedState;
	digitalWrite(PIN_BLINKING_LED, blinkingLedState);

	ballValve->processBallValveSwitches();

	if (halfSecondTicks % PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC == 0)
	{
#ifndef SIMULATION_MODE
		processWaterLevels();
#endif
	}

	if ((halfSecondTicks % 2) == 0)
		oncePerSecond();
}

void onRelayCommand(bool state, HASwitch *sender)
{
	HASwitchX *sw = (HASwitchX *)sender;
	digitalWrite(sw->getPin(), state ^ sw->IsInverted() ? LOW : HIGH);
	sender->setState(state); // report state back to the Home Assistant
}

void onValveCommand(HAValveX::ValveCommand command, HAValveX *sender)
{
	switch (command)
	{
	case HAValveX::CommandOpen:
		ballValve->openBallValve();
		break;

	case HAValveX::CommandClose:
	case HAValveX::CommandStop:
		ballValve->closeBallValve();
		break;
	}
}

void setup()
{
#ifndef SIMULATION_MODE
	wdt_disable();
#endif

	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Initializing.. ver. 4.0.0"));

	pinMode(PIN_BLINKING_LED, OUTPUT);
	digitalWrite(PIN_BLINKING_LED, LOW); // Turn on led at start

#ifndef SIMULATION_MODE
	initNetwork(device, client);
#else
	initNetwork(device);
#endif
	delay(500);

	mqtt.begin(MQTT_BROKER, MQTT_USERNAME, MQTT_PASSWORD);

	device.enableSharedAvailability();
	device.enableLastWill();
#ifndef SIMULATION_MODE
	device.setName("Water tank controller");
#else
	device.setName("Water tank controller (Simulated)");
#endif
	device.setSoftwareVersion("4.0.0");
	device.setManufacturer("Gregory Nikolaishvili");

	valveOpenSwitch.setName("Valve Open Switch");
	valveOpenSwitch.setIcon("mdi:valve-open");

	valveCloseSwitch.setName("Valve Close Switch");
	valveCloseSwitch.setIcon("mdi:valve-closed");

	waterValve.setName("Water Valve");
	waterValve.setIcon("mdi:pipe-valve");
	waterValve.onCommand(onValveCommand);

	pressureSensor.setName("Voltage");
	pressureSensor.setDeviceClass("voltage");
	pressureSensor.setUnitOfMeasurement("mV");

	waterTankSensor.setName("Water Tank Level");
	waterTankSensor.setUnitOfMeasurement("%");
	waterTankSensor.setIcon("mdi:car-coolant-level");

	gardenPumpBig.onCommand(onRelayCommand);
	gardenPumpSmall.onCommand(onRelayCommand);
	waterPump1.onCommand(onRelayCommand);
	waterPump2.onCommand(onRelayCommand);
	waterPump3.onCommand(onRelayCommand);
	waterPump1.setCurrentState(true);
	waterPump2.setCurrentState(true);

	valveOpenSwitch.setCurrentState(true);
	valveCloseSwitch.setCurrentState(true);

	mqtt.loop();
	pressureSensorX = new PressureSensorX(pressureSensor);
	ballValve = new BallValve(&waterValve, &valveOpenSwitch, &valveCloseSwitch, pressureSensorX);

	mqtt.loop();
	pressureSensorX->processPressureSensor(mqtt, true);
	waterTankSensor.setCurrentValue(pressureSensorX->getWaterTankPercent());

	ballValve->initializeBallValve();

#ifndef SIMULATION_MODE
	wdt_enable(WDTO_8S);
#endif

	Serial.println(F("Start"));
}

void loop()
{
#ifndef SIMULATION_MODE
	Ethernet.maintain();
#endif

	static unsigned long previousMillis = 0; // will store last time LED was updated
	unsigned long _current_millis = millis();

	uint32_t dt = previousMillis > _current_millis ? 1 + previousMillis + ~_current_millis : _current_millis - previousMillis;

	if (dt >= 500)
	{
#ifndef SIMULATION_MODE
		wdt_reset();
#endif

		// save the last time we blinked the LED
		previousMillis = _current_millis;
		oncePerHalfSecond();
	}

	mqtt.loop();
}
