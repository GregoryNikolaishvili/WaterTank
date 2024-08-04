#ifndef _BALL_VALVE_h
#define _BALL_VALVE_h

#include <Arduino.h>
#include <ArduinoHA.h>

#include "main.h"
#include "pressure.h"

class BallValve
{
public:
    BallValve(HASensor *valveState, HABinarySensor *valveOpenSwitch, HABinarySensor *valveCloseSwitch, PressureSensorX *pressureSensor);

    void initializeBallValve();
    void setBallValve(bool value, unsigned long secondTicks);
    void closeBallValve();
    void openBallValve();
    void processBallValve();
    void processBallValveSwitches();

private:
    int _ball_valve_state;
    PressureSensorX *_pressureSensor;

    HASensor *_valveState;
    HABinarySensor *_valveOpenSwitch;
    HABinarySensor *_valveCloseSwitch;

#ifndef SIMULATION_MODE
    byte _hBridgePins[2 * HBRIDGE_COUNT] = {
        PIN_HBRIDGE_BIG_IN1,
        PIN_HBRIDGE_BIG_IN2,
        PIN_HBRIDGE_SMALL_IN1,
        PIN_HBRIDGE_SMALL_IN2};
#endif
    void setHBridge(byte id, char value);
};

#endif