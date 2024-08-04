#include <Arduino.h>
#include <ArduinoHA.h>
#include <MovingAverageFilter.h>

// https://wiki.dfrobot.com/Throw-in_Type_Liquid_Level_Transmitter_SKU_KIT0139
// Output Signal : 4-20mA

// https://wiki.dfrobot.com/Gravity__Analog_Current_to_Voltage_Converter_for_4~20mA_Application__SKU_SEN0262
// Detection Range : 0~25mA DC
// This current-to-voltage module linearly converts 0-25mA current signals into 0-3V voltage signals. Industrial sensors or devices usually have the current signal output of 4-20 mA.
// With this current-to-voltage module, your main control board can easily read the current signals output from industrial sensors or devices.
// Normally, current signals lower than 4mA can be used for fault diagnosis, and current signals higher than 20mA can be used for overrun detection.
// Therefore, this module is designed with a wide range of 0-25mA detection range, which is compatible with fault detection, overrun detection applications.

static const long REFERENCE_VOLTAGE_MV = 1100; // Internal 1.1V = 1100 mV

struct MinMaxVoltageSettingStructure
{
	int full;  // Millivolt
	int empty; // Millivolt
};

class PressureSensorX
{
public:
	PressureSensorX(HASensorNumber &pressureSensor);

	void processPressureSensor(HAMqtt &mqtt);
	int getWaterTankPercent();
private:
	HASensorNumber *_pressureSensor;
	MinMaxVoltageSettingStructure _tankVoltageSettings;

	MovingAverageFilter *_voltageAvg;

	int readVoltageMV(HAMqtt &mqtt);
	void readSettings();
};
