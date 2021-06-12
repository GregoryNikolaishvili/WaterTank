#include <arduino.h>


//#define SIMULATION_MODE

#define PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC 120

static const byte RELAY_COUNT = 5;

static const byte TANK_COUNT = 2;

static const byte HBRIDGE_COUNT = 2;

static const int VOLTAGE_ERR_VALUE = 0x7FFF;
static const int PERCENT_ERR_VALUE = 255;


static const int BALL_VALVE_FULLY_CLOSED = -0xFF;
static const int BALL_VALVE_FULLY_OPEN = 0xFF;
static const byte BALL_VALVE_OPEN_CLOSE_SECONDS = 10;

static const uint8_t PIN_FLOAT_SWITCH_BIG = 8;
static const uint8_t PIN_PRESSURE_SENSOR_BIG = A0; // 5+5 m3 tank

static const uint8_t PIN_FLOAT_SWITCH_SMALL = 9;
static const uint8_t PIN_PRESSURE_SENSOR_SMALL = A1; // 2 m3 tank

//static const uint8_t PIN_FLOAT_SWITCH_3 = 10;

static const uint8_t PIN_HBRIDGE_BIG_IN1 = 32; //ok
static const uint8_t PIN_HBRIDGE_BIG_IN2 = 30; //ok

static const uint8_t PIN_HBRIDGE_SMALL_IN1 = 34; //ok
static const uint8_t PIN_HBRIDGE_SMALL_IN2 = 36; //ok

static const uint8_t PIN_BALL_VALVE_BIG_OPEN = 38;
static const uint8_t PIN_BALL_VALVE_BIG_CLOSED = 39;

static const uint8_t PIN_BALL_VALVE_SMALL_OPEN = 40;
static const uint8_t PIN_BALL_VALVE_SMALL_CLOSED = 41;

static const uint8_t PIN_RELAY_GARDEN_PUMP_BIG = 31;
static const uint8_t PIN_RELAY_TECH_WATER_PUMP = 29;
static const uint8_t PIN_RELAY_CLEAN_WATER_PUMP = 27;
static const uint8_t PIN_RELAY_GARDEN_PUMP_SMALL = 25;
static const uint8_t PIN_RELAY_RESERVE_4 = 23;

const byte PIN_BLINKING_LED = LED_BUILTIN; // 13 in MEGA

#ifndef SIMULATION_MODE
const int BALL_VALVE_ON_DELAY_SEC = 300;
const int BALL_VALVE_OFF_DELAY_SEC = 600;
#else
const int BALL_VALVE_ON_DELAY_SEC = 20;
const int BALL_VALVE_OFF_DELAY_SEC = 30;
#endif


struct MinMaxVoltageSettingStructure {

	int full; // Millivolt
	int empty; // Millivolt
};
