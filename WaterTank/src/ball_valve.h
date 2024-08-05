#ifndef _BALL_VALVE_H
#define _BALL_VALVE_H

#include <Arduino.h>
#include <ArduinoHA.h>
#include <HAValveX.h>

#include "main.h"
#include "pressure_reader.h"

class BallValve
{
public:
    BallValve(HAValveX *waterValve, HABinarySensor *valveOpenSwitch, HABinarySensor *valveCloseSwitch, PressureReader *pressureSensor);

    void initializeBallValve();
    void setBallValve(bool value, unsigned long secondTicks);
    void closeBallValve();
    void openBallValve();
    void processBallValve();
    void processBallValveSwitches();

private:
    int _ball_valve_state;
    PressureReader *_pressureSensor;

    HAValveX *_waterValve;
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