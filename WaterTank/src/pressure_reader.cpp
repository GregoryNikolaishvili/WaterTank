#include "pressure_reader.h"
#include "main.h"

const byte DEF_SETTINGS_VERSION = 0x00;
const int STORAGE_ADDRESS_SETTINGS = 0;

PressureReader::PressureReader(HASensorNumber &pressureSensor)
{
	_pressureSensor = &pressureSensor;

	_voltageAvg = new MovingAverageFilter(10, VOLTAGE_ERR_VALUE);
#ifndef SIMULATION_MODE
	analogReference(INTERNAL1V1);
#endif
	pinMode(PIN_PRESSURE_SENSOR_BIG, INPUT);

	readSettings();
}

void PressureReader::processPressureSensor(HAMqtt &mqtt, bool isInitialization)
{
	int milliVolts = readVoltageMV(mqtt);

	int oldValue = _voltageAvg->getCurrentValue();
	int newValue = _voltageAvg->process(milliVolts);

	newValue /= 10;
	newValue *= 10; // Round

	if (isInitialization)
	{
		_pressureSensor->setCurrentValue(newValue);
		return;
	}

	if (oldValue != newValue)
		_pressureSensor->setValue(newValue);
}

int PressureReader::readVoltageMV(HAMqtt &mqtt)
{
	int sum = 0;
	for (byte i = 0; i < 16; i++)
	{
		sum += analogRead(PIN_PRESSURE_SENSOR_BIG);
		mqtt.loop();
	}

	return (sum * REFERENCE_VOLTAGE_MV) / 16 / 1024;
}

int PressureReader::getWaterTankPercent()
{
	int voltage = (_voltageAvg->getCurrentValue() / 10) * 10; // Round

	// Divide all voltages by 10 and compare with empty and full values
	voltage = voltage / 10;
	int empty = _tankVoltageSettings.empty / 10;
	int full = _tankVoltageSettings.full / 10;

	if ((voltage < empty - 100) || (voltage > full + 100)) // mV
		return PERCENT_ERR_VALUE;

	if (voltage <= empty)
		return 0;
	if (voltage >= full)
		return 100;
	return (voltage - empty) * (long)100 / (full - empty);
}

void PressureReader::readSettings()
{
	// Big tank
	_tankVoltageSettings.empty = 580; // mv
	_tankVoltageSettings.full = 1050; // mv
									  //// Small tank
									  // tankVoltageSettings[1].empty = 520; //mv
									  // tankVoltageSettings[1].full = 1000; //mv

#ifndef SIMULATION_MODE
	byte v = eeprom_read_byte((uint8_t *)STORAGE_ADDRESS_SETTINGS);
	if (v != DEF_SETTINGS_VERSION)
	{
		eeprom_update_byte((uint8_t *)STORAGE_ADDRESS_SETTINGS, DEF_SETTINGS_VERSION);
		eeprom_update_block((const void *)&_tankVoltageSettings, (void *)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(_tankVoltageSettings));
	}
	else
	{
		eeprom_read_block((void *)&_tankVoltageSettings, (void *)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(_tankVoltageSettings));
	}
#endif
}
