#include <arduino.h>
#include <TimeLib.h>					// http://www.pjrc.com/teensy/td_libs_Time.html


#define SIMULATION_MODE

const byte RELAY_COUNT = 5;

const byte TANK_COUNT = 3;
const byte BALL_VALVE_COUNT = 4;

const int MAX_DISTANCE = 0x7FFF;

const unsigned char FLOAT_SWITCH_1 = 0;
const unsigned char FLOAT_SWITCH_2 = 1;
const unsigned char FLOAT_SWITCH_3 = 2;

const unsigned char ULTRASOUND_SENSOR_1 = 0;
const unsigned char ULTRASOUND_SENSOR_2 = 1;
const unsigned char ULTRASOUND_SENSOR_3 = 2;

const unsigned int ERR_GENERAL = 1;
const unsigned int ERR_ULTRASONIC_1 = 2;
const unsigned int ERR_ULTRASONIC_2 = 4;
const unsigned int ERR_ULTRASONIC_3 = 8;

#define state_is_error_bit_set(__mask__) ((waterLevelControllerState & (__mask__)) != 0)

#define BALL_VALVE_OPEN_CLOSE_SECONDS 5

static const uint8_t PIN_FLOAT_SWITCH_1 = A0;
static const uint8_t PIN_ULTRASONIC_1_TX = 2;
static const uint8_t PIN_ULTRASONIC_1_RX = 3;

static const uint8_t PIN_FLOAT_SWITCH_2 = A1;
static const uint8_t PIN_ULTRASONIC_2_TX = 4;
static const uint8_t PIN_ULTRASONIC_2_RX = 5;

static const uint8_t PIN_FLOAT_SWITCH_3 = A0;
static const uint8_t PIN_ULTRASONIC_3_TX = 6;
static const uint8_t PIN_ULTRASONIC_3_RX = 7;

static const uint8_t PIN_HBRIDGE1_IN1 = 22;
static const uint8_t PIN_HBRIDGE1_IN2 = 24;
static const uint8_t PIN_HBRIDGE2_IN1 = 26;
static const uint8_t PIN_HBRIDGE2_IN2 = 28;
static const uint8_t PIN_HBRIDGE3_IN1 = 30;
static const uint8_t PIN_HBRIDGE3_IN2 = 32;
static const uint8_t PIN_HBRIDGE4_IN1 = 34;
static const uint8_t PIN_HBRIDGE4_IN2 = 36;

static const uint8_t PIN_RELAY_CLEAN_WATER_PUMP = 31;
static const uint8_t PIN_RELAY_TECH_WATER_PUMP = 29;
static const uint8_t PIN_RELAY_GARDEN_PUMP = 27;
static const uint8_t PIN_RELAY_RESERVE_3 = 25;
static const uint8_t PIN_RELAY_RESERVE_4 = 23;

// W5100 mini
//NSS   – Pin 10 from Arduino
//MO    – Pin 11 from Arduino(MOSI)
//MI    – Pin 12 from Arduino(MISO)
//SOK   – Pin 13 from Arduino(SCK)

#define PIN_BLINKING_LED 9


// A4(SDA) and A5(SCL)


#ifndef SIMULATION_MODE
const int BALL_VALVE_ON_DELAY_SEC = 300;
const int BALL_VALVE_OFF_DELAY_SEC = 600;
#else
const int BALL_VALVE_ON_DELAY_SEC = 10;
const int BALL_VALVE_OFF_DELAY_SEC = 15;
#endif


struct SettingStructure {
	int MaxDistance;
	int MinDistance;
};
