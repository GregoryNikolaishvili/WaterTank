#include <arduino.h>
#include <TimeLib.h>					// http://www.pjrc.com/teensy/td_libs_Time.html


#define SIMULATION_MODE


const unsigned char TANK_COUNT = 3;

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

#define PIN_FLOAT_SWITCH_1 A0
#define PIN_ULTRASONIC_1_TX 2
#define PIN_ULTRASONIC_1_RX 3

#define PIN_FLOAT_SWITCH_2 A1
#define PIN_ULTRASONIC_2_TX 4
#define PIN_ULTRASONIC_2_RX 5

#define PIN_FLOAT_SWITCH_3 A0
#define PIN_ULTRASONIC_3_TX 6
#define PIN_ULTRASONIC_3_RX 7

#define PIN_SOLENOID_IN1 30
#define PIN_SOLENOID_IN2 31
#define PIN_SOLENOID_IN3 32
#define PIN_SOLENOID_IN4 33
#define PIN_SOLENOID_IN5 34
#define PIN_SOLENOID_IN6 35

// W5100 mini
//NSS   – Pin 10 from Arduino
//MO    – Pin 11 from Arduino(MOSI)
//MI    – Pin 12 from Arduino(MISO)
//SOK   – Pin 13 from Arduino(SCK)

#define PIN_BLINKING_LED 9


// A4(SDA) and A5(SCL)


#ifndef SIMULATION_MODE
const int SOLENOID_ON_DELAY_SEC = 300;
const int SOLENOID_OFF_DELAY_SEC = 600;
#else
const int SOLENOID_ON_DELAY_SEC = 10;
const int SOLENOID_OFF_DELAY_SEC = 15;
#endif


struct SettingStructure {
	int MaxDistance;
	int MinDistance;
};
