#include <arduino.h>
#include <TimeLib.h>					// http://www.pjrc.com/teensy/td_libs_Time.html


//#define SIMULATION_MODE

#define PROCESS_INTERVAL_WATER_LEVEL_HALF_SEC 60

static const byte RELAY_COUNT = 5;

static const byte TANK_COUNT = 3;
static const byte HBRIDGE_COUNT = 4;

static const int MAX_DISTANCE = 0x7FFF;

//const unsigned char FLOAT_SWITCH_1 = 0;
//const unsigned char FLOAT_SWITCH_2 = 1;
//const unsigned char FLOAT_SWITCH_3 = 2;

//const unsigned char ULTRASOUND_SENSOR_1 = 0;
//const unsigned char ULTRASOUND_SENSOR_2 = 1;
//const unsigned char ULTRASOUND_SENSOR_3 = 2;

//static const unsigned int ERR_GENERAL = 1;
//static const unsigned int ERR_ULTRASONIC_1 = 2;
//static const unsigned int ERR_ULTRASONIC_2 = 4;
//static const unsigned int ERR_ULTRASONIC_3 = 8;

//#define state_is_error_bit_set(__mask__) ((waterLevelControllerState & (__mask__)) != 0)

static const byte BALL_VALVE_OPEN_CLOSE_SECONDS = 10;

static const uint8_t PIN_FLOAT_SWITCH_1 = 8;
static const uint8_t PIN_ULTRASONIC_1_TX = 2;
static const uint8_t PIN_ULTRASONIC_1_RX = 3;

static const uint8_t PIN_FLOAT_SWITCH_2 = 9;
static const uint8_t PIN_ULTRASONIC_2_TX = 4;
static const uint8_t PIN_ULTRASONIC_2_RX = 5;

static const uint8_t PIN_FLOAT_SWITCH_3 = 10;
static const uint8_t PIN_ULTRASONIC_3_TX = 6;
static const uint8_t PIN_ULTRASONIC_3_RX = 7;

static const uint8_t PIN_HBRIDGE1_IN1 = 32; //ok
static const uint8_t PIN_HBRIDGE1_IN2 = 30; //ok

static const uint8_t PIN_HBRIDGE2_IN1 = 34; //ok
static const uint8_t PIN_HBRIDGE2_IN2 = 36; //ok

static const uint8_t PIN_HBRIDGE3_IN1 = 26; //ok
static const uint8_t PIN_HBRIDGE3_IN2 = 28; //ok

static const uint8_t PIN_HBRIDGE4_IN1 = 22;
static const uint8_t PIN_HBRIDGE4_IN2 = 24;

static const uint8_t PIN_BALL_VALVE1_OPEN = 38;
static const uint8_t PIN_BALL_VALVE1_CLOSED = 39;
static const uint8_t PIN_BALL_VALVE2_OPEN = 40;
static const uint8_t PIN_BALL_VALVE2_CLOSED = 41;
static const uint8_t PIN_BALL_VALVE3_OPEN = 42;
static const uint8_t PIN_BALL_VALVE3_CLOSED = 43; 

static const uint8_t PIN_RELAY_CLEAN_WATER_PUMP = 31;
static const uint8_t PIN_RELAY_TECH_WATER_PUMP = 29;
static const uint8_t PIN_RELAY_GARDEN_PUMP = 27;
static const uint8_t PIN_RELAY_RESERVE_3 = 25;
static const uint8_t PIN_RELAY_RESERVE_4 = 23;

const byte PIN_BLINKING_LED = LED_BUILTIN; // 13 in MEGA

#ifndef SIMULATION_MODE
const int BALL_VALVE_ON_DELAY_SEC = 300;
const int BALL_VALVE_OFF_DELAY_SEC = 600;
#else
const int BALL_VALVE_ON_DELAY_SEC = 20;
const int BALL_VALVE_OFF_DELAY_SEC = 30;
#endif


struct SettingStructure {
	int MaxDistance;
	int MinDistance;
};
