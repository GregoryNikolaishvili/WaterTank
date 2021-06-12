const byte DEF_SETTINGS_VERSION = 0x03;
const int STORAGE_ADDRESS_SETTINGS = 0;


MinMaxVoltageSettingStructure tankVoltageSettings[TANK_COUNT];

void readSettings()
{
	// Big tank
	tankVoltageSettings[0].empty = 0; //mv
	tankVoltageSettings[0].full = 1050; //mv
	// Small tank
	tankVoltageSettings[1].empty = 0; //mv
	tankVoltageSettings[1].full = 100; //mv

	byte v = eeprom_read_byte((uint8_t*)STORAGE_ADDRESS_SETTINGS);
	if (v != DEF_SETTINGS_VERSION)
	{
		eeprom_update_byte((uint8_t*)STORAGE_ADDRESS_SETTINGS, DEF_SETTINGS_VERSION);
		saveSettings(false);
	}
	else
	{
		eeprom_read_block((void*)&tankVoltageSettings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(tankVoltageSettings));
		settingsChanged(false);
	}
}

void saveSettings(bool publish)
{
	eeprom_update_block((const void*)&tankVoltageSettings, (void*)(STORAGE_ADDRESS_SETTINGS + 1), sizeof(tankVoltageSettings));

	settingsChanged(publish);
}

void settingsChanged(bool publish)
{
	PublishAllStates();

	if (publish)
		PublishSettings();
}
