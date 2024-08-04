#include "ball_valve.h"

BallValve::BallValve(HASensor *valveState, HABinarySensor *valveOpenSwitch, HABinarySensor *valveCloseSwitch, PressureSensorX *pressureSensor)
{
	_valveState = valveState;
	_valveOpenSwitch = valveOpenSwitch;
	_valveCloseSwitch = valveCloseSwitch;
	_pressureSensor = pressureSensor;

	pinMode(PIN_BALL_VALVE_BIG_OPEN, OUTPUT);
	pinMode(PIN_BALL_VALVE_BIG_CLOSED, OUTPUT);

#ifndef SIMULATION_MODE
	for (byte i = 0; i < 2 * HBRIDGE_COUNT; i++)
	{
		pinMode(_hBridgePins[i], OUTPUT);
		digitalWrite(_hBridgePins[i], LOW);
	}
#endif
}

void BallValve::initializeBallValve()
{
	processBallValveSwitches();

	//////////////
	// SetHBridge(0, -1); // Start closing
	// delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	// SetHBridge(0, 0); // remove power from ball valves

	// SetHBridge(0, 1); // Start closing
	// delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	// SetHBridge(0, 0); // remove power from ball valves

	// SetHBridge(0, -1); // Start closing
	// delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	// SetHBridge(0, 0); // remove power from ball valves

	// SetHBridge(0, 1); // Start closing
	// delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
	// SetHBridge(0, 0); // remove power from ball valves
	/////////

	Serial.println(F("Closing valve"));
	if (!_valveCloseSwitch->getCurrentState())
	{
		setHBridge(0, -1);							 // Start closing
		delay(BALL_VALVE_OPEN_CLOSE_SECONDS * 1000); // wait for ball valves to close
		setHBridge(0, 0);							 // remove power from ball valves
	}
	
	_ball_valve_state = BALL_VALVE_FULLY_CLOSED;
	_valveState->setValue("close");
	Serial.println(F("Closed"));
}

void BallValve::setBallValve(bool value, unsigned long secondTicks)
{
	static unsigned long prevOffSeconds = 0;

	if (value > 0)
	{
		if (_ball_valve_state <= 0 && secondTicks < (prevOffSeconds + BALL_VALVE_ON_DELAY_SEC)) // 5 min
		{
			Serial.print(F("Delaying ball valve ON. Seconds left:"));
			Serial.println((prevOffSeconds + BALL_VALVE_ON_DELAY_SEC) - secondTicks);
			return;
		}

		// if tank is full (at least one sensors)
		if (_pressureSensor->getWaterTankPercent() >= 100)
		{
			Serial.println("Tank is full");
			return;
		}
	}

	// If open is requested and valve is closed (or closing) OR
	// if close is requested and valve is open (or opening)
	if ((value && _ball_valve_state < 0) || (!value && _ball_valve_state > 0))
	{
		if (value)
		{
			openBallValve();
		}
		else
		{
			prevOffSeconds = secondTicks;
			closeBallValve();
		}
	}
}

void BallValve::closeBallValve()
{
	Serial.println("Closing ball valve");
	_ball_valve_state = -BALL_VALVE_OPEN_CLOSE_SECONDS;
	setHBridge(0, -1); // Negative, start closing
	_valveState->setValue("closing");
}

void BallValve::openBallValve()
{
	Serial.println("Opening ball valve");
	_ball_valve_state = BALL_VALVE_OPEN_CLOSE_SECONDS;
	setHBridge(0, 1); // Positive, start opening
	_valveState->setValue("opening");
}

void BallValve::processBallValve()
{
	if (_ball_valve_state == BALL_VALVE_FULLY_CLOSED || _ball_valve_state == BALL_VALVE_FULLY_OPEN)
		return;

	bool opening;
	if (_ball_valve_state > 0)
	{
		_ball_valve_state--;
		opening = true;
	}
	else
	{
		_ball_valve_state++;
		opening = false;
	}

	if (_ball_valve_state == 0)
	{
		setHBridge(0, 0); // Neutral
		if (opening)
		{
			_ball_valve_state = BALL_VALVE_FULLY_OPEN; // Open
			_valveState->setValue("open");
		}
		else
		{
			_ball_valve_state = BALL_VALVE_FULLY_CLOSED; // Closed
			_valveState->setValue("close");
		}
	}
}

void BallValve::processBallValveSwitches()
{
	_valveOpenSwitch->setState(digitalRead(PIN_BALL_VALVE_BIG_OPEN));
	_valveCloseSwitch->setState(digitalRead(PIN_BALL_VALVE_BIG_CLOSED));
}

void BallValve::setHBridge(byte id, char value)
{
#ifndef SIMULATION_MODE
	byte pinIn1 = _hBridgePins[id * 2];
	byte pinIn2 = _hBridgePins[id * 2 + 1];

	digitalWrite(pinIn1, LOW);
	digitalWrite(pinIn2, LOW);

	if (value > 0)
		digitalWrite(pinIn1, HIGH);
	else if (value < 0)
		digitalWrite(pinIn2, HIGH);
#endif
	//
	//	Serial.print(F("HBridge #"));
	//	Serial.print(id);
	//	Serial.print(F(": "));
	//	Serial.println((int)value);
}
